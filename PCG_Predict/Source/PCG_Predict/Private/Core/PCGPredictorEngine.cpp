#include "Core/PCGPredictorEngine.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Math/RandomStream.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"

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

TArray<FPCGCandidate> FPCGPredictorEngine::Predict(EPCGPredictPinDirection Direction)
{
    TArray<FPCGCandidate> Candidates;

    if (!OnnxRuntime.IsValid() || !OnnxRuntime->IsValid()) {
      // 返回示例数据用于测试
      return GetSampleCandidates();
    }

    // TODO: 构建输入张量
    // TODO: 运行 ONNX 推理
    // TODO: 解析输出并排序

    // 临时：返回示例数据
    return GetSampleCandidates();
}

void FPCGPredictorEngine::SetIntent(const FString& Text)
{
  CurrentIntent = Text;
  UE_LOG(LogTemp, Log, TEXT("Intent set: %s"), *Text);

  // TODO: 解析意图并更新内部状态
}

TArray<FPCGCandidate> FPCGPredictorEngine::GetSampleCandidates() const {
  TArray<FPCGCandidate> Samples;

  if (NodeRegistry.Num() == 0) {
    UE_LOG(LogTemp, Warning, TEXT("Node registry is empty"));
    return Samples;
  }

  // 随机选择 5 个节点
  TArray<int32> SelectedIndices;
  int32 NumToSelect = FMath::Min(5, NodeRegistry.Num());

  while (SelectedIndices.Num() < NumToSelect) {
    int32 RandomIndex = FMath::RandRange(0, NodeRegistry.Num() - 1);

    // 避免重复
    if (!SelectedIndices.Contains(RandomIndex)) {
      SelectedIndices.Add(RandomIndex);

      const FPCGNodeRegistryEntry& Entry = NodeRegistry[RandomIndex];

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

void FPCGPredictorEngine::LoadNodeRegistry() {
  NodeRegistry.Empty();

  // 构建配置文件路径 - 使用绝对路径或相对于项目根目录
  FString ConfigPath;

  // 尝试多个可能的路径
  TArray<FString> PossiblePaths = {
    FPaths::ProjectDir() / TEXT("python/config/node_registry.json"),
    FPaths::ProjectDir() / TEXT("../python/config/node_registry.json"),
    TEXT("D:/experiment/UE_PCG_Predict/python/config/node_registry.json")
  };

  bool bFileFound = false;
  for (const FString& Path : PossiblePaths) {
    if (FPaths::FileExists(Path)) {
      ConfigPath = Path;
      bFileFound = true;
      break;
    }
  }

  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Attempting to load node registry..."));
  UE_LOG(LogTemp, Log, TEXT("[PCGPredictor] Project Dir: %s"), *FPaths::ProjectDir());

  if (!bFileFound) {
    UE_LOG(LogTemp, Warning, TEXT("[PCGPredictor] Node registry JSON not found in any of these paths:"));
    for (const FString& Path : PossiblePaths) {
      UE_LOG(LogTemp, Warning, TEXT("[PCGPredictor]   - %s"), *Path);
    }
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
