#include "UI/SPCGIntentInput.h"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PCGIntentInput"

void SPCGIntentInput::Construct(const FArguments &InArgs) {
  OnIntentConfirmedDelegate = InArgs._OnIntentConfirmed;

  SceneQuickSelects = {{TEXT("森林"), TEXT("Forest"), TEXT("🌲")},
                       {TEXT("城市"), TEXT("City"), TEXT("🏙️")},
                       {TEXT("建筑"), TEXT("Building"), TEXT("🏠")},
                       {TEXT("摊位"), TEXT("Stall"), TEXT("🏪")},
                       {TEXT("山地"), TEXT("Mountain"), TEXT("⛰️")}};

  ChildSlot
      [SNew(SBorder)
           .BorderImage(FCoreStyle::Get().GetBrush("Menu.Background"))
           .Padding(15)
               [SNew(SVerticalBox)

                + SVerticalBox::Slot().AutoHeight()
                      [SNew(STextBlock)
                           .Text(LOCTEXT("Title", "🎯 输入场景描述"))
                           .Font(FCoreStyle::GetDefaultFontStyle("Bold", 14))]

                + SVerticalBox::Slot().AutoHeight().Padding(
                      0, 10, 0, 15)[SNew(SSeparator)]

                +
                SVerticalBox::Slot().AutoHeight()
                    [SAssignNew(IntentTextBox, SEditableTextBox)
                         .HintText(LOCTEXT(
                             "HintText", "例如：茂密森林，高低错落，避开道路"))
                         .MinDesiredWidth(400)
                         .Font(FCoreStyle::GetDefaultFontStyle("Regular", 11))]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 15, 0, 10)
                      [SNew(STextBlock)
                           .Text(LOCTEXT("QuickSelect", "快速选择:"))
                           .Font(FCoreStyle::GetDefaultFontStyle("Regular", 10))
                           .ColorAndOpacity(FLinearColor::Gray)]

                + SVerticalBox::Slot().AutoHeight().Padding(
                      0, 0, 0, 15)[this->BuildQuickSelectionRow()]

                + SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0,
                                                            0)[SNew(SSeparator)]

                +
                SVerticalBox::Slot().AutoHeight().Padding(0, 10, 0, 0)
                    [SNew(SHorizontalBox) +
                     SHorizontalBox::Slot().FillWidth(1.0f)
                         [SNew(SButton)
                              .HAlign(HAlign_Center)
                              .OnClicked_Lambda(
                                  [this]() { return OnConfirmClicked(); })
                              .ContentPadding(
                                  10)[SNew(STextBlock)
                                          .Text(LOCTEXT("Confirm", "✓ 确认"))]]

                     + SHorizontalBox::Slot().AutoWidth().Padding(10, 0, 0, 0)
                           [SNew(SButton)
                                .HAlign(HAlign_Center)
                                .OnClicked_Lambda(
                                    [this]() { return OnCancelClicked(); })
                                .ContentPadding(
                                    10)[SNew(STextBlock)
                                            .Text(LOCTEXT("Cancel",
                                                          "✗ 取消"))]]]]];

  FSlateApplication::Get().SetKeyboardFocus(IntentTextBox);
}

void SPCGIntentInput::SetPresetIntent(const FString &PresetText) {
  if (IntentTextBox.IsValid()) {
    IntentTextBox->SetText(FText::FromString(PresetText));
  }
}

void SPCGIntentInput::CloseInput() {
  if (PopupWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        PopupWindow.ToSharedRef());
  }
}

TSharedRef<SWidget> SPCGIntentInput::BuildQuickSelectionRow() {
  TSharedRef<SHorizontalBox> QuickSelectBox = SNew(SHorizontalBox);

  for (const auto &Scene : SceneQuickSelects) {
    QuickSelectBox->AddSlot().AutoWidth().Padding(0, 0, 8, 0)
        [SNew(SButton)
             .OnClicked_Lambda([this, SceneType = Scene.SceneType]() {
               return OnQuickSceneSelected(SceneType);
             })
             .ContentPadding(
                 5)[SNew(SHorizontalBox) +
                    SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)
                        [SNew(STextBlock).Text(FText::FromString(Scene.Icon))]

                    + SHorizontalBox::Slot()
                          .AutoWidth()
                          .Padding(5, 0, 0, 0)
                          .VAlign(VAlign_Center)[SNew(STextBlock)
                                                     .Text(FText::FromString(
                                                         Scene.DisplayName))]]];
  }

  return QuickSelectBox;
}

FReply SPCGIntentInput::OnConfirmClicked() {
  if (IntentTextBox.IsValid()) {
    FString IntentText = IntentTextBox->GetText().ToString();
    if (!IntentText.IsEmpty()) {
      OnIntentConfirmedDelegate.ExecuteIfBound(IntentText);
    }
    CloseInput();
  }

  return FReply::Handled();
}

FReply SPCGIntentInput::OnCancelClicked() {
  CloseInput();
  return FReply::Handled();
}

FReply SPCGIntentInput::OnQuickSceneSelected(const FString &SceneType) {
  if (IntentTextBox.IsValid()) {
    FString CurrentText = IntentTextBox->GetText().ToString();
    FString NewText =
        CurrentText.IsEmpty() ? SceneType : CurrentText + ", " + SceneType;
    IntentTextBox->SetText(FText::FromString(NewText));
  }

  return FReply::Handled();
}

FReply SPCGIntentInput::OnKeyDown(const FGeometry &MyGeometry,
                                  const FKeyEvent &InKeyEvent) {
  if (InKeyEvent.GetKey() == EKeys::Escape) {
    CloseInput();
    return FReply::Handled();
  }

  if (InKeyEvent.GetKey() == EKeys::Enter) {
    OnConfirmClicked();
    return FReply::Handled();
  }

  return SCompoundWidget::OnKeyDown(MyGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
