"""
features.py
特征编码函数，供 dataset 和推理时使用。

维度约定（与 config/train_config.yaml 对齐）：
  node_feature_dim  = 20
  history_step_dim  = 20
  cursor_dim        = 24
  intent_dim        = 64
  num_node_types    = 195  (node_registry.json 共 195 个节点，ID 0-194)
"""

import numpy as np
from typing import Optional


NUM_NODE_TYPES = 195

DATA_TYPE_MAP = {
    "Point":     0,
    "Spatial":   1,
    "Param":     2,
    "Execution": 3,
    "Concrete":  4,
    "Landscape": 5,
    "Spline":    6,
    "Volume":    7,
    "Any":       8,
    "None":      9,
}
NUM_DATA_TYPES = 10  # 包含 Any / None


def encode_node(node_type_id: int,
                in_degree: int = 0,
                out_degree: int = 0,
                depth: int = 0,
                input_type_ids: Optional[list[int]] = None,
                output_type_ids: Optional[list[int]] = None) -> np.ndarray:
    """
    编码单个节点为 20 维特征向量。

    布局：
      [0]      归一化节点类型 ID  (node_type_id / NUM_NODE_TYPES)
      [1]      归一化入度         (in_degree / 10, clip 1)
      [2]      归一化出度         (out_degree / 10, clip 1)
      [3]      归一化拓扑深度     (depth / 20, clip 1)
      [4-13]   输入数据类型 one-hot (10维, NUM_DATA_TYPES)
      [14-19]  保留（全 0）
    """
    feat = np.zeros(20, dtype=np.float32)

    feat[0] = node_type_id / NUM_NODE_TYPES
    feat[1] = min(in_degree  / 10.0, 1.0)
    feat[2] = min(out_degree / 10.0, 1.0)
    feat[3] = min(depth      / 20.0, 1.0)

    # 输入数据类型 one-hot（取第一个输入类型）
    if input_type_ids:
        t = input_type_ids[0] if input_type_ids[0] < NUM_DATA_TYPES else 8  # fallback Any
        feat[4 + t] = 1.0

    return feat


def encode_history_step(node_type_id: int,
                        action_type: int = 0,
                        data_type_str: str = "Any") -> np.ndarray:
    """
    编码历史序列中的一步为 20 维特征向量。

    布局：
      [0]      归一化动作类型     (action_type / 10, clip 1)
      [1]      归一化节点类型 ID  (node_type_id / NUM_NODE_TYPES)
      [2-11]   数据类型 one-hot   (10维)
      [12-19]  保留（全 0）
    """
    feat = np.zeros(20, dtype=np.float32)

    feat[0] = min(action_type   / 10.0, 1.0)
    feat[1] = node_type_id / NUM_NODE_TYPES

    dt = DATA_TYPE_MAP.get(data_type_str, DATA_TYPE_MAP["Any"])
    feat[2 + dt] = 1.0

    return feat


def encode_cursor(pin_direction: str = "output",
                  data_type_str: str = "Any",
                  node_type_id: int = 0) -> np.ndarray:
    """
    编码当前 cursor（悬停的 Pin）为 24 维特征向量。

    布局：
      [0-1]    pin 方向 one-hot  (input=0, output=1)
      [2-11]   数据类型 one-hot  (10维)
      [12]     归一化节点类型 ID
      [13-23]  保留（全 0）
    """
    feat = np.zeros(24, dtype=np.float32)

    # pin 方向
    if pin_direction.lower() == "input":
        feat[0] = 1.0
    else:
        feat[1] = 1.0

    # 数据类型
    dt = DATA_TYPE_MAP.get(data_type_str, DATA_TYPE_MAP["Any"])
    feat[2 + dt] = 1.0

    # 节点类型
    feat[12] = node_type_id / NUM_NODE_TYPES

    return feat


def encode_intent(intent_hint: str,
                  intent_dim: int = 64) -> np.ndarray:
    """
    将意图文本编码为 64 维向量（MVP：关键词 one-hot 投影）。
    后续可替换为 sentence-transformer 嵌入。
    """
    KEYWORDS = [
        "forest", "tree", "dense", "scatter", "random",
        "city", "building", "urban", "road", "street",
        "mountain", "rock", "cliff", "terrain", "height",
        "river", "water", "lake", "ocean", "flow",
        "grass", "field", "meadow", "plain", "flat",
        "cave", "dungeon", "interior", "room", "wall",
        "stall", "market", "shop", "village", "town",
        "sparse", "cluster", "grid", "pattern", "noise",
        "large", "small", "tall", "short", "wide",
        "color", "texture", "material", "surface", "detail",
        "dynamic", "static", "animated", "physics", "collision",
        "lod", "instanced", "merged", "split", "filter",
    ]
    vec = np.zeros(intent_dim, dtype=np.float32)
    if not intent_hint:
        return vec

    lower = intent_hint.lower()
    for i, kw in enumerate(KEYWORDS[:intent_dim]):
        if kw in lower:
            vec[i] = 1.0

    # L2 归一化（避免全零时除零）
    norm = np.linalg.norm(vec)
    if norm > 0:
        vec = vec / norm

    return vec


def build_adj_matrix(context_node_ids: list[int],
                     context_edges: list[list[int]],
                     max_nodes: int = 7) -> np.ndarray:
    """
    将上下文节点和边构建为固定大小的有向邻接矩阵 [max_nodes, max_nodes]。
    节点按 context_node_ids 的顺序映射到矩阵索引。
    """
    adj = np.zeros((max_nodes, max_nodes), dtype=np.float32)

    # 只取前 max_nodes 个节点
    nodes = context_node_ids[:max_nodes]
    type_to_idx: dict[int, int] = {}
    for idx, nid in enumerate(nodes):
        # 同类型节点可能出现多次，取最后一次（简化处理）
        type_to_idx[nid] = idx

    for edge in context_edges:
        if len(edge) < 2:
            continue
        src_id, dst_id = edge[0], edge[1]
        if src_id in type_to_idx and dst_id in type_to_idx:
            i = type_to_idx[src_id]
            j = type_to_idx[dst_id]
            if i < max_nodes and j < max_nodes:
                adj[i][j] = 1.0

    return adj


def build_node_features(context_node_ids: list[int],
                        context_edges: list[list[int]],
                        max_nodes: int = 7) -> tuple[np.ndarray, np.ndarray]:
    """
    构建节点特征矩阵和节点掩码。

    返回：
      node_features: [max_nodes, 20]
      node_mask:     [max_nodes]  (1=有效, 0=padding)
    """
    node_features = np.zeros((max_nodes, 20), dtype=np.float32)
    node_mask     = np.zeros(max_nodes, dtype=np.float32)

    # 计算每个节点的入度/出度（基于 type_id）
    in_deg:  dict[int, int] = {}
    out_deg: dict[int, int] = {}
    for edge in context_edges:
        if len(edge) < 2:
            continue
        src, dst = edge[0], edge[1]
        out_deg[src] = out_deg.get(src, 0) + 1
        in_deg[dst]  = in_deg.get(dst, 0) + 1

    nodes = context_node_ids[:max_nodes]
    for idx, nid in enumerate(nodes):
        node_features[idx] = encode_node(
            node_type_id=nid,
            in_degree=in_deg.get(nid, 0),
            out_degree=out_deg.get(nid, 0),
            depth=idx,
        )
        node_mask[idx] = 1.0

    return node_features, node_mask


def build_history_features(history_sequence: list[int],
                           history_len: int = 5,
                           step_dim: int = 20) -> np.ndarray:
    """
    构建历史序列特征矩阵 [history_len, step_dim]，不足时用零填充。
    """
    hist = np.zeros((history_len, step_dim), dtype=np.float32)
    seq = history_sequence[-history_len:]  # 取最近 history_len 步
    for i, nid in enumerate(seq):
        hist[i] = encode_history_step(nid)
    return hist
