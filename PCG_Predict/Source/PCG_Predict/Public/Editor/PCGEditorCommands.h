#pragma once

#include "CoreMinimal.h"

class FPCGPredictorEngine;

/**
 * PCG 编辑器命令（简化版）
 * 提供工具栏按钮功能
 */
class FPCGEditorCommands
{
public:
    void Initialize();
    void Shutdown();

    /** 设置预测引擎 */
    void SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine);

    /** 触发预测 */
    void OnTriggerPredict();

    /** 设置意图 */
    void OnSetIntent();

  private:
    /** 预测引擎 */
    TSharedPtr<FPCGPredictorEngine> PredictorEngine;

    /** 获取当前选中的 Pin 信息 */
    void GetSelectedPinInfo(FString& OutPinName, FString& OutDirection, FString& OutNodeName);
};
