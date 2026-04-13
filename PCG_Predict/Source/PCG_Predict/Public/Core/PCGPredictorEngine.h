#pragma once

#include "CoreMinimal.h"
#include "PCGPredictionTypes.h"

class FPCGOnnxRuntime;
class FPCGFastPredictor;
class FPCGDeepPredictor;
struct FPCGDeepPredictResult;

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
 * 双模型架构：
 *   - FastPredictor：n-gram 查表，< 1ms，立即返回
 *   - DeepPredictor：Graph Transformer ONNX，异步后台推理，结果通过回调更新 UI
 */
class PCG_PREDICT_API FPCGPredictorEngine
{
public:
    void Initialize(const FString& ModelPath);
    void Shutdown();

    /**
     * 同步预测（立即返回 Fast 结果）。
     * 同时提交 Deep 异步请求，完成后通过 OnDeepResultReady 回调。
     */
    TArray<FPCGCandidate> Predict(
        EPCGPredictPinDirection Direction,
        class UEdGraphPin* ContextPin = nullptr);

    TArray<FPCGCandidate> PredictStarterNodes();

    void SetIntent(const FString& Text);

    /**
     * 设置 Deep 推理完成回调（在游戏线程调用）。
     * 参数：合并后的最终候选列表。
     */
    using FOnDeepResultReady = TFunction<void(const TArray<FPCGCandidate>&)>;
    void SetOnDeepResultReady(FOnDeepResultReady Callback)
    {
        OnDeepResultReady = MoveTemp(Callback);
    }

    /** 根据 NodeTypeId 查找节点名称 */
    FString GetNodeName(int32 NodeTypeId) const;

private:
    TSharedPtr<FPCGOnnxRuntime>  OnnxRuntime;   // 保留兼容旧接口
    TSharedPtr<FPCGFastPredictor> FastPredictor;
    TSharedPtr<FPCGDeepPredictor> DeepPredictor;

    FString CurrentIntent;
    TArray<FPCGNodeRegistryEntry> NodeRegistry;

    FOnDeepResultReady OnDeepResultReady;

    /** 从 ContextPin 提取最近操作的节点 type_id 序列 */
    TArray<int32> ExtractRecentNodeIds(class UEdGraphPin* Pin) const;

    /** 从 ContextPin 提取已连接节点名称（用于日志） */
    TArray<FString> ExtractConnectedNodeTypes(class UEdGraphPin* Pin) const;

    /** 将 NodeTypeId 填充为真实节点名称 */
    void FillNodeNames(TArray<FPCGCandidate>& Candidates) const;

    /**
     * 合并 Fast 和 Deep 结果。
     * final_score = fast_score * 0.4 + deep_score * 0.6
     */
    static TArray<FPCGCandidate> MergeResults(
        const TArray<FPCGCandidate>& FastResults,
        const TArray<FPCGCandidate>& DeepResults,
        int32 TopK = 10);

    /** 获取随机示例候选（无模型时的 fallback） */
    TArray<FPCGCandidate> GetSampleCandidates(
        EPCGPredictPinDirection Direction,
        class UEdGraphPin* ContextPin) const;

    void LoadNodeRegistry();
};
