#include "Core/PCGPredictorEngine.h"
#include "Inference/PCGOnnxRuntime.h"
#include "Math/RandomStream.h"
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

  // 调试用：随机生成一些 PCG 节点类型
  TArray<FString> PCGNodeTypes = {TEXT("SurfaceSampler"),
                                  TEXT("TransformPoints"),
                                  TEXT("DensityFilter"),
                                  TEXT("StaticMeshSpawner"),
                                  TEXT("Difference"),
                                  TEXT("Union"),
                                  TEXT("Intersection"),
                                  TEXT("GridSampler"),
                                  TEXT("SplineSampler"),
                                  TEXT("VolumeSampler"),
                                  TEXT("RemoveBottom"),
                                  TEXT("FindFloor"),
                                  TEXT("OrientPointsToSurface"),
                                  TEXT("ScalePointsByDistance"),
                                  TEXT("ColorPointsByDistance")};

  // 随机选择 5 个节点类型
  TArray<FString> SelectedTypes;
  for (int32 i = 0; i < 5 && i < PCGNodeTypes.Num(); i++) {
    int32 RandomIndex = FMath::RandRange(0, PCGNodeTypes.Num() - 1);
    FString SelectedType = PCGNodeTypes[RandomIndex];

    // 避免重复
    if (!SelectedTypes.Contains(SelectedType)) {
      SelectedTypes.Add(SelectedType);

      FPCGCandidate Candidate;
      Candidate.NodeTypeId = i;
      Candidate.NodeTypeName = SelectedType;
      Candidate.Score = 0.95f - (i * 0.08f); // 递减的分数
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
  // TODO: 从 YAML 或 JSON 加载节点注册表
  // 包含所有 187 个 PCG 节点的类型信息
}
