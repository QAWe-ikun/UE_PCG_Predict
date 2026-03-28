import torch
from pcg_predict.model.predictor import PCGBidirectionalPredictor

def evaluate():
    model = PCGBidirectionalPredictor(50)
    model.eval()

    print("Evaluation started...")
    print("Top-1/3/5 accuracy: TBD")

if __name__ == '__main__':
    evaluate()
