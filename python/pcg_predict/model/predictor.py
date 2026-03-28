import torch
import torch.nn as nn
import torch.nn.functional as F
from .gnn_encoder import GNNEncoder
from .history_encoder import HistoryEncoder
from .intent_projector import IntentProjector

class PCGBidirectionalPredictor(nn.Module):
    def __init__(self, num_node_types=50):
        super().__init__()
        self.gnn = GNNEncoder(20, 64)
        self.history = HistoryEncoder(20, 64, 4, 2)
        self.intent = IntentProjector(64, 32)
        self.fusion = nn.Sequential(
            nn.Linear(160, 128),
            nn.ReLU(),
            nn.Linear(128, 128)
        )
        self.reverse_head = nn.Linear(128, num_node_types)
        self.forward_head = nn.Linear(128, num_node_types)
    
    def forward(self, local_graph, history, intent=None, direction='forward'):
        gnn_out = self.gnn(local_graph.x, local_graph.edge_index)
        trans_out = self.history(history)
        intent_out = self.intent(intent)
        fused = F.normalize(self.fusion(torch.cat([gnn_out, trans_out, intent_out])), p=2, dim=-1)
        logits = self.reverse_head(fused) if direction == 'reverse' else self.forward_head(fused)
        return logits, fused
