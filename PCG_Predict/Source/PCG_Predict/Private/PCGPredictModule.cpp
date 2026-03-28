#include "PCGPredictModule.h"
#include "Editor/PCGEditorExtension.h"
#include "Editor/PCGToolbarExtension.h"

#define LOCTEXT_NAMESPACE "FPCGPredictModule"

void FPCGPredictModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("PCGPredict module started"));

    // 初始化工具栏扩展
    ToolbarExtension = MakeShareable(new FPCGToolbarExtension());
    ToolbarExtension->Initialize();
}

void FPCGPredictModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("PCGPredict module shutdown"));

    // 关闭工具栏扩展
    if (ToolbarExtension.IsValid()) {
      ToolbarExtension->Shutdown();
      ToolbarExtension.Reset();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPCGPredictModule, PCG_Predict)
