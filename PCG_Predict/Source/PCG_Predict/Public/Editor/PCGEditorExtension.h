#pragma once

#include "Core/PCGPredictionTypes.h"
#include "CoreMinimal.h"
#include "Input/Reply.h"

class SPCGPredictionPopup;
class SPCGIntentInput;
class FPCGPredictorEngine;

/**
 * PCG 预测编辑器扩展
 * 集成到 PCG Graph 编辑器中
 */
class FPCGEditorExtension {
public:
  void Initialize();
  void Shutdown();

  /** 显示预测浮层 */
  void ShowPredictionPopup(EPCGPredictPinDirection Direction);

  /** 显示意图输入窗口 */
  void ShowIntentInput();

  /** 设置当前意图 */
  void SetCurrentIntent(const FString &Intent);

  /** 获取预测引擎 */
  TSharedPtr<FPCGPredictorEngine> GetPredictorEngine() {
    return PredictorEngine;
  }

private:
  /** 预测引擎 */
  TSharedPtr<FPCGPredictorEngine> PredictorEngine;

  /** 预测浮层窗口 */
  TSharedPtr<SWindow> PredictionPopupWindow;

  /** 意图输入窗口 */
  TSharedPtr<SWindow> IntentInputWindow;

  /** 当前意图 */
  FString CurrentIntent;

  /** 创建预测浮层窗口 */
  void CreatePredictionPopupWindow();

  /** 创建意图输入窗口 */
  void CreateIntentInputWindow();

  /** 注册编辑器命令 */
  void RegisterCommands();

  /** 绑定到 PCG Graph 编辑器 */
  void BindToPCGGraphEditor();
};
