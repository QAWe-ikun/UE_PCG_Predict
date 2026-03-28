from setuptools import setup, find_packages

setup(
    name="pcg_predict",
    version="0.1.0",
    packages=find_packages(),
    install_requires=[
        "torch>=2.0",
        "torch-geometric>=2.4",
        "faiss-cpu>=1.7",
        "onnx>=1.14",
        "onnxruntime>=1.15",
        "pyyaml>=6.0",
    ],
)
