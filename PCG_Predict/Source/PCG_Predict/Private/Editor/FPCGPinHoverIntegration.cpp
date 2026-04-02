#include "Editor/FPCGPinHoverIntegration.h"
#include "Core/PCGPredictorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/SPCGPredictionPopup.h"
#include "Widgets/SWindow.h"

void FPCGPinHoverIntegration::Initialize() {
  UE_LOG(LogTemp, Log, TEXT("========================================"));
  UE_LOG(LogTemp, Log, TEXT("PCG Pin Hover Integration Initialized"));
  UE_LOG(LogTemp, Log, TEXT("Frame-based polling (every 10 frames)"));
  UE_LOG(LogTemp, Log, TEXT("========================================"));

  ConsecutiveDetectionCount = 0;

  UE_LOG(LogTemp, Log, TEXT("✓ Initialization complete"));
}

void FPCGPinHoverIntegration::Shutdown() {
  if (PredictionPopupWindow.IsValid()) {
    PredictionPopupWindow.Reset();
  }
}

void FPCGPinHoverIntegration::SetPredictorEngine(
    TSharedPtr<FPCGPredictorEngine> Engine) {
  PredictorEngine = Engine;
}

void FPCGPinHoverIntegration::DetectPinUnderCursor() {
  UE_LOG(LogTemp, VeryVerbose, TEXT("[Tick] DetectPinUnderCursor called"));

  // 获取焦点下的 Widget
  TSharedPtr<SWidget> FocusedWidget =
      FSlateApplication::Get().GetUserFocusedWidget(0);

  if (!FocusedWidget.IsValid()) {
    UE_LOG(LogTemp, VeryVerbose, TEXT("[Tick] No focused widget"));
    HidePrediction();
    ConsecutiveDetectionCount = 0;
    LastDetectedWidgetType = TEXT("");
    return;
  }

  UE_LOG(LogTemp, Verbose, TEXT("[Tick] Focused Widget: %s"),
         *FocusedWidget->GetType().ToString());

  // 从焦点 Widget 开始向上查找 Pin 类型
  TSharedPtr<SWidget> CurrentWidget = FocusedWidget;
  int32 Depth = 0;

  while (CurrentWidget.IsValid() && Depth < 20) {
    const FName WidgetTypeName = CurrentWidget->GetType();
    FString TypeNameStr = WidgetTypeName.ToString();
    FString LowerTypeName = TypeNameStr.ToLower();

    UE_LOG(LogTemp, VeryVerbose, TEXT("[Tick] Widget[%d]: %s"), Depth,
           *TypeNameStr);

    // 检查是否是 Pin 类型
    bool bIsPin = LowerTypeName.Contains(TEXT("graphpin")) ||
                  LowerTypeName.Contains(TEXT("pcgpin")) ||
                  LowerTypeName.Contains(TEXT("spcg"));

    if (bIsPin) {
      UE_LOG(LogTemp, Log, TEXT("✓ Detected Pin Widget: %s (Depth: %d)"),
             *TypeNameStr, Depth);

      ConsecutiveDetectionCount++;
      UE_LOG(LogTemp, Verbose, TEXT("[Tick] ConsecutiveDetectionCount: %d"),
             ConsecutiveDetectionCount);

      if (ConsecutiveDetectionCount < 3) {
        return;
      }

      if (TypeNameStr == LastDetectedWidgetType &&
          PredictionPopupWindow.IsValid()) {
        UE_LOG(LogTemp, VeryVerbose, TEXT("[Tick] Same widget, skip"));
        return;
      }

      LastDetectedWidgetType = TypeNameStr;

      FString PinName = TEXT("Pin");
      FString Direction = TEXT("Output");
      FString NodeName = TEXT("Node");

      FString AccessibleText = CurrentWidget->GetAccessibleText().ToString();
      if (!AccessibleText.IsEmpty()) {
        PinName = AccessibleText;
        UE_LOG(LogTemp, Log, TEXT("  PinName: %s"), *PinName);
      }

      UE_LOG(LogTemp, Log, TEXT("  Showing prediction..."));
      ShowPrediction(PinName, Direction, NodeName);
      return;
    }

    CurrentWidget = CurrentWidget->GetParentWidget();
    Depth++;
  }

  if (ConsecutiveDetectionCount > 0) {
    UE_LOG(LogTemp, Verbose, TEXT("[Tick] Left Pin widget"));
  }

  HidePrediction();
  ConsecutiveDetectionCount = 0;
  LastDetectedWidgetType = TEXT("");
}

void FPCGPinHoverIntegration::ShowPrediction(const FString &PinName,
                                             const FString &Direction,
                                             const FString &NodeName) {
  UE_LOG(LogTemp, Log, TEXT(">>> Show Prediction: %s.%s (%s)"), *NodeName,
         *PinName, *Direction);

  if (!PredictionPopupWindow.IsValid()) {
    CreatePredictionPopup();
  }

  UpdatePredictionContent(PinName, Direction);

  if (PredictionPopupWindow.IsValid()) {
    FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
    PredictionPopupWindow->MoveWindowTo(
        FIntPoint(MousePos.X - 175, MousePos.Y));
    PredictionPopupWindow->ShowWindow();
    UE_LOG(LogTemp, Log, TEXT(">>> Prediction window shown"));
  }
}

void FPCGPinHoverIntegration::HidePrediction() {
  if (PredictionPopupWindow.IsValid()) {
    PredictionPopupWindow->HideWindow();
    UE_LOG(LogTemp, Verbose, TEXT("<<< Hide prediction window"));
  }
}

void FPCGPinHoverIntegration::TriggerPredictionFromToolbar() {
  UE_LOG(LogTemp, Log, TEXT("Trigger prediction from toolbar"));

  if (!PredictionPopupWindow.IsValid()) {
    CreatePredictionPopup();
  }

  if (PredictorEngine.IsValid()) {
    TArray<FPCGCandidate> Candidates =
        PredictorEngine->Predict(EPCGPredictPinDirection::Output);

    if (PredictionPopupWindow.IsValid()) {
      TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
      if (Content.IsValid()) {
        TSharedPtr<SPCGPredictionPopup> Popup =
            StaticCastSharedPtr<SPCGPredictionPopup>(Content);

        if (Popup.IsValid()) {
          Popup->SetPredictorEngine(PredictorEngine.Get());
          Popup->UpdatePredictions(Candidates, EPCGPredictPinDirection::Output);
        }
      }
    }

    if (PredictionPopupWindow.IsValid()) {
      FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
      PredictionPopupWindow->MoveWindowTo(
          FIntPoint(MousePos.X - 175, MousePos.Y));
      PredictionPopupWindow->ShowWindow();
    }
  }
}

void FPCGPinHoverIntegration::CreatePredictionPopup() {
  PredictionPopupWindow =
      SNew(SWindow)
          .ClientSize(FVector2D(350, 400))
          .AutoCenter(EAutoCenter::None)
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
  UE_LOG(LogTemp, Log, TEXT("Created prediction popup window"));
}

void FPCGPinHoverIntegration::UpdatePredictionContent(
    const FString &PinName, const FString &Direction) {
  if (!PredictorEngine.IsValid()) {
    return;
  }

  EPCGPredictPinDirection PredictDirection = GetPredictDirection(Direction);
  TArray<FPCGCandidate> Candidates = PredictorEngine->Predict(PredictDirection);

  if (PredictionPopupWindow.IsValid()) {
    TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
    if (Content.IsValid()) {
      TSharedPtr<SPCGPredictionPopup> Popup =
          StaticCastSharedPtr<SPCGPredictionPopup>(Content);

      if (Popup.IsValid()) {
        Popup->SetPredictorEngine(PredictorEngine.Get());
        Popup->UpdatePredictions(Candidates, PredictDirection);
      }
    }
  }
}

EPCGPredictPinDirection
FPCGPinHoverIntegration::GetPredictDirection(const FString &Direction) {
  return (Direction == TEXT("Input") || Direction == TEXT("EGPD_Input"))
             ? EPCGPredictPinDirection::Input
             : EPCGPredictPinDirection::Output;
}
