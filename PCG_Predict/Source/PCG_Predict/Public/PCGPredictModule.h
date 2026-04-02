#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPCGToolbarExtension;
class FPCGEditorExtension;
class FPCGEditorCommands;
class FPCGPinHoverIntegration;

/**
 * PCG 预测模块
 * 提供智能节点预测功能
 *
 * 功能:
 * - 工具栏按钮触发预测
 * - Pin 悬停自动触发预测（轮询检测，无需修改引擎）
 * - 意图驱动推荐排序
 */
class FPCGPredictModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    /** 获取 Pin 悬停集成实例 */
    FPCGPinHoverIntegration *GetPinHoverIntegration() {
      return PinHoverIntegration.Get();
    }

  private:
    /** 工具栏扩展 */
    TSharedPtr<FPCGToolbarExtension> ToolbarExtension;

    /** 编辑器扩展 */
    TSharedPtr<FPCGEditorExtension> EditorExtension;

    /** 编辑器命令 */
    TSharedPtr<FPCGEditorCommands> EditorCommands;

    /** Pin 悬停集成 */
    TSharedPtr<FPCGPinHoverIntegration> PinHoverIntegration;

    /** 帧计数器 */
    int32 FrameCounter;
};

/** 全局 Tick 函数 */
void PCGPredict_Tick();
