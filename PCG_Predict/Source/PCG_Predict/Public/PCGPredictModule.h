#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPCGToolbarExtension;
class FPCGEditorExtension;

/**
 * PCG 预测模块
 * 提供智能节点预测功能
 */
class FPCGPredictModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

  private:
    /** 工具栏扩展 */
    TSharedPtr<FPCGToolbarExtension> ToolbarExtension;

    /** 编辑器扩展 */
    TSharedPtr<FPCGEditorExtension> EditorExtension;
};
