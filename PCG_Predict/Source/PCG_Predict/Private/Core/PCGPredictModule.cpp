#include "Core/PCGPredictModule.h"
#include "Core/PCGPredictorEngine.h"
#include "Editor/FPCGPinHoverIntegration.h"
#include "Editor/PCGEditorCommands.h"
#include "Editor/PCGEditorExtension.h"
#include "Editor/PCGToolbarExtension.h"
#include "Config/PCGPredictSettings.h"

#define LOCTEXT_NAMESPACE "FPCGPredictModule"

void FPCGPredictModule::StartupModule()
{
  UE_LOG(LogTemp, Log, TEXT("========================================"));
  UE_LOG(LogTemp, Log, TEXT("PCGPredict module started"));
  UE_LOG(LogTemp, Log, TEXT("========================================"));

  // 加载配置
  const UPCGPredictSettings* Settings = GetDefault<UPCGPredictSettings>();

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
  PinHoverIntegration->SetEnabled(Settings->bEnablePinHoverPrediction); // 从配置读取

  // 注册到 FCoreDelegates 的 Tick，使用配置的检测间隔
  FCoreDelegates::OnEndFrame.AddLambda([this, Interval = Settings->HoverDetectionInterval]() {
    static int32 TickCounter = 0;

    TickCounter++;

    // 使用配置的间隔执行 Pin 检测
    if (TickCounter % Interval == 0) {
      if (PinHoverIntegration.IsValid()) {
        PinHoverIntegration->DetectPinUnderCursor();
      }
    }
  });

  UE_LOG(LogTemp, Log, TEXT("✓ Registered to FCoreDelegates::OnEndFrame"));
  UE_LOG(LogTemp, Log, TEXT("✓ Pin hover detection interval: %d frames"), Settings->HoverDetectionInterval);
  UE_LOG(LogTemp, Log, TEXT("✓ Pin hover prediction: %s"),
         Settings->bEnablePinHoverPrediction ? TEXT("enabled") : TEXT("disabled"));
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
