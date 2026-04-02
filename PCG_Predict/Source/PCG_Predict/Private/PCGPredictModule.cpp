#include "PCGPredictModule.h"
#include "Editor/FPCGPinHoverIntegration.h"
#include "Editor/PCGEditorCommands.h"
#include "Editor/PCGEditorExtension.h"
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

  // 初始化编辑器命令
  EditorCommands = MakeShareable(new FPCGEditorCommands());
  EditorCommands->Initialize();

  // 初始化 Pin 悬停集成
  PinHoverIntegration = MakeShareable(new FPCGPinHoverIntegration());
  PinHoverIntegration->Initialize();

  // 使用 FCoreDelegates 注册 Tick
  FCoreDelegates::OnEndFrame.AddLambda([]() {
    static int32 TickCounter = 0;
    static const int32 TickInterval = 10;

    TickCounter++;

    // 每 60 帧输出一次心跳日志（立即显示，不等待）
    if (TickCounter % 60 == 0) {
      UE_LOG(LogTemp, Log, TEXT("[Ticker] Heartbeat #%d"), TickCounter);
    }

    // 每 10 帧执行一次 Pin 检测
    if (TickCounter % TickInterval != 0) {
      return;
    }

    UE_LOG(LogTemp, Verbose, TEXT("[Ticker] Running Pin detection..."));

    // 调用 Pin 检测 - 使用模块管理器获取实例
    if (FModuleManager::Get().IsModuleLoaded("PCG_Predict")) {
      FPCGPredictModule *Module =
          FModuleManager::GetModulePtr<FPCGPredictModule>("PCG_Predict");
      if (Module && Module->PinHoverIntegration.IsValid()) {
        Module->PinHoverIntegration->DetectPinUnderCursor();
      }
    }
  });

  UE_LOG(LogTemp, Log, TEXT("✓ Registered to FCoreDelegates::OnEndFrame"));
  UE_LOG(LogTemp, Log, TEXT("✓ Pin hover detection enabled (100ms interval)"));
  UE_LOG(LogTemp, Log, TEXT("✓ Look for [Ticker] Heartbeat logs"));

  UE_LOG(LogTemp, Log, TEXT("PCGPredict module initialization complete"));
  UE_LOG(LogTemp, Log, TEXT("========================================"));
}

void FPCGPredictModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("PCGPredict module shutdown"));

    if (PinHoverIntegration.IsValid()) {
      PinHoverIntegration->Shutdown();
      PinHoverIntegration.Reset();
    }

    if (EditorCommands.IsValid()) {
      EditorCommands->Shutdown();
      EditorCommands.Reset();
    }

    if (ToolbarExtension.IsValid()) {
      ToolbarExtension->Shutdown();
      ToolbarExtension.Reset();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPCGPredictModule, PCG_Predict)
