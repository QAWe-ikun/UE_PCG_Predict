#pragma once

#include "Core/PCGPredictionTypes.h"
#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"

class FPCGPredictorEngine;
class SPCGPredictionPopup;

/**
 * PCG Pin 悬停集成
 * 使用 Slate 轮询检测实现（无需外部集成）
 */
class FPCGPinHoverIntegration
    : public TSharedFromThis<FPCGPinHoverIntegration> {
public:
  void Initialize();
  void Shutdown();

  /** 设置预测引擎 */
  void SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine);

  /** 显示预测 */
  void ShowPrediction(const FString &PinName, const FString &Direction,
                      const FString &NodeName);

  /** 隐藏预测 */
  void HidePrediction();

  /** 手动触发（从工具栏） */
  void TriggerPredictionFromToolbar();

  /** 检测鼠标下的 Pin（每帧调用） */
  void DetectPinUnderCursor();

  /** 获取预测引擎（供外部访问） */
  FPCGPredictorEngine *GetPredictorEngine() { return PredictorEngine.Get(); }

private:
  /** 预测引擎 */
  TSharedPtr<FPCGPredictorEngine> PredictorEngine;

  /** 预测浮层窗口 */
  TSharedPtr<SWindow> PredictionPopupWindow;

  /** 连续检测计数（防抖） */
  int32 ConsecutiveDetectionCount;

  /** 上次检测到的 Widget 类型 */
  FString LastDetectedWidgetType;

  /** 上次悬停时间（防抖） */
  double LastHoverTime;

  /** 创建预测浮层 */
  void CreatePredictionPopup();

  /** 更新预测内容 */
  void UpdatePredictionContent(const FString &PinName,
                               const FString &Direction);

  /** 获取预测方向 */
  EPCGPredictPinDirection GetPredictDirection(const FString &Direction);
};
