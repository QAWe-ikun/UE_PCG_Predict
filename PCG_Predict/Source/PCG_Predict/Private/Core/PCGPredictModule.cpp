#include "Core/PCGPredictModule.h"
#include "Config/PCGPredictSettings.h"
#include "Core/PCGPredictorEngine.h"
#include "Editor/PCGEditorExtension.h"
#include "Editor/FPCGPinHoverIntegration.h"
#include "Editor/PCGToolbarExtension.h"


#define LOCTEXT_NAMESPACE "FPCGPredictModule"

void FPCGPredictModule::StartupModule()
{
  UE_LOG(LogTemp, Log, TEXT("========================================"));
  UE_LOG(LogTemp, Log, TEXT("PCGPredict module started"));
  UE_LOG(LogTemp, Log, TEXT("========================================"));

  // 加载配置
  const UPCGPredictSettings* Settings = GetDefault<UPCGPredictSettings>();

  // 初始化预测引擎
  TSharedPtr<FPCGPredictorEngine> Engine = MakeShareable(new FPCGPredictorEngine());
  Engine->Initialize(TEXT(""));

  // 初始化编辑器扩展（统一入口）
  EditorExtension = MakeShareable(new FPCGEditorExtension());
  EditorExtension->SetPredictorEngine(Engine);
  EditorExtension->Initialize();

  // 初始化工具栏扩展，传入共享的 EditorExtension
  ToolbarExtension = MakeShareable(new FPCGToolbarExtension());
  ToolbarExtension->Initialize(EditorExtension);

  // 初始化 Pin 悬停集成
  PinHoverIntegration = MakeShareable(new FPCGPinHoverIntegration());
  PinHoverIntegration->Initialize();
  PinHoverIntegration->SetPredictorEngine(Engine);
  PinHoverIntegration->SetEnabled(Settings->bEnablePinHoverPrediction);

  // 注册帧回调，按配置间隔检测 Pin 悬停
  FCoreDelegates::OnEndFrame.AddLambda([this, Interval = Settings->HoverDetectionInterval]() {
    static int32 TickCounter = 0;
    TickCounter++;
    if (TickCounter % Interval == 0) {
      if (PinHoverIntegration.IsValid()) {
        PinHoverIntegration->DetectPinUnderCursor();
      }
    }
  });

  UE_LOG(LogTemp, Log, TEXT("✓ PredictorEngine initialized"));
  UE_LOG(LogTemp, Log, TEXT("✓ EditorExtension initialized"));
  UE_LOG(LogTemp, Log, TEXT("✓ ToolbarExtension initialized"));
  UE_LOG(LogTemp, Log, TEXT("✓ Pin hover detection interval: %d frames"), Settings->HoverDetectionInterval);
  UE_LOG(LogTemp, Log, TEXT("✓ Pin hover prediction: %s"),
         Settings->bEnablePinHoverPrediction ? TEXT("enabled") : TEXT("disabled"));
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

    if (ToolbarExtension.IsValid()) {
      ToolbarExtension->Shutdown();
      ToolbarExtension.Reset();
    }

    if (EditorExtension.IsValid()) {
      EditorExtension->Shutdown();
      EditorExtension.Reset();
    }
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPCGPredictModule, PCG_Predict)
