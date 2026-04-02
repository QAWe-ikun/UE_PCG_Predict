#include "PCGPredictModule.h"
#include "Editor/PCGEditorCommands.h"
#include "Editor/PCGEditorExtension.h"
#include "Editor/PCGGraphIntegration.h"
#include "Editor/PCGToolbarExtension.h"

#define LOCTEXT_NAMESPACE "FPCGPredictModule"

void FPCGPredictModule::StartupModule()
{
  UE_LOG(LogTemp, Log, TEXT("========================================"));
  UE_LOG(LogTemp, Log, TEXT("PCGPredict module started"));
  UE_LOG(LogTemp, Log, TEXT("========================================"));

  // 初始化工具栏扩展
  ToolbarExtension = MakeShareable(new FPCGToolbarExtension());
  ToolbarExtension->Initialize();

  // 初始化编辑器命令（快捷键）
  EditorCommands = MakeShareable(new FPCGEditorCommands());
  EditorCommands->Initialize();

  // 初始化 Graph 集成
  GraphIntegration = MakeShareable(new FPCGGraphIntegration());
  GraphIntegration->Initialize();

  UE_LOG(LogTemp, Log, TEXT("PCGPredict module initialization complete"));
}

void FPCGPredictModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("PCGPredict module shutdown"));

    // 关闭 Graph 集成
    if (GraphIntegration.IsValid()) {
      GraphIntegration->Shutdown();
      GraphIntegration.Reset();
    }

    // 关闭编辑器命令
    if (EditorCommands.IsValid()) {
      EditorCommands->Shutdown();
      EditorCommands.Reset();
    }

    // 关闭工具栏扩展
    if (ToolbarExtension.IsValid()) {
      ToolbarExtension->Shutdown();
      ToolbarExtension.Reset();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPCGPredictModule, PCG_Predict)
