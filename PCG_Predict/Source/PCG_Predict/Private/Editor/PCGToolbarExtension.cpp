#include "Editor/PCGToolbarExtension.h"
#include "Editor/PCGEditorExtension.h"
#include "Editor/FPCGPinHoverIntegration.h"
#include "Config/PCGPredictSettings.h"
#include "Core/PCGPredictModule.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PCGToolbarExtension"

void FPCGToolbarExtension::Initialize(TSharedPtr<FPCGEditorExtension> InEditorExtension) {
  EditorExtension = InEditorExtension;
  ExtendToolbar();
  UE_LOG(LogTemp, Log, TEXT("PCG Toolbar Extension Initialized"));
}

void FPCGToolbarExtension::Shutdown() {
  EditorExtension.Reset();
}

void FPCGToolbarExtension::ExtendToolbar() {
  // 获取 Level Editor 模块
  FLevelEditorModule *LevelEditorModule =
      FModuleManager::GetModulePtr<FLevelEditorModule>(TEXT("LevelEditor"));
  if (!LevelEditorModule) {
    UE_LOG(LogTemp, Warning, TEXT("LevelEditor module not found"));
    return;
  }

  // 创建扩展器
  TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender());

  // 添加到工具栏 - 放在 Play 按钮之后
  ToolbarExtender->AddToolBarExtension(
      "Play", EExtensionHook::After, nullptr,
      FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder &Builder) {
        Builder.AddSeparator();

        // 预测开关按钮（新增）
        Builder.AddToolBarButton(
            FUIAction(
                FExecuteAction::CreateLambda([this]() { OnTogglePredictionClicked(); }),
                FCanExecuteAction(),
                FIsActionChecked::CreateLambda([]() {
                  return GetDefault<UPCGPredictSettings>()->bEnablePinHoverPrediction;
                })
            ),
            NAME_None,
            TAttribute<FText>::Create(TAttribute<FText>::FGetter::CreateLambda([]() {
              const UPCGPredictSettings* Settings = GetDefault<UPCGPredictSettings>();
              return Settings->bEnablePinHoverPrediction
                  ? LOCTEXT("PredictionOn", "⚡ 预测: 开")
                  : LOCTEXT("PredictionOff", "⚡ 预测: 关");
            })),
            LOCTEXT("TogglePredictionTooltip", "切换 Pin 悬停预测功能"),
            FSlateIcon(),
            EUserInterfaceActionType::ToggleButton
        );

        // 意图按钮（保留）
        Builder.AddToolBarButton(
            FUIAction(FExecuteAction::CreateLambda(
                [this]() { OnIntentButtonClicked(); })),
            NAME_None, LOCTEXT("IntentButtonLabel", "🎯 Intent"),
            LOCTEXT("IntentButtonTooltip", "输入意图并创建 PCG Graph"), FSlateIcon());
      }));

  // 注册扩展器
  LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(
      ToolbarExtender);

  UE_LOG(LogTemp, Log, TEXT("PCG Toolbar Extension registered at 'Play'"));
}

void FPCGToolbarExtension::OnTogglePredictionClicked() {
  UPCGPredictSettings* Settings = GetMutableDefault<UPCGPredictSettings>();
  Settings->bEnablePinHoverPrediction = !Settings->bEnablePinHoverPrediction;
  Settings->SaveConfig();

  // 通知 PinHoverIntegration
  FPCGPredictModule& Module = FModuleManager::GetModuleChecked<FPCGPredictModule>("PCG_Predict");
  if (Module.GetPinHoverIntegration()) {
    Module.GetPinHoverIntegration()->SetEnabled(Settings->bEnablePinHoverPrediction);
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGToolbar] Prediction toggled: %s"),
         Settings->bEnablePinHoverPrediction ? TEXT("ON") : TEXT("OFF"));
}

void FPCGToolbarExtension::OnIntentButtonClicked() {
  if (EditorExtension.IsValid()) {
    EditorExtension->ShowIntentInput();
  }
}

#undef LOCTEXT_NAMESPACE
