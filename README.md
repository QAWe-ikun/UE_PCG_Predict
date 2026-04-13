# UE_PCG_Predict

UE5 PCG Graph 智能节点预测插件。在 PCG Graph 编辑器中，当鼠标悬停在节点的 Pin 上时，自动预测下一个最可能连接的节点，并以浮层形式展示候选列表。

## 项目结构

```
UE_PCG_Predict/
├── PCG_Predict/                    # UE5 编辑器插件
│   ├── Source/PCG_Predict/
│   │   ├── Public/
│   │   │   ├── Core/
│   │   │   │   ├── PCGPredictorEngine.h    # 预测引擎（双模型）
│   │   │   │   └── PCGPredictionTypes.h    # 数据结构定义
│   │   │   ├── Inference/
│   │   │   │   ├── PCGFastPredictor.h      # 统计查表模型
│   │   │   │   ├── PCGDeepPredictor.h      # 神经网络异步推理
│   │   │   │   └── PCGOnnxRuntime.h        # ONNX Runtime 封装
│   │   │   ├── Editor/
│   │   │   │   ├── PCGEditorExtension.h    # 编辑器扩展（弹窗管理）
│   │   │   │   ├── PCGToolbarExtension.h   # 工具栏按钮
│   │   │   │   ├── PCGGraphFactory.h       # PCG Graph 创建工厂
│   │   │   │   └── FPCGPinHoverIntegration.h  # Pin 悬停检测
│   │   │   └── UI/
│   │   │       ├── SPCGPredictionPopup.h   # 预测候选浮层
│   │   │       └── SPCGIntentInput.h       # 意图输入窗口
│   │   └── Private/                        # 对应实现文件
│   └── Content/Config/
│       └── node_registry.json              # 节点注册表（运行时加载）
└── PCG_Predict_Models/                     # Python 训练管线
    ├── src/                                # 训练代码
    ├── config/                             # 配置文件
    ├── data/                               # 数据集
    └── models/                             # 训练产物（.onnx, .json）
```

## 功能

- **Pin 悬停预测**：鼠标悬停在 PCG 节点的 Pin 上，自动弹出候选节点列表
- **双模型推理**：
  - FastPredictor（< 1ms）：基于 n-gram 统计查表，立即响应
  - DeepPredictor（异步）：Graph Transformer 神经网络，后台推理后更新结果
- **意图驱动**：支持输入自然语言意图（如"茂密森林"），影响推荐排序
- **一键创建**：点击候选节点自动创建并连线
- **拖拽保护**：拖拽引线时不触发预测弹窗

## 环境要求

- Unreal Engine 5.4+
- PCG 插件（引擎内置）
- Windows 10/11

## 安装

1. 将 `PCG_Predict/` 目录复制到 UE5 项目的 `Plugins/` 目录下
2. 重新生成项目文件（右键 `.uproject` → Generate Visual Studio project files）
3. 编译项目
4. 在编辑器中启用插件：Edit → Plugins → 搜索 PCG Predict

## 使用

### Pin 悬停预测

打开任意 PCG Graph，将鼠标悬停在节点的输入或输出 Pin 上，约 0.3 秒后弹出预测浮层。

- 键盘 `↑↓` 导航候选列表
- `Enter` 或数字键 `1-9` 选择候选节点
- `Esc` 关闭浮层

### 意图输入

点击工具栏的 **🎯 Intent** 按钮，输入意图描述（如"散布树木"），后续预测将优先推荐与意图相关的节点。

### 预测开关

点击工具栏的 **⚡ 预测** 按钮可切换 Pin 悬停预测功能的开关状态。

## 训练自定义模型

参见 [PCG_Predict_Models/README.md](PCG_Predict_Models/README.md)。

基本流程：
```bash
cd PCG_Predict_Models
pip install -r requirements.txt

# 准备数据 → 提取样本 → 构建查找表 → 训练 → 导出
python src/data_collector.py
python src/build_fast_lookup.py
python src/train_deep.py
python src/export_onnx.py
```

将生成的 `models/fast_lookup.json` 和 `models/deep_predictor.onnx` 放入项目的 `PCG_Predict_Models/models/` 目录，引擎启动时自动加载。

## 架构概览

```
FPCGPredictModule
  ├── FPCGEditorExtension      预测弹窗 + 意图输入窗口
  ├── FPCGToolbarExtension     工具栏按钮（调用 EditorExtension）
  └── FPCGPinHoverIntegration  每帧检测鼠标下的 Pin
        └── FPCGPredictorEngine
              ├── FPCGFastPredictor   n-gram 查表，同步，< 1ms
              └── FPCGDeepPredictor   ONNX 推理，后台线程异步
```

推理流程：
1. 悬停检测到 Pin → 调用 `PredictorEngine::Predict()`
2. FastPredictor 立即返回初始候选列表 → 显示弹窗
3. DeepPredictor 在后台线程推理（~500ms）
4. 推理完成 → 回调游戏线程 → 合并结果重排（Fast×0.4 + Deep×0.6）

## 许可证

Apache 2.0
