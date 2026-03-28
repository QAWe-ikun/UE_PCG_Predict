#include "Editor/PCGEditorExtension.h"
#include "Core/PCGPredictorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/SPCGIntentInput.h"
#include "UI/SPCGPredictionPopup.h"
#include "Widgets/SWindow.h"

void FPCGEditorExtension::Initialize() {
  PredictorEngine = MakeShareable(new FPCGPredictorEngine());
  PredictorEngine->Initialize(TEXT(""));

  RegisterCommands();
  BindToPCGGraphEditor();

  UE_LOG(LogTemp, Log, TEXT("PCG Editor Extension Initialized"));
}

void FPCGEditorExtension::Shutdown() {
  if (PredictionPopupWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        PredictionPopupWindow.ToSharedRef());
  }

  if (IntentInputWindow.IsValid()) {
    FSlateApplication::Get().DestroyWindowImmediately(
        IntentInputWindow.ToSharedRef());
  }

  PredictorEngine.Reset();
}

void FPCGEditorExtension::RegisterCommands() {}

void FPCGEditorExtension::BindToPCGGraphEditor() {}

void FPCGEditorExtension::ShowPredictionPopup(
    EPCGPredictPinDirection Direction) {
  if (!PredictionPopupWindow.IsValid()) {
    CreatePredictionPopupWindow();
  }

  TArray<FPCGCandidate> Candidates = PredictorEngine->Predict(Direction);

  if (PredictionPopupWindow.IsValid()) {
    TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
    if (Content.IsValid()) {
      TSharedPtr<SPCGPredictionPopup> Popup =
          StaticCastSharedPtr<SPCGPredictionPopup>(Content);

      if (Popup.IsValid()) {
        Popup->SetPredictorEngine(PredictorEngine.Get());
        Popup->UpdatePredictions(Candidates, Direction);
        Popup->SetCurrentIntent(CurrentIntent);
      }
    }
  }

  if (PredictionPopupWindow.IsValid()) {
    PredictionPopupWindow->ShowWindow();
  }
}

void FPCGEditorExtension::ShowIntentInput() {
  if (!IntentInputWindow.IsValid()) {
    CreateIntentInputWindow();
  }

  if (IntentInputWindow.IsValid()) {
    IntentInputWindow->ShowWindow();
  }
}

void FPCGEditorExtension::SetCurrentIntent(const FString &Intent) {
  CurrentIntent = Intent;
  PredictorEngine->SetIntent(Intent);
}

void FPCGEditorExtension::CreatePredictionPopupWindow() {
  PredictionPopupWindow =
      SNew(SWindow)
          .ClientSize(FVector2D(350, 400))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SupportsMaximize(false)
          .SupportsMinimize(false)
          .SizingRule(ESizingRule::Autosized)
          .IsTopmostWindow(true)[SNew(SPCGPredictionPopup).MaxCandidates(5)];

  TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
  if (Content.IsValid()) {
    TSharedPtr<SPCGPredictionPopup> Popup =
        StaticCastSharedPtr<SPCGPredictionPopup>(Content);
    if (Popup.IsValid()) {
      Popup->SetPopupWindow(PredictionPopupWindow);
    }
  }

  FSlateApplication::Get().AddWindow(PredictionPopupWindow.ToSharedRef());
}

void FPCGEditorExtension::CreateIntentInputWindow() {
  IntentInputWindow =
      SNew(SWindow)
          .ClientSize(FVector2D(450, 300))
          .AutoCenter(EAutoCenter::PreferredWorkArea)
          .SupportsMaximize(false)
          .SupportsMinimize(false)
          .SizingRule(ESizingRule::Autosized)
          .IsTopmostWindow(true)[SNew(SPCGIntentInput)
                                     .OnIntentConfirmed_Lambda(
                                         [this](const FString &NewIntent) {
                                           SetCurrentIntent(NewIntent);
                                           UE_LOG(LogTemp, Log,
                                                  TEXT("Intent set to: %s"),
                                                  *NewIntent);
                                         })];

  TSharedPtr<SWidget> Content = IntentInputWindow->GetContent();
  if (Content.IsValid()) {
    TSharedPtr<SPCGIntentInput> Input =
        StaticCastSharedPtr<SPCGIntentInput>(Content);
    if (Input.IsValid()) {
      Input->SetPopupWindow(IntentInputWindow);
    }
  }

  FSlateApplication::Get().AddWindow(IntentInputWindow.ToSharedRef());
}
