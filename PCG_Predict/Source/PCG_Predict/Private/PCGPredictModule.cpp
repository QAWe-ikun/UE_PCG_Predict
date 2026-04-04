#include "PCGPredictModule.h"
#include "Core/PCGPredictorEngine.h"
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

  // 初始化预测引擎
  TSharedPtr<FPCGPredictorEngine> Engine =
      MakeShareable(new FPCGPredictorEngine());
  Engine->Initialize(TEXT("")); // 使用默认模型路径

  // 初始化 Pin 悬停集成，并设置预测引擎
  PinHoverIntegration = MakeShareable(new FPCGPinHoverIntegration());
  PinHoverIntegration->Initialize();
  PinHoverIntegration->SetPredictorEngine(Engine);

  // 注册到 FCoreDelegates 的 Tick
  FCoreDelegates::OnEndFrame.AddLambda([this]() {
    static int32 TickCounter = 0;
    static const int32 TickInterval =
        10; // 每 10 帧检测一次（约 100ms @ 100fps）

    TickCounter++;

    // 每 60 帧输出一次心跳日志
    if (TickCounter % 60 == 0) {
      UE_LOG(LogTemp, Log, TEXT("[Ticker] Heartbeat #%d"), TickCounter);
    }

    // 每 10 帧执行一次 Pin 检测
    if (TickCounter % TickInterval == 0) {
      UE_LOG(LogTemp, Verbose, TEXT("[Ticker] Running Pin detection..."));

      if (PinHoverIntegration.IsValid()) {
        PinHoverIntegration->DetectPinUnderCursor();
      }
    }
  });

  UE_LOG(LogTemp, Log, TEXT("✓ Registered to FCoreDelegates::OnEndFrame"));
  UE_LOG(LogTemp, Log, TEXT("✓ Pin hover detection enabled (100ms interval)"));
  UE_LOG(LogTemp, Log, TEXT("✓ PredictorEngine initialized"));

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
