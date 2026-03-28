import torch
from pcg_predict.model.predictor import PCGBidirectionalPredictor

def export_onnx():
    model = PCGBidirectionalPredictor(50)
    model.eval()

    print("Exporting to ONNX...")
    torch.onnx.export(model, (None, None, None),
                      '../plugin/PCGPredict/Resources/Models/pcg_predictor.onnx')
    print("Export completed")

if __name__ == '__main__':
    export_onnx()
