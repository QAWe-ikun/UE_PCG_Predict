#include "Core/PCGPredictorEngine.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Inference/PCGFastPredictor.h"
#include "Inference/PCGDeepPredictor.h"
#include "Math/RandomStream.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"

void FPCGPredictorEngine::Initialize(const FString& ModelPath)
{
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Initializing predictor engine"));

    // 加载节点注册表
    LoadNodeRegistry();
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Node registry: %d nodes"), NodeRegistry.Num());

    // 模型根目录（PCG_Predict_Models/models/）
    FString ModelsDir = FPaths::ProjectDir() / TEXT("PCG_Predict_Models/models");

    // --- FastPredictor ---
    FString LookupPath = ModelsDir / TEXT("fast_lookup.json");
    FastPredictor = MakeShareable(new FPCGFastPredictor());
    if (FastPredictor->Initialize(LookupPath))
    {
        UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] FastPredictor ready"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PCGPredictor] FastPredictor not ready (no lookup table)"));
    }

    // --- DeepPredictor ---
    FString OnnxPath = ModelsDir / TEXT("deep_predictor.onnx");
    DeepPredictor = MakeShareable(new FPCGDeepPredictor());
    if (DeepPredictor->Initialize(OnnxPath))
    {
        UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] DeepPredictor ready"));
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[PCGPredictor] DeepPredictor not ready (no ONNX model)"));
    }

    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Engine initialized"));
}

void FPCGPredictorEngine::Shutdown()
{
    if (DeepPredictor.IsValid())
    {
        DeepPredictor->Shutdown();
        DeepPredictor.Reset();
    }
    FastPredictor.Reset();
}

TArray<FPCGCandidate> FPCGPredictorEngine::Predict(
    EPCGPredictPinDirection Direction, UEdGraphPin* ContextPin)
{
    TArray<int32> RecentNodeIds = ExtractRecentNodeIds(ContextPin);

    // --- Fast 推理（同步，立即返回）---
    TArray<FPCGCandidate> FastResults;
    if (FastPredictor.IsValid() && FastPredictor->IsValid())
    {
        FastResults = FastPredictor->Query(RecentNodeIds, Direction, 10);
        FillNodeNames(FastResults);
    }
    else
    {
        // Fallback：随机样本
        FastResults = GetSampleCandidates(Direction, ContextPin);
    }

    // --- Deep 推理（异步，完成后回调更新 UI）---
    if (DeepPredictor.IsValid() && DeepPredictor->IsValid())
    {
        FPCGDeepPredictRequest Req;
        Req.ContextNodeIds  = RecentNodeIds;
        Req.HistorySequence = RecentNodeIds;
        Req.Direction       = Direction;

        // 保存 Fast 结果的副本，供合并使用
        TArray<FPCGCandidate> FastCopy = FastResults;

        DeepPredictor->SubmitRequest(Req,
            FOnDeepPredictComplete::CreateLambda(
                [this, FastCopy](const FPCGDeepPredictResult& Result)
                {
                    if (!Result.bSuccess) return;

                    TArray<FPCGCandidate> DeepResults = Result.Candidates;
                    FillNodeNames(DeepResults);

                    TArray<FPCGCandidate> Merged = MergeResults(FastCopy, DeepResults, 10);

                    if (OnDeepResultReady)
                    {
                        OnDeepResultReady(Merged);
                    }
                }));
    }

    return FastResults;
}

TArray<FPCGCandidate> FPCGPredictorEngine::PredictStarterNodes()
{
    // 空历史 → FastPredictor 的 starter 表
    if (FastPredictor.IsValid() && FastPredictor->IsValid())
    {
        TArray<int32> Empty;
        TArray<FPCGCandidate> Results =
            FastPredictor->Query(Empty, EPCGPredictPinDirection::Output, 5);
        FillNodeNames(Results);
        if (Results.Num() > 0) return Results;
    }

    // Fallback：查找无输入 pin 的节点
    TArray<FPCGCandidate> Candidates;
    for (const FPCGNodeRegistryEntry& Entry : NodeRegistry)
    {
        if (Entry.Id == 130 || Entry.Name == TEXT("None")) continue;
        if (Entry.InputTypes.Num() == 0 && Entry.OutputTypes.Num() > 0)
        {
            FPCGCandidate C;
            C.NodeTypeId   = Entry.Id;
            C.NodeTypeName = Entry.Name;
            C.Score        = 0.5f;
            C.Source       = EPCGCandidateSource::CreateNew;
            Candidates.Add(C);
        }
    }
    if (Candidates.Num() > 5) Candidates.SetNum(5);
    return Candidates;
}

void FPCGPredictorEngine::SetIntent(const FString& Text)
{
    CurrentIntent = Text;
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Intent set: %s"), *Text);
}

FString FPCGPredictorEngine::GetNodeName(int32 NodeTypeId) const
{
    for (const FPCGNodeRegistryEntry& Entry : NodeRegistry)
    {
        if (Entry.Id == NodeTypeId) return Entry.Name;
    }
    return FString::Printf(TEXT("Node_%d"), NodeTypeId);
}

// ---------------------------------------------------------------------------
// 内部辅助
// ---------------------------------------------------------------------------

TArray<int32> FPCGPredictorEngine::ExtractRecentNodeIds(UEdGraphPin* Pin) const
{
    TArray<int32> Ids;
    if (!Pin) return Ids;

    // 从已连接节点的名称反查 type_id
    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
    {
        if (!LinkedPin || !LinkedPin->GetOwningNode()) continue;
        FString Title = LinkedPin->GetOwningNode()
                            ->GetNodeTitle(ENodeTitleType::ListView).ToString();
        for (const FPCGNodeRegistryEntry& Entry : NodeRegistry)
        {
            if (Entry.Name == Title)
            {
                Ids.AddUnique(Entry.Id);
                break;
            }
        }
    }
    return Ids;
}

TArray<FString> FPCGPredictorEngine::ExtractConnectedNodeTypes(UEdGraphPin* Pin) const
{
    TArray<FString> Types;
    if (!Pin) return Types;
    for (UEdGraphPin* LinkedPin : Pin->LinkedTo)
    {
        if (LinkedPin && LinkedPin->GetOwningNode())
        {
            FString Title = LinkedPin->GetOwningNode()
                                ->GetNodeTitle(ENodeTitleType::ListView).ToString();
            if (!Title.IsEmpty()) Types.AddUnique(Title);
        }
    }
    return Types;
}

void FPCGPredictorEngine::FillNodeNames(TArray<FPCGCandidate>& Candidates) const
{
    for (FPCGCandidate& C : Candidates)
    {
        C.NodeTypeName = GetNodeName(C.NodeTypeId);
    }
}

TArray<FPCGCandidate> FPCGPredictorEngine::MergeResults(
    const TArray<FPCGCandidate>& FastResults,
    const TArray<FPCGCandidate>& DeepResults,
    int32 TopK)
{
    // 建立 NodeTypeId → 分数映射
    TMap<int32, float> ScoreMap;

    for (const FPCGCandidate& C : FastResults)
    {
        ScoreMap.FindOrAdd(C.NodeTypeId) += C.Score * 0.4f;
    }
    for (const FPCGCandidate& C : DeepResults)
    {
        ScoreMap.FindOrAdd(C.NodeTypeId) += C.Score * 0.6f;
    }

    // 排序
    TArray<TPair<int32, float>> Sorted;
    for (auto& Pair : ScoreMap) Sorted.Add({Pair.Key, Pair.Value});
    Sorted.Sort([](const TPair<int32,float>& A, const TPair<int32,float>& B){
        return A.Value > B.Value;
    });

    // 构建结果（名称从 FastResults 或 DeepResults 中取）
    TMap<int32, FString> NameMap;
    for (const FPCGCandidate& C : FastResults)  NameMap.Add(C.NodeTypeId, C.NodeTypeName);
    for (const FPCGCandidate& C : DeepResults)  NameMap.FindOrAdd(C.NodeTypeId) = C.NodeTypeName;

    TArray<FPCGCandidate> Merged;
    int32 Count = FMath::Min(TopK, Sorted.Num());
    for (int32 i = 0; i < Count; ++i)
    {
        FPCGCandidate C;
        C.NodeTypeId   = Sorted[i].Key;
        C.NodeTypeName = NameMap.FindRef(Sorted[i].Key);
        C.Score        = Sorted[i].Value;
        C.Source       = EPCGCandidateSource::CreateNew;
        Merged.Add(C);
    }
    return Merged;
}

TArray<FPCGCandidate> FPCGPredictorEngine::GetSampleCandidates(
    EPCGPredictPinDirection Direction, UEdGraphPin* ContextPin) const
{
    TArray<FPCGCandidate> Samples;
    if (NodeRegistry.Num() == 0) return Samples;

    TArray<int32> ValidIndices;
    for (int32 i = 0; i < NodeRegistry.Num(); i++)
    {
        const FPCGNodeRegistryEntry& E = NodeRegistry[i];
        if (E.Id == 130 || E.Name == TEXT("None")) continue;
        if (Direction == EPCGPredictPinDirection::Output && E.InputTypes.Num() > 0)
            ValidIndices.Add(i);
        else if (Direction == EPCGPredictPinDirection::Input && E.OutputTypes.Num() > 0)
            ValidIndices.Add(i);
    }
    if (ValidIndices.Num() == 0) return Samples;

    TArray<int32> Selected;
    int32 Num = FMath::Min(5, ValidIndices.Num());
    while (Selected.Num() < Num)
    {
        int32 Idx = ValidIndices[FMath::RandRange(0, ValidIndices.Num() - 1)];
        if (!Selected.Contains(Idx))
        {
            Selected.Add(Idx);
            FPCGCandidate C;
            C.NodeTypeId   = NodeRegistry[Idx].Id;
            C.NodeTypeName = NodeRegistry[Idx].Name;
            C.Score        = 0.95f - Selected.Num() * 0.08f;
            C.Source       = EPCGCandidateSource::CreateNew;
            Samples.Add(C);
        }
    }
    Samples.Sort([](const FPCGCandidate& A, const FPCGCandidate& B){ return A.Score > B.Score; });
    return Samples;
}

void FPCGPredictorEngine::LoadNodeRegistry()
{
    NodeRegistry.Empty();

    FString PluginContentDir = FPaths::ProjectPluginsDir() / TEXT("PCG_Predict/Content/Config/");
    FString ConfigPath = PluginContentDir / TEXT("node_registry.json");

    if (!FPaths::FileExists(ConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] node_registry.json not found: %s"), *ConfigPath);
        return;
    }

    FString JsonString;
    if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] Failed to read: %s"), *ConfigPath);
        return;
    }

    TSharedPtr<FJsonObject> JsonObject;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);
    if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] Failed to parse node_registry.json"));
        return;
    }

    const TArray<TSharedPtr<FJsonValue>>* NodesArray;
    if (!JsonObject->TryGetArrayField(TEXT("nodes"), NodesArray)) return;

    for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
    {
        const TSharedPtr<FJsonObject>* NodeObj;
        if (!NodeValue->TryGetObject(NodeObj)) continue;

        FPCGNodeRegistryEntry Entry;
        (*NodeObj)->TryGetNumberField(TEXT("id"),       Entry.Id);
        (*NodeObj)->TryGetStringField(TEXT("name"),     Entry.Name);
        (*NodeObj)->TryGetStringField(TEXT("class"),    Entry.ClassName);
        (*NodeObj)->TryGetStringField(TEXT("category"), Entry.Category);

        const TArray<TSharedPtr<FJsonValue>>* Arr;
        if ((*NodeObj)->TryGetArrayField(TEXT("input_types"), Arr))
            for (auto& V : *Arr) Entry.InputTypes.Add(V->AsString());
        if ((*NodeObj)->TryGetArrayField(TEXT("output_types"), Arr))
            for (auto& V : *Arr) Entry.OutputTypes.Add(V->AsString());

        NodeRegistry.Add(Entry);
    }

    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Loaded %d nodes"), NodeRegistry.Num());
}
