import torch
import torch.nn as nn

class HistoryEncoder(nn.Module):
    def __init__(self, step_dim=20, hidden_dim=64, nhead=4, num_layers=2):
        super().__init__()
        self.proj = nn.Linear(step_dim, hidden_dim)
        encoder_layer = nn.TransformerEncoderLayer(d_model=hidden_dim, nhead=nhead, batch_first=True)
        self.transformer = nn.TransformerEncoder(encoder_layer, num_layers=num_layers)
    
    def forward(self, history_seq):
        x = self.proj(history_seq)
        x = self.transformer(x.unsqueeze(0))
        return x[:, -1, :].squeeze(0)
