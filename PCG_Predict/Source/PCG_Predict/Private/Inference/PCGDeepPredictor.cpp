#include "Inference/PCGDeepPredictor.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Async/Async.h"
#include "HAL/RunnableThread.h"

FPCGDeepPredictor::FPCGDeepPredictor()
{
    WorkEvent = FPlatformProcess::GetSynchEventFromPool(false);
}

FPCGDeepPredictor::~FPCGDeepPredictor()
{
    Shutdown();
    if (WorkEvent)
    {
        FPlatformProcess::ReturnSynchEventToPool(WorkEvent);
        WorkEvent = nullptr;
    }
}

bool FPCGDeepPredictor::Initialize(const FString& ModelPath)
{
    OnnxRuntime = MakeShareable(new FPCGOnnxRuntime());
    if (!OnnxRuntime->Initialize(ModelPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[DeepPredictor] Failed to initialize ONNX runtime"));
        return false;
    }

    bShouldStop  = false;
    WorkerThread = FRunnableThread::Create(this, TEXT("PCGDeepPredictorWorker"),
                                           0, TPri_BelowNormal);
    if (!WorkerThread)
    {
        UE_LOG(LogTemp, Error, TEXT("[DeepPredictor] Failed to create worker thread"));
        return false;
    }

    bInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[DeepPredictor] Initialized, worker thread started"));
    return true;
}

void FPCGDeepPredictor::Shutdown()
{
    bShouldStop = true;
    if (WorkEvent) WorkEvent->Trigger();

    if (WorkerThread)
    {
        WorkerThread->WaitForCompletion();
        delete WorkerThread;
        WorkerThread = nullptr;
    }
    bInitialized = false;
}

void FPCGDeepPredictor::Stop()
{
    bShouldStop = true;
    if (WorkEvent) WorkEvent->Trigger();
}

uint64 FPCGDeepPredictor::SubmitRequest(
    const FPCGDeepPredictRequest& Request,
    FOnDeepPredictComplete OnComplete)
{
    uint64 Id = NextRequestId.IncrementExchange();
    FPCGDeepPredictRequest ReqCopy = Request;
    ReqCopy.RequestId = Id;

    {
        FScopeLock Lock(&QueueMutex);
        RequestQueue.Add({ReqCopy, OnComplete});
    }

    if (WorkEvent) WorkEvent->Trigger();
    return Id;
}

uint32 FPCGDeepPredictor::Run()
{
    while (!bShouldStop)
    {
        // 等待新请求（最多 100ms 超时，避免永久阻塞）
        WorkEvent->Wait(100);

        while (true)
        {
            TPair<FPCGDeepPredictRequest, FOnDeepPredictComplete> Item;
            {
                FScopeLock Lock(&QueueMutex);
                if (RequestQueue.Num() == 0) break;
                Item = RequestQueue[0];
                RequestQueue.RemoveAt(0);
            }

            FPCGDeepPredictResult Result = RunSingleRequest(Item.Key);

            // 回调到游戏线程
            FOnDeepPredictComplete Callback = Item.Value;
            AsyncTask(ENamedThreads::GameThread, [Result, Callback]()
            {
                Callback.ExecuteIfBound(Result);
            });
        }
    }
    return 0;
}

// ---------------------------------------------------------------------------
// 推理实现
// ---------------------------------------------------------------------------

FPCGDeepPredictResult FPCGDeepPredictor::RunSingleRequest(
    const FPCGDeepPredictRequest& Req) const
{
    FPCGDeepPredictResult Result;
    Result.RequestId = Req.RequestId;
    Result.bSuccess  = false;

    if (!OnnxRuntime || !OnnxRuntime->IsValid())
    {
        return Result;
    }

    TArray<FPCGTensorInput> Inputs = BuildInputTensors(Req);
    TArray<float> Logits = OnnxRuntime->RunInferenceMulti(Inputs);

    if (Logits.Num() != NUM_TYPES)
    {
        UE_LOG(LogTemp, Warning,
               TEXT("[DeepPredictor] Unexpected logits size: %d (expected %d)"),
               Logits.Num(), NUM_TYPES);
        return Result;
    }

    Result.Candidates = LogitsToCandidates(Logits, 10);
    Result.bSuccess   = true;
    return Result;
}

TArray<FPCGTensorInput> FPCGDeepPredictor::BuildInputTensors(
    const FPCGDeepPredictRequest& Req) const
{
    TArray<FPCGTensorInput> Inputs;

    // --- node_features [1, 7, 20] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("node_features");
        T.Shape = {1, MAX_NODES, NODE_FEAT};
        T.Data.SetNumZeroed(MAX_NODES * NODE_FEAT);

        // 计算入度/出度
        TMap<int32, int32> InDeg, OutDeg;
        for (const auto& Edge : Req.ContextEdges)
        {
            OutDeg.FindOrAdd(Edge.Key)++;
            InDeg.FindOrAdd(Edge.Value)++;
        }

        int32 Count = FMath::Min(Req.ContextNodeIds.Num(), MAX_NODES);
        for (int32 i = 0; i < Count; ++i)
        {
            int32 NId = Req.ContextNodeIds[i];
            EncodeNode(T.Data.GetData() + i * NODE_FEAT,
                       NId,
                       InDeg.FindRef(NId),
                       OutDeg.FindRef(NId),
                       i);
        }
        Inputs.Add(MoveTemp(T));
    }

    // --- adj_matrix [1, 7, 7] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("adj_matrix");
        T.Shape = {1, MAX_NODES, MAX_NODES};
        T.Data.SetNumZeroed(MAX_NODES * MAX_NODES);

        // 构建 type_id → 矩阵索引映射
        TMap<int32, int32> TypeToIdx;
        int32 Count = FMath::Min(Req.ContextNodeIds.Num(), MAX_NODES);
        for (int32 i = 0; i < Count; ++i)
        {
            TypeToIdx.FindOrAdd(Req.ContextNodeIds[i]) = i;
        }

        for (const auto& Edge : Req.ContextEdges)
        {
            const int32* SrcIdx = TypeToIdx.Find(Edge.Key);
            const int32* DstIdx = TypeToIdx.Find(Edge.Value);
            if (SrcIdx && DstIdx && *SrcIdx < MAX_NODES && *DstIdx < MAX_NODES)
            {
                T.Data[(*SrcIdx) * MAX_NODES + (*DstIdx)] = 1.0f;
            }
        }
        Inputs.Add(MoveTemp(T));
    }

    // --- history_seq [1, 5, 20] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("history_seq");
        T.Shape = {1, HISTORY_LEN, STEP_DIM};
        T.Data.SetNumZeroed(HISTORY_LEN * STEP_DIM);

        int32 Start = FMath::Max(0, Req.HistorySequence.Num() - HISTORY_LEN);
        int32 Len   = Req.HistorySequence.Num() - Start;
        for (int32 i = 0; i < Len; ++i)
        {
            EncodeHistoryStep(T.Data.GetData() + i * STEP_DIM,
                              Req.HistorySequence[Start + i]);
        }
        Inputs.Add(MoveTemp(T));
    }

    // --- intent_vector [1, 64] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("intent_vector");
        T.Shape = {1, INTENT_DIM};
        T.Data.SetNumZeroed(INTENT_DIM);
        if (Req.IntentVector.Num() == INTENT_DIM)
        {
            T.Data = Req.IntentVector;
        }
        Inputs.Add(MoveTemp(T));
    }

    // --- cursor_features [1, 24] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("cursor_features");
        T.Shape = {1, CURSOR_DIM};
        T.Data.SetNumZeroed(CURSOR_DIM);
        if (Req.CursorFeatures.Num() == CURSOR_DIM)
        {
            T.Data = Req.CursorFeatures;
        }
        Inputs.Add(MoveTemp(T));
    }

    // --- node_mask [1, 7] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("node_mask");
        T.Shape = {1, MAX_NODES};
        T.Data.SetNumZeroed(MAX_NODES);
        int32 Count = FMath::Min(Req.ContextNodeIds.Num(), MAX_NODES);
        for (int32 i = 0; i < Count; ++i) T.Data[i] = 1.0f;
        Inputs.Add(MoveTemp(T));
    }

    // --- direction_flag [1, 1] ---
    {
        FPCGTensorInput T;
        T.Name  = TEXT("direction_flag");
        T.Shape = {1, 1};
        T.Data  = {Req.Direction == EPCGPredictPinDirection::Input ? 1.0f : 0.0f};
        Inputs.Add(MoveTemp(T));
    }

    return Inputs;
}

TArray<FPCGCandidate> FPCGDeepPredictor::LogitsToCandidates(
    const TArray<float>& Logits, int32 TopK)
{
    // Softmax
    float MaxVal = Logits[0];
    for (float V : Logits) MaxVal = FMath::Max(MaxVal, V);
    TArray<float> Probs;
    Probs.SetNum(Logits.Num());
    float Sum = 0.0f;
    for (int32 i = 0; i < Logits.Num(); ++i)
    {
        Probs[i] = FMath::Exp(Logits[i] - MaxVal);
        Sum += Probs[i];
    }
    for (float& P : Probs) P /= Sum;

    // 排序取 Top-K
    TArray<TPair<int32, float>> Sorted;
    Sorted.Reserve(Probs.Num());
    for (int32 i = 0; i < Probs.Num(); ++i)
    {
        Sorted.Add({i, Probs[i]});
    }
    Sorted.Sort([](const TPair<int32,float>& A, const TPair<int32,float>& B){
        return A.Value > B.Value;
    });

    TArray<FPCGCandidate> Results;
    int32 Count = FMath::Min(TopK, Sorted.Num());
    Results.Reserve(Count);
    for (int32 i = 0; i < Count; ++i)
    {
        FPCGCandidate C;
        C.NodeTypeId   = Sorted[i].Key;
        C.NodeTypeName = FString::FromInt(Sorted[i].Key); // Engine 层填充真实名称
        C.Score        = Sorted[i].Value;
        C.Source       = EPCGCandidateSource::CreateNew;
        Results.Add(C);
    }
    return Results;
}

void FPCGDeepPredictor::EncodeNode(float* Out, int32 NodeTypeId,
                                    int32 InDegree, int32 OutDegree, int32 Depth)
{
    // 与 Python features.py::encode_node() 保持一致
    Out[0] = static_cast<float>(NodeTypeId) / NUM_TYPES;
    Out[1] = FMath::Min(InDegree  / 10.0f, 1.0f);
    Out[2] = FMath::Min(OutDegree / 10.0f, 1.0f);
    Out[3] = FMath::Min(Depth     / 20.0f, 1.0f);
    // [4-19] 保留为 0
}

void FPCGDeepPredictor::EncodeHistoryStep(float* Out, int32 NodeTypeId)
{
    // 与 Python features.py::encode_history_step() 保持一致
    Out[0] = 0.0f; // action_type（默认 0）
    Out[1] = static_cast<float>(NodeTypeId) / NUM_TYPES;
    // [2-19] 保留为 0
}
