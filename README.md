# UE5 PCG Graph 智能预测系统

**版本**: v4.1-final
**日期**: 2026-03-28
**定位**: UE5 原生编辑器插件，深度集成 PCG Graph

---

## 📋 目录

1. [项目概述](#1-项目概述)
   - [1.1 背景](#11-背景)
   - [1.2 解决方案](#12-解决方案)
   - [1.3 核心指标](#13-核心指标)
2. [快速开始](#2-快速开始)
   - [2.1 环境要求](#21-环境要求)
   - [2.2 安装步骤](#22-安装步骤)
3. [核心设计原则](#3-核心设计原则)
   - [3.1 交互范式](#31-交互范式)
   - [3.2 双源候选机制](#32-双源候选机制)
   - [3.3 关键决策](#33-关键决策)
4. [系统架构](#4-系统架构)
   - [4.1 整体架构](#41-整体架构)
   - [4.2 编辑器层](#42-编辑器层)
   - [4.3 预测引擎层](#43-预测引擎层)
5. [神经网络模型](#5-神经网络模型)
   - [5.1 输入特征](#51-输入特征)
   - [5.2 模型结构](#52-模型结构)
   - [5.3 训练配置](#53-训练配置)
6. [双向预测机制](#6-双向预测机制)
   - [6.1 反向预测](#61-反向预测)
   - [6.2 正向预测](#62-正向预测)
   - [6.3 多连接场景处理](#63-多连接场景处理)
7. [意图驱动层](#7-意图驱动层)
   - [7.1 意图解析](#71-意图解析)
   - [7.2 意图到技术配置映射](#72-意图到技术配置映射)
8. [向量空间与动态扩展](#8-向量空间与动态扩展)
   - [8.1 节点嵌入空间](#81-节点嵌入空间)
   - [8.2 在线适应](#82-在线适应)
9. [UI/UX 设计](#9-uiux 设计)
   - [9.1 预测浮层](#91-预测浮层)
   - [9.2 键盘导航](#92-键盘导航)
   - [9.3 意图输入](#93-意图输入)
   - [9.4 细节面板嵌入](#94-细节面板嵌入)
   - [9.5 多连接 UI 修正](#95-多连接 ui 修正)
10. [Debug 集成](#10-debug 集成)
    - [10.1 定位](#101-定位)
    - [10.2 操作](#102-操作)
    - [10.3 代码实现](#103-代码实现)
11. [实施与交付](#11-实施与交付)
    - [11.1 开发路线图](#111-开发路线图)
    - [11.2 交付清单](#112-交付清单)
    - [11.3 验收标准](#113-验收标准)
12. [未来扩展](#12-未来扩展)
    - [12.1 意图驱动的参数匹配](#121-意图驱动的参数匹配)
    - [12.2 技术实现](#122-技术实现)
    - [12.3 UI 展示](#123-ui 展示)
    - [12.4 参数学习机制](#124-参数学习机制)
    - [12.5 与其他扩展的关系](#125-与其他扩展的关系)
13. [总结](#13-总结)

---

# 1. 项目概述

## 1.1 背景

UE5 PCG（Procedural Content Generation）系统采用可视化节点图架构，开发者面临以下痛点：

| 痛点 | 影响 |
|-----|------|
| 节点类型繁多（187+） | 学习成本高，选择困难 |
| 类型系统复杂（Concrete→Spatial→Point） | 频繁的类型不匹配错误 |
| 缺乏智能辅助 | 从想法到可运行 Graph 周期长 |
| 数据流调试困难 | 中间状态不可见，问题定位难 |

## 1.2 解决方案

构建**原生集成的智能预测系统**，具备以下核心能力：

- **双向预测**：输入接口预测上游，输出接口预测下游
- **双源候选**：推荐创建新节点 + 连接已有节点
- **意图驱动**：自然语言描述指导推荐优先级
- **环路检测**：防止循环连接，保证 DAG 结构
- **向量空间**：节点语义嵌入，支持动态扩展
- **敏捷调试**：预测浮层内置 Debug 节点快捷入口

## 1.3 核心指标

| 指标 | 目标 |
|-----|------|
| 预测精度 | Top-3 采纳率 > 90% |
| 响应延迟 | P95 < 50ms |
| 启动时间 | 意图→首个可运行 Graph < 30 秒 |
| 新增节点 | 无需重训练，< 100ms 生效 |

---

# 2. 快速开始

## 2.1 环境要求

- **Python**: 3.8+
- **UE5**: 5.4+
- **依赖**: PyTorch 2.0+, PyG 2.4+, FAISS, ONNX Runtime

## 2.2 安装步骤

### Step 1: 克隆项目

```bash
git clone https://github.com/your-repo/UE_PCG_Predict.git
cd UE_PCG_Predict
```

### Step 2: 安装 Python 依赖

```bash
cd python
pip install -r requirements.txt
```

### Step 3: 导出 PCG 节点信息

将 `ExportPCGNodes.cpp` 添加到你的 UE5 项目：

```cpp
// 在项目 Build.cs 中添加 PCG 依赖
PublicDependencyModuleNames.AddRange(new string[] {
    "Core", "CoreUObject", "Engine", "PCG"
});
```

编译并在编辑器中调用 `ExportPCGNodes()` 生成节点数据。

### Step 4: 训练模型

```bash
cd python
python scripts/train.py
python scripts/export_onnx.py
```

### Step 5: 安装插件

```bash
# 复制插件到 UE5 项目
cp -r PCG_Predict /path/to/YourProject/Plugins/
```

重新生成项目文件并编译。

---

# 3. 核心设计原则

## 3.1 交互范式

```
┌─────────────────────────────────────────┐
│           三种核心交互场景               │
├─────────────────────────────────────────┤
│                                         │
│  场景 1: 空白图启动（意图驱动）             │
│  用户："生成茂密森林"                      │
│  系统：推荐首个源节点（SurfaceSampler 等）    │
│                                         │
│  场景 2: 局部补全（反向预测）                │
│  光标：[节点].InputPin(Point)             │
│  系统：推荐上游节点（源节点/转换节点）        │
│                                         │
│  场景 3: 后续扩展（正向预测）                │
│  光标：[节点].OutputPin(Point)            │
│  系统：推荐下游节点（处理/生成节点）          │
│                                         │
└─────────────────────────────────────────┘
```

## 3.2 双源候选机制

预测候选包含两类来源，统一排序：

1. **创建新节点** (CreateNew)
   - 神经网络预测的新节点类型
   - 插入后自动连线

2. **连接已有节点** (ConnectExisting)
   - 扫描当前 Graph 中已存在的兼容节点
   - 空输入接口 → 搜索已有节点的兼容输出接口
   - 输出接口 → 搜索已有节点的兼容输入接口
   - 环路检测：防止形成循环连接

## 3.3 关键决策

| 决策 | 选择 | 理由 |
|-----|------|------|
| 预测方向 | 双向（输入/输出接口） | 覆盖完整工作流 |
| 候选来源 | 双源（新建 + 已有） | 提升连接效率 |
| 预测粒度 | 节点级 | 平衡自动化与用户控制 |
| 意图输入 | 可选自然语言 | 提升但不强制 |
| 向量空间 | 神经网络嵌入 | 语义相似度，支持扩展 |
| Debug 方案 | 预测浮层底部固定入口 | 复用 UE5 原生，不参与预测 |

---

# 4. 系统架构

## 4.1 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                         UE5 编辑器层                              │
│  ┌─────────────────────────────────────────────────────────────┐ │
│  │                    PCG Graph 编辑器                           │ │
│  │  ┌─────────┐    ┌─────────┐    ┌─────────────────────────┐ │ │
│  │  │ 工具栏   │    │ Graph 画布│    │ 细节面板 (Details)      │ │ │
│  │  │[🔮预测▼]│    │ 可视化  │    │ 🎯意图显示/修改        │ │ │
│  │  └─────────┘    └─────────┘    └─────────────────────────┘ │ │
│  │                              │                             │ │
│  │  ┌─────────────────────────────────────────────────────────┐│ │
│  │  │              预测浮层 (Slate 原生)                      ││ │
│  │  │  ┌─────────────────────────────────────────────────┐  ││ │
│  │  │  │ 🔮 预测结果 (Top-K 工作节点)                      │  ││ │
│  │  │  │ 1. TransformPoints      94%  [左键插入]          │  ││ │
│  │  │  │ 2. DensityFilter        88%  [左键插入]          │  ││ │
│  │  │  │ ...                                              │  ││ │
│  │  │  ├─────────────────────────────────────────────────┤  ││ │
│  │  │  │ 🔍 插入 Debug 节点 [左键]  (UE5 原生 PCGDebug)        │  ││ │
│  │  │  └─────────────────────────────────────────────────┘  ││ │
│  │  └─────────────────────────────────────────────────────────┘│ │
│  └─────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                      智能预测引擎层                              │
│                                                                 │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    共享编码器 (Shared Encoder)               ││
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────────┐ ││
│  │  │   GNN 层      │  │  Transformer │  │    意图投影层        │ ││
│  │  │  (GraphSAGE) │  │   (4 层)      │  │    (MLP)            │ ││
│  │  │  64 维输出   │  │  64 维输出   │  │    32 维输出         │ ││
│  │  └─────────────┘  └─────────────┘  └─────────────────────┘ ││
│  │              ↓              ↓                  ↓            ││
│  │              └─────────────┬─────────────────┘            ││
│  │                            ▼                               ││
│  │                 ┌─────────────────────┐                    ││
│  │                 │   融合层 (128 维)     │                    ││
│  │                 │   L2 归一化输出       │                    ││
│  │                 └─────────────────────┘                    ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    双向预测头 (Bidirectional Heads)         ││
│  │  ┌─────────────────────┐  ┌─────────────────────┐            ││
│  │  │    反向预测头         │  │    正向预测头         │            ││
│  │  │  (Reverse Head)      │  │  (Forward Head)      │            ││
│  │  │  输入接口→上游节点    │  │  输出接口→下游节点    │            ││
│  │  └─────────────────────┘  └─────────────────────┘            ││
│  └─────────────────────────────────────────────────────────────┘│
│                              │                                  │
│                              ▼                                  │
│  ┌─────────────────────────────────────────────────────────────┐│
│  │                    约束解码层                                ││
│  │  • 类型约束掩码  • 转换节点处理  • 意图加权  • Top-K 排序       ││
│  └─────────────────────────────────────────────────────────────┘│
└─────────────────────────────────────────────────────────────────┘
```

## 4.2 编辑器层

编辑器层包含以下组件：

- **工具栏**：预测功能入口按钮
- **Graph 画布**：可视化节点编辑区域
- **细节面板**：意图显示与修改
- **预测浮层**：Slate 原生的预测结果展示 UI

## 4.3 预测引擎层

预测引擎层包含以下模块：

- **共享编码器**：GNN + Transformer + 意图投影，生成统一特征表示
- **双向预测头**：反向预测头（上游）和正向预测头（下游）
- **约束解码层**：类型约束、意图加权、Top-K 排序

---

# 5. 神经网络模型

## 5.1 输入特征

```python
INPUT_SPEC = {
    # 1. 光标上下文 (24 维)
    'cursor': {
        'pin_type': 2,        # Input/Output
        'data_type': 8,       # Point/Spatial/Param/Execution/Concrete/Landscape/Spline/Volume
        'owner_node': 14,     # 当前节点类型嵌入
    },

    # 2. 局部拓扑 (GNN 输入，变长→64 维)
    'local_graph': {
        'max_nodes': 7,
        'node_features': 20,  # 类型 + 拓扑 + 数据类型
        'edge_index': [2, 20],
    },

    # 3. 历史序列 (Transformer 输入，5 步→64 维)
    'history': {
        'sequence_length': 5,
        'step_features': 20,
    },

    # 4. 意图向量 (可选，64→32 维)
    'intent': {
        'scene_type': 10,     # Forest/City/Building/Stall/...
        'modifiers': 54,      # Dense/Sparse/Uneven/...
    }
}
```

## 5.2 模型结构

```python
class PCGBidirectionalPredictor(nn.Module):
    def __init__(self, num_node_types=50):
        super().__init__()

        # 共享编码器
        self.gnn_conv1 = SAGEConv(20, 64)
        self.gnn_conv2 = SAGEConv(64, 64)

        self.history_proj = nn.Linear(20, 64)
        self.transformer = nn.TransformerEncoder(
            nn.TransformerEncoderLayer(d_model=64, nhead=4, batch_first=True),
            num_layers=2
        )

        self.intent_proj = nn.Sequential(
            nn.Linear(64, 32)
        )

        self.fusion = nn.Sequential(
            nn.Linear(64+64+32, 128),
            nn.ReLU(),
            nn.Linear(128, 128)
        )

        # 双向预测头
        self.reverse_head = nn.Linear(128, num_node_types)   # 输入接口→上游
        self.forward_head = nn.Linear(128, num_node_types)   # 输出接口→下游

    def forward(self, local_graph, history, intent=None, direction='forward'):
        # GNN 编码
        x = F.relu(self.gnn_conv1(local_graph.x, local_graph.edge_index))
        x = self.gnn_conv2(x, local_graph.edge_index)
        gnn_out = x.mean(dim=0)  # [64]

        # Transformer 编码
        h = self.history_proj(history)  # [5, 64]
        h = self.transformer(h.unsqueeze(0))
        trans_out = h[:, -1, :].squeeze(0)  # [64]

        # 意图编码
        intent_out = self.intent_proj(intent) if intent else torch.zeros(32)

        # 融合
        fused = F.normalize(self.fusion(torch.cat([gnn_out, trans_out, intent_out])), p=2, dim=-1)

        # 方向选择
        logits = self.reverse_head(fused) if direction == 'reverse' else self.forward_head(fused)

        return logits, fused
```

## 5.3 训练配置

| 配置项 | 值 |
|-------|---|
| 优化器 | AdamW (lr=1e-3, weight_decay=0.01) |
| 学习率调度 | Cosine annealing, warm-up 100 steps |
| Batch size | 128 |
| Epochs | 100 (early stopping patience=10) |
| 损失函数 | CrossEntropy + TripletMargin |

---

# 6. 双向预测机制

## 6.1 反向预测（光标在输入接口）

```python
def predict_reverse(target_data_type, context, intent_vector, k=10):
    """
    预测上游节点（数据源）
    候选：源节点 + 转换路径
    """
    candidates = []

    # 直接源节点
    for node in get_source_nodes_with_output(target_data_type):
        candidates.append({
            'node_type': node,
            'connection': 'direct',
            'cost': 0,
            'base_score': compute_score(node, context)
        })

    # 转换路径
    for source_type in get_convertible_types(target_data_type):
        converter = get_converter(source_type, target_data_type)
        for up_node in get_nodes_with_output(source_type):
            candidates.append({
                'node_type': up_node,
                'connection': 'converted',
                'converter': converter,
                'cost': 1,
                'base_score': compute_score(up_node, context) * 0.85
            })

    # 意图加权
    for cand in candidates:
        cand['final_score'] = cand['base_score'] * (1 + compute_intent_bonus(cand['node_type'], intent_vector))

    return sorted(candidates, key=lambda x: (x['cost'], -x['final_score']))[:k]
```

## 6.2 正向预测（光标在输出接口）

```python
def predict_forward(source_data_type, context, intent_vector, k=10):
    """
    预测下游节点（处理/生成）
    候选：直接兼容节点 + 转换后兼容节点
    """
    candidates = []

    # 直接兼容
    for node in get_nodes_with_input(source_data_type):
        candidates.append({
            'node_type': node,
            'connection': 'direct',
            'cost': 0,
            'base_score': compute_score(node, context)
        })

    # 转换后兼容
    for target_type in get_convertible_targets(source_data_type):
        converter = get_converter(source_data_type, target_type)
        for down_node in get_nodes_with_input(target_type):
            candidates.append({
                'node_type': down_node,
                'connection': 'converted',
                'converter': converter,
                'cost': 1,
                'base_score': compute_score(down_node, context) * 0.85
            })

    # 意图加权（同反向）
    for cand in candidates:
        cand['final_score'] = cand['base_score'] * (1 + compute_intent_bonus(cand['node_type'], intent_vector))

    return sorted(candidates, key=lambda x: (x['cost'], -x['final_score']))[:k]
```

## 6.3 多连接场景处理

### 核心场景

```
场景 A: 输入接口已连接
┌─────────┐         ┌─────────┐
│ Node A  │────────▶│ Node B  │
│ (上游)  │         │ (当前)  │
│         │         │ [光标]  │
└─────────┘         └─────────┘
                          ▲
                          │
                    Input Pin 已连接

预测行为：显示当前连接 + 推荐替代/补充方案


场景 B: 输出接口多连接
┌─────────┐         ┌─────────┐
│         │────────▶│ Node C  │
│ Node A  │         └─────────┘
│ (当前)  │────────▶┌─────────┐
│ [光标]  │         │ Node D  │
│         │         └─────────┘
│         │────────▶┌─────────┐
│         │         │ Node E  │
│         │         └─────────┘
└─────────┘
     ▲
     │
Output Pin 已连 3 个节点

预测行为：显示现有 3 个连接 + 推荐第 4 个可能连接
```

### 代码实现

```cpp
struct FPCGPredictionContext
{
    UPCGPin* TargetPin;
    EPCGPinDirection PinDirection;  // Input/Output

    // 关键：现有连接信息
    TArray<UPCGPin*> ExistingConnections;  // 已连接的对面 Pin

    bool bIsConnected() const { return ExistingConnections.Num() > 0; }
    int32 GetConnectionCount() const { return ExistingConnections.Num(); }
};

class FPCGPredictorEngine
{
public:
    FPCGPredictionResult Predict(const FPCGPredictionContext& Context)
    {
        // 1. 获取基础预测（不考虑现有连接）
        auto BaseCandidates = NeuralNetworkPredict(Context);

        // 2. 分类处理
        if (Context.PinDirection == EPCGPinDirection::Input)
        {
            return ProcessInputPin(Context, BaseCandidates);
        }
        else
        {
            return ProcessOutputPin(Context, BaseCandidates);
        }
    }

private:
    FPCGPredictionResult ProcessInputPin(const FPCGPredictionContext& Context,
                                          const TArray<FPCGCandidate>& BaseCandidates)
    {
        FPCGPredictionResult Result;

        // 已连接：显示当前连接
        if (Context.bIsConnected())
        {
            Result.CurrentConnections = Context.ExistingConnections;

            // 过滤掉已连接的节点类型（避免重复推荐）
            auto ConnectedTypes = GetConnectedNodeTypes(Context.ExistingConnections);

            for (const auto& Cand : BaseCandidates)
            {
                if (!ConnectedTypes.Contains(Cand.NodeType))
                {
                    Result.AlternativeCandidates.Add(Cand);
                }
            }
        }
        else
        {
            // 未连接：正常推荐
            Result.AlternativeCandidates = BaseCandidates;
        }

        return Result;
    }

    FPCGPredictionResult ProcessOutputPin(const FPCGPredictionContext& Context,
                                           const TArray<FPCGCandidate>& BaseCandidates)
    {
        FPCGPredictionResult Result;

        // 始终显示现有连接（可能有多个）
        Result.CurrentConnections = Context.ExistingConnections;

        // 过滤已连接的类型，推荐新的
        auto ConnectedTypes = GetConnectedNodeTypes(Context.ExistingConnections);

        for (const auto& Cand : BaseCandidates)
        {
            if (!ConnectedTypes.Contains(Cand.NodeType))
            {
                Result.AdditionalCandidates.Add(Cand);
            }
        }

        return Result;
    }
};
```

---

# 7. 意图驱动层

## 7.1 意图解析

```python
class IntentParser:
    def parse(self, natural_language):
        # 示例："茂密森林，高低错落，避开道路"
        # 输出:
        #   scene: Forest (0.98)
        #   modifiers: [Dense(0.9), Uneven(0.85), AvoidPath(0.8)]

        embeddings = self.keyword_encoder(natural_language)

        scene_logits = self.scene_classifier(embeddings.mean(dim=0))
        scene = SCENE_CLASSES[scene_logits.argmax()]

        modifier_logits = self.modifier_detector(embeddings)
        modifiers = [MODIFIER_CLASSES[i] for i, v in enumerate(modifier_logits) if v > 0.5]

        # 构建 64 维意图向量
        intent_vector = torch.zeros(64)
        intent_vector[:10] = F.softmax(scene_logits, dim=-1)
        intent_vector[10:30] = torch.sigmoid(modifier_logits)

        return {
            'scene': scene,
            'modifiers': modifiers,
            'vector': intent_vector,
            'confidence': scene_logits.max().item()
        }
```

## 7.2 意图到技术配置映射

```python
INTENT_MAPPING = {
    'Forest': {
        'preferred_sources': ['SurfaceSampler', 'GetLandscapeData'],
        'preferred_processors': ['DensityFilter', 'TransformPoints'],
        'avoid_nodes': ['SplineSampler'],
        'default_params': {
            'SurfaceSampler.density': 0.8,
            'TransformPoints.scale_variance': 0.3
        }
    },
    'Stall': {
        'preferred_sources': ['CreatePoints', 'GridGenerator'],
        'preferred_processors': ['RandomSelection', 'AttributeSet'],
        'default_params': {
            'GridGenerator.spacing': 2.0,
            'RandomSelection.ratio': 0.6
        }
    }
}
```

---

# 8. 向量空间与动态扩展

## 8.1 节点嵌入空间

```python
class NodeEmbeddingSpace:
    def __init__(self):
        self.embeddings = {}  # node_id -> 128-dim vector
        self.index = faiss.IndexFlatIP(128)

    def encode(self, node_config):
        features = [
            self.encode_topology(node_config),      # 32 维
            self.encode_data_types(node_config),    # 32 维
            self.encode_semantic_tags(node_config),   # 32 维
            self.encode_intent_affinity(node_config) # 32 维
        ]
        vector = F.normalize(torch.cat(features), p=2, dim=-1)
        return vector

    def add_new_node(self, node_config):
        """
        动态新增节点，无需重训练
        基于规则编码 + 相似度插值
        """
        # 规则生成基础嵌入
        base_embed = self.rule_based_encode(node_config)

        # K 近邻插值精化
        similar = self.find_similar(base_embed, k=5)
        refined = self.interpolate(base_embed, [self.embeddings[n] for n in similar])

        self.embeddings[node_config['id']] = refined
        self.index.add(refined.unsqueeze(0).numpy())

        return refined
```

## 8.2 在线适应

```python
def online_update(node_id, usage_records):
    """
    基于使用记录轻量级微调
    只更新该节点嵌入，不影响其他
    """
    for record in usage_records:
        cooccur_node = record['cooccur_node']
        target_sim = 1.0 if record['connected'] else 0.0

        current_sim = F.cosine_similarity(embeddings[node_id], embeddings[cooccur_node])
        loss = (current_sim - target_sim) ** 2

        # 梯度下降，只更新当前节点
        loss.backward()
        with torch.no_grad():
            embeddings[node_id] -= lr * embeddings[node_id].grad
            embeddings[node_id] = F.normalize(embeddings[node_id], p=2, dim=-1)
```

---

# 9. UI/UX 设计

## 9.1 预测浮层（Slate 原生）

```
┌─────────────────────────────────────────┐
│ 🔮 预测下游节点 (从 SurfaceSampler 输出)  │
├─────────────────────────────────────────┤
│                                         │
│ 1. 🟢 TransformPoints      94%          │
│    输入：Point | 点云变换（位置/旋转/缩放）│
│    [左键插入]                            │
│                                         │
│ 2. 🟢 DensityFilter        88%          │
│    输入：Point | 密度过滤（噪声控制）       │
│    [左键插入]                            │
│                                         │
│ 3. 🟢 StaticMeshSpawner    82%          │
│    输入：Point | 生成静态网格实例           │
│    [左键插入]                            │
│                                         │
│ 4. 🟡 ToPoint→VolumeSampler 65%         │
│    [需转换] Spatial→Point→体积采样       │
│    [左键插入]                            │
│                                         │
├─────────────────────────────────────────┤
│                                         │
│ 🔍 诊断工具                               │
│ [左键插入 Debug 节点]                       │
│ 在当前连接插入 UE5 原生 PCGDebug 节点           │
│ 用于观察数据流状态                         │
│                                         │
└─────────────────────────────────────────┘
```

## 9.2 键盘导航

| 按键 | 操作 |
|-----|------|
| `↑/↓` | 在候选和 Debug 选项间切换 |
| `Enter` | 插入当前选中项 |
| `1-9` | 直接选择第 N 个工作候选 |
| `0` | 选择 Debug 选项 |
| `Esc` | 关闭浮层 |
| `Tab` | 手动触发/刷新预测 |

## 9.3 意图输入（内联）

```
Shift+Tab 或右键"设置意图":
┌─────────────────────────────────────────┐
│ 🎯 输入场景描述                          │
│ ┌─────────────────────────────────────┐ │
│ │ 茂密森林，高低错落，避开道路           │ │
│ └─────────────────────────────────────┘ │
│                                         │
│ 快速选择:                               │
│ 🌲森林  🏙️城市  🏠建筑  🏪摊位  ⛰️山地   │
│                                         │
│ [✓ 确认]  [✗ 取消]                      │
└─────────────────────────────────────────┘
```

## 9.4 细节面板嵌入

```
选中节点时的 Details 面板:
┌─────────────────────────────────────────┐
│ 节点：SurfaceSampler                    │
├─────────────────────────────────────────┤
│ [基本参数] [高级] [🔮 智能预测]          │
├─────────────────────────────────────────┤
│                                         │
│ 🎯 当前意图：森林 (茂密) [修改]            │
│ 匹配度：92%                              │
│                                         │
│ 📈 预测统计                              │
│ • 常用下游：TransformPoints (87%)        │
│ • 典型完成时间：2.5 分钟                   │
│                                         │
│ 🔮 快速操作                              │
│ [预测下游] [查找相似节点] [保存为模板]     │
│                                         │
└─────────────────────────────────────────┘
```

## 9.5 多连接 UI 修正

### 输入接口（已连接）

```
┌─────────────────────────────────────────┐
│ 🔮 当前上游节点 (已连接)                  │
│ 1. 🟢 SurfaceSampler (当前连接)           │
│    [断开] [替换为...] [并行添加...]        │
├─────────────────────────────────────────┤
│ 🔮 替代/补充方案                          │
│ 2. 🟢 CreatePoints        88%            │
│ 3. 🟢 SplineSampler       76%            │
│ ...                                     │
└─────────────────────────────────────────┘
```

### 输出接口（多连接）

```
┌─────────────────────────────────────────┐
│ 🔮 当前下游节点 (3 个已连接)                │
│ 1. 🟢 TransformPoints (已有)              │
│ 2. 🟢 DensityFilter (已有)                │
│ 3. 🟢 StaticMeshSpawner (已有)            │
│    [查看/跳转] [断开]                     │
├─────────────────────────────────────────┤
│ 🔮 推荐第 4 个连接                          │
│ 4. 🟢 Difference          72%            │
│ 5. 🟢 Union               65%            │
│ ...                                     │
├─────────────────────────────────────────┤
│ 🔍 插入 Debug 节点                          │
└─────────────────────────────────────────┘
```

### Slate UI 代码更新

```cpp
void SPCGPredictionPopup::BuildUI(const FPCGPredictionResult& Result)
{
    // 1. 当前连接（如果有）
    if (Result.CurrentConnections.Num() > 0)
    {
        MainBox->AddSlot()
        [
            SNew(STextBlock)
            .Text(FText::Format(
                NSLOCTEXT("PCG", "CurrentConnections", "当前连接 ({0})"),
                FText::AsNumber(Result.CurrentConnections.Num())
            ))
            .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
        ];

        for (auto* ConnectedPin : Result.CurrentConnections)
        {
            MainBox->AddSlot()
            [
                BuildCurrentConnectionWidget(ConnectedPin)
            ];
        }

        // 分隔线
        MainBox->AddSlot() [SNew(SSeparator)];
    }

    // 2. 新推荐（替代或补充）
    const auto& NewCandidates = (Result.PinDirection == EPCGPinDirection::Input)
        ? Result.AlternativeCandidates
        : Result.AdditionalCandidates;

    FString HeaderText = (Result.PinDirection == EPCGPinDirection::Input)
        ? TEXT("替代方案")
        : TEXT("推荐新连接");

    MainBox->AddSlot()
    [
        SNew(STextBlock)
        .Text(FText::FromString(HeaderText))
        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 11))
    ];

    for (int32 i = 0; i < NewCandidates.Num(); ++i)
    {
        MainBox->AddSlot()
        [
            BuildCandidateWidget(NewCandidates[i], i)
        ];
    }

    // 3. Debug 入口（固定）
    MainBox->AddSlot() [SNew(SSeparator)];
    MainBox->AddSlot() [BuildDebugEntry()];
}
```

---

# 10. Debug 集成

## 10.1 定位

| 特性 | 说明 |
|-----|------|
| **性质** | 独立诊断工具，非预测节点 |
| **位置** | 预测浮层底部固定入口 |
| **功能** | 在现有连接中插入 UE5 原生 PCGDebug 节点 |
| **用途** | 观察数据流状态，验证中间结果 |

## 10.2 操作

```
预测浮层底部:
├─────────────────────────────────────────┤
│ 🔍 诊断工具                               │
│ [左键插入 Debug 节点]                       │
│ 在当前连接插入 UE5 原生 PCGDebug 节点           │
│ 用于观察数据流状态                         │
└─────────────────────────────────────────┘

点击后:
1. 在 SourcePin 和 TargetPin 之间创建 PCGDebug 节点
2. 自动连线：Source → Debug → Target
3. 打开 PCG Debugger 面板
4. 3D 视口显示数据可视化
```

## 10.3 代码实现

```cpp
void SPCGPredictionPopup::BuildUI()
{
    // Top-K 工作候选
    for (int32 i = 0; i < Candidates.Num(); ++i)
    {
        CandidateList->AddSlot()
        [
            BuildCandidateWidget(Candidates[i], i)
        ];
    }

    // 分隔线
    CandidateList->AddSlot() [SNew(SSeparator)];

    // Debug 入口（固定，非预测）
    CandidateList->AddSlot()
    [
        SNew(SButton)
        .OnClicked(this, &SPCGPredictionPopup::OnDebugInsertClicked)
        .Content()
        [
            SNew(SHorizontalBox)
            +SHorizontalBox::Slot().AutoWidth()
            [
                SNew(STextBlock).Text(FText::FromString("🔍"))
            ]
            +SHorizontalBox::Slot().Padding(8, 0)
            [
                SNew(SVerticalBox)
                +SVerticalBox::Slot()
                [
                    SNew(STextBlock).Text(FText::FromString("插入 Debug 节点"))
                    .Font(Bold)
                ]
                +SVerticalBox::Slot()
                [
                    SNew(STextBlock).Text(FText::FromString("观察当前数据流状态"))
                    .Color(Subdued)
                ]
            ]
        ]
    ];
}
```

---

# 11. 实施与交付

## 11.1 开发路线图

| 阶段 | 周期 | 任务 | 产出 |
|-----|------|------|------|
| Phase 1 | 3 周 | 数据管道 + 基础模型 | 训练数据集，ONNX 模型 |
| Phase 2 | 3 周 | UE5 插件核心 | Slate UI，双向预测 |
| Phase 3 | 2 周 | 意图层+Debug 集成 | 完整功能系统 |
| Phase 4 | 2 周 | 优化测试文档 | 生产就绪版本 |

## 11.2 交付清单

| 组件 | 规格 | 状态 |
|-----|------|------|
| 神经网络模型 | ONNX INT8, ~4MB | 需训练 |
| C++ 推理引擎 | ONNX Runtime 1.15+ | 需开发 |
| Slate UI | 预测浮层/意图输入/细节面板 | 需开发 |
| 编辑器扩展 | 工具栏/右键菜单/快捷键 | 需开发 |
| **Debug 功能** | **预测浮层底部入口，调用 UE5 原生** | **复用已有** |

## 11.3 验收标准

| 指标 | 目标 | 测试方法 |
|-----|------|---------|
| 预测精度 | Top-3 采纳率>90% | 100 个真实任务 A/B 测试 |
| 响应延迟 | P95<50ms | 自动化性能测试 |
| 双向覆盖 | 输入/输出预测精度差异<5% | 对比实验 |
| 意图提升 | 有意图时精度提升>15% | 消融实验 |
| 新增节点 | <100ms 生效 | 自动化测试 |

---

# 12. 未来扩展

## 12.1 意图驱动的参数匹配

**当前局限**：预测系统推荐节点类型，但参数需用户手动配置。

**未来目标**：根据意图自动预配置节点参数。

```
当前行为:
系统推荐：TransformPoints
用户插入后：需手动设置 ScaleMin=0.7, ScaleMax=1.3, RotationY=(0,360)

未来行为:
系统推荐：TransformPoints (预设：森林茂密场景)
自动配置:
  - ScaleMin = 0.7      (意图：Uneven → 高度变化)
  - ScaleMax = 1.3      (意图：Uneven → 高度变化)
  - RotationY = (0,360) (意图：Random → 随机朝向)
  - Density = 0.8       (意图：Dense → 茂密)
```

## 12.2 技术实现

```python
class IntentDrivenParameterConfig:
    def __init__(self):
        self.param_rules = {
            'TransformPoints': {
                'scale_variance': {
                    'condition': 'Uneven' in intent.modifiers,
                    'value': (0.7, 1.3),
                    'confidence': 0.9
                },
                'rotation_y': {
                    'condition': 'Random' in intent.modifiers or 'Forest' == intent.scene,
                    'value': (0, 360),
                    'confidence': 0.85
                }
            },
            'SurfaceSampler': {
                'density': {
                    'Dense': 0.8,
                    'Sparse': 0.2,
                    'Normal': 0.5
                },
                'bUnbounded': {
                    'condition': 'Forest' == intent.scene,
                    'value': False,  # 森林通常有边界
                    'confidence': 0.8
                }
            },
            # ... 其他节点参数规则
        }

    def configure_node(self, node_type, intent_vector):
        """
        根据意图自动配置节点参数
        """
        config = {}
        rules = self.param_rules.get(node_type, {})

        for param_name, rule in rules.items():
            if isinstance(rule, dict) and 'condition' in rule:
                # 条件规则
                if self.evaluate_condition(rule['condition'], intent_vector):
                    config[param_name] = {
                        'value': rule['value'],
                        'confidence': rule['confidence'],
                        'source': 'intent_rule'
                    }
            elif isinstance(rule, dict) and intent_vector.scene in rule:
                # 场景映射规则
                config[param_name] = {
                    'value': rule[intent_vector.scene],
                    'confidence': 0.9,
                    'source': 'scene_mapping'
                }

        return config

    def evaluate_condition(self, condition, intent_vector):
        """评估条件表达式"""
        # 支持简单逻辑：'A in B', 'A == B', 'A and B'
        if ' in ' in condition:
            modifier, _ = condition.split(' in ')
            return modifier in intent_vector.modifiers
        return False
```

## 12.3 UI 展示

```
预测浮层（含参数预览）:
┌─────────────────────────────────────────┐
│ 🔮 预测下游节点                          │
├─────────────────────────────────────────┤
│                                         │
│ 1. 🟢 TransformPoints      94%          │
│    [左键插入]                            │
│                                         │
│    建议参数 (基于意图"森林茂密"):          │
│    ├─ Scale: [0.7, 1.3]  (高低错落)      │
│    ├─ Rotation Y: [0°, 360°] (随机朝向)   │
│    └─ Density: 0.8        (茂密分布)      │
│                                         │
│    [应用建议参数] [保持默认] [自定义...]     │
│                                         │
│ 2. 🟢 DensityFilter        88%          │
│    建议参数：Threshold=0.6 (过滤稀疏点)     │
│    [左键插入]                            │
│                                         │
├─────────────────────────────────────────┤
│ 🔍 插入 Debug 节点                         │
└─────────────────────────────────────────┘
```

## 12.4 参数学习机制

```python
class ParameterLearning:
    def learn_from_user_adjustment(self, node_type, suggested_params, actual_params):
        """
        学习用户调整，优化参数推荐
        """
        # 记录：意图上下文 → 建议参数 → 用户实际使用的参数
        self.adjustment_log.append({
            'timestamp': datetime.now(),
            'intent': self.current_intent,
            'node_type': node_type,
            'suggested': suggested_params,
            'actual': actual_params,
            'deviation': self.compute_deviation(suggested_params, actual_params)
        })

        # 周期性聚类分析，发现更好的参数模式
        if len(self.adjustment_log) > 100:
            self.update_param_rules()

    def update_param_rules(self):
        """
        基于用户调整历史，更新参数规则
        """
        # 对同一意图 - 节点类型的调整进行聚类
        # 发现用户偏好的参数范围，更新规则库
        pass
```

## 12.5 与其他扩展的关系

```
未来扩展路线图:
├── Phase 2 (6 个月后): 参数匹配
│   └── 意图 → 节点类型 + 参数预配置
│
├── Phase 3 (1 年后): 子图模板
│   └── 意图 → 完整子图文件（备选方案）
│   └── 当前节点级预测的补充
│
└── Phase 4 (2 年后): 端到端生成
    └── 意图 → 完整 PCG Graph（可控生成）
```

---

# 13. 总结

## 13.1 核心体验

> 开发者在 PCG Graph 编辑器中自然工作，光标悬停 Pin 即获智能建议（Top-K 工作节点 + 底部 Debug 入口），意图输入内联便捷，全程不离开编辑器上下文。

## 13.2 技术亮点

- 原生 Slate UI，与 UE5 无缝融合
- 双向预测覆盖完整工作流
- 意图驱动提升推荐精准度
- 向量空间支持动态扩展
- Debug 作为独立诊断工具，不参与预测，仅提供快捷入口

## 13.3 当前交付（v4.1）

- 双向节点级预测
- 意图感知排序
- 向量空间动态扩展
- Debug 快捷入口
- 多连接场景处理

## 13.4 未来演进

- **参数匹配**：意图 → 节点类型 + 智能参数预配置
- **子图模板**：意图 → 完整功能模块（备选）
- **端到端生成**：意图 → 可控完整 Graph

---

## 附录 A: PCG 节点导出工具

```cpp
// PCGExportTool.h
#pragma once

#include "Modules/ModuleManager.h"

class FPCGExportTool : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

private:
    void RegisterMenuExtension();
    static void ExportNodes();
};
```

```cpp
// PCGExportTool.cpp
#include "PCGExportTool.h"
#include "LevelEditor.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"

// 引入 ExportPCGNodes 函数
extern void ExportPCGNodes();

void FPCGExportTool::StartupModule()
{
    RegisterMenuExtension();
}

void FPCGExportTool::ShutdownModule()
{
}

void FPCGExportTool::RegisterMenuExtension()
{
    FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

    TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
    MenuExtender->AddMenuExtension(
        "WindowLayout",
        EExtensionHook::After,
        nullptr,
        FMenuExtensionDelegate::CreateStatic(&FPCGExportTool::ExportNodes)
    );

    LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);
}

void FPCGExportTool::ExportNodes()
{
    ExportPCGNodes();
}
```

---

## 附录 B: 关键修正总结

| 场景 | 处理 |
|-----|------|
| 输入接口已连接 | 显示当前上游 + 推荐替代方案（过滤已连接类型） |
| 输入接口未连接 | 正常推荐上游节点 |
| 输出接口已连接 N 个 | 显示全部 N 个现有连接 + 推荐第 N+1 个新连接 |
| 输出接口未连接 | 正常推荐下游节点 |

**核心原则**：预测推荐**新连接**，不重复推荐已连接的类型，但始终显示现有连接状态。
