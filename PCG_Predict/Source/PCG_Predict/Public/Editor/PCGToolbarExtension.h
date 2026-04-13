#pragma once

#include "CoreMinimal.h"

/**
 * PCG 预测工具栏扩展
 * 在编辑器工具栏添加预测功能入口
 */
class FPCGEditorExtension;

class FPCGToolbarExtension : public TSharedFromThis<FPCGToolbarExtension> {
public:
  void Initialize(TSharedPtr<FPCGEditorExtension> InEditorExtension);
  void Shutdown();

private:
  TSharedPtr<FPCGEditorExtension> EditorExtension;

  /** 扩展工具栏 */
  void ExtendToolbar();

  /** 工具栏按钮回调 */
  void OnTogglePredictionClicked();
  void OnIntentButtonClicked();
};
