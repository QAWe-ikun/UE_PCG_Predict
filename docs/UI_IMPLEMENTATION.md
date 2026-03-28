# UE5 UI 实现完成报告

## 已创建文件

### UI 组件 (4 个文件)

| 文件 | 路径 | 功能 |
|-----|------|------|
| `SPCGPredictionPopup.h` | `Public/UI/` | 预测浮层头文件 |
| `SPCGPredictionPopup.cpp` | `Private/UI/` | 预测浮层实现（Top-K 候选 + Debug 入口） |
| `SPCGIntentInput.h` | `Public/UI/` | 意图输入窗口头文件 |
| `SPCGIntentInput.cpp` | `Private/UI/` | 意图输入窗口实现（自然语言输入） |

### 编辑器集成 (4 个文件)

| 文件 | 路径 | 功能 |
|-----|------|------|
| `PCGEditorExtension.h` | `Public/Editor/` | 编辑器扩展头文件 |
| `PCGEditorExtension.cpp` | `Private/Editor/` | 编辑器扩展实现（窗口管理） |
| `PCGToolbarExtension.h` | `Public/Editor/` | 工具栏扩展头文件 |
| `PCGToolbarExtension.cpp` | `Private/Editor/` | 工具栏扩展实现（按钮添加） |

### 核心引擎 (2 个文件)

| 文件 | 路径 | 功能 |
|-----|------|------|
| `PCGPredictorEngine.cpp` | `Private/Core/` | 预测引擎实现（ONNX 集成） |
| `PCGOnnxRuntime.cpp` | `Private/Inference/` | ONNX 运行时封装 |

### 模块更新 (2 个文件)

| 文件 | 路径 | 功能 |
|-----|------|------|
| `PCGPredictModule.h` | `Public/` | 模块头文件（更新） |
| `PCGPredictModule.cpp` | `Private/` | 模块实现（更新） |
| `PCG_Predict.Build.cs` | `Source/` | 构建配置（更新） |

---

## UI 功能特性

### 1. 预测浮层 (SPCGPredictionPopup)

**功能**:
- ✅ 显示 Top-5 候选节点
- ✅ 显示预测分数百分比
- ✅ 支持键盘导航（↑↓选择，1-5 快速选择，Enter 确认）
- ✅ Debug 入口（固定底部，0 键选择）
- ✅ 显示当前连接状态（如果有）
- ✅ 区分输入/输出预测方向

**键盘快捷键**:
| 按键 | 功能 |
|-----|------|
| `↑/↓` | 在候选项间切换 |
| `1-5` | 直接选择第 N 个候选 |
| `0` | 选择 Debug 选项 |
| `Enter` | 确认选择 |
| `Esc` | 关闭浮层 |

### 2. 意图输入窗口 (SPCGIntentInput)

**功能**:
- ✅ 文本输入框（自然语言描述）
- ✅ 快速场景选择按钮（森林/城市/建筑/摊位/山地）
- ✅ 确认/取消按钮
- ✅ 支持 Enter 确认，Esc 取消

**快速选择**:
- 🌲 森林
- 🏙️ 城市
- 🏠 建筑
- 🏪 摊位
- ⛰️ 山地

### 3. 工具栏集成 (PCGToolbarExtension)

**功能**:
- ✅ 在 Level Editor 工具栏添加按钮
- ✅ 🔮 PCG 预测按钮
- ✅ 🎯 意图按钮

### 4. 预测引擎 (PCGPredictorEngine)

**功能**:
- ✅ ONNX 模型加载
- ✅ 推理接口（临时模拟模式）
- ✅ 意图设置
- ✅ 节点注册表加载（待实现）

---

## 目录结构

```
PCG_Predict/
└── Source/
    └── PCG_Predict/
        ├── Public/
        │   ├── Core/
        │   │   ├── PCGPredictionTypes.h
        │   │   └── PCGPredictorEngine.h
        │   ├── Inference/
        │   │   └── PCGOnnxRuntime.h
        │   ├── UI/
        │   │   ├── SPCGPredictionPopup.h       ← 新增
        │   │   └── SPCGIntentInput.h           ← 新增
        │   ├── Editor/
        │   │   ├── PCGEditorExtension.h        ← 新增
        │   │   └── PCGToolbarExtension.h       ← 新增
        │   ├── PCGPredictModule.h              ← 更新
        │   └── PCGPredict.h
        ├── Private/
        │   ├── Core/
        │   │   └── PCGPredictorEngine.cpp      ← 更新
        │   ├── Inference/
        │   │   └── PCGOnnxRuntime.cpp          ← 新增
        │   ├── UI/
        │   │   ├── SPCGPredictionPopup.cpp     ← 新增
        │   │   └── SPCGIntentInput.cpp         ← 新增
        │   ├── Editor/
        │   │   ├── PCGEditorExtension.cpp      ← 新增
        │   │   └── PCGToolbarExtension.cpp     ← 新增
        │   └── PCGPredictModule.cpp            ← 更新
        └── PCG_Predict.Build.cs                ← 更新
```

---

## 下一步工作

### 1. 集成 PCG Graph 编辑器事件

需要绑定到 PCG Graph 编辑器的 Pin 悬停事件：

```cpp
// 在 PCGEditorExtension.cpp 中
void FPCGEditorExtension::BindToPCGGraphEditor()
{
    // TODO: 获取 PCG Graph 编辑器实例
    // TODO: 绑定到 Pin 悬停/点击事件
    // TODO: 触发预测浮层显示
}
```

### 2. 实现 ONNX Runtime 集成

需要添加 ONNX Runtime 依赖：

1. 下载 ONNX Runtime C++ API
2. 添加到 `ThirdParty` 目录
3. 更新 `Build.cs`
4. 实现真正的推理逻辑

### 3. 节点类型映射

将模型输出索引映射到实际 PCG 节点类型：

```cpp
// 从 node_registry.yaml 加载
NodeRegistry = {
    0: "TransformPoints",
    1: "DensityFilter",
    2: "StaticMeshSpawner",
    ...
}
```

### 4. 实际节点插入逻辑

实现选中候选后自动创建并连接节点：

```cpp
void InsertNode(const FPCGCandidate& Candidate)
{
    // 1. 创建节点实例
    // 2. 自动连线
    // 3. 刷新 Graph
}
```

---

## 编译说明

### 前置条件

- UE5 5.4+
- PCG 插件已启用

### 编译步骤

1. 复制插件到项目：
```bash
cp -r PCG_Predict /path/to/YourProject/Plugins/
```

2. 重新生成项目文件：
```bash
# 右键 YourProject.uproject → Generate Visual Studio project files
```

3. 编译项目：
```bash
# 在 Visual Studio 中编译
# 或使用 UnrealBuildTool
```

### 依赖模块

已在 `Build.cs` 中添加：
- `LevelEditor` - 工具栏集成
- `ApplicationCore` - 窗口管理
- `PCGEditor` - PCG 编辑器 API

---

## 测试方法

### 1. 工具栏按钮测试

启动 UE5 编辑器后：
1. 在 Level Editor 工具栏找到 `🔮 PCG 预测` 按钮
2. 点击应显示预测浮层
3. 点击 `🎯 意图` 按钮应显示意图输入窗口

### 2. 键盘导航测试

打开预测浮层后：
1. 按 `↑/↓` 应切换选中项
2. 按 `1-5` 应直接选择对应候选
3. 按 `Enter` 应确认选择
4. 按 `Esc` 应关闭浮层

### 3. 日志输出

打开 Output Log，应看到：
```
PCGPredict module started
PCG Editor Extension Initialized
Intent set to: xxx
```

---

## 注意事项

### 临时模拟模式

当前 ONNX 运行时在模型文件不存在时进入模拟模式，返回固定示例数据：
- TransformPoints (94%)
- DensityFilter (88%)
- StaticMeshSpawner (82%)
- Difference (75%)
- Union (68%)

### 待集成部分

以下功能需要进一步集成 PCG Graph 编辑器内部 API：
- Pin 悬停自动触发预测
- 实际节点创建和连接
- 类型检查（Point/Spatial 等）
- 环路检测

---

## 总结

✅ **已完成** (13 个文件):
- UI 组件（预测浮层、意图输入）
- 编辑器集成（工具栏、窗口管理）
- 预测引擎框架
- ONNX 运行时封装

⏳ **待完成**:
- PCG Graph 编辑器事件绑定
- ONNX Runtime 正式集成
- 节点创建和连接逻辑
- 类型检查和环路检测
