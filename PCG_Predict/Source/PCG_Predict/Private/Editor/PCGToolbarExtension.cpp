#include "Editor/PCGToolbarExtension.h"
#include "Editor/PCGEditorExtension.h"
#include "Framework/Commands/UIAction.h"
#include "Framework/MultiBox/MultiBoxBuilder.h"
#include "LevelEditor.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "PCGToolbarExtension"

static TSharedPtr<FPCGEditorExtension> GEditorExtension;

void FPCGToolbarExtension::Initialize() {
  // 创建编辑器扩展实例
  if (!GEditorExtension.IsValid()) {
    GEditorExtension = MakeShareable(new FPCGEditorExtension());
    GEditorExtension->Initialize();
  }

  // 注册工具栏扩展
  ExtendToolbar();

  UE_LOG(LogTemp, Log, TEXT("PCG Toolbar Extension Initialized"));
}

void FPCGToolbarExtension::Shutdown() {
  if (GEditorExtension.IsValid()) {
    GEditorExtension->Shutdown();
    GEditorExtension.Reset();
  }
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

  // 添加到工具栏 - 尝试放在 Play 按钮附近
  ToolbarExtender->AddToolBarExtension(
      "Play", EExtensionHook::After, nullptr,
      FToolBarExtensionDelegate::CreateLambda([this](FToolBarBuilder &Builder) {
        Builder.AddSeparator();

        // 预测按钮 - 使用 Lambda
        Builder.AddToolBarButton(
            FUIAction(FExecuteAction::CreateLambda(
                [this]() { OnPredictButtonClicked(); })),
            NAME_None, LOCTEXT("PredictButtonLabel", "🔮 PCG Predict"),
            LOCTEXT("PredictButtonTooltip", "显示 PCG 节点预测"), FSlateIcon());

        // 意图按钮 - 使用 Lambda
        Builder.AddToolBarButton(
            FUIAction(FExecuteAction::CreateLambda(
                [this]() { OnIntentButtonClicked(); })),
            NAME_None, LOCTEXT("IntentButtonLabel", "🎯 Intent"),
            LOCTEXT("IntentButtonTooltip", "设置场景意图"), FSlateIcon());
      }));

  // 注册扩展器
  LevelEditorModule->GetToolBarExtensibilityManager()->AddExtender(
      ToolbarExtender);

  UE_LOG(LogTemp, Log, TEXT("PCG Toolbar Extension registered at 'Play'"));
}

void FPCGToolbarExtension::OnPredictButtonClicked() {
  if (GEditorExtension.IsValid()) {
    GEditorExtension->ShowPredictionPopup(EPCGPredictPinDirection::Output);
  }
}

void FPCGToolbarExtension::OnIntentButtonClicked() {
  if (GEditorExtension.IsValid()) {
    GEditorExtension->ShowIntentInput();
  }
}

#undef LOCTEXT_NAMESPACE
