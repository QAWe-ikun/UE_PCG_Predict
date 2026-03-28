import torch
import torch.nn as nn

class IntentProjector(nn.Module):
    def __init__(self, intent_dim=64, proj_dim=32):
        super().__init__()
        self.proj = nn.Linear(intent_dim, proj_dim)
    
    def forward(self, intent_vector):
        if intent_vector is None:
            return torch.zeros(32)
        return self.proj(intent_vector)
