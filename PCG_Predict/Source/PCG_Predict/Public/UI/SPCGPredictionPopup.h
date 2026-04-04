#pragma once

#include "Core/PCGPredictionTypes.h"
#include "CoreMinimal.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Text/STextBlock.h"

class FPCGPredictorEngine;

/**
 * PCG 预测结果浮层 UI
 * 显示选中的节点名称和随机推荐节点
 */
class SPCGPredictionPopup : public SCompoundWidget {
public:
  SLATE_BEGIN_ARGS(SPCGPredictionPopup) : _MaxCandidates(5) {}
  SLATE_ARGUMENT(int32, MaxCandidates)
  SLATE_END_ARGS()

  void Construct(const FArguments &InArgs);

  /** 设置预测引擎引用 */
  void SetPredictorEngine(FPCGPredictorEngine *InEngine);

  /** 更新预测结果 */
  void UpdatePredictions(const TArray<FPCGCandidate> &InCandidates,
                         EPCGPredictPinDirection Direction);

  /** 设置当前意图 */
  void SetCurrentIntent(const FString &InIntent);

  /** 关闭浮层 */
  void ClosePopup();

  /** 设置窗口引用 */
  void SetPopupWindow(TSharedPtr<SWindow> InWindow) { PopupWindow = InWindow; }

  /** 设置选中的节点名称 */
  void SetSelectedNodeName(const FString &InNodeName);

  /** 重建候选列表 */
  void RebuildCandidateList();

protected:
  /** 构建候选列表 UI */
  TSharedRef<SWidget> BuildCandidateList();

  /** 构建单个候选项 */
  TSharedRef<SWidget> BuildCandidateWidget(const FPCGCandidate &Candidate,
                                           int32 Index);

  /** 构建 Debug 入口 */
  TSharedRef<SWidget> BuildDebugEntry();

  /** 构建当前连接显示（如果有） */
  TSharedRef<SWidget> BuildCurrentConnections();

  // 事件处理
  FReply OnCandidateClicked(int32 Index);
  FReply OnDebugInsertClicked();
  FReply OnKeyDown(const FGeometry &MyGeometry, const FKeyEvent &InKeyEvent);

private:
  /** 预测引擎 */
  FPCGPredictorEngine *PredictorEngine;

  /** 当前候选列表 */
  TArray<FPCGCandidate> Candidates;

  /** 当前预测方向 */
  EPCGPredictPinDirection CurrentDirection;

  /** 当前意图文本 */
  FString CurrentIntent;

  /** 选中的索引 */
  int32 SelectedIndex;

  /** 是否显示当前连接 */
  bool bHasCurrentConnection;

  /** 当前连接的节点名称 */
  FString CurrentConnectedNode;

  /** 当前选中的节点名称 */
  FString SelectedNodeName;

  /** 弹窗窗口引用 */
  TSharedPtr<SWindow> PopupWindow;

  /** 候选列表 VerticalBox */
  TSharedPtr<SVerticalBox> CandidateListBox;

  /** 节点名称 TextBlock 引用 */
  TSharedPtr<STextBlock> SelectedNodeText;
};
