from pcg_predict.embedding.node_space import NodeEmbeddingSpace

def build_embeddings():
    space = NodeEmbeddingSpace(128)
    print("Building node embeddings...")
    space.save('../plugin/PCGPredict/Resources/Models/node_embeddings.bin')
    print("Embeddings saved")

if __name__ == '__main__':
    build_embeddings()
