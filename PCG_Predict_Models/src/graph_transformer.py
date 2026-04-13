"""
graph_transformer.py
DeepPredictor 模型定义：Graph Transformer + 历史编码器 + 意图投影。

所有输入均为固定形状张量，可直接导出 ONNX（opset 17）。
"""

import torch
import torch.nn as nn
import torch.nn.functional as F


class GraphTransformerLayer(nn.Module):
    """
    单层 Graph Transformer。
    使用邻接矩阵作为注意力偏置，保留图结构信息。
    全部使用标准 PyTorch 算子，ONNX 可导出。
    """

    def __init__(self, d_model: int = 64, nhead: int = 4, dropout: float = 0.1):
        super().__init__()
        self.nhead   = nhead
        self.d_model = d_model

        self.attn  = nn.MultiheadAttention(d_model, nhead, dropout=dropout, batch_first=True)
        self.norm1 = nn.LayerNorm(d_model)
        self.ffn   = nn.Sequential(
            nn.Linear(d_model, d_model * 2),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(d_model * 2, d_model),
        )
        self.norm2 = nn.LayerNorm(d_model)

    def forward(self,
                x: torch.Tensor,          # [B, N, d_model]
                adj: torch.Tensor,        # [B, N, N]  有向邻接矩阵
                node_mask: torch.Tensor,  # [B, N]     1=有效, 0=padding
                ) -> torch.Tensor:
        B, N, _ = x.shape

        # key_padding_mask: True 表示忽略该位置（padding 节点）
        key_padding_mask = (node_mask == 0)  # [B, N]

        # 邻接矩阵作为注意力偏置：[B, N, N] → [B*nhead, N, N]
        # 有边的位置加正偏置，无边的位置加负偏置（引导注意力沿图结构流动）
        attn_bias = adj.unsqueeze(1).expand(B, self.nhead, N, N)  # [B, nhead, N, N]
        attn_bias = attn_bias.reshape(B * self.nhead, N, N)       # [B*nhead, N, N]
        # 将 0/1 映射到 0/-inf 偏置（只有边存在时才给正向偏置）
        attn_bias = attn_bias * 2.0 - 1.0  # 0→-1, 1→+1（软偏置，不用 -inf 避免梯度问题）

        attn_out, _ = self.attn(
            x, x, x,
            key_padding_mask=key_padding_mask,
            attn_mask=attn_bias,
        )
        x = self.norm1(x + attn_out)
        x = self.norm2(x + self.ffn(x))
        return x


class PCGGraphTransformerPredictor(nn.Module):
    """
    PCG Graph Transformer 预测器。

    输入（全部固定形状，ONNX 友好）：
      node_features   [B, max_nodes=7, node_feat_dim=20]
      adj_matrix      [B, max_nodes=7, max_nodes=7]
      history_seq     [B, history_len=5, step_dim=20]
      intent_vector   [B, intent_dim=64]
      cursor_features [B, cursor_dim=24]
      node_mask       [B, max_nodes=7]   1=有效节点, 0=padding
      direction_flag  [B, 1]             0=forward(预测下游), 1=reverse(预测上游)

    输出：
      logits  [B, num_node_types=195]
    """

    def __init__(
        self,
        num_node_types: int = 195,
        max_nodes:      int = 7,
        node_feat_dim:  int = 20,
        history_len:    int = 5,
        step_dim:       int = 20,
        intent_dim:     int = 64,
        cursor_dim:     int = 24,
        d_model:        int = 64,
        nhead:          int = 4,
        gt_layers:      int = 2,
        hist_layers:    int = 2,
        dropout:        float = 0.1,
    ):
        super().__init__()
        self.num_node_types = num_node_types

        # 节点特征投影
        self.node_proj = nn.Linear(node_feat_dim, d_model)

        # Graph Transformer 层
        self.gt_layers = nn.ModuleList([
            GraphTransformerLayer(d_model, nhead, dropout)
            for _ in range(gt_layers)
        ])

        # 历史序列编码
        self.history_proj = nn.Linear(step_dim, d_model)
        hist_layer = nn.TransformerEncoderLayer(
            d_model=d_model, nhead=nhead, batch_first=True,
            dim_feedforward=d_model * 2, dropout=dropout,
        )
        self.history_encoder = nn.TransformerEncoder(hist_layer, num_layers=hist_layers)

        # 意图投影
        self.intent_proj = nn.Linear(intent_dim, 32)

        # Cursor 特征投影
        self.cursor_proj = nn.Linear(cursor_dim, 32)

        # 融合层：d_model(graph) + d_model(history) + 32(intent) + 32(cursor)
        fusion_in = d_model + d_model + 32 + 32  # = 192
        self.fusion = nn.Sequential(
            nn.Linear(fusion_in, 128),
            nn.ReLU(),
            nn.Dropout(dropout),
            nn.Linear(128, 128),
        )

        # 双向预测头
        self.forward_head = nn.Linear(128, num_node_types)
        self.reverse_head = nn.Linear(128, num_node_types)

    def forward(
        self,
        node_features:   torch.Tensor,  # [B, 7, 20]
        adj_matrix:      torch.Tensor,  # [B, 7, 7]
        history_seq:     torch.Tensor,  # [B, 5, 20]
        intent_vector:   torch.Tensor,  # [B, 64]
        cursor_features: torch.Tensor,  # [B, 24]
        node_mask:       torch.Tensor,  # [B, 7]
        direction_flag:  torch.Tensor,  # [B, 1]
    ) -> torch.Tensor:                  # [B, 195]

        # --- Graph Transformer 编码 ---
        x = self.node_proj(node_features)  # [B, N, d_model]
        for layer in self.gt_layers:
            x = layer(x, adj_matrix, node_mask)

        # Masked mean pooling（忽略 padding 节点）
        mask_exp  = node_mask.unsqueeze(-1)                          # [B, N, 1]
        graph_repr = (x * mask_exp).sum(dim=1) / (mask_exp.sum(dim=1) + 1e-8)  # [B, d_model]

        # --- 历史序列编码 ---
        h = self.history_proj(history_seq)   # [B, L, d_model]
        h = self.history_encoder(h)
        history_repr = h[:, -1, :]           # [B, d_model]（取最后一步）

        # --- 意图 & Cursor ---
        intent_repr = self.intent_proj(intent_vector)    # [B, 32]
        cursor_repr = self.cursor_proj(cursor_features)  # [B, 32]

        # --- 融合 ---
        fused = self.fusion(
            torch.cat([graph_repr, history_repr, intent_repr, cursor_repr], dim=-1)
        )  # [B, 128]

        # --- 双向预测头（ONNX 兼容的软选择，避免 if/else）---
        forward_logits = self.forward_head(fused)  # [B, num_node_types]
        reverse_logits = self.reverse_head(fused)

        # direction_flag: 0=forward, 1=reverse
        flag = direction_flag  # [B, 1]
        logits = forward_logits * (1.0 - flag) + reverse_logits * flag  # [B, num_node_types]

        return logits
