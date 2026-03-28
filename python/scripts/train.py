import torch
import torch.nn as nn
import yaml
from pcg_predict.model.predictor import PCGBidirectionalPredictor

def train():
    with open('../config/train_config.yaml') as f:
        config = yaml.safe_load(f)

    model = PCGBidirectionalPredictor(config['model']['num_node_types'])
    optimizer = torch.optim.AdamW(model.parameters(), lr=config['training']['lr'])
    criterion = nn.CrossEntropyLoss()

    print("Training started...")

if __name__ == '__main__':
    train()
