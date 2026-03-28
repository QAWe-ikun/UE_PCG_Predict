import torch

SCENE_CLASSES = ['Forest', 'City', 'Building', 'Stall', 'Mountain', 'Water', 'Road', 'Cave', 'Desert', 'Custom']
MODIFIER_CLASSES = ['Dense', 'Sparse', 'Uneven', 'Random', 'Aligned', 'AvoidPath', 'NearWater']

class IntentParser:
    def parse(self, text):
        text_lower = text.lower()
        scene_scores = torch.zeros(10)
        for i, scene in enumerate(SCENE_CLASSES):
            if scene.lower() in text_lower:
                scene_scores[i] = 1.0
        
        modifier_scores = torch.zeros(54)
        for i, mod in enumerate(MODIFIER_CLASSES):
            if mod.lower() in text_lower:
                modifier_scores[i] = 1.0
        
        intent_vector = torch.zeros(64)
        intent_vector[:10] = torch.softmax(scene_scores, dim=-1)
        intent_vector[10:17] = modifier_scores[:7]
        
        return {
            'scene': SCENE_CLASSES[scene_scores.argmax().item()],
            'modifiers': [MODIFIER_CLASSES[i] for i in range(len(MODIFIER_CLASSES)) if modifier_scores[i] > 0],
            'vector': intent_vector,
            'confidence': scene_scores.max().item()
        }
