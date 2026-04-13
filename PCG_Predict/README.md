# PCG_Predict - UE5 PCG 智能节点预测插件

**版本**: v1.0  
**引擎**: Unreal Engine 5.6.1
**类型**: 编辑器插件

---

## 📋 目录

1. [项目概述](#项目概述)
2. [核心功能](#核心功能)
3. [快速开始](#快速开始)
4. [使用指南](#使用指南)
5. [系统架构](#系统架构)
6. [配置说明](#配置说明)
7. [开发说明](#开发说明)

---

## 项目概述

PCG_Predict 是一个为 Unreal Engine 5 PCG（Procedural Content Generation）系统设计的智能节点预测插件。通过 Pin 悬停自动预测和推荐下一个可能的节点，大幅提升 PCG Graph 的构建效率。

### 核心特性

- **智能预测**：基于 Pin 方向和已连接节点智能推荐候选节点
- **Pin 悬停触发**：鼠标悬停在 Pin 上自动显示预测结果
- **方向感知**：输出 Pin 推荐有输入的节点，输入 Pin 推荐有输出的节点
- **上下文感知**：显示已连接的节点信息
- **一键创建**：点击候选节点自动创建并连接
- **Debug 快捷入口**：输出 Pin 提供快速插入 Debug 节点功能
- **智能定位**：新节点自动定位在源节点旁边
- **可配置**：支持开关预测功能、调整检测间隔等

---

## 核心功能

### 1. Pin 悬停预测

当鼠标悬停在 PCG Graph 节点的 Pin 上时，自动显示预测窗口：

```
┌─────────────────────────────────┐
│ 🔮 PCG Predict    [节点名称]    │
├─────────────────────────────────┤
│ Connected Nodes                 │
│ 🔗 已连接节点1                   │
│ 🔗 已连接节点2                   │
├─────────────────────────────────┤
│ Recommended Nodes               │
│ [1] 候选节点1      (0.95)       │
│ [2] 候选节点2      (0.87)       │
│ [3] 候选节点3      (0.79)       │
│ [4] 候选节点4      (0.71)       │
│ [5] 候选节点5      (0.63)       │
├─────────────────────────────────┤
│ 🔍 Insert Debug Node            │
│    Observe data flow state      │
└─────────────────────────────────┘
```

### 2. 方向感知过滤

- **输出 Pin 预测**：只推荐有输入 Pin 的节点
- **输入 Pin 预测**：只推荐有输出 Pin 的节点
- **自动排除**：
  - Debug 节点（仅通过快捷按钮插入）

### 3. 智能连接

- **Pin 兼容性检查**：主数据 Pin（"In"/"Out"）可连接任何数据 Pin
- **自动选择**：选择第一个类型匹配的 Pin 进行连接
- **智能定位**：
  - 输出 Pin → 新节点创建在右侧（+400px）
  - 输入 Pin → 新节点创建在左侧（-400px）

### 4. Debug 节点快捷入口

- **仅输出 Pin 显示**：只在输出 Pin 悬停时显示 Debug 按钮
- **一键插入**：快速在数据流中插入 Debug 节点观察状态
- **自动连接**：Debug 节点自动连接到源节点

### 5. 工具栏集成

在编辑器工具栏提供两个按钮：

- **⚡ 预测: 开/关**：切换 Pin 悬停预测功能
- **🎯 Intent**：输入意图创建新的 PCG Graph

<img src="PCG_Predict/Resources/UI.png">

*左边为预测开关，右边为Intent*

---

## 快速开始

### 环境要求

- Unreal Engine 5.6.1
- Windows 10/11
- Visual Studio 2022

### 安装步骤

1. **克隆仓库**
   ```bash
   git clone https://github.com/QAWe-ikun/UE_PCG_Predict
   cd UE_PCG_Predict
   ```

2. **放置插件**
   将 `PCG_Predict` 文件夹复制到项目的 `Plugins` 目录：
   ```
   YourProject/
   └── Plugins/
       └── PCG_Predict/
   ```

3. **生成项目文件**
   右键点击 `.uproject` 文件 → "Generate Visual Studio project files"

4. **编译插件**
   打开 Visual Studio 解决方案，编译项目

5. **启用插件**
   - 启动编辑器
   - Edit → Plugins
   - 搜索 "PCG Predict"
   - 勾选启用
   - 重启编辑器

### 验证安装

1. 打开任意 PCG Graph
2. 将鼠标悬停在节点的 Pin 上
3. 应该看到预测窗口自动弹出

---

## 使用指南

### 基本使用

1. **打开 PCG Graph**
   - 在 Content Browser 中双击 PCG Graph 资产

2. **悬停预测**
   - 将鼠标悬停在任意节点的 Pin 上
   - 等待约 0.3 秒，预测窗口自动显示

3. **选择节点**
   - 点击候选节点创建并自动连接

4. **插入 Debug 节点**
   - 在输出 Pin 上悬停
   - 点击 "Insert Debug Node" 按钮

### 键盘快捷键

- **ESC**：关闭预测窗口

### 开关预测功能

点击工具栏的 **⚡ 预测** 按钮切换功能开关：
- **⚡ 预测: 开**：启用 Pin 悬停预测
- **⚡ 预测: 关**：禁用 Pin 悬停预测

---

## 系统架构

### 模块结构

```
PCG_Predict/
├── Source/
│   └── PCG_Predict/
│       ├── Public/
│       │   ├── Core/
│       │   │   ├── PCGPredictModule.h          # 模块主类
│       │   │   ├── PCGPredictorEngine.h        # 预测引擎
│       │   │   └── PCGPredictionTypes.h        # 类型定义
│       │   ├── Editor/
│       │   │   ├── FPCGPinHoverIntegration.h   # Pin 悬停检测
│       │   │   ├── PCGGraphActions.h           # 节点创建/连接
│       │   │   ├── PCGGraphFactory.h           # Graph 工厂
│       │   │   ├── PCGToolbarExtension.h       # 工具栏扩展
│       │   │   └── PCGEditorExtension.h        # 编辑器扩展
│       │   ├── UI/
│       │   │   ├── SPCGPredictionPopup.h       # 预测弹窗
│       │   │   └── SPCGIntentInput.h           # 意图输入
│       │   └── Config/
│       │       └── PCGPredictSettings.h        # 配置类
│       └── Private/
│           └── (对应的 .cpp 实现文件)
├── Content/
│   └── Config/
│       └── node_registry.json                  # 节点注册表
└── Resources/
    └── Icon128.png                             # 插件图标
```

### 核心组件

#### 1. PCGPredictorEngine（预测引擎）

- 加载节点注册表（node_registry.json）
- 根据 Pin 方向过滤候选节点
- 提取已连接节点信息
- 返回排序后的候选列表

#### 2. FPCGPinHoverIntegration（Pin 悬停集成）

- 使用 `FCoreDelegates::OnEndFrame` 轮询鼠标位置
- 检测鼠标下的 Widget 类型
- 识别 Pin Widget 并提取信息
- 触发预测并显示弹窗

#### 3. PCGGraphActions（节点操作）

- 创建 PCG 节点（数据层 + 可视化层）
- Pin 兼容性检查
- 自动连接节点
- 支持 Undo/Redo

#### 4. SPCGPredictionPopup（预测弹窗）

- 显示已连接节点
- 显示推荐候选节点
- Debug 按钮（仅输出 Pin）
- 键盘导航支持

---

## 配置说明

### 编辑器配置

打开 **Edit → Editor Preferences → Plugins → PCG Predict**：

| 配置项 | 说明 | 默认值 |
|--------|------|--------|
| Enable Pin Hover Prediction | 启用 Pin 悬停预测 | true |
| Max Candidates | 最大候选数量 | 5 |
| Hover Detection Interval | 悬停检测间隔（帧数） | 10 |
| Default Graph Save Path | PCG Graph 默认保存路径 | /Game/PCG/Generated |

### 节点注册表

`Content/Config/node_registry.json` 包含所有 PCG 节点的元数据：

```json
{
  "nodes": [
    {
      "id": 0,
      "name": "AddComponent",
      "class": "PCGAddComponentSettings",
      "category": "Unknown",
      "input_types": ["Point", "Any"],
      "output_types": ["Point"],
      "class_path": "/Script/PCG.PCGAddComponentSettings"
    }
  ]
}
```

---

## 开发说明

### 构建配置

`PCG_Predict.Build.cs` 依赖模块：

```csharp
PublicDependencyModuleNames.AddRange(new string[]
{
    "Core",
    "CoreUObject",
    "Engine",
    "Slate",
    "SlateCore",
    "UnrealEd",
    "PCG",
    "GraphEditor",
    "DeveloperSettings"
});
```

### 关键实现

#### Pin 方向检测

```cpp
// 从 UEdGraphPin 获取真实方向
if (PinObj) {
    Direction = (PinObj->Direction == EGPD_Input) ? TEXT("Input") : TEXT("Output");
}
```

#### 节点过滤

```cpp
// 排除 Debug 节点（id=130）和 "None" 节点
if (Entry.Id == 130 || Entry.Name == TEXT("None")) {
    continue;
}

// 根据方向过滤
if (Direction == EPCGPredictPinDirection::Output) {
    if (Entry.InputTypes.Num() > 0) {
        ValidIndices.Add(i);
    }
}
```

#### Pin 兼容性

```cpp
// 主数据 Pin 可以连接任何数据 Pin
bool bPinAIsMainData = (PinAName == TEXT("In") || PinAName == TEXT("Out"));
bool bPinBIsMainData = (PinBName == TEXT("In") || PinBName == TEXT("Out"));

if (bPinAIsMainData || bPinBIsMainData) {
    // 排除特殊 Pin
    if (!bIsSpecialPin) {
        return true;
    }
}
```

### 调试技巧

1. **启用详细日志**
   ```cpp
   UE_LOG(LogTemp, Log, TEXT("Your debug message"));
   ```

2. **检查 Pin 方向**
   日志会显示：`Pin direction from object: Input/Output`

3. **验证节点创建**
   日志会显示：`Node created successfully, Pins count: X`
