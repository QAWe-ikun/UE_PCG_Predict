#pragma once

#include "CoreMinimal.h"
#include "PCGPredictionTypes.h"

class FPCGPredictorEngine
{
public:
    void Initialize(const FString& ModelPath);
    TArray<FPCGCandidate> Predict(EPCGPredictPinDirection Direction);
    void SetIntent(const FString& Text);
};
