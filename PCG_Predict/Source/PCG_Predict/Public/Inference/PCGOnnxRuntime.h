#pragma once

#include "CoreMinimal.h"

/**
 * 单个命名张量输入，用于多输入 ONNX 推理。
 */
struct FPCGTensorInput
{
    FString        Name;   // 与 ONNX 模型 input_names 对应
    TArray<float>  Data;   // 展平的 float32 数据
    TArray<int64>  Shape;  // 张量形状，例如 {1, 7, 20}
};

/**
 * ONNX Runtime 封装
 * 负责加载和运行 PCG 预测模型
 */
class PCG_PREDICT_API FPCGOnnxRuntime
{
public:
    bool Initialize(const FString& ModelPath);
    bool IsValid() const { return bInitialized; }

    /** 单输入推理（兼容旧接口） */
    TArray<float> RunInference(const TArray<float>& Input);

    /**
     * 多输入推理（DeepPredictor 使用）。
     * @param Inputs  命名张量列表，顺序和名称须与 ONNX 模型一致
     * @return        第一个输出张量的展平 float32 数据；失败返回空数组
     */
    TArray<float> RunInferenceMulti(const TArray<FPCGTensorInput>& Inputs);

private:
    bool    bInitialized = false;
    FString ModelFilePath;

    // ONNX Runtime 内部指针（集成 onnxruntime 后替换为真实类型）
    void* OrtEnv     = nullptr;
    void* OrtSession = nullptr;
};
