#include "Editor/PCGEditorExtension.h"
#include "Core/PCGPredictorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/SPCGIntentInput.h"
#include "UI/SPCGPredictionPopup.h"
#include "Widgets/SWindow.h"

void FPCGEditorExtension::Initialize()
{
    BindToPCGGraphEditor();
    UE_LOG(LogTemp, Log, TEXT("[PCGEditorExtension] Initialized"));
}

void FPCGEditorExtension::Shutdown()
{
    PredictionPopupWindow.Reset();
    IntentInputWindow.Reset();
    PredictorEngine.Reset();
    UE_LOG(LogTemp, Log, TEXT("[PCGEditorExtension] Shutdown"));
}

void FPCGEditorExtension::SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine)
{
    PredictorEngine = Engine;
}

void FPCGEditorExtension::OnTriggerPredict()
{
    ShowPredictionPopup(EPCGPredictPinDirection::Output);
}

void FPCGEditorExtension::GetSelectedPinInfo(FString& OutPinName, FString& OutDirection, FString& OutNodeName)
{
    OutPinName = TEXT("");
    OutDirection = TEXT("");
    OutNodeName = TEXT("");
    // TODO: 从当前选中的 PCG Graph 编辑器中获取 Pin 信息
}

void FPCGEditorExtension::BindToPCGGraphEditor() {}

void FPCGEditorExtension::ShowPredictionPopup(EPCGPredictPinDirection Direction, UEdGraphPin* ContextPin)
{
    if (PredictionPopupWindow.IsValid())
    {
        PredictionPopupWindow->RequestDestroyWindow();
        PredictionPopupWindow.Reset();
    }

    CreatePredictionPopupWindow(Direction, ContextPin);

    TArray<FPCGCandidate> Candidates = PredictorEngine->Predict(Direction, ContextPin);

    if (PredictionPopupWindow.IsValid())
    {
        TSharedPtr<SPCGPredictionPopup> Popup = StaticCastSharedRef<SPCGPredictionPopup>(PredictionPopupWindow->GetContent()).ToSharedPtr();
        if (Popup.IsValid())
        {
            Popup->SetPredictorEngine(PredictorEngine.Get());
            Popup->UpdatePredictions(Candidates, Direction);
            Popup->SetCurrentIntent(CurrentIntent);
        }
        PredictionPopupWindow->ShowWindow();
    }
}

void FPCGEditorExtension::ShowIntentInput()
{
    if (IntentInputWindow.IsValid())
    {
        IntentInputWindow->RequestDestroyWindow();
        IntentInputWindow.Reset();
    }

    CreateIntentInputWindow();

    if (IntentInputWindow.IsValid())
    {
        IntentInputWindow->ShowWindow();
    }
}

void FPCGEditorExtension::SetCurrentIntent(const FString& Intent)
{
    CurrentIntent = Intent;
    if (PredictorEngine.IsValid())
    {
        PredictorEngine->SetIntent(Intent);
    }
}

void FPCGEditorExtension::CreatePredictionPopupWindow(EPCGPredictPinDirection Direction, UEdGraphPin* ContextPin)
{
    PredictionPopupWindow =
        SNew(SWindow)
        .ClientSize(FVector2D(350, 400))
        .AutoCenter(EAutoCenter::PreferredWorkArea)
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .SizingRule(ESizingRule::Autosized)
        .IsTopmostWindow(true)
        [SNew(SPCGPredictionPopup).MaxCandidates(5)];

    TSharedPtr<SPCGPredictionPopup> Popup = StaticCastSharedRef<SPCGPredictionPopup>(PredictionPopupWindow->GetContent()).ToSharedPtr();
    if (Popup.IsValid())
    {
        Popup->SetPopupWindow(PredictionPopupWindow);
    }

    FSlateApplication::Get().AddWindow(PredictionPopupWindow.ToSharedRef());
}

void FPCGEditorExtension::CreateIntentInputWindow()
{
    IntentInputWindow =
        SNew(SWindow)
        .ClientSize(FVector2D(450, 300))
        .AutoCenter(EAutoCenter::PreferredWorkArea)
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .SizingRule(ESizingRule::Autosized)
        .IsTopmostWindow(true)
        [SNew(SPCGIntentInput)
            .OnIntentConfirmed_Lambda([this](const FString& NewIntent)
            {
                SetCurrentIntent(NewIntent);
                UE_LOG(LogTemp, Log, TEXT("[PCGEditorExtension] Intent set: %s"), *NewIntent);
            })];

    TSharedPtr<SPCGIntentInput> Input = StaticCastSharedRef<SPCGIntentInput>(IntentInputWindow->GetContent()).ToSharedPtr();
    if (Input.IsValid())
    {
        Input->SetPopupWindow(IntentInputWindow);
    }

    FSlateApplication::Get().AddWindow(IntentInputWindow.ToSharedRef());
}

