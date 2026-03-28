import torch
from pcg_predict.model.predictor import PCGBidirectionalPredictor
from torch_geometric.data import Data

def test_model_forward():
    model = PCGBidirectionalPredictor(50)
    graph = Data(x=torch.randn(5, 20), edge_index=torch.tensor([[0,1],[1,2]]))
    history = torch.randn(5, 20)
    logits, embedding = model(graph, history)
    assert logits.shape == (50,)
    assert embedding.shape == (128,)
    print("Model test passed")

if __name__ == '__main__':
    test_model_forward()
