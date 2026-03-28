#pragma once

#include "CoreMinimal.h"
#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE_OneParam(FOnIntentConfirmed, const FString &)

    /**
     * 意图输入窗口
     * 允许用户输入自然语言描述场景意图
     */
    class SPCGIntentInput : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SPCGIntentInput) {}
  SLATE_EVENT(FOnIntentConfirmed, OnIntentConfirmed)
  SLATE_END_ARGS()

  void Construct(const FArguments &InArgs);

  /** 设置预填充的意图文本 */
  void SetPresetIntent(const FString &PresetText);

  /** 关闭输入窗口 */
  void CloseInput();

  /** 设置窗口引用 */
  void SetPopupWindow(TSharedPtr<SWindow> InWindow) { PopupWindow = InWindow; }

protected:
  // 事件处理
  FReply OnConfirmClicked();
  FReply OnCancelClicked();
  FReply OnQuickSceneSelected(const FString &SceneType);
  FReply OnKeyDown(const FGeometry &MyGeometry, const FKeyEvent &InKeyEvent);

  /** 构建快速选择按钮 */
  TSharedRef<SWidget> BuildQuickSelectionRow();

private:
  /** 意图文本输入框 */
  TSharedPtr<SEditableTextBox> IntentTextBox;

  /** 确认回调 */
  FOnIntentConfirmed OnIntentConfirmedDelegate;

  /** 快速场景选择 */
  struct FSceneQuickSelect {
    FString DisplayName;
    FString SceneType;
    FString Icon;
  };

  TArray<FSceneQuickSelect> SceneQuickSelects;

  /** 弹窗窗口引用 */
  TSharedPtr<SWindow> PopupWindow;
};
