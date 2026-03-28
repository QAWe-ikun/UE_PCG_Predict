# 工具栏故障排查指南

## 修改内容

已将插件配置修改为 **Editor 模块**，确保在编辑器中正确加载：

**PCG_Predict.uplugin**:
- `"Type": "Editor"` (之前是 Runtime)
- `"LoadingPhase": "PostDefault"` (确保在默认模块之后加载)

---

## 检查步骤

### 1. 重新编译项目

```bash
# 重新生成项目文件
右键 YourProject.uproject → Generate Visual Studio project files

# 在 VS 中重新编译
Build → Build Solution (Ctrl+Shift+B)
```

### 2. 启动 UE5 编辑器

打开你的 UE5 项目。

### 3. 查看 Output Log

启动后，打开 **Output Log** 窗口：
- 菜单：**Window** → **Developer Tools** → **Output Log**
- 快捷键：**Ctrl+Shift+L**

查找以下日志：

```
LogTemp: PCGPredict module started
LogTemp: PCG Editor Extension Initialized
LogTemp: PCG Toolbar Extension registered
```

如果看到这些日志，说明插件已成功加载。

### 4. 查找工具栏按钮

在 **Level Editor** 主界面，查看顶部工具栏：

```
[保存] [构建] [播放] ... [GameSettings] [🔮 PCG] [🎯 Intent]
```

按钮应该在工具栏右侧，靠近 **GameSettings** 下拉菜单。

---

## 如果看不到工具栏按钮

### 方法 1: 检查插件是否启用

1. **Edit** → **Plugins**
2. 搜索 "PCG"
3. 确认 **PCG_Predict** 已勾选
4. 如果未勾选，勾选后重启编辑器

### 方法 2: 自定义工具栏

1. 右键点击工具栏空白处
2. 选择 **Customize Toolbar...**
3. 查找是否有 PCG 相关命令
4. 如果有，拖到工具栏上

### 方法 3: 检查模块加载

在 Output Log 中搜索：
- `PCGPredict` - 查看模块是否加载
- `PCG Toolbar` - 查看工具栏扩展是否注册

### 方法 4: 使用命令行测试

启动编辑器时添加参数：

```bash
YourProject.exe -log
```

查看完整启动日志。

---

## 替代使用方式

如果工具栏仍然不显示，可以使用以下替代方法：

### 方式 1: 编辑器命令

在 UE5 编辑器中按 **`~`** 键打开控制台，输入：

```
pcg.predict
```

（需要实现命令绑定）

### 方式 2: 右键菜单

在 PCG Graph 编辑器中右键点击空白处，应该有预测选项。

（需要实现右键菜单扩展）

### 方式 3: 快捷键

按 **Tab** 键触发预测。

（需要实现键盘绑定）

---

## 代码调试

### 添加更多日志

在 `PCGToolbarExtension.cpp` 中添加：

```cpp
void FPCGToolbarExtension::Initialize()
{
    UE_LOG(LogTemp, Log, TEXT("=== PCG Toolbar Initialize ==="));
    
    if (!GEditorExtension.IsValid())
    {
        GEditorExtension = MakeShareable(new FPCGEditorExtension());
        GEditorExtension->Initialize();
    }

    ExtendToolbar();
    
    UE_LOG(LogTemp, Log, TEXT("=== PCG Toolbar Initialized ==="));
}
```

### 检查 LevelEditor 模块

```cpp
void FPCGToolbarExtension::ExtendToolbar()
{
    FLevelEditorModule* LevelEditorModule = 
        FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
    
    if (!LevelEditorModule)
    {
        UE_LOG(LogTemp, Error, TEXT("LevelEditor module NOT found!"));
        return;
    }
    
    UE_LOG(LogTemp, Log, TEXT("LevelEditor module found"));
    
    // ... 继续
}
```

---

## 确认编译成功

编译后应该看到：

```
1>[8/8] Compile [x64] PCGToolbarExtension.cpp
1>[9/9] Link [x64] UE5-Target.exe
1>Build succeeded in 15.32s
```

---

## 快速测试

编译完成后：

1. 启动 UE5 编辑器
2. 立即查看 Output Log
3. 寻找 `PCG Toolbar Extension registered`
4. 如果有，查看工具栏
5. 找到 **🔮 PCG** 按钮

---

## 联系支持

如果仍然无法显示，请提供：
1. Output Log 完整内容
2. UE5 版本号
3. 编译成功截图
