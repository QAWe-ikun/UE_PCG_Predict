import torch
import torch.nn as nn
import torch.nn.functional as F
from torch_geometric.nn import SAGEConv

class GNNEncoder(nn.Module):
    def __init__(self, in_dim=20, hidden_dim=64):
        super().__init__()
        self.conv1 = SAGEConv(in_dim, hidden_dim)
        self.conv2 = SAGEConv(hidden_dim, hidden_dim)
    
    def forward(self, x, edge_index):
        if x.size(0) == 0:
            return torch.zeros(64)
        x = F.relu(self.conv1(x, edge_index))
        x = self.conv2(x, edge_index)
        return x.mean(dim=0)
