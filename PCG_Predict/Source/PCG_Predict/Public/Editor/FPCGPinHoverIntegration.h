#pragma once

#include "Core/PCGPredictionTypes.h"
#include "CoreMinimal.h"
#include "Input/Reply.h"
#include "Widgets/SWidget.h"

class FPCGPredictorEngine;
class SPCGPredictionPopup;

/**
 * PCG Pin 悬停集成（无需修改引擎源码）
 * 使用 Frame Tick 轮询检测实现
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

  /** 检测 Pin（公开给模块 Tick 调用） */
  void DetectPinUnderCursor();

private:
  /** 预测引擎 */
  TSharedPtr<FPCGPredictorEngine> PredictorEngine;

  /** 预测浮层窗口 */
  TSharedPtr<SWindow> PredictionPopupWindow;

  /** 创建预测浮层 */
  void CreatePredictionPopup();

  /** 更新预测内容 */
  void UpdatePredictionContent(const FString &PinName,
                               const FString &Direction);

  /** 获取预测方向 */
  EPCGPredictPinDirection GetPredictDirection(const FString &Direction);

  /** 上次检测到的 Widget 类型 */
  FString LastDetectedWidgetType;

  /** 连续检测计数（防抖） */
  int32 ConsecutiveDetectionCount;
};
