#pragma once

#include "CoreMinimal.h"
#include "PCGPredictionTypes.h"

class FPCGOnnxRuntime;

/**
 * PCG 节点注册信息
 */
struct FPCGNodeRegistryEntry
{
    int32 Id;
    FString Name;
    FString ClassName;
    FString Category;
    TArray<FString> InputTypes;
    TArray<FString> OutputTypes;
};

/**
 * PCG 预测引擎
 * 负责加载模型、运行推理、返回候选节点
 */
class FPCGPredictorEngine
{
public:
    void Initialize(const FString& ModelPath);
    TArray<FPCGCandidate> Predict(EPCGPredictPinDirection Direction, class UEdGraphPin* ContextPin = nullptr);
    TArray<FPCGCandidate> PredictStarterNodes();
    void SetIntent(const FString& Text);

  private:
    /** ONNX 推理运行时 */
    TSharedPtr<FPCGOnnxRuntime> OnnxRuntime;

    /** 当前意图 */
    FString CurrentIntent;

    /** 节点注册表 */
    TArray<FPCGNodeRegistryEntry> NodeRegistry;

    /** 获取示例候选（临时用） */
    TArray<FPCGCandidate> GetSampleCandidates(EPCGPredictPinDirection Direction, class UEdGraphPin* ContextPin) const;

    /** 从已连接的节点提取上下文信息 */
    TArray<FString> ExtractConnectedNodeTypes(class UEdGraphPin* Pin) const;

    /** 加载节点注册表 */
    void LoadNodeRegistry();
};
