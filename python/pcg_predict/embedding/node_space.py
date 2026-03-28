import torch
import torch.nn.functional as F
import faiss
import pickle

class NodeEmbeddingSpace:
    def __init__(self, dim=128):
        self.dim = dim
        self.embeddings = {}
        self.index = faiss.IndexFlatIP(dim)
    
    def add_node(self, node_id, embedding):
        self.embeddings[node_id] = embedding
        self.index.add(embedding.unsqueeze(0).numpy())
    
    def search(self, query_vector, k=10):
        scores, indices = self.index.search(query_vector.unsqueeze(0).numpy(), k)
        return [(list(self.embeddings.keys())[i], scores[0][j]) for j, i in enumerate(indices[0])]
    
    def save(self, path):
        with open(path, 'wb') as f:
            pickle.dump({'embeddings': self.embeddings, 'dim': self.dim}, f)
    
    def load(self, path):
        with open(path, 'rb') as f:
            data = pickle.load(f)
            self.embeddings = data['embeddings']
            self.dim = data['dim']
