#include "Editor/PCGEditorCommands.h"
#include "Editor/PCGEditorExtension.h"
#include "Core/PCGPredictorEngine.h"
#include "Framework/Commands/UICommandList.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Framework/Commands/Commands.h"

// 自定义命令
#define LOCTEXT_NAMESPACE "PCGEditorCommands"

namespace PCGEditorCommands
{
    UE_COMMAND_INFO(TriggerPredict, "Trigger Predict", "Trigger PCG node prediction", EUserInterfaceActionType::Button, FInputChord(EKeys::Tab, EModifierKey::None))
    UE_COMMAND_INFO(SetIntent, "Set Intent", "Set scene intent", EUserInterfaceActionType::Button, FInputChord(EKeys::Tab, EModifierKey::Shift))
}

#undef LOCTEXT_NAMESPACE

static TSharedPtr<FPCGEditorExtension> GEditorExtension;
static TSharedPtr<UICommandList> GCommandList;

void FPCGEditorCommands::Initialize()
{
    // 创建编辑器扩展
    if (!GEditorExtension.IsValid())
    {
        GEditorExtension = MakeShareable(new FPCGEditorExtension());
        GEditorExtension->Initialize();
    }

    // 创建命令列表
    GCommandList = MakeShareable(new UICommandList);

    // 绑定命令
    GCommandList->MapAction(
        PCGEditorCommands::TriggerPredict,
        FExecuteAction::CreateRaw(this, &FPCGEditorCommands::OnTriggerPredict)
    );

    GCommandList->MapAction(
        PCGEditorCommands::SetIntent,
        FExecuteAction::CreateRaw(this, &FPCGEditorCommands::OnSetIntent)
    );

    // 注册到全局命令
    RegisterCommands();

    UE_LOG(LogTemp, Log, TEXT("PCG Editor Commands Initialized"));
}

void FPCGEditorCommands::Shutdown()
{
    if (GEditorExtension.IsValid())
    {
        GEditorExtension->Shutdown();
        GEditorExtension.Reset();
    }

    GCommandList.Reset();
}

void FPCGEditorCommands::SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine)
{
    PredictorEngine = Engine;
}

void FPCGEditorCommands::RegisterCommands()
{
    // 注册到 Level Editor
    FLevelEditorModule* LevelEditorModule = FModuleManager::GetModulePtr<FLevelEditorModule>("LevelEditor");
    if (LevelEditorModule)
    {
        // 添加快捷键到编辑器
        LevelEditorModule->GetGlobalLevelEditorTabManager()->InvokeTab("LevelEditor");
    }
}

void FPCGEditorCommands::OnTriggerPredict()
{
    if (GEditorExtension.IsValid())
    {
        // 获取当前选中的 Pin
        FString PinName, Direction, NodeName;
        GetSelectedPinInfo(PinName, Direction, NodeName);

        if (!PinName.IsEmpty())
        {
            UE_LOG(LogTemp, Log, TEXT("Trigger predict for pin: %s.%s (%s)"), *NodeName, *PinName, *Direction);
            
            EPCGPredictPinDirection PredictDirection = 
                (Direction == "Input") ? EPCGPredictPinDirection::Input : EPCGPredictPinDirection::Output;
            
            GEditorExtension->ShowPredictionPopup(PredictDirection);
        }
        else
        {
            // 没有选中 Pin，显示默认预测
            GEditorExtension->ShowPredictionPopup(EPCGPredictPinDirection::Output);
        }
    }
}

void FPCGEditorCommands::OnSetIntent()
{
    if (GEditorExtension.IsValid())
    {
        GEditorExtension->ShowIntentInput();
    }
}

void FPCGEditorCommands::GetSelectedPinInfo(FString& OutPinName, FString& OutDirection, FString& OutNodeName)
{
    // 简化实现：返回空字符串
    // 实际需要访问 PCG Graph 编辑器内部结构
    OutPinName = "";
    OutDirection = "";
    OutNodeName = "";
}
