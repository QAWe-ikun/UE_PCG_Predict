#include "Core/PCGPredictorEngine.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Misc/Paths.h"

void FPCGPredictorEngine::Initialize(const FString& ModelPath)
{
    UE_LOG(LogTemp, Log, TEXT("Initializing predictor engine"));

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
    LoadNodeRegistry();

    UE_LOG(LogTemp, Log, TEXT("Predictor engine initialized. Model: %s"),
           *FullModelPath);
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

  // 示例候选
  Samples.Add(
      {0, TEXT("TransformPoints"), 0.94f, EPCGCandidateSource::CreateNew});
  Samples.Add(
      {1, TEXT("DensityFilter"), 0.88f, EPCGCandidateSource::CreateNew});
  Samples.Add(
      {2, TEXT("StaticMeshSpawner"), 0.82f, EPCGCandidateSource::CreateNew});
  Samples.Add({3, TEXT("Difference"), 0.75f, EPCGCandidateSource::CreateNew});
  Samples.Add({4, TEXT("Union"), 0.68f, EPCGCandidateSource::CreateNew});

  return Samples;
}

void FPCGPredictorEngine::LoadNodeRegistry() {
  // TODO: 从 YAML 或 JSON 加载节点注册表
  // 包含所有 187 个 PCG 节点的类型信息
}
