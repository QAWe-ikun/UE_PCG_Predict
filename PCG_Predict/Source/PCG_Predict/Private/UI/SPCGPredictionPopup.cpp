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
               [SNew(SVerticalBox) +
                SVerticalBox::Slot().AutoHeight()
                    [SNew(SHorizontalBox) +
                     SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                         [SNew(STextBlock)
                              .Text(LOCTEXT("PredictionTitle", "🔮 PCG 预测"))
                              .Font(
                                  FCoreStyle::GetDefaultFontStyle("Bold", 12))]]

                + SVerticalBox::Slot().AutoHeight().Padding(
                      0, 5, 0, 10)[SNew(SSeparator)]

                + SVerticalBox::Slot().FillHeight(
                      1.0f)[this->BuildCandidateList()]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0,
                                                            5)[SNew(SSeparator)]

                + SVerticalBox::Slot().AutoHeight()[this->BuildDebugEntry()]]];
}

void SPCGPredictionPopup::SetPredictorEngine(FPCGPredictorEngine *InEngine) {
  PredictorEngine = InEngine;
}

void SPCGPredictionPopup::UpdatePredictions(
    const TArray<FPCGCandidate> &InCandidates,
    EPCGPredictPinDirection Direction) {
  Candidates = InCandidates;
  CurrentDirection = Direction;
  SelectedIndex = 0;
}

void SPCGPredictionPopup::SetCurrentIntent(const FString &InIntent) {
  CurrentIntent = InIntent;
}

void SPCGPredictionPopup::ClosePopup() {
  if (PopupWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        PopupWindow.ToSharedRef());
  }
}

TSharedRef<SWidget> SPCGPredictionPopup::BuildCandidateList() {
  TSharedRef<SVerticalBox> CandidateBox = SNew(SVerticalBox);

  if (bHasCurrentConnection) {
    CandidateBox->AddSlot().AutoHeight().Padding(
        5)[SNew(SHorizontalBox) +
           SHorizontalBox::Slot().AutoWidth()
               [SNew(STextBlock)
                    .Text(FText::Format(
                        LOCTEXT("CurrentConnection", "当前连接：{0}"),
                        FText::FromString(CurrentConnectedNode)))
                    .ColorAndOpacity(FLinearColor::Gray)]];

    CandidateBox->AddSlot().AutoHeight().Padding(0, 0, 0, 10)[SNew(SSeparator)];
  }

  FString HeaderText = (CurrentDirection == EPCGPredictPinDirection::Input)
                           ? TEXT("替代方案")
                           : TEXT("推荐新连接");

  CandidateBox->AddSlot().AutoHeight().Padding(
      0, 0, 0, 5)[SNew(STextBlock)
                      .Text(FText::FromString(HeaderText))
                      .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))];

  for (int32 i = 0; i < Candidates.Num() && i < 5; ++i) {
    CandidateBox->AddSlot().AutoHeight().Padding(
        5, 2)[BuildCandidateWidget(Candidates[i], i)];
  }

  return CandidateBox;
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

           + SHorizontalBox::Slot().AutoWidth().Padding(10, 0).VAlign(
                 VAlign_Center)
                 [SNew(SVerticalBox) +
                  SVerticalBox::Slot().AutoHeight()
                      [SNew(STextBlock)
                           .Text(LOCTEXT("DebugTitle", "插入 Debug 节点"))
                           .Font(FCoreStyle::GetDefaultFontStyle("Bold", 10))] +
                  SVerticalBox::Slot().AutoHeight()
                      [SNew(STextBlock)
                           .Text(LOCTEXT("DebugDesc", "观察当前数据流状态"))
                           .Font(FCoreStyle::GetDefaultFontStyle("Regular", 9))
                           .ColorAndOpacity(FLinearColor::Gray)]]];
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

#undef LOCTEXT_NAMESPACE
