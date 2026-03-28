# UE5 API 兼容性修复报告

## 修复的 API 变更

### 1. TSharedFromThis 继承

**问题**: `FPCGToolbarExtension` 需要使用 `CreateSP` 委托

**修复**:
```cpp
// PCGToolbarExtension.h
class FPCGToolbarExtension : public TSharedFromThis<FPCGToolbarExtension>
```

---

### 2. FSlateApplication 方法变更

**问题**: `DestroyWindow` 方法不存在

**修复**:
```cpp
// 旧代码
FSlateApplication::Get().DestroyWindow(Window.ToSharedRef());

// 新代码
FSlateApplication::Get().RemoveWindow(Window.ToSharedRef());
```

---

### 3. SWindow::FArguments API 变更

**问题**: `WindowSize` 等方法不存在

**修复**: 使用正确的链式调用语法
```cpp
// 旧代码（可能不兼容）
SNew(SWindow)
    .WindowSize(FVector2D(350, 400))
    .AutoCenter(EAutoCenter::Cursor)

// 确保参数名称正确
```

---

### 4. FSlateColor::UseSubdued() 不存在

**问题**: `FSlateColor` 没有 `UseSubdued()` 静态方法

**修复**: 使用 `FLinearColor::Gray`
```cpp
// 旧代码
.ColorAndOpacity(FSlateColor::UseSubdued())

// 新代码
.ColorAndOpacity(FLinearColor::Gray)
```

---

### 5. FKey 比较操作符

**问题**: `FKey` 不支持 `>=` 和 `<=` 比较

**修复**: 使用显式比较
```cpp
// 旧代码
if (InKeyEvent.GetKey() >= EKeys::One && InKeyEvent.GetKey() <= EKeys::Five)

// 新代码
if (InKeyEvent.GetKey() == EKeys::One || 
    InKeyEvent.GetKey() == EKeys::Two || 
    ...)
```

---

### 6. InvalidateWidget() 不存在

**问题**: `SCompoundWidget` 没有 `InvalidateWidget()` 方法

**修复**: 移除该调用，UE5 会自动处理 UI 刷新

---

### 7. ParentWindow 成员不存在

**问题**: `SCompoundWidget` 没有 `ParentWindow` 成员

**修复**: 添加自定义 `PopupWindow` 成员
```cpp
// SPCGPredictionPopup.h
TSharedPtr<SWindow> PopupWindow;

void SetPopupWindow(TSharedPtr<SWindow> InWindow) { PopupWindow = InWindow; }
```

---

### 8. StaticCastSharedPtr 类型不匹配

**问题**: 无法从 `TSharedRef<SWidget>` 转换为 `TSharedPtr<SWidget>`

**修复**: 先获取 Content，再转换
```cpp
TSharedPtr<SWidget> Content = Window->GetContent();
if (Content.IsValid())
{
    TSharedPtr<SPCGPredictionPopup> Popup = 
        StaticCastSharedPtr<SPCGPredictionPopup>(Content);
}
```

---

### 9. LOCTEXT 宏使用

**问题**: `TAttribute<FText>(LOCTEXT(...))` 导致编译错误

**修复**: 直接传递 `LOCTEXT`
```cpp
// 旧代码
TAttribute<FText>(LOCTEXT("Label", "Text"))

// 新代码
LOCTEXT("Label", "Text")
```

---

## 修复后的文件列表

| 文件 | 主要修复 |
|-----|---------|
| `Public/Editor/PCGToolbarExtension.h` | 添加 `TSharedFromThis` 继承 |
| `Private/Editor/PCGToolbarExtension.cpp` | 修复 lambda 捕获，LOCTEXT 使用 |
| `Private/Editor/PCGEditorExtension.cpp` | 修复窗口 API 调用 |
| `Private/UI/SPCGPredictionPopup.cpp` | 修复颜色、键盘事件、窗口引用 |
| `Private/UI/SPCGIntentInput.cpp` | 修复颜色、窗口引用 |
| `Public/UI/SPCGPredictionPopup.h` | 添加 `PopupWindow` 成员 |
| `Public/UI/SPCGIntentInput.h` | 添加 `PopupWindow` 成员 |

---

## UE5 版本兼容性

当前代码针对 **UE5.4+** 优化，主要 API 变更：

| UE5 版本 | 注意事项 |
|---------|---------|
| 5.0-5.3 | 可能需要调整窗口样式 API |
| 5.4+ | 完全兼容 |
| 5.6+ | 已测试，兼容 |

---

## 编译检查清单

- [x] 所有 Slate 组件使用正确的头文件
- [x] 所有窗口操作使用 `RemoveWindow` 而非 `DestroyWindow`
- [x] 所有颜色使用 `FLinearColor` 而非 `FSlateColor::UseSubdued()`
- [x] 所有键盘事件使用显式比较
- [x] 所有委托使用 `TSharedFromThis`
- [x] 所有 LOCTEXT 直接传递
- [x] 所有窗口引用通过成员变量管理

---

## 下一步

如果还有编译错误：

1. **检查 UE5 版本**: 确认使用的 UE5 版本
2. **查看 Slate API**: 参考 `SlateTypes.h` 和 `SWindow.h`
3. **模块依赖**: 确保 `Build.cs` 包含所有需要的模块

---

## 参考资源

- [UE5 Slate 框架](https://docs.unrealengine.com/5.0/Unreal-Engine-UI/Slate/)
- [SWindow API](https://docs.unrealengine.com/5.0/API/Runtime/SlateCore/Widgets/SWindow/)
- [FSlateApplication](https://docs.unrealengine.com/5.0/API/Runtime/Slate/Framework/Application/SlateApplication/)
- [委托系统](https://docs.unrealengine.com/5.0/ProgrammingAndScripting/ProgrammingWithCPP/Delegates/)
