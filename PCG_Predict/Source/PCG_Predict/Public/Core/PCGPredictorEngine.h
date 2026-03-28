#pragma once

#include "CoreMinimal.h"
#include "PCGPredictionTypes.h"

class FPCGOnnxRuntime;

/**
 * PCG 预测引擎
 * 负责加载模型、运行推理、返回候选节点
 */
class FPCGPredictorEngine
{
public:
    void Initialize(const FString& ModelPath);
    TArray<FPCGCandidate> Predict(EPCGPredictPinDirection Direction);
    void SetIntent(const FString& Text);

  private:
    /** ONNX 推理运行时 */
    TSharedPtr<FPCGOnnxRuntime> OnnxRuntime;

    /** 当前意图 */
    FString CurrentIntent;

    /** 获取示例候选（临时用） */
    TArray<FPCGCandidate> GetSampleCandidates() const;

    /** 加载节点注册表 */
    void LoadNodeRegistry();
};
