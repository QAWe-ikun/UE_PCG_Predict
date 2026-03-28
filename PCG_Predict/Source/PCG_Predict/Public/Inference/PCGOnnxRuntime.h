#pragma once

#include "CoreMinimal.h"

class FPCGOnnxRuntime
{
public:
    bool Initialize(const FString& ModelPath);
    TArray<float> RunInference(const TArray<float>& Input);
    bool IsValid() const { return bInitialized; }

private:
    bool bInitialized = false;
    void* OrtSession = nullptr;
};
