#include "Core/PCGPredictorEngine.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Math/RandomStream.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EdGraph/EdGraphPin.h"
#include "EdGraph/EdGraphNode.h"

void FPCGPredictorEngine::Initialize(const FString& ModelPath)
{
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] ========================================"));
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Initializing predictor engine"));

    // 构建模型路径
    FString FullModelPath = ModelPath;
    if (FullModelPath.IsEmpty()) {
      // 默认路径：插件目录/Models/pcg_predict.onnx
      FString PluginPath =
          FPaths::ProjectPluginsDir() / TEXT("PCG_Predict/Models/");
      FullModelPath = PluginPath / TEXT("pcg_predict.onnx");
    }

    // 初始化 ONNX 运行时
    if (OnnxRuntime.IsValid()) {
      OnnxRuntime->Initialize(FullModelPath);
    } else {
      OnnxRuntime = MakeShareable(new FPCGOnnxRuntime());
      OnnxRuntime->Initialize(FullModelPath);
    }

    // 加载节点注册表
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] About to load node registry..."));
    LoadNodeRegistry();
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Node registry loaded: %d nodes"), NodeRegistry.Num());

    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Predictor engine initialized. Model: %s"),
           *FullModelPath);
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] ========================================"));
}

TArray<FPCGCandidate> FPCGPredictorEngine::Predict(EPCGPredictPinDirection Direction, UEdGraphPin* ContextPin)
{
    TArray<FPCGCandidate> Candidates;

    if (!OnnxRuntime.IsValid() || !OnnxRuntime->IsValid()) {
      // 返回示例数据用于测试
      return GetSampleCandidates(Direction, ContextPin);
    }

    // TODO: 构建输入张量
    // TODO: 运行 ONNX 推理
    // TODO: 解析输出并排序

    // 临时：返回示例数据
    return GetSampleCandidates(Direction, ContextPin);
}

TArray<FPCGCandidate> FPCGPredictorEngine::PredictStarterNodes()
{
    TArray<FPCGCandidate> Candidates;

    if (NodeRegistry.Num() == 0) {
      UE_LOG(LogTemp, Warning, TEXT("Node registry is empty"));
      return Candidates;
    }

    // 查找没有输入pin的节点（起始节点/数据源节点）
    TArray<int32> StarterIndices;
    for (int32 i = 0; i < NodeRegistry.Num(); i++) {
      const FPCGNodeRegistryEntry& Entry = NodeRegistry[i];

      // 排除 Debug 节点和 None 节点
      if (Entry.Id == 130 || Entry.Name == TEXT("None")) {
        continue;
      }

      // 没有输入pin的节点
      if (Entry.InputTypes.Num() == 0 && Entry.OutputTypes.Num() > 0) {
        StarterIndices.Add(i);
      }
    }

    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Found %d starter nodes (no input pins)"),
           StarterIndices.Num());

    if (StarterIndices.Num() == 0) {
      UE_LOG(LogTemp, Warning, TEXT("No starter nodes found"));
      return Candidates;
    }

    // 随机选择 5 个起始节点
    TArray<int32> SelectedIndices;
    int32 NumToSelect = FMath::Min(5, StarterIndices.Num());

    while (SelectedIndices.Num() < NumToSelect) {
      int32 RandomIdx = FMath::RandRange(0, StarterIndices.Num() - 1);
      int32 ActualIndex = StarterIndices[RandomIdx];

      if (!SelectedIndices.Contains(ActualIndex)) {
        SelectedIndices.Add(ActualIndex);

        const FPCGNodeRegistryEntry& Entry = NodeRegistry[ActualIndex];

        FPCGCandidate Candidate;
        Candidate.NodeTypeId = Entry.Id;
        Candidate.NodeTypeName = Entry.Name;
        Candidate.Score = 0.95f - (SelectedIndices.Num() * 0.08f);
        Candidate.Source = EPCGCandidateSource::CreateNew;

        Candidates.Add(Candidate);
      }
    }

    // 按分数排序
    Candidates.Sort([](const FPCGCandidate &A, const FPCGCandidate &B) {
      return A.Score > B.Score;
    });

    return Candidates;
}

void FPCGPredictorEngine::SetIntent(const FString& Text)
{
  CurrentIntent = Text;
  UE_LOG(LogTemp, Log, TEXT("Intent set: %s"), *Text);

  // TODO: 解析意图并更新内部状态
}

TArray<FPCGCandidate> FPCGPredictorEngine::GetSampleCandidates(EPCGPredictPinDirection Direction, UEdGraphPin* ContextPin) const {
  TArray<FPCGCandidate> Samples;

  if (NodeRegistry.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("Node registry is empty"));
    return Samples;
  }

  // 提取已连接节点的类型信息（仅用于显示）
  TArray<FString> ConnectedNodeTypes = ExtractConnectedNodeTypes(ContextPin);

  if (ConnectedNodeTypes.Num() > 0) {
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Pin has %d connected nodes:"), ConnectedNodeTypes.Num());
    for (const FString& NodeType : ConnectedNodeTypes) {
      UE_LOG(LogTemp, Log, TEXT("  - %s"), *NodeType);
    }
  } else {
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Pin has no existing connections"));
  }

  // 根据方向过滤节点
  TArray<int32> ValidIndices;
  for (int32 i = 0; i < NodeRegistry.Num(); i++) {
    const FPCGNodeRegistryEntry& Entry = NodeRegistry[i];

    // 排除 Debug 节点（id=130）
    if (Entry.Id == 130) {
      continue;
    }

    // 排除名为 "None" 的节点
    if (Entry.Name == TEXT("None")) {
      continue;
    }

    // 输出pin预测 → 需要有输入pin的节点
    // 输入pin预测 → 需要有输出pin的节点
    if (Direction == EPCGPredictPinDirection::Output) {
      if (Entry.InputTypes.Num() > 0) {
        ValidIndices.Add(i);
      }
    } else {
      if (Entry.OutputTypes.Num() > 0) {
        ValidIndices.Add(i);
      }
    }
  }

  if (ValidIndices.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("No valid nodes found for direction: %s"),
           Direction == EPCGPredictPinDirection::Output ? TEXT("Output") : TEXT("Input"));
    return Samples;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Found %d valid nodes for direction: %s"),
         ValidIndices.Num(),
         Direction == EPCGPredictPinDirection::Output ? TEXT("Output") : TEXT("Input"));

  // 随机选择 5 个节点
  TArray<int32> SelectedIndices;
  int32 NumToSelect = FMath::Min(5, ValidIndices.Num());

  while (SelectedIndices.Num() < NumToSelect) {
    int32 RandomValidIndex = FMath::RandRange(0, ValidIndices.Num() - 1);
    int32 ActualIndex = ValidIndices[RandomValidIndex];

    if (!SelectedIndices.Contains(ActualIndex)) {
      SelectedIndices.Add(ActualIndex);

      const FPCGNodeRegistryEntry& Entry = NodeRegistry[ActualIndex];

      FPCGCandidate Candidate;
      Candidate.NodeTypeId = Entry.Id;
      Candidate.NodeTypeName = Entry.Name;
      Candidate.Score = 0.95f - (SelectedIndices.Num() * 0.08f); // 递减的分数
      Candidate.Source = EPCGCandidateSource::CreateNew;

      Samples.Add(Candidate);
    }
  }

  // 按分数排序
  Samples.Sort([](const FPCGCandidate &A, const FPCGCandidate &B) {
    return A.Score > B.Score;
  });

  return Samples;
}

TArray<FString> FPCGPredictorEngine::ExtractConnectedNodeTypes(UEdGraphPin* Pin) const {
  TArray<FString> NodeTypes;

  if (!Pin) {
    return NodeTypes;
  }

  // 遍历所有连接
  for (UEdGraphPin* LinkedPin : Pin->LinkedTo) {
    if (LinkedPin && LinkedPin->GetOwningNode()) {
      UEdGraphNode* LinkedNode = LinkedPin->GetOwningNode();
      FString NodeTitle = LinkedNode->GetNodeTitle(ENodeTitleType::ListView).ToString();

      if (!NodeTitle.IsEmpty() && !NodeTypes.Contains(NodeTitle)) {
        NodeTypes.Add(NodeTitle);
      }
    }
  }

  return NodeTypes;
}

void FPCGPredictorEngine::LoadNodeRegistry() {
  NodeRegistry.Empty();

  // 构建配置文件路径 - 使用插件 Content 目录
  FString PluginContentDir = FPaths::ProjectPluginsDir() / TEXT("PCG_Predict/Content/Config/");
  FString ConfigPath = PluginContentDir / TEXT("node_registry.json");

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] ========================================"));
  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Attempting to load node registry..."));
  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] ProjectPluginsDir: %s"), *FPaths::ProjectPluginsDir());
  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] PluginContentDir: %s"), *PluginContentDir);
  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Config path: %s"), *ConfigPath);
  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] File exists: %s"), FPaths::FileExists(ConfigPath) ? TEXT("YES") : TEXT("NO"));

  if (!FPaths::FileExists(ConfigPath)) {
    UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] Node registry JSON not found at: %s"), *ConfigPath);
    UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] ========================================"));
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Found config at: %s"), *ConfigPath);

  // 读取文件
  FString JsonString;
  if (!FFileHelper::LoadFileToString(JsonString, *ConfigPath)) {
    UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] Failed to load node registry: %s"), *ConfigPath);
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] File loaded, size: %d bytes"), JsonString.Len());

  // 解析 JSON
  TSharedPtr<FJsonObject> JsonObject;
  TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonString);

  if (!FJsonSerializer::Deserialize(Reader, JsonObject) || !JsonObject.IsValid()) {
    UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] Failed to parse node registry JSON"));
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] JSON parsed successfully"));

  // 读取节点数组
  const TArray<TSharedPtr<FJsonValue>>* NodesArray;
  if (!JsonObject->TryGetArrayField(TEXT("nodes"), NodesArray)) {
    UE_LOG(LogTemp, Error, TEXT("[PCGPredictor] No 'nodes' array found in JSON"));
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Found nodes array with %d entries"), NodesArray->Num());

  // 解析每个节点
  for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray) {
    const TSharedPtr<FJsonObject>* NodeObj;
    if (!NodeValue->TryGetObject(NodeObj)) {
      continue;
    }

    FPCGNodeRegistryEntry Entry;

    // 读取字段
    (*NodeObj)->TryGetNumberField(TEXT("id"), Entry.Id);
    (*NodeObj)->TryGetStringField(TEXT("name"), Entry.Name);
    (*NodeObj)->TryGetStringField(TEXT("class"), Entry.ClassName);
    (*NodeObj)->TryGetStringField(TEXT("category"), Entry.Category);

    // 读取输入类型数组
    const TArray<TSharedPtr<FJsonValue>>* InputTypesArray;
    if ((*NodeObj)->TryGetArrayField(TEXT("input_types"), InputTypesArray)) {
      for (const TSharedPtr<FJsonValue>& TypeValue : *InputTypesArray) {
        Entry.InputTypes.Add(TypeValue->AsString());
      }
    }

    // 读取输出类型数组
    const TArray<TSharedPtr<FJsonValue>>* OutputTypesArray;
    if ((*NodeObj)->TryGetArrayField(TEXT("output_types"), OutputTypesArray)) {
      for (const TSharedPtr<FJsonValue>& TypeValue : *OutputTypesArray) {
        Entry.OutputTypes.Add(TypeValue->AsString());
      }
    }

    NodeRegistry.Add(Entry);
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Loaded %d PCG nodes from registry"), NodeRegistry.Num());
}
