#include "Editor/PCGGraphIntegration.h"
#include "Core/PCGPredictorEngine.h"
#include "Framework/Application/SlateApplication.h"
#include "UI/SPCGPredictionPopup.h"
#include "Widgets/SWindow.h"

void FPCGGraphIntegration::Initialize()
{
    UE_LOG(LogTemp, Log, TEXT("PCG Graph Integration Initialized"));

    // 当前版本不绑定到 PCG Graph 编辑器
    // 使用工具栏按钮代替
}

void FPCGGraphIntegration::Shutdown()
{
    if (PredictionPopupWindow.IsValid())
    {
        PredictionPopupWindow.Reset();
    }
}

void FPCGGraphIntegration::SetPredictorEngine(TSharedPtr<FPCGPredictorEngine> Engine)
{
    PredictorEngine = Engine;
}

void FPCGGraphIntegration::BindToPCGGraphEditor()
{
  // 暂不实现 - 需要访问 PCG Graph 编辑器内部
  UE_LOG(LogTemp, Log, TEXT("PCG Graph Editor binding not implemented yet"));
}

void FPCGGraphIntegration::ShowPredictionAtPin(
    const FString& PinName, 
    const FString& Direction, 
    const FString& OwnerNode)
{
    UE_LOG(LogTemp, Log, TEXT("Pin hovered: %s.%s (%s)"), *OwnerNode, *PinName, *Direction);

    // 保存当前 Pin 信息
    CurrentHoverPin = PinName;
    CurrentHoverPinDirection = Direction;
    CurrentHoverNode = OwnerNode;

    // 确定预测方向
    EPCGPredictPinDirection PredictDirection = 
        (Direction == "Input") ? EPCGPredictPinDirection::Input : EPCGPredictPinDirection::Output;

    // 创建窗口（如果需要）
    if (!PredictionPopupWindow.IsValid())
    {
        CreatePredictionPopupWindow();
    }

    // 获取预测结果
    if (PredictorEngine.IsValid())
    {
        TArray<FPCGCandidate> Candidates = PredictorEngine->Predict(PredictDirection);

        // 更新并显示浮层
        if (PredictionPopupWindow.IsValid())
        {
            TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
            if (Content.IsValid())
            {
                TSharedPtr<SPCGPredictionPopup> Popup = 
                    StaticCastSharedPtr<SPCGPredictionPopup>(Content);
            
                if (Popup.IsValid())
                {
                    Popup->SetPredictorEngine(PredictorEngine.Get());
                    Popup->UpdatePredictions(Candidates, PredictDirection);
                }
            }
        }

        if (PredictionPopupWindow.IsValid())
        {
            // 定位到鼠标位置
            FVector2D MousePos = FSlateApplication::Get().GetCursorPos();
            PredictionPopupWindow->MoveWindowTo(FIntPoint(MousePos.X, MousePos.Y));
            PredictionPopupWindow->ShowWindow();
        }
    }
}

void FPCGGraphIntegration::OnPinHovered(const FPCGPinHoverEvent& Event)
{
    ShowPredictionAtPin(Event.PinName, Event.PinDirection, Event.OwnerNodeName);
}

void FPCGGraphIntegration::OnPinUnhovered()
{
    if (PredictionPopupWindow.IsValid())
    {
        PredictionPopupWindow->HideWindow();
    }
}

void FPCGGraphIntegration::CreatePredictionPopupWindow()
{
    PredictionPopupWindow = SNew(SWindow)
        .ClientSize(FVector2D(350, 400))
        .AutoCenter(EAutoCenter::None)
        .SupportsMaximize(false)
        .SupportsMinimize(false)
        .SizingRule(ESizingRule::Autosized)
        .IsTopmostWindow(true)
        [
            SNew(SPCGPredictionPopup)
            .MaxCandidates(5)
        ];

    TSharedPtr<SWidget> Content = PredictionPopupWindow->GetContent();
    if (Content.IsValid())
    {
        TSharedPtr<SPCGPredictionPopup> Popup = 
            StaticCastSharedPtr<SPCGPredictionPopup>(Content);
        if (Popup.IsValid())
        {
            Popup->SetPopupWindow(PredictionPopupWindow);
        }
    }

    FSlateApplication::Get().AddWindow(PredictionPopupWindow.ToSharedRef());
}
