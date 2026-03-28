import torch

DATA_TYPE_MAP = {
    'Point': 0, 'Spatial': 1, 'Param': 2, 'Execution': 3,
    'Concrete': 4, 'Landscape': 5, 'Spline': 6, 'Volume': 7
}

def encode_cursor(pin_type, data_type, owner_node_id):
    """编码光标特征 24维"""
    cursor = torch.zeros(24)
    cursor[pin_type] = 1  # 2维 one-hot
    cursor[2 + DATA_TYPE_MAP[data_type]] = 1  # 8维 one-hot
    cursor[10:24] = torch.randn(14) * 0.1  # 14维节点嵌入占位
    return cursor

def encode_node(node_type_id, topology_features):
    """编码节点特征 20维"""
    node_feat = torch.zeros(20)
    node_feat[0] = node_type_id / 50.0  # 归一化
    node_feat[1:] = topology_features[:19]
    return node_feat

def encode_history_step(action_type, node_type_id):
    """编码历史步骤 20维"""
    step = torch.zeros(20)
    step[0] = action_type / 10.0
    step[1] = node_type_id / 50.0
    return step
