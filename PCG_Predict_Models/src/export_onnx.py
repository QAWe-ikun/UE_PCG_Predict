"""
export_onnx.py
将训练好的 DeepPredictor 导出为 ONNX 格式。

用法：
  cd PCG_Predict_Models
  python src/export_onnx.py                          # 使用最佳 checkpoint
  python src/export_onnx.py --checkpoint models/deep_predictor_best.pt

输出：models/deep_predictor.onnx
"""

import argparse
import sys
from pathlib import Path

import torch

sys.path.insert(0, str(Path(__file__).parent))
from graph_transformer import PCGGraphTransformerPredictor


BASE_DIR   = Path(__file__).parent.parent
MODELS_DIR = BASE_DIR / "models"
ONNX_PATH  = MODELS_DIR / "deep_predictor.onnx"
CKPT_PATH  = MODELS_DIR / "deep_predictor_best.pt"


def export(checkpoint_path: Path = CKPT_PATH,
           output_path: Path     = ONNX_PATH,
           num_node_types: int   = 195):

    model = PCGGraphTransformerPredictor(num_node_types=num_node_types)

    if checkpoint_path.exists():
        model.load_state_dict(torch.load(checkpoint_path, map_location="cpu"))
        print(f"[export] Loaded checkpoint: {checkpoint_path}")
    else:
        print(f"[export] No checkpoint found at {checkpoint_path}, exporting untrained model")

    model.eval()

    # 固定形状的虚拟输入（batch=1）
    B = 1
    dummy_inputs = (
        torch.zeros(B, 7,  20),  # node_features
        torch.zeros(B, 7,  7),   # adj_matrix
        torch.zeros(B, 5,  20),  # history_seq
        torch.zeros(B, 64),      # intent_vector
        torch.zeros(B, 24),      # cursor_features
        torch.ones( B, 7),       # node_mask（全1=全有效）
        torch.zeros(B, 1),       # direction_flag（0=forward）
    )

    input_names = [
        "node_features",
        "adj_matrix",
        "history_seq",
        "intent_vector",
        "cursor_features",
        "node_mask",
        "direction_flag",
    ]
    output_names = ["logits"]

    # batch 维度设为动态（推理时 batch=1，但保留灵活性）
    dynamic_axes = {name: {0: "batch"} for name in input_names}
    dynamic_axes["logits"] = {0: "batch"}

    output_path.parent.mkdir(parents=True, exist_ok=True)

    torch.onnx.export(
        model,
        dummy_inputs,
        str(output_path),
        input_names=input_names,
        output_names=output_names,
        dynamic_axes=dynamic_axes,
        opset_version=17,
        do_constant_folding=True,
        verbose=False,
    )

    size_mb = output_path.stat().st_size / 1024 / 1024
    print(f"[export] Exported to: {output_path}  ({size_mb:.1f} MB)")
    print(f"[export] Input shapes (batch=1):")
    print(f"  node_features   [1, 7, 20]")
    print(f"  adj_matrix      [1, 7, 7]")
    print(f"  history_seq     [1, 5, 20]")
    print(f"  intent_vector   [1, 64]")
    print(f"  cursor_features [1, 24]")
    print(f"  node_mask       [1, 7]")
    print(f"  direction_flag  [1, 1]")
    print(f"[export] Output shape: logits [1, {num_node_types}]")

    # 简单验证：用 onnxruntime 跑一次推理
    try:
        import onnxruntime as ort
        import numpy as np

        sess = ort.InferenceSession(str(output_path))
        feeds = {
            "node_features":   np.zeros((1, 7, 20),  dtype=np.float32),
            "adj_matrix":      np.zeros((1, 7, 7),   dtype=np.float32),
            "history_seq":     np.zeros((1, 5, 20),  dtype=np.float32),
            "intent_vector":   np.zeros((1, 64),     dtype=np.float32),
            "cursor_features": np.zeros((1, 24),     dtype=np.float32),
            "node_mask":       np.ones( (1, 7),      dtype=np.float32),
            "direction_flag":  np.zeros((1, 1),      dtype=np.float32),
        }
        outputs = sess.run(None, feeds)
        logits  = outputs[0]
        print(f"[export] Validation OK: logits shape={logits.shape}, "
              f"top1={logits[0].argmax()}")
    except ImportError:
        print("[export] onnxruntime not installed, skipping validation")
    except Exception as e:
        print(f"[export] Validation failed: {e}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--checkpoint", type=Path, default=CKPT_PATH)
    parser.add_argument("--output",     type=Path, default=ONNX_PATH)
    parser.add_argument("--num_node_types", type=int, default=195)
    args = parser.parse_args()

    export(args.checkpoint, args.output, args.num_node_types)
