#include "Core/PCGPredictorEngine.h"

void FPCGPredictorEngine::Initialize(const FString& ModelPath)
{
    UE_LOG(LogTemp, Log, TEXT("Initializing predictor engine"));
}

TArray<FPCGCandidate> FPCGPredictorEngine::Predict(EPCGPredictPinDirection Direction)
{
    TArray<FPCGCandidate> Candidates;
    return Candidates;
}

void FPCGPredictorEngine::SetIntent(const FString& Text)
{
    UE_LOG(LogTemp, Log, TEXT("Intent set: %s"), *Text);
}
