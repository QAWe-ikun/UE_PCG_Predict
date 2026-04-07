#pragma once

#include "Core/PCGPredictionTypes.h"
#include "CoreMinimal.h"
#include "EdGraph/EdGraphPin.h"
#include "Input/Reply.h"
#include "SGraphPanel.h"
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
                      const FString &NodeName,
                      TSharedPtr<SGraphPanel> GraphPanel, UEdGraphPin *Pin);

  /** 设置候选点击回调 */
  using FOnCandidateClicked = TFunction<void(const FPCGCandidate &, int32)>;
  void SetOnCandidateClicked(FOnCandidateClicked Callback);

  /** 获取当前目标 Pin */
  UEdGraphPin *GetCurrentTargetPin() const { return CurrentTargetPin; }

  /** 获取当前节点名称 */
  FString GetCurrentNodeName() const { return CurrentNodeName; }

  /** 获取 Pin 方向 */
  EEdGraphPinDirection GetCurrentPinDirection() const {
    return CurrentPinDirection;
  }

  /** 获取当前 GraphPanel */
  TSharedPtr<SGraphPanel> GetCurrentGraphPanel() const {
    return CurrentGraphPanel;
  }

  /** 隐藏预测 */
  void HidePrediction();

  /** 显示起始节点预测（空白区域） */
  void ShowStarterNodePrediction(TSharedPtr<SGraphPanel> GraphPanel, FVector2D MousePos);

  /** 手动触发（从工具栏） */
  void TriggerPredictionFromToolbar();

  /** 检测鼠标下的 Pin（每帧调用） */
  void DetectPinUnderCursor();

  /** 获取预测引擎（供外部访问） */
  FPCGPredictorEngine *GetPredictorEngine() { return PredictorEngine.Get(); }

  /** 设置启用/禁用状态 */
  void SetEnabled(bool bEnabled);

  /** 获取启用状态 */
  bool IsEnabled() const { return bIsEnabled; }

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

  /** 当前目标 Pin */
  UEdGraphPin *CurrentTargetPin;

  /** 当前节点名称 */
  FString CurrentNodeName;

  /** 当前 Pin 方向 */
  EEdGraphPinDirection CurrentPinDirection;

  /** 当前 GraphPanel */
  TSharedPtr<SGraphPanel> CurrentGraphPanel;

  /** 当前鼠标位置 */
  FVector2D CurrentMousePosition;

  /** 候选点击回调 */
  FOnCandidateClicked OnCandidateClickedCallback;

  /** 是否启用预测功能 */
  bool bIsEnabled = true;

  /** 创建预测浮层 */
  void CreatePredictionPopup();

  /** 更新预测内容 */
  void UpdatePredictionContent(const FString &PinName,
                               const FString &Direction);

  /** 获取预测方向 */
  EPCGPredictPinDirection GetPredictDirection(const FString &Direction);
};
