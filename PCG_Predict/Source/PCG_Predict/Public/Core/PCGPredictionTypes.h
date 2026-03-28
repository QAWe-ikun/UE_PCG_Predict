#pragma once

#include "CoreMinimal.h"

enum class EPCGPredictDataType : uint8
{
    Point, Spatial, Param, Execution, Concrete, Landscape, Spline, Volume
};

enum class EPCGPredictPinDirection : uint8
{
    Input, Output
};

enum class EPCGCandidateSource : uint8
{
    CreateNew, ConnectExisting
};

struct FPCGCandidate
{
    int32 NodeTypeId;
    FString NodeTypeName;
    float Score;
    EPCGCandidateSource Source;
};
