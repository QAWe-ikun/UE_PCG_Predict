# UE5.6 API 兼容性修复

## 关键 API 变更 (UE5.6)

### 1. 窗口销毁方法

**旧 API**: `DestroyWindow()` 或 `RemoveWindow()`
**新 API**: `DestroyWindowImmediately()`

```cpp
// UE5.6
FSlateApplication::Get().DestroyWindowImmediately(Window.ToSharedRef());
```

---

### 2. SWindow 参数名称

**旧 API**: `.WindowSize()`
**新 API**: `.InitialSize()`

```cpp
// UE5.6
SNew(SWindow)
    .InitialSize(FVector2D(350, 400))
```

---

### 3. EAutoCenter 枚举

**旧 API**: `EAutoCenter::Cursor`
**新 API**: `EAutoCenter::PreferredArea`

```cpp
// UE5.6
.AutoCenter(EAutoCenter::PreferredArea)
```

---

### 4. SButton OnClicked 委托

**问题**: 带参数的成员函数指针无法直接绑定

**修复**: 使用 Lambda 捕获参数

```cpp
// 错误写法
.OnClicked(this, &Class::Method, Param)

// 正确写法 (UE5.6)
.OnClicked_Lambda([this, Param]() 
{ 
    return Method(Param); 
})
```

---

### 5. DrawWindowIfNecessary 移除

**旧 API**: `FSlateApplication::Get().DrawWindowIfNecessary()`
**修复**: 直接调用 `ShowWindow()`，UE 自动处理绘制

```cpp
// 只需调用
Window->ShowWindow();
```

---

## 修复后的完整代码示例

### SWindow 创建 (UE5.6)

```cpp
TSharedPtr<SWindow> Window = SNew(SWindow)
    .InitialSize(FVector2D(400, 300))
    .AutoCenter(EAutoCenter::PreferredArea)
    .SupportsMaximize(false)
    .SupportsMinimize(false)
    .SizingRule(ESizingRule::Autosized)
    .IsTopmostWindow(true)
    [
        // 内容 Widget
    ];

FSlateApplication::Get().AddWindow(Window.ToSharedRef());
```

---

### SButton 带参数点击事件

```cpp
// 使用 Lambda 捕获
SNew(SButton)
    .OnClicked_Lambda([this, Value]() 
    { 
        return OnButtonClicked(Value); 
    })

// 或使用 CreateSP 如果函数无参数
SNew(SButton)
    .OnClicked(FSimpleDelegate::CreateSP(this, &Class::OnClicked))
```

---

### 窗口关闭

```cpp
void SMyWidget::Close()
{
    if (ParentWindow.IsValid())
    {
        FSlateApplication::Get().DestroyWindowImmediately(
            ParentWindow.ToSharedRef()
        );
    }
}
```

---

## 已修复的文件

| 文件 | 修复内容 |
|-----|---------|
| `SPCGIntentInput.cpp` | Lambda 绑定，DestroyWindowImmediately |
| `SPCGPredictionPopup.cpp` | Lambda 绑定，DestroyWindowImmediately |
| `PCGEditorExtension.cpp` | InitialSize, PreferredArea, 移除 DrawWindowIfNecessary |

---

## UE5 版本 API 对比

| 功能 | UE5.0-5.5 | UE5.6+ |
|-----|----------|--------|
| 关闭窗口 | `RemoveWindow()` | `DestroyWindowImmediately()` |
| 窗口大小 | `.WindowSize()` | `.InitialSize()` |
| 自动居中 | `EAutoCenter::Cursor` | `EAutoCenter::PreferredArea` |
| 按钮委托 | `.OnClicked(this, &Method, args)` | `.OnClicked_Lambda(...)` |
| 绘制窗口 | `DrawWindowIfNecessary()` | 自动处理 |

---

## 编译通过检查清单

- [x] 所有窗口使用 `DestroyWindowImmediately()`
- [x] 所有窗口大小使用 `.InitialSize()`
- [x] 所有自动居中使用 `EAutoCenter::PreferredArea`
- [x] 所有带参数按钮点击使用 Lambda
- [x] 移除所有 `DrawWindowIfNecessary()` 调用

---

## 参考

- [UE5.6 Release Notes](https://docs.unrealengine.com/5.6/en-US/unreal-engine-5.6-release-notes/)
- [Slate Framework](https://docs.unrealengine.com/5.6/en-US/slate-ui-framework/)
- [SWindow API](https://docs.unrealengine.com/5.6/en-US/API/Runtime/SlateCore/Widgets/SWindow/)
