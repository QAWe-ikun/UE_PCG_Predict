#include "Editor/PCGEditorCommands.h"
#include "Core/PCGPredictorEngine.h"
#include "Editor/PCGEditorExtension.h"

#define LOCTEXT_NAMESPACE "PCGEditorCommands"

static TSharedPtr<FPCGEditorExtension> GEditorExtension;

void FPCGEditorCommands::Initialize()
{
    // 创建编辑器扩展
    if (!GEditorExtension.IsValid())
    {
        GEditorExtension = MakeShareable(new FPCGEditorExtension());
        GEditorExtension->Initialize();
    }

    UE_LOG(LogTemp, Log, TEXT("PCG Editor Commands Initialized"));
}

void FPCGEditorCommands::Shutdown()
{
    if (GEditorExtension.IsValid())
    {
        GEditorExtension->Shutdown();
        GEditorExtension.Reset();
    }
}

void FPCGEditorCommands::SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine)
{
    PredictorEngine = Engine;
}

void FPCGEditorCommands::OnTriggerPredict()
{
    if (GEditorExtension.IsValid())
    {
      // 显示默认预测
      GEditorExtension->ShowPredictionPopup(EPCGPredictPinDirection::Output);
    }
}

void FPCGEditorCommands::OnSetIntent()
{
    if (GEditorExtension.IsValid())
    {
        GEditorExtension->ShowIntentInput();
    }
}

void FPCGEditorCommands::GetSelectedPinInfo(FString &OutPinName,
                                            FString &OutDirection,
                                            FString &OutNodeName) {
  OutPinName = "";
  OutDirection = "";
  OutNodeName = "";
}

#undef LOCTEXT_NAMESPACE
