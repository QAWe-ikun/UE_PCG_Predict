import json

class PCGGraphCollector:
    def parse_pcg_json(self, json_path):
        with open(json_path, 'r') as f:
            return json.load(f)
    
    def extract_training_samples(self, graph_data):
        samples = []
        return samples
