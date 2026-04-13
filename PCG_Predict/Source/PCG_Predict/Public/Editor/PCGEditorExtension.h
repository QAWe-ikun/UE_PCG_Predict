#pragma once

#include "Core/PCGPredictionTypes.h"
#include "CoreMinimal.h"
#include "Input/Reply.h"

class SPCGPredictionPopup;
class SPCGIntentInput;
class FPCGPredictorEngine;

/**
 * PCG 预测编辑器扩展
 * 集成到 PCG Graph 编辑器中，统一管理预测引擎、浮层窗口、意图输入
 */
class FPCGEditorExtension
{
public:
    void Initialize();
    void Shutdown();

    /** 设置预测引擎 */
    void SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine);

    /** 获取预测引擎 */
    TSharedPtr<FPCGPredictorEngine> GetPredictorEngine() { return PredictorEngine; }

    /** 触发预测并显示浮层 */
    void OnTriggerPredict();

    /** 显示预测浮层 */
    void ShowPredictionPopup(EPCGPredictPinDirection Direction, class UEdGraphPin* ContextPin = nullptr);

    /** 显示意图输入窗口 */
    void ShowIntentInput();

    /** 设置当前意图 */
    void SetCurrentIntent(const FString& Intent);

    /** 获取当前选中的 Pin 信息 */
    void GetSelectedPinInfo(FString& OutPinName, FString& OutDirection, FString& OutNodeName);

private:
    /** 预测引擎 */
    TSharedPtr<FPCGPredictorEngine> PredictorEngine;

    /** 预测浮层窗口 */
    TSharedPtr<SWindow> PredictionPopupWindow;

    /** 意图输入窗口 */
    TSharedPtr<SWindow> IntentInputWindow;

    /** 当前意图 */
    FString CurrentIntent;

    /** 创建预测浮层窗口 */
    void CreatePredictionPopupWindow(EPCGPredictPinDirection Direction, class UEdGraphPin* ContextPin);

    /** 创建意图输入窗口 */
    void CreateIntentInputWindow();

    /** 绑定到 PCG Graph 编辑器 */
    void BindToPCGGraphEditor();
};

