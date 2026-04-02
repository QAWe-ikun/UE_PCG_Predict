#pragma once

#include "CoreMinimal.h"
#include "Input/Reply.h"

class SPCGPredictionPopup;
class SPCGIntentInput;
class FPCGPredictorEngine;
struct FPCGPinHoverEvent
{
    UObject* Pin;
    FString PinName;
    FString PinDirection;  // "Input" or "Output"
    FString OwnerNodeName;
    FString PinType;
};

DECLARE_DELEGATE_OneParam(FOnPCGPinHover, const FPCGPinHoverEvent&)

/**
 * PCG Graph 编辑器集成
 * 监听 Pin 悬停事件并触发预测
 */
class FPCGGraphIntegration
{
public:
    void Initialize();
    void Shutdown();

    /** 设置预测引擎 */
    void SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine);

    /** 显示预测浮层 */
    void ShowPredictionAtPin(const FString& PinName, const FString& Direction, const FString& OwnerNode);

private:
    /** 预测引擎 */
    TSharedPtr<FPCGPredictorEngine> PredictorEngine;

    /** 预测浮层窗口 */
    TSharedPtr<SWindow> PredictionPopupWindow;

    /** 当前悬停的 Pin 信息 */
    FString CurrentHoverPin;
    FString CurrentHoverPinDirection;
    FString CurrentHoverNode;

    /** 创建预测浮层窗口 */
    void CreatePredictionPopupWindow();

    /** 绑定到 PCG Graph 编辑器 */
    void BindToPCGGraphEditor();

    /** Pin 悬停事件处理 */
    void OnPinHovered(const FPCGPinHoverEvent& Event);

    /** Pin 离开事件处理 */
    void OnPinUnhovered();
};
