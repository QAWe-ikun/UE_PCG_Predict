#include "UI/SPCGPredictionPopup.h"
#include "Core/PCGPredictorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PCGPredictionPopup"

void SPCGPredictionPopup::Construct(const FArguments &InArgs) {
  SelectedIndex = 0;
  bHasCurrentConnection = false;

  ChildSlot
      [SNew(SBorder)
           .BorderImage(FCoreStyle::Get().GetBrush("Menu.Background"))
           .Padding(10)
               [SNew(SVerticalBox)

                // 标题行：显示选中的节点名称
                +
                SVerticalBox::Slot().AutoHeight()
                    [SNew(SHorizontalBox) +
                     SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                         [SNew(STextBlock)
                              .Text(
                                  LOCTEXT("PredictionTitle", "🔮 PCG Predict"))
                              .Font(
                                  FCoreStyle::GetDefaultFontStyle("Bold", 12))]

                     + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]

                     // 显示选中的节点名称
                     + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                           [SAssignNew(SelectedNodeText, STextBlock)
                                .Text_Lambda([this]() {
                                  return SelectedNodeName.IsEmpty()
                                             ? FText::FromString(
                                                   TEXT("No selection"))
                                             : FText::FromString(
                                                   SelectedNodeName);
                                })
                                .Font(FCoreStyle::GetDefaultFontStyle("Italic",
                                                                      10))
                                .ColorAndOpacity(FLinearColor::Gray)]]

                + SVerticalBox::Slot().AutoHeight().Padding(
                      0, 5, 0, 10)[SNew(SSeparator)]

                // 候选列表标题
                +
                SVerticalBox::Slot().AutoHeight().Padding(0, 0, 0, 5)
                    [SNew(STextBlock)
                         .Text(LOCTEXT("CandidatesTitle", "Recommended Nodes"))
                         .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                         .ColorAndOpacity(FLinearColor::Gray)]

                // 候选列表
                + SVerticalBox::Slot().FillHeight(
                      1.0f)[SAssignNew(CandidateListBox, SVerticalBox)]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0,
                                                            5)[SNew(SSeparator)]

                + SVerticalBox::Slot().AutoHeight()[this->BuildDebugEntry()]]];

  // 初始构建空列表
  RebuildCandidateList();
}

void SPCGPredictionPopup::SetSelectedNodeName(const FString &InNodeName) {
  SelectedNodeName = InNodeName;
  UE_LOG(LogTemp, Log, TEXT("[SPCGPredictionPopup] Set node name: %s"),
         *InNodeName);

  // 更新节点名称显示
  if (SelectedNodeText.IsValid()) {
    SelectedNodeText->SetText(SelectedNodeName.IsEmpty()
                                  ? FText::FromString(TEXT("No selection"))
                                  : FText::FromString(SelectedNodeName));
  }
}

void SPCGPredictionPopup::SetPredictorEngine(FPCGPredictorEngine *InEngine) {
  PredictorEngine = InEngine;
}

void SPCGPredictionPopup::SetCurrentIntent(const FString &InIntent) {
  CurrentIntent = InIntent;
  UE_LOG(LogTemp, Log, TEXT("[SPCGPredictionPopup] Set intent: %s"), *InIntent);
}

void SPCGPredictionPopup::UpdatePredictions(
    const TArray<FPCGCandidate> &InCandidates,
    EPCGPredictPinDirection Direction) {
  Candidates = InCandidates;
  CurrentDirection = Direction;
  SelectedIndex = 0;

  UE_LOG(LogTemp, Log, TEXT("[SPCGPredictionPopup] Updated with %d candidates"),
         Candidates.Num());

  // 重建候选列表
  RebuildCandidateList();
}

void SPCGPredictionPopup::RebuildCandidateList() {
  if (!CandidateListBox.IsValid()) {
    UE_LOG(LogTemp, Warning,
           TEXT("[SPCGPredictionPopup] CandidateListBox is not valid"));
    return;
  }

  // 清空现有内容
  CandidateListBox->ClearChildren();

  UE_LOG(LogTemp, Log,
         TEXT("[SPCGPredictionPopup] Building %d candidate items"),
         Candidates.Num());

  // 候选列表
  for (int32 i = 0; i < Candidates.Num() && i < 5; ++i) {
    CandidateListBox->AddSlot().AutoHeight().Padding(
        5, 2)[BuildCandidateWidget(Candidates[i], i)];

    UE_LOG(LogTemp, Log, TEXT("[SPCGPredictionPopup] Added candidate %d: %s"),
           i, *Candidates[i].NodeTypeName);
  }

  if (Candidates.Num() == 0) {
    UE_LOG(LogTemp, Log,
           TEXT("[SPCGPredictionPopup] No candidates to display"));
    CandidateListBox->AddSlot().AutoHeight().Padding(
        5, 2)[SNew(STextBlock)
                  .Text(LOCTEXT("NoCandidates", "No predictions available"))
                  .ColorAndOpacity(FLinearColor::Gray)];
  }
}

TSharedRef<SWidget>
SPCGPredictionPopup::BuildCandidateWidget(const FPCGCandidate &Candidate,
                                          int32 Index) {
  FString ScoreText =
      FString::FromInt(FMath::FloorToInt(Candidate.Score * 100)) + TEXT("%");
  FString IndexStr = FString::FromInt(Index + 1);

  return SNew(SButton)
      .OnClicked_Lambda([this, Index]() { return OnCandidateClicked(Index); })
      .HAlign(HAlign_Left)
      .ContentPadding(
          5)[SNew(SHorizontalBox) +
             SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                 [SNew(STextBlock)
                      .Text(FText::FromString(IndexStr))
                      .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                      .ColorAndOpacity(FLinearColor::Gray)]

             + SHorizontalBox::Slot().AutoWidth().Padding(10, 0).VAlign(
                   VAlign_Center)
                   [SNew(STextBlock)
                        .Text(FText::FromString(Candidate.NodeTypeName))
                        .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))]

             + SHorizontalBox::Slot().FillWidth(1.0f)[SNew(SSpacer)]

             + SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                   [SNew(STextBlock)
                        .Text(FText::FromString(ScoreText))
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))
                        .ColorAndOpacity(FLinearColor::White)]];
}

TSharedRef<SWidget> SPCGPredictionPopup::BuildDebugEntry() {
  return SNew(SButton)
      .OnClicked_Lambda([this]() { return OnDebugInsertClicked(); })
      .HAlign(HAlign_Left)
      .ContentPadding(5)
          [SNew(SHorizontalBox) +
           SHorizontalBox::Slot().AutoWidth().VAlign(
               VAlign_Center)[SNew(STextBlock).Text(LOCTEXT("DebugIcon", "🔍"))]

           +
           SHorizontalBox::Slot().AutoWidth().Padding(10, 0).VAlign(
               VAlign_Center)
               [SNew(SVerticalBox) +
                SVerticalBox::Slot().AutoHeight()
                    [SNew(STextBlock)
                         .Text(LOCTEXT("DebugTitle", "Insert Debug Node"))
                         .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))]

                +
                SVerticalBox::Slot().AutoHeight()
                    [SNew(STextBlock)
                         .Text(LOCTEXT("DebugDesc", "Observe data flow state"))
                         .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                         .ColorAndOpacity(FLinearColor::Gray)]]];
}

TSharedRef<SWidget> SPCGPredictionPopup::BuildCandidateList() {
  TSharedRef<SVerticalBox> CandidateBox = SNew(SVerticalBox);

  if (bHasCurrentConnection) {
    CandidateBox->AddSlot().AutoHeight().Padding(
        5)[SNew(SHorizontalBox) +
           SHorizontalBox::Slot().AutoWidth()
               [SNew(STextBlock)
                    .Text(FText::Format(
                        LOCTEXT("CurrentConnection", "Current: {0}"),
                        FText::FromString(CurrentConnectedNode)))
                    .ColorAndOpacity(FLinearColor::Gray)]];

    CandidateBox->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[SNew(SSeparator)];
  }

  for (int32 i = 0; i < Candidates.Num() && i < 5; ++i) {
    CandidateBox->AddSlot().AutoHeight().Padding(
        5, 2)[BuildCandidateWidget(Candidates[i], i)];
  }

  return CandidateBox;
}

TSharedRef<SWidget> SPCGPredictionPopup::BuildCurrentConnections() {
  return SNew(SVerticalBox);
}

FReply SPCGPredictionPopup::OnCandidateClicked(int32 Index) {
  if (Index >= 0 && Index < Candidates.Num()) {
    UE_LOG(LogTemp, Log, TEXT("Selected candidate %d: %s"), Index,
           *Candidates[Index].NodeTypeName);
    ClosePopup();
  }

  return FReply::Handled();
}

FReply SPCGPredictionPopup::OnDebugInsertClicked() {
  UE_LOG(LogTemp, Log, TEXT("Insert Debug node clicked"));
  ClosePopup();

  return FReply::Handled();
}

FReply SPCGPredictionPopup::OnKeyDown(const FGeometry &MyGeometry,
                                      const FKeyEvent &InKeyEvent) {
  if (InKeyEvent.GetKey() == EKeys::Escape) {
    ClosePopup();
    return FReply::Handled();
  }

  if (InKeyEvent.GetKey() == EKeys::One || InKeyEvent.GetKey() == EKeys::Two ||
      InKeyEvent.GetKey() == EKeys::Three ||
      InKeyEvent.GetKey() == EKeys::Four ||
      InKeyEvent.GetKey() == EKeys::Five) {
    int32 Index = InKeyEvent.GetKey() == EKeys::One     ? 0
                  : InKeyEvent.GetKey() == EKeys::Two   ? 1
                  : InKeyEvent.GetKey() == EKeys::Three ? 2
                  : InKeyEvent.GetKey() == EKeys::Four  ? 3
                                                        : 4;

    if (Index < Candidates.Num()) {
      OnCandidateClicked(Index);
    }
    return FReply::Handled();
  }

  if (InKeyEvent.GetKey() == EKeys::Zero) {
    OnDebugInsertClicked();
    return FReply::Handled();
  }

  if (InKeyEvent.GetKey() == EKeys::Up) {
    SelectedIndex = FMath::Clamp(SelectedIndex - 1, 0, Candidates.Num() - 1);
    return FReply::Handled();
  }

  if (InKeyEvent.GetKey() == EKeys::Down) {
    SelectedIndex = FMath::Clamp(SelectedIndex + 1, 0, Candidates.Num() - 1);
    return FReply::Handled();
  }

  if (InKeyEvent.GetKey() == EKeys::Enter) {
    OnCandidateClicked(SelectedIndex);
    return FReply::Handled();
  }

  return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

void SPCGPredictionPopup::ClosePopup() {
  if (PopupWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        PopupWindow.ToSharedRef());
  }
}

#undef LOCTEXT_NAMESPACE
