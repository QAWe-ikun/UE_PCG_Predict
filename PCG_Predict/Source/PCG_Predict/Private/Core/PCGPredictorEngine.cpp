#include "Core/PCGPredictorEngine.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Inference/PCGDeepPredictor.h"
#include "Inference/PCGFastPredictor.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Math/RandomStream.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Nodes/PCGEditorGraphNode.h"
#include "PCGNode.h"
#include "PCGSettings.h"
#include "Serialization/JsonSerializer.h"

void FPCGPredictorEngine::Initialize(const FString& ModelPath)
{
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Initializing predictor engine"));

    // 加载节点注册表
    LoadNodeRegistry();
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Node registry: %d nodes"), NodeRegistry.Num());

    // 模型目录（PCG_Predict/Content/Models/）
    FString ModelsDir =
        FPaths::ProjectPluginsDir() / TEXT("PCG_Predict/Content/Models");

    // --- FastPredictor ---
    FString LookupPath = ModelsDir / TEXT("fast_lookup.json");
    FastPredictor = MakeShareable(new FPCGFastPredictor());
    if (!FastPredictor->Initialize(LookupPath)) {
      UE_LOG(LogTemp, Error,
             TEXT("[PCGPredictor] FastPredictor failed to load: %s"),
             *LookupPath);
      UE_LOG(LogTemp, Error,
             TEXT("[PCGPredictor] Prediction is disabled. Run: python "
                  "PCG_Predict_Models/src/build_fast_lookup.py"));
      FastPredictor.Reset();
    } else {
      UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] FastPredictor ready"));
    }

    // --- DeepPredictor ---
    FString OnnxPath = ModelsDir / TEXT("deep_predictor.onnx");
    DeepPredictor = MakeShareable(new FPCGDeepPredictor());
    if (!DeepPredictor->Initialize(OnnxPath)) {
      UE_LOG(LogTemp, Warning,
             TEXT("[PCGPredictor] DeepPredictor not available: %s"), *OnnxPath);
      UE_LOG(LogTemp, Warning,
             TEXT("[PCGPredictor] Run: python "
                  "PCG_Predict_Models/src/export_onnx.py"));
      DeepPredictor.Reset();
    } else {
      UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] DeepPredictor ready"));
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
  if (!FastPredictor.IsValid()) {
    UE_LOG(
        LogTemp, Error,
        TEXT(
            "[PCGPredictor] Predict() called but FastPredictor is not loaded"));
    return {};
  }

    TArray<int32> RecentNodeIds = ExtractRecentNodeIds(ContextPin);

    // --- Fast 推理（同步，立即返回）---
    TArray<FPCGCandidate> FastResults =
        FastPredictor->Query(RecentNodeIds, Direction, 20);
    FillNodeNames(FastResults);
    if (FastResults.Num() > 10)
      FastResults.SetNum(10);

    // --- Deep 推理（异步，完成后回调更新 UI）---
    if (DeepPredictor.IsValid() && DeepPredictor->IsValid())
    {
        FPCGDeepPredictRequest Req;
        Req.ContextNodeIds  = RecentNodeIds;
        Req.HistorySequence = RecentNodeIds;
        Req.Direction       = Direction;

        TArray<FPCGCandidate> FastCopy = FastResults;

        DeepPredictor->SubmitRequest(
            Req,
            FOnDeepPredictComplete::CreateLambda(
                [this, FastCopy](const FPCGDeepPredictResult &Result) {
                  if (!Result.bSuccess)
                    return;

                  TArray<FPCGCandidate> DeepResults = Result.Candidates;
                  FillNodeNames(DeepResults);

                  TArray<FPCGCandidate> Merged =
                      MergeResults(FastCopy, DeepResults, 10);

                  if (OnDeepResultReady) {
                    OnDeepResultReady(Merged);
                  }
                }));
    }

    return FastResults;
}

TArray<FPCGCandidate> FPCGPredictorEngine::PredictStarterNodes()
{
  if (!FastPredictor.IsValid()) {
    UE_LOG(LogTemp, Error,
           TEXT("[PCGPredictor] PredictStarterNodes() called but FastPredictor "
                "is not loaded"));
    return {};
  }

  // 先从注册表收集所有合法 starter 节点（input_types 为空或全是 "None"）
  // 排除 OutputNode(181) 和 None(44)
  TArray<FPCGCandidate> AllStarters;
  for (const FPCGNodeRegistryEntry &Entry : NodeRegistry) {
    if (Entry.Id == 44 || Entry.Id == 181)
      continue; // None / OutputNode
    bool bHasDataInput = false;
    for (const FString &T : Entry.InputTypes) {
      if (T != TEXT("None")) {
        bHasDataInput = true;
        break;
      }
    }
    if (!bHasDataInput) {
      FPCGCandidate C;
      C.NodeTypeId = Entry.Id;
      C.NodeTypeName = Entry.Name;
      C.Score = 0.5f;
      C.Source = EPCGCandidateSource::CreateNew;
      AllStarters.Add(C);
    }
  }

  // 用 starter 表的概率给合法节点打分
  TArray<int32> Empty;
  TArray<FPCGCandidate> Scored =
      FastPredictor->Query(Empty, EPCGPredictPinDirection::Output, 40);
  FillNodeNames(Scored);

  // 把 starter 表的分数覆盖到 AllStarters
  for (FPCGCandidate &S : AllStarters) {
    for (const FPCGCandidate &Q : Scored) {
      if (Q.NodeTypeId == S.NodeTypeId) {
        S.Score = Q.Score;
        break;
      }
    }
  }

  AllStarters.Sort([](const FPCGCandidate &A, const FPCGCandidate &B) {
    return A.Score > B.Score;
  });
  if (AllStarters.Num() > 5)
    AllStarters.SetNum(5);
  return AllStarters;
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

    UEdGraphNode *OwningNode = Pin->GetOwningNode();
    if (!OwningNode)
      return Ids;

    // 收集当前节点 + 上游已连接节点，按拓扑顺序（最近的排最后）
    // 用 BFS 向上游遍历，最多 5 步
    TArray<UEdGraphNode *> Visited;
    TArray<UEdGraphNode *> Queue;
    Queue.Add(OwningNode);

    while (Queue.Num() > 0 && Visited.Num() < 6) {
      UEdGraphNode *Current = Queue[0];
      Queue.RemoveAt(0);
      if (Visited.Contains(Current))
        continue;
      Visited.Add(Current);

      // 遍历当前节点的所有输入 pin，找上游节点
      for (UEdGraphPin *P : Current->Pins) {
        if (!P || P->Direction != EGPD_Input)
          continue;
        for (UEdGraphPin *Linked : P->LinkedTo) {
          if (Linked && Linked->GetOwningNode() &&
              !Visited.Contains(Linked->GetOwningNode()))
            Queue.Add(Linked->GetOwningNode());
        }
      }
    }

    // Visited 顺序是 BFS（当前节点优先），反转后变成上游→当前的顺序
    for (int32 i = Visited.Num() - 1; i >= 0; --i) {
      UEdGraphNode *Node = Visited[i];

      // 优先用 Settings class name 匹配（不受本地化影响）
      FString ClassName;
      if (UPCGEditorGraphNode *PCGNode = Cast<UPCGEditorGraphNode>(Node)) {
        if (PCGNode->GetPCGNode() && PCGNode->GetPCGNode()->GetSettings())
          ClassName =
              PCGNode->GetPCGNode()->GetSettings()->GetClass()->GetName();
      }

      for (const FPCGNodeRegistryEntry &Entry : NodeRegistry) {
        bool bMatch =
            (!ClassName.IsEmpty() && Entry.ClassName == ClassName) ||
            Entry.Name ==
                Node->GetNodeTitle(ENodeTitleType::ListView).ToString();
        if (bMatch) {
          Ids.AddUnique(Entry.Id);
          UE_LOG(LogTemp, Log,
                 TEXT("[PCGPredictor] Matched node '%s' -> id=%d"), *Entry.Name,
                 Entry.Id);
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
