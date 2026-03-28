#include "Inference/PCGOnnxRuntime.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

bool FPCGOnnxRuntime::Initialize(const FString& ModelPath)
{
    ModelFilePath = ModelPath;

    // 检查模型文件是否存在
    if (!FPaths::FileExists(ModelPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("ONNX model not found: %s"), *ModelPath);
        UE_LOG(LogTemp, Warning, TEXT("Running in mock mode"));
        
        // 模型不存在时进入模拟模式（用于开发测试）
        bInitialized = true;  // 设为 true 允许继续运行
        return true;
    }

    // TODO: 初始化 ONNX Runtime
    // 1. 创建环境
    // 2. 加载模型
    // 3. 获取输入/输出名称
    
    UE_LOG(LogTemp, Log, TEXT("ONNX Runtime initialized with: %s"), *ModelPath);
    bInitialized = true;
    
    return true;
}

TArray<float> FPCGOnnxRuntime::RunInference(const TArray<float>& Input)
{
    if (!bInitialized)
    {
        UE_LOG(LogTemp, Error, TEXT("ONNX Runtime not initialized"));
        return TArray<float>();
    }

    // TODO: 运行 ONNX 推理
    // 1. 创建输入张量
    // 2. 运行推理
    // 3. 获取输出张量
    
    // 临时：返回模拟输出（10 个类别的分数）
    TArray<float> MockOutput;
    MockOutput.SetNum(10);
    
    // 生成递减的模拟分数
    for (int32 i = 0; i < 10; ++i)
    {
        MockOutput[i] = 0.9f - i * 0.08f;
    }
    
    return MockOutput;
}
