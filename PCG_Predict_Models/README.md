# PCG_Predict_Models

PCG Graph 节点预测模型的训练管线，包含快速统计模型和深度神经网络模型两套方案。

## 目录结构

```
PCG_Predict_Models/
├── config/
│   ├── node_registry.json      # 195 个 PCG 节点定义（id/name/class/category/input_types/output_types）
│   └── train_config.yaml       # 训练超参数配置
├── data/
│   ├── raw/                    # 原始 PCG Graph JSON 文件（用户提供）
│   └── samples.jsonl           # 提取的训练样本（由 data_collector.py 生成）
├── models/
│   ├── fast_lookup.json        # FastPredictor 统计查找表
│   └── deep_predictor.onnx     # DeepPredictor ONNX 模型
└── src/
    ├── data_collector.py       # 从 raw JSON 提取训练样本
    ├── build_fast_lookup.py    # 构建 n-gram 统计查找表
    ├── features.py             # 特征编码函数
    ├── graph_transformer.py    # Graph Transformer 模型定义
    ├── train_deep.py           # 深度模型训练脚本
    └── export_onnx.py          # 导出 ONNX 模型
```

## 环境安装

```bash
pip install -r requirements.txt
```

PyTorch 建议使用 CUDA 版本以加速训练：

```bash
pip install torch --index-url https://download.pytorch.org/whl/cu121
pip install torch-geometric
```

## 两套模型

### FastPredictor（统计模型）

基于节点共现的 n-gram 查找表，推理时间 < 1ms，无需 GPU。

原理：统计训练样本中节点转移概率，构建三级查找表：
- **unigram**：`P(next | last_node)`
- **bigram**：`P(next | last_2_nodes)`
- **trigram**：`P(next | last_3_nodes)`
- **starter**：空图时的先验分布

### DeepPredictor（神经网络模型）

基于 Graph Transformer 的深度模型，在后台线程异步推理。

架构：
```
node_features [1,7,20] ─→ GraphTransformerLayer×2 ─→ masked mean ─→ [64]
history_seq   [1,5,20] ─→ TransformerEncoder      ─→ last token  ─→ [64]
intent_vector [1,64]   ─→ Linear                  ─→             ─→ [32]
cursor_feat   [1,24]   ─→ Linear                  ─→             ─→ [32]
                                    cat([64,64,32,32]=192)
                                    ─→ Linear(192→128) → ReLU → Linear(128→128)
                                    ─→ forward_head 或 reverse_head
                                    ─→ logits [1, 195]
```

## 使用流程

### 1. 准备原始数据

将 PCG Graph 导出为 JSON 格式，放入 `data/raw/`：

```json
{
  "graph_id": "graph_001",
  "intent_hint": "dense forest",
  "nodes": [
    {"node_uid": "n0", "node_type_id": 87, "node_type_name": "SurfaceSampler", "pos_x": 0, "pos_y": 0}
  ],
  "edges": [
    {"src_uid": "n0", "src_pin": 0, "dst_uid": "n1", "dst_pin": 0, "data_type": "Point"}
  ]
}
```

### 2. 提取训练样本

```bash
cd PCG_Predict_Models
python src/data_collector.py
# 输出：data/samples.jsonl
```

### 3. 构建 FastPredictor 查找表

```bash
python src/build_fast_lookup.py
# 输出：models/fast_lookup.json
```

### 4. 训练 DeepPredictor

```bash
python src/train_deep.py
# 输出：models/deep_predictor_best.pt
```

### 5. 导出 ONNX

```bash
python src/export_onnx.py
# 输出：models/deep_predictor.onnx
```

## 训练样本格式

`data/samples.jsonl` 每行一个 JSON：

```json
{
  "sample_id": "graph_001_step_1",
  "graph_id": "graph_001",
  "context_node_ids": [87, 36],
  "context_edges": [[87, 36]],
  "history_sequence": [87, 36],
  "cursor_pin_direction": "output",
  "cursor_pin_data_type": "Point",
  "intent_hint": "dense forest",
  "target_node_id": 100
}
```

字段说明：

| 字段 | 说明 |
|------|------|
| `context_node_ids` | 当前局部子图的节点 type_id（最多 7 个） |
| `context_edges` | 边列表 `[src_type_id, dst_type_id]` |
| `history_sequence` | 按添加顺序的节点 type_id 序列（最多 5 步） |
| `cursor_pin_direction` | 当前悬停 Pin 的方向（input/output） |
| `cursor_pin_data_type` | 当前 Pin 的数据类型 |
| `intent_hint` | 用户意图描述（可为空） |
| `target_node_id` | 监督标签，下一个应添加的节点 type_id |

## ONNX 模型规格

| 输入名 | 形状 | 说明 |
|--------|------|------|
| `node_features` | `[1, 7, 20]` | 局部子图节点特征 |
| `adj_matrix` | `[1, 7, 7]` | 有向邻接矩阵 |
| `history_seq` | `[1, 5, 20]` | 历史操作序列 |
| `intent_vector` | `[1, 64]` | 意图向量 |
| `cursor_features` | `[1, 24]` | Cursor 特征 |
| `node_mask` | `[1, 7]` | 有效节点掩码（1=有效，0=padding） |
| `direction_flag` | `[1, 1]` | 预测方向（0=forward，1=reverse） |
| **输出** `logits` | `[1, 195]` | 195 个节点类型的预测分数 |

## 配置说明

`config/train_config.yaml` 关键参数：

```yaml
model:
  num_node_types: 195   # PCG 节点总数
  fusion_dim: 128       # 融合层维度

training:
  lr: 0.001
  batch_size: 128
  epochs: 100
  early_stopping_patience: 10
```
