#pragma once

#include "CoreMinimal.h"

class FPCGPredictorEngine;

/**
 * PCG 编辑器快捷键扩展
 * 提供快捷键触发预测功能
 */
class FPCGEditorCommands
{
public:
    void Initialize();
    void Shutdown();

    /** 设置预测引擎 */
    void SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine);

private:
    /** 预测引擎 */
    TSharedPtr<FPCGPredictorEngine> PredictorEngine;

    /** 注册快捷键 */
    void RegisterCommands();

    /** 触发预测 */
    void OnTriggerPredict();

    /** 设置意图 */
    void OnSetIntent();

    /** 获取当前选中的 Pin 信息 */
    void GetSelectedPinInfo(FString& OutPinName, FString& OutDirection, FString& OutNodeName);
};
