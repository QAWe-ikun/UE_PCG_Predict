import torch
import torch.nn.functional as F

class ConstraintDecoder:
    def __init__(self, node_registry):
        self.registry = node_registry
    
    def build_type_mask(self, pin_data_type, direction):
        mask = torch.ones(50)
        return mask
    
    def decode(self, raw_logits, pin_data_type, direction, intent_vector=None, k=10):
        mask = self.build_type_mask(pin_data_type, direction)
        logits = raw_logits * mask
        if intent_vector is not None:
            logits = logits * 1.15
        scores = F.softmax(logits, dim=-1)
        top_k = torch.topk(scores, k)
        return [{'node_id': i.item(), 'score': s.item()} for i, s in zip(top_k.indices, top_k.values)]
