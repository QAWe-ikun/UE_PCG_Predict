# UE PCG Predict 使用指南

## 快速开始

### 1. 安装插件

#### 步骤 1: 复制插件到你的 UE5 项目

```bash
# 将插件复制到你的 UE5 项目 Plugins 目录
cp -r PCG_Predict /path/to/YourUE5Project/Plugins/
```

或者直接复制整个 `PCG_Predict` 文件夹到你的项目：

```
YourUE5Project/
└── Plugins/
    └── PCG_Predict/
        ├── PCG_Predict.uplugin
        ├── Source/
        └── ...
```

#### 步骤 2: 重新生成项目文件

右键点击你的 `.uproject` 文件 → **Generate Visual Studio project files**

#### 步骤 3: 编译项目

在 Visual Studio 中打开项目并编译，或使用命令行：

```bash
# 编译你的 UE5 项目
/path/to/UE5/Engine/Build/BatchFiles/Build.bat YourProject YourProject Win64 Development
```

---

### 2. 启动插件

#### 方法 1: 工具栏按钮

启动 UE5 编辑器后，在 **Level Editor 工具栏** 找到：

- **🔮 PCG 预测** - 显示预测浮层
- **🎯 意图** - 设置场景意图

#### 方法 2: 快捷键（待实现）

- `Tab` - 触发预测
- `Shift+Tab` - 设置意图

---

### 3. 基本使用流程

#### 场景 1: 空白 PCG Graph 启动

1. 打开 PCG Graph 编辑器
2. 点击工具栏 **🔮 PCG 预测** 按钮
3. 查看推荐的源节点（如 SurfaceSampler）
4. 按数字键 `1-5` 选择节点
5. 按 `Enter` 确认插入

#### 场景 2: 为节点添加输入

1. 在 Graph 中选择一个节点
2. 将光标悬停在 **输入接口** 上
3. 预测浮层显示推荐的上游节点
4. 选择并插入

#### 场景 3: 为节点添加输出

1. 在 Graph 中选择一个节点
2. 将光标悬停在 **输出接口** 上
3. 预测浮层显示的下游节点
4. 选择并插入

---

### 4. 意图设置

#### 使用意图输入窗口

1. 点击工具栏 **🎯 意图** 按钮
2. 输入场景描述，例如：
   - "茂密森林，高低错落"
   - "城市建筑群"
   - "稀疏树木"
3. 或使用快速选择按钮：
   - 🌲 森林
   - 🏙️ 城市
   - 🏠 建筑
   - 🏪 摊位
   - ⛰️ 山地
4. 点击 **✓ 确认** 或按 `Enter`

#### 意图效果

设置意图后，预测系统会根据你的场景类型调整推荐优先级：

| 意图 | 推荐节点 |
|-----|---------|
| 森林 | SurfaceSampler, TransformPoints, DensityFilter |
| 城市 | GridGenerator, StaticMeshSpawner |
| 建筑 | CreatePoints, AttributeSet |

---

### 5. 预测浮层操作

#### 键盘导航

| 按键 | 功能 |
|-----|------|
| `↑` / `↓` | 在候选项间切换 |
| `1` - `5` | 直接选择第 N 个候选 |
| `0` | 选择 Debug 选项 |
| `Enter` | 确认选择 |
| `Esc` | 关闭浮层 |
| `Tab` | 刷新预测 |

#### 鼠标操作

- **左键点击** 候选项 - 插入节点
- **左键点击** Debug - 插入调试节点

---

### 6. Debug 功能

#### 插入 Debug 节点

1. 打开预测浮层
2. 点击底部的 **🔍 插入 Debug 节点**
3. 或按数字键 `0`

#### Debug 节点作用

- 观察数据流状态
- 查看中间结果
- 验证连接是否正确

---

## 配置说明

### 模型文件位置

ONNX 模型应放在：

```
YourUE5Project/Plugins/PCG_Predict/Models/
└── pcg_predict.onnx
```

### 节点注册表

节点类型映射文件：

```
python/config/node_registry.yaml
```

包含 187 个 PCG 节点的类型信息。

---

## 常见问题

### Q1: 编译错误

**问题**: 编译时出现模块依赖错误

**解决**:
```bash
# 确保 Build.cs 包含所有需要的模块
PrivateDependencyModuleNames.AddRange(new string[]
{
    "UnrealEd",
    "Slate",
    "SlateCore",
    "LevelEditor"
});
```

### Q2: 预测浮层不显示

**问题**: 点击预测按钮后没有反应

**解决**:
1. 检查 Output Log 是否有错误
2. 确认插件已加载
3. 查看 `FPCGPredictModule::StartupModule()` 是否被调用

### Q3: 模型文件找不到

**问题**: `ONNX model not found` 警告

**解决**:
- 这是正常的（如果还没训练模型）
- 系统会进入模拟模式，返回示例数据
- 训练模型后，将 `.onnx` 文件放到 `Models/` 目录

---

## 训练模型（可选）

### 前置条件

- Python 3.8+
- PyTorch 2.0+
- PyG 2.4+

### 步骤

1. **安装依赖**:
```bash
cd python
pip install -r requirements.txt
```

2. **准备训练数据**:
```bash
python scripts/prepare_data.py
```

3. **训练模型**:
```bash
python scripts/train.py
```

4. **导出 ONNX**:
```bash
python scripts/export_onnx.py
```

5. **复制模型到插件**:
```bash
cp models/pcg_predict.onnx ../PCG_Predict/Models/
```

---

## 项目结构

```
UE_PCG_Predict/
├── PCG_Predict/              # UE5 插件
│   ├── Source/
│   │   ├── Public/
│   │   │   ├── Core/        # 核心类型
│   │   │   ├── UI/          # UI 组件
│   │   │   ├── Editor/      # 编辑器扩展
│   │   │   └── Inference/   # ONNX 推理
│   │   └── Private/
│   └── PCG_Predict.uplugin
├── python/                   # Python 训练代码
│   ├── pcg_predict/         # 模型定义
│   ├── config/              # 配置文件
│   └── scripts/             # 训练脚本
└── docs/                     # 文档
    ├── README.md
    ├── UI_IMPLEMENTATION.md
    ├── BUILD_FIX_REPORT.md
    ├── UE5_API_FIXES.md
    └── UE5_6_API_FIXES.md
```

---

## 下一步

### 当前状态

✅ **已完成**:
- UI 组件（预测浮层、意图输入）
- 编辑器集成（工具栏、窗口管理）
- 预测引擎框架
- ONNX 运行时封装

⏳ **待完成**:
- PCG Graph 编辑器事件绑定
- ONNX 模型训练
- 实际节点创建和连接逻辑
- 类型检查和环路检测

### 贡献

欢迎提交 Issue 和 Pull Request！

---

## 参考资源

- [README.md](../README.md) - 项目总览
- [UI_IMPLEMENTATION.md](UI_IMPLEMENTATION.md) - UI 实现细节
- [UE5_6_API_FIXES.md](UE5_6_API_FIXES.md) - UE5.6 API 兼容性

---

## 技术支持

如有问题，请查看：
1. Output Log 窗口
2. `docs/` 目录下的文档
3. 项目 Issue 页面
