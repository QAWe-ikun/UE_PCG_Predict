"""
train_deep.py
DeepPredictor (PCGGraphTransformerPredictor) 训练脚本。

用法：
  cd PCG_Predict_Models
  python src/train_deep.py

依赖：
  pip install torch pyyaml
"""

import json
import os
import sys
from pathlib import Path

import numpy as np
import torch
import torch.nn as nn
import yaml
from torch.utils.data import DataLoader, Dataset, random_split

# 将 src 加入路径
sys.path.insert(0, str(Path(__file__).parent))

from features import (
    build_adj_matrix,
    build_history_features,
    build_node_features,
    encode_cursor,
    encode_intent,
)
from graph_transformer import PCGGraphTransformerPredictor


BASE_DIR     = Path(__file__).parent.parent
CONFIG_PATH  = BASE_DIR / "config" / "train_config.yaml"
SAMPLES_PATH = BASE_DIR / "data" / "samples.jsonl"
MODELS_DIR   = BASE_DIR / "models"


# ---------------------------------------------------------------------------
# Dataset
# ---------------------------------------------------------------------------

class PCGSampleDataset(Dataset):
    def __init__(self, samples: list[dict], cfg: dict):
        self.samples = samples
        self.max_nodes   = cfg["data"]["max_local_nodes"]
        self.history_len = cfg["data"]["history_seq_len"]
        self.step_dim    = cfg["data"]["history_step_dim"]
        self.intent_dim  = cfg["model"]["intent_dim"]
        self.cursor_dim  = cfg["data"]["cursor_dim"]

    def __len__(self):
        return len(self.samples)

    def __getitem__(self, idx):
        s = self.samples[idx]

        context_node_ids = s.get("context_node_ids", [])
        context_edges    = s.get("context_edges", [])
        history_sequence = s.get("history_sequence", [])
        intent_hint      = s.get("intent_hint", "")
        cursor_dir       = s.get("cursor_pin_direction", "output")
        cursor_dtype     = s.get("cursor_pin_data_type", "Any")
        target_id        = s.get("target_node_id", 0)

        # 节点特征 + 掩码
        node_features, node_mask = build_node_features(
            context_node_ids, context_edges, self.max_nodes)

        # 邻接矩阵
        adj_matrix = build_adj_matrix(
            context_node_ids, context_edges, self.max_nodes)

        # 历史序列
        history_seq = build_history_features(
            history_sequence, self.history_len, self.step_dim)

        # 意图向量
        intent_vector = encode_intent(intent_hint, self.intent_dim)

        # Cursor 特征
        cursor_features = encode_cursor(cursor_dir, cursor_dtype)

        # direction_flag：默认 forward（0）
        direction_flag = np.array([0.0], dtype=np.float32)

        return {
            "node_features":   torch.from_numpy(node_features),
            "adj_matrix":      torch.from_numpy(adj_matrix),
            "history_seq":     torch.from_numpy(history_seq),
            "intent_vector":   torch.from_numpy(intent_vector),
            "cursor_features": torch.from_numpy(cursor_features),
            "node_mask":       torch.from_numpy(node_mask),
            "direction_flag":  torch.from_numpy(direction_flag),
            "target":          torch.tensor(target_id, dtype=torch.long),
        }


# ---------------------------------------------------------------------------
# 训练
# ---------------------------------------------------------------------------

def load_samples(path: Path) -> list[dict]:
    samples = []
    with open(path, encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if line:
                try:
                    samples.append(json.loads(line))
                except json.JSONDecodeError:
                    pass
    return samples


def train():
    with open(CONFIG_PATH, encoding="utf-8") as f:
        cfg = yaml.safe_load(f)

    device = torch.device("cuda" if torch.cuda.is_available() else "cpu")
    print(f"[train] Device: {device}")

    # 加载样本
    if not SAMPLES_PATH.exists():
        print(f"[train] samples.jsonl not found: {SAMPLES_PATH}")
        print("  Run: python src/data_collector.py")
        return

    samples = load_samples(SAMPLES_PATH)
    print(f"[train] Loaded {len(samples)} samples")

    if len(samples) == 0:
        print("[train] No samples, abort.")
        return

    # 数据集划分
    dataset = PCGSampleDataset(samples, cfg)
    val_size   = max(1, int(len(dataset) * 0.1))
    train_size = len(dataset) - val_size
    train_ds, val_ds = random_split(dataset, [train_size, val_size])

    train_loader = DataLoader(train_ds, batch_size=cfg["training"]["batch_size"],
                              shuffle=True,  drop_last=True)
    val_loader   = DataLoader(val_ds,   batch_size=cfg["training"]["batch_size"],
                              shuffle=False, drop_last=False)

    # 模型
    model = PCGGraphTransformerPredictor(
        num_node_types=cfg["model"]["num_node_types"],
    ).to(device)

    optimizer = torch.optim.AdamW(
        model.parameters(),
        lr=cfg["training"]["lr"],
        weight_decay=cfg["training"]["weight_decay"],
    )
    scheduler = torch.optim.lr_scheduler.CosineAnnealingLR(
        optimizer, T_max=cfg["training"]["epochs"]
    )
    criterion = nn.CrossEntropyLoss()

    best_val_loss = float("inf")
    patience      = cfg["training"]["early_stopping_patience"]
    no_improve    = 0

    MODELS_DIR.mkdir(parents=True, exist_ok=True)
    ckpt_path = MODELS_DIR / "deep_predictor_best.pt"

    for epoch in range(1, cfg["training"]["epochs"] + 1):
        # --- 训练 ---
        model.train()
        train_loss = 0.0
        for batch in train_loader:
            node_features   = batch["node_features"].to(device)
            adj_matrix      = batch["adj_matrix"].to(device)
            history_seq     = batch["history_seq"].to(device)
            intent_vector   = batch["intent_vector"].to(device)
            cursor_features = batch["cursor_features"].to(device)
            node_mask       = batch["node_mask"].to(device)
            direction_flag  = batch["direction_flag"].to(device)
            target          = batch["target"].to(device)

            optimizer.zero_grad()
            logits = model(node_features, adj_matrix, history_seq,
                           intent_vector, cursor_features, node_mask, direction_flag)
            loss = criterion(logits, target)
            loss.backward()
            torch.nn.utils.clip_grad_norm_(model.parameters(), 1.0)
            optimizer.step()
            train_loss += loss.item()

        train_loss /= len(train_loader)

        # --- 验证 ---
        model.eval()
        val_loss = 0.0
        correct  = 0
        total    = 0
        with torch.no_grad():
            for batch in val_loader:
                node_features   = batch["node_features"].to(device)
                adj_matrix      = batch["adj_matrix"].to(device)
                history_seq     = batch["history_seq"].to(device)
                intent_vector   = batch["intent_vector"].to(device)
                cursor_features = batch["cursor_features"].to(device)
                node_mask       = batch["node_mask"].to(device)
                direction_flag  = batch["direction_flag"].to(device)
                target          = batch["target"].to(device)

                logits = model(node_features, adj_matrix, history_seq,
                               intent_vector, cursor_features, node_mask, direction_flag)
                loss = criterion(logits, target)
                val_loss += loss.item()

                preds    = logits.argmax(dim=-1)
                correct += (preds == target).sum().item()
                total   += target.size(0)

        val_loss /= len(val_loader)
        val_acc   = correct / total if total > 0 else 0.0
        scheduler.step()

        print(f"Epoch {epoch:3d} | train_loss={train_loss:.4f} "
              f"val_loss={val_loss:.4f} val_acc={val_acc:.3f}")

        # Early stopping
        if val_loss < best_val_loss:
            best_val_loss = val_loss
            no_improve    = 0
            torch.save(model.state_dict(), ckpt_path)
            print(f"  → Saved best model to {ckpt_path}")
        else:
            no_improve += 1
            if no_improve >= patience:
                print(f"[train] Early stopping at epoch {epoch}")
                break

    print(f"[train] Done. Best val_loss={best_val_loss:.4f}")


if __name__ == "__main__":
    train()
