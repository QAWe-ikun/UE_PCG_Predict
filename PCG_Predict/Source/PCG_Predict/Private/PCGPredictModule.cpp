#include "PCGPredictModule.h"

#define LOCTEXT_NAMESPACE "FPCGPredictModule"

void FPCGPredictModule::StartupModule()
{
    UE_LOG(LogTemp, Log, TEXT("PCGPredict module started"));
}

void FPCGPredictModule::ShutdownModule()
{
    UE_LOG(LogTemp, Log, TEXT("PCGPredict module shutdown"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FPCGPredictModule, PCG_Predict)
