#pragma once

#include "CoreMinimal.h"

/**
 * ONNX Runtime 封装
 * 负责加载和运行 PCG 预测模型
 */
class FPCGOnnxRuntime
{
public:
    bool Initialize(const FString& ModelPath);
    TArray<float> RunInference(const TArray<float>& Input);
    bool IsValid() const { return bInitialized; }

private:
    bool bInitialized = false;
    FString ModelFilePath;

    // ONNX Runtime 内部指针（需要包含 onnxruntime_cxx_api.h）
    void *OrtEnv = nullptr;
    void* OrtSession = nullptr;
    void *InputNames = nullptr;
    void *OutputNames = nullptr;
};
