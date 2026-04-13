"""
data_collector.py
从原始 PCG Graph JSON 文件提取训练样本，输出到 data/samples.jsonl

原始格式 (data/raw/graph_xxx.json):
  {
    "graph_id": "graph_001",
    "intent_hint": "dense forest",
    "nodes": [{"node_uid": "n0", "node_type_id": 87, "node_type_name": "SurfaceSampler", "pos_x": 0, "pos_y": 0}],
    "edges": [{"src_uid": "n0", "src_pin": 0, "dst_uid": "n1", "dst_pin": 0, "data_type": "Point"}]
  }

输出样本格式 (data/samples.jsonl, 每行一个 JSON):
  {
    "sample_id": "graph_001_step_1",
    "context_node_ids": [87, 36],
    "context_edges": [[87, 36]],
    "history_sequence": [87, 36],
    "cursor_pin_direction": "output",
    "cursor_pin_data_type": "Point",
    "intent_hint": "dense forest",
    "target_node_id": 100
  }
"""

import json
import os
from pathlib import Path
from typing import Optional


BASE_DIR = Path(__file__).parent.parent
DATA_RAW_DIR    = BASE_DIR / "data" / "raw"
OUTPUT_PATH     = BASE_DIR / "data" / "samples.jsonl"
REGISTRY_PATH   = BASE_DIR / "config" / "node_registry.json"

MAX_CONTEXT_NODES = 7
MAX_HISTORY_LEN   = 5


def _load_valid_node_ids(registry_path: Path = REGISTRY_PATH) -> set[int]:
    """加载 node_registry.json，返回所有合法的 node_type_id 集合。"""
    with open(registry_path, encoding="utf-8") as f:
        registry = json.load(f)
    # 支持顶层为列表或 {"nodes": [...]} 两种格式
    nodes = registry if isinstance(registry, list) else registry.get("nodes", [])
    return {int(n["id"]) for n in nodes if "id" in n}


def _build_adjacency(nodes: list[dict], edges: list[dict]) -> dict[str, list[str]]:
    """构建 uid → 下游 uid 列表的邻接表（有向图，按数据流方向）"""
    adj: dict[str, list[str]] = {n["node_uid"]: [] for n in nodes}
    for e in edges:
        src = e.get("src_uid", "")
        dst = e.get("dst_uid", "")
        if src in adj:
            adj[src].append(dst)
    return adj


def _topological_order(nodes: list[dict], edges: list[dict]) -> list[str]:
    """Kahn 算法拓扑排序，返回 uid 列表（数据流顺序）"""
    in_degree: dict[str, int] = {n["node_uid"]: 0 for n in nodes}
    adj = _build_adjacency(nodes, edges)

    for e in edges:
        dst = e.get("dst_uid", "")
        if dst in in_degree:
            in_degree[dst] += 1

    queue = [uid for uid, deg in in_degree.items() if deg == 0]
    order: list[str] = []

    while queue:
        uid = queue.pop(0)
        order.append(uid)
        for downstream in adj.get(uid, []):
            in_degree[downstream] -= 1
            if in_degree[downstream] == 0:
                queue.append(downstream)

    # 如果有环（理论上 PCG Graph 不应有），追加剩余节点
    remaining = [uid for uid in in_degree if uid not in order]
    order.extend(remaining)
    return order


def _get_data_type_for_edge(uid: str, edges: list[dict]) -> str:
    """获取某节点输出边的数据类型（取第一条输出边）"""
    for e in edges:
        if e.get("src_uid") == uid:
            return e.get("data_type", "Any")
    return "Any"


def extract_samples_from_graph(graph: dict,
                               valid_node_ids: Optional[set[int]] = None) -> list[dict]:
    """
    从单个 PCG Graph 提取训练样本。

    策略：按拓扑顺序遍历节点，对每个节点 N_i（i >= 1），
    以 N_0..N_{i-1} 为上下文，N_i 为预测目标，生成一个样本。

    若提供 valid_node_ids，则过滤掉含未知节点的样本：
    context_node_ids、history_sequence、target_node_id 中任何一个
    不在注册表内，该样本直接丢弃。
    """
    graph_id    = graph.get("graph_id", "unknown")
    intent_hint = graph.get("intent_hint", "")
    nodes       = graph.get("nodes", [])
    edges       = graph.get("edges", [])

    if len(nodes) < 2:
        return []

    uid_to_node = {n["node_uid"]: n for n in nodes}
    topo_order  = _topological_order(nodes, edges)

    samples: list[dict] = []

    for step_idx in range(1, len(topo_order)):
        target_uid  = topo_order[step_idx]
        target_node = uid_to_node.get(target_uid)
        if not target_node:
            continue

        target_type_id = target_node.get("node_type_id", -1)
        if target_type_id < 0:
            continue

        # 上下文：目标节点之前的所有节点（最多 MAX_CONTEXT_NODES 个）
        context_uids = topo_order[:step_idx]
        context_uids = context_uids[-MAX_CONTEXT_NODES:]

        context_node_ids = [
            uid_to_node[u]["node_type_id"]
            for u in context_uids
            if u in uid_to_node
        ]

        # 上下文内的边（只保留两端都在上下文内的边，用 type_id 表示）
        context_uid_set = set(context_uids)
        context_edges: list[list[int]] = []
        for e in edges:
            src, dst = e.get("src_uid"), e.get("dst_uid")
            if src in context_uid_set and dst in context_uid_set:
                src_id = uid_to_node[src]["node_type_id"]
                dst_id = uid_to_node[dst]["node_type_id"]
                context_edges.append([src_id, dst_id])

        # 历史序列：最近 MAX_HISTORY_LEN 步
        history_sequence = context_node_ids[-MAX_HISTORY_LEN:]

        # cursor 信息：取上下文中最后一个节点的输出边数据类型
        cursor_uid       = context_uids[-1] if context_uids else ""
        cursor_data_type = _get_data_type_for_edge(cursor_uid, edges)

        sample = {
            "sample_id":           f"{graph_id}_step_{step_idx}",
            "graph_id":            graph_id,
            "context_node_ids":    context_node_ids,
            "context_edges":       context_edges,
            "history_sequence":    history_sequence,
            "cursor_pin_direction": "output",
            "cursor_pin_data_type": cursor_data_type,
            "intent_hint":         intent_hint,
            "target_node_id":      target_type_id,
        }

        # 过滤含未知节点的样本
        if valid_node_ids is not None:
            all_ids = set(context_node_ids) | set(history_sequence) | {target_type_id}
            if not all_ids.issubset(valid_node_ids):
                continue

        samples.append(sample)

    return samples


def collect_all(raw_dir: Path = DATA_RAW_DIR,
                output_path: Path = OUTPUT_PATH,
                registry_path: Path = REGISTRY_PATH,
                verbose: bool = True) -> int:
    """
    遍历 raw_dir 下所有 .json 文件，提取样本写入 output_path。
    加载 node_registry.json 过滤含未知节点的样本。
    返回总样本数。
    """
    # 加载注册表
    valid_node_ids: Optional[set[int]] = None
    if registry_path.exists():
        valid_node_ids = _load_valid_node_ids(registry_path)
        if verbose:
            print(f"[data_collector] Loaded registry: {len(valid_node_ids)} valid node IDs")
    else:
        print(f"[data_collector] WARNING: registry not found at {registry_path}, skipping validation")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    raw_files = sorted(raw_dir.glob("*.json"))

    if not raw_files:
        print(f"[data_collector] No JSON files found in {raw_dir}")
        return 0

    total = 0
    with open(output_path, "w", encoding="utf-8") as out_f:
        for json_path in raw_files:
            try:
                with open(json_path, encoding="utf-8") as f:
                    graph = json.load(f)
                samples = extract_samples_from_graph(graph, valid_node_ids)
                for s in samples:
                    out_f.write(json.dumps(s, ensure_ascii=False) + "\n")
                total += len(samples)
                if verbose:
                    print(f"  {json_path.name}: {len(samples)} samples")
            except Exception as e:
                print(f"  [WARN] Failed to process {json_path.name}: {e}")

    if verbose:
        print(f"[data_collector] Total: {total} samples → {output_path}")
    return total


if __name__ == "__main__":
    collect_all()
