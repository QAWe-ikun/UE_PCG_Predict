#pragma once

#include "CoreMinimal.h"
#include "Core/PCGPredictionTypes.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"
#include "HAL/ThreadSafeBool.h"

class FPCGOnnxRuntime;
struct FPCGTensorInput;

/**
 * DeepPredictor 推理请求
 */
struct FPCGDeepPredictRequest
{
    uint64 RequestId = 0;

    // 上下文节点（最多 7 个，按拓扑顺序）
    TArray<int32> ContextNodeIds;
    // 边列表：[src_type_id, dst_type_id]
    TArray<TPair<int32, int32>> ContextEdges;
    // 历史操作序列（最多 5 步）
    TArray<int32> HistorySequence;
    // 意图向量（64 维，可为空）
    TArray<float> IntentVector;
    // Cursor 特征（24 维）
    TArray<float> CursorFeatures;
    // 预测方向
    EPCGPredictPinDirection Direction = EPCGPredictPinDirection::Output;
};

/**
 * DeepPredictor 推理结果
 */
struct FPCGDeepPredictResult
{
    uint64 RequestId = 0;
    TArray<FPCGCandidate> Candidates;
    bool bSuccess = false;
};

DECLARE_DELEGATE_OneParam(FOnDeepPredictComplete, const FPCGDeepPredictResult&);

/**
 * PCGDeepPredictor
 * 基于 Graph Transformer ONNX 模型的深度推理器，在后台线程异步执行。
 *
 * 使用方式：
 *   1. Initialize(ModelPath) — 加载 ONNX 模型
 *   2. SubmitRequest(Req, Callback) — 提交推理请求
 *   3. 推理完成后，Callback 在游戏线程被调用
 */
class PCG_PREDICT_API FPCGDeepPredictor : public FRunnable
{
public:
    FPCGDeepPredictor();
    virtual ~FPCGDeepPredictor();

    bool Initialize(const FString& ModelPath);
    void Shutdown();
    bool IsValid() const { return bInitialized; }

    /**
     * 提交异步推理请求。
     * @return 请求 ID（可用于取消或匹配结果）
     */
    uint64 SubmitRequest(
        const FPCGDeepPredictRequest& Request,
        FOnDeepPredictComplete OnComplete);

    // FRunnable interface
    virtual bool Init() override { return true; }
    virtual uint32 Run() override;
    virtual void Stop() override;

private:
    bool bInitialized = false;
    FThreadSafeBool bShouldStop;

    TSharedPtr<FPCGOnnxRuntime> OnnxRuntime;
    FRunnableThread* WorkerThread = nullptr;
    FEvent* WorkEvent = nullptr;

    // 请求队列（线程安全）
    FCriticalSection QueueMutex;
    TArray<TPair<FPCGDeepPredictRequest, FOnDeepPredictComplete>> RequestQueue;

    TAtomic<uint64> NextRequestId{1};

    static constexpr int32 MAX_NODES    = 7;
    static constexpr int32 HISTORY_LEN  = 5;
    static constexpr int32 NODE_FEAT    = 20;
    static constexpr int32 STEP_DIM     = 20;
    static constexpr int32 INTENT_DIM   = 64;
    static constexpr int32 CURSOR_DIM   = 24;
    static constexpr int32 NUM_TYPES    = 195;

    /** 执行单次推理，返回 Top-K 候选 */
    FPCGDeepPredictResult RunSingleRequest(
        const FPCGDeepPredictRequest& Req) const;

    /** 构建 ONNX 多输入张量 */
    TArray<FPCGTensorInput> BuildInputTensors(
        const FPCGDeepPredictRequest& Req) const;

    /** 将 logits 转换为 Top-K FPCGCandidate */
    static TArray<FPCGCandidate> LogitsToCandidates(
        const TArray<float>& Logits, int32 TopK = 10);

    /** 编码单个节点特征（20 维） */
    static void EncodeNode(float* Out, int32 NodeTypeId,
                           int32 InDegree, int32 OutDegree, int32 Depth);

    /** 编码历史步骤（20 维） */
    static void EncodeHistoryStep(float* Out, int32 NodeTypeId);
};
