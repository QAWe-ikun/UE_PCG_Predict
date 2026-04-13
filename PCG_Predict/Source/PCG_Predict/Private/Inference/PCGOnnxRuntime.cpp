#include "Inference/PCGOnnxRuntime.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

bool FPCGOnnxRuntime::Initialize(const FString& ModelPath)
{
    ModelFilePath = ModelPath;

    if (!FPaths::FileExists(ModelPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("[OnnxRuntime] Model not found: %s"), *ModelPath);
        bInitialized = false;
        return false;
    }

    // TODO: 集成 ONNX Runtime 库后替换为真实初始化
    // Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "PCGPredict");
    // Ort::SessionOptions opts;
    // opts.SetIntraOpNumThreads(1);
    // OrtSession = new Ort::Session(env, *ModelPath, opts);

    UE_LOG(LogTemp, Log, TEXT("[OnnxRuntime] Initialized: %s"), *ModelPath);
    bInitialized = true;
    return true;
}

TArray<float> FPCGOnnxRuntime::RunInference(const TArray<float>& Input)
{
    if (!bInitialized) return {};

    // TODO: 真实推理（ONNX Runtime 集成后实现）
    return {};
}

TArray<float> FPCGOnnxRuntime::RunInferenceMulti(const TArray<FPCGTensorInput>& Inputs)
{
    if (!bInitialized) return {};

    if (Inputs.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[OnnxRuntime] RunInferenceMulti: empty inputs"));
        return {};
    }

    // TODO: 集成 ONNX Runtime 后替换为真实多输入推理：
    //
    // TArray<const char*> InputNames;
    // TArray<Ort::Value> InputTensors;
    // Ort::MemoryInfo MemInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
    //
    // for (const FPCGTensorInput& T : Inputs)
    // {
    //     InputNames.Add(TCHAR_TO_ANSI(*T.Name));
    //     TArray<int64> Shape = T.Shape;
    //     InputTensors.Add(Ort::Value::CreateTensor<float>(
    //         MemInfo,
    //         const_cast<float*>(T.Data.GetData()), T.Data.Num(),
    //         Shape.GetData(), Shape.Num()
    //     ));
    // }
    //
    // const char* OutputName = "logits";
    // auto Outputs = Session->Run(Ort::RunOptions{nullptr},
    //     InputNames.GetData(), InputTensors.GetData(), InputTensors.Num(),
    //     &OutputName, 1);
    //
    // float* OutData = Outputs[0].GetTensorMutableData<float>();
    // int64 OutSize  = Outputs[0].GetTensorTypeAndShapeInfo().GetElementCount();
    // return TArray<float>(OutData, OutSize);

    return {};
}
