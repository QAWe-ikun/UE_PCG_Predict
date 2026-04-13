#include "Inference/PCGFastPredictor.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

bool FPCGFastPredictor::Initialize(const FString& LookupTablePath)
{
    bInitialized = false;

    if (LookupTablePath.IsEmpty())
    {
        UE_LOG(LogTemp, Warning, TEXT("[FastPredictor] Empty lookup table path"));
        return false;
    }

    if (!FPaths::FileExists(LookupTablePath))
    {
        UE_LOG(LogTemp, Warning,
               TEXT("[FastPredictor] Lookup table not found: %s"), *LookupTablePath);
        UE_LOG(LogTemp, Warning,
               TEXT("[FastPredictor] Run: python src/build_fast_lookup.py"));
        return false;
    }

    bInitialized = LoadFromJson(LookupTablePath);

    if (bInitialized)
    {
        UE_LOG(LogTemp, Log,
               TEXT("[FastPredictor] Loaded: unigram=%d bigram=%d trigram=%d starter=%d"),
               UnigramTable.Num(), BigramTable.Num(),
               TrigramTable.Num(), StarterTable.Num());
    }
    return bInitialized;
}

bool FPCGFastPredictor::LoadFromJson(const FString& JsonPath)
{
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("[FastPredictor] Failed to read file: %s"), *JsonPath);
        return false;
    }

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("[FastPredictor] Failed to parse JSON: %s"), *JsonPath);
        return false;
    }

    // 解析 unigram
    const TSharedPtr<FJsonObject>* UnigramObj;
    if (Root->TryGetObjectField(TEXT("unigram"), UnigramObj))
    {
        for (auto& Pair : (*UnigramObj)->Values)
        {
            const TSharedPtr<FJsonObject>* CandObj;
            if (Pair.Value->TryGetObject(CandObj))
            {
                UnigramTable.Add(Pair.Key, ParseCandidateMap(*CandObj));
            }
        }
    }

    // 解析 bigram
    const TSharedPtr<FJsonObject>* BigramObj;
    if (Root->TryGetObjectField(TEXT("bigram"), BigramObj))
    {
        for (auto& Pair : (*BigramObj)->Values)
        {
            const TSharedPtr<FJsonObject>* CandObj;
            if (Pair.Value->TryGetObject(CandObj))
            {
                BigramTable.Add(Pair.Key, ParseCandidateMap(*CandObj));
            }
        }
    }

    // 解析 trigram
    const TSharedPtr<FJsonObject>* TrigramObj;
    if (Root->TryGetObjectField(TEXT("trigram"), TrigramObj))
    {
        for (auto& Pair : (*TrigramObj)->Values)
        {
            const TSharedPtr<FJsonObject>* CandObj;
            if (Pair.Value->TryGetObject(CandObj))
            {
                TrigramTable.Add(Pair.Key, ParseCandidateMap(*CandObj));
            }
        }
    }

    // 解析 starter（格式不同：直接是 {node_id: prob}）
    const TSharedPtr<FJsonObject>* StarterObj;
    if (Root->TryGetObjectField(TEXT("starter"), StarterObj))
    {
        for (auto& Pair : (*StarterObj)->Values)
        {
            int32 NodeId = FCString::Atoi(*Pair.Key);
            float Prob   = static_cast<float>(Pair.Value->AsNumber());
            StarterTable.Add(NodeId, Prob);
        }
    }

    return true;
}

TMap<int32, float> FPCGFastPredictor::ParseCandidateMap(
    const TSharedPtr<FJsonObject>& Obj)
{
    TMap<int32, float> Result;
    if (!Obj.IsValid()) return Result;

    for (auto& Pair : Obj->Values)
    {
        int32 NodeId = FCString::Atoi(*Pair.Key);
        float Prob   = static_cast<float>(Pair.Value->AsNumber());
        Result.Add(NodeId, Prob);
    }
    return Result;
}

FString FPCGFastPredictor::MakeKey(const TArray<int32>& Ids, int32 NGram)
{
    int32 Start = FMath::Max(0, Ids.Num() - NGram);
    FString Key;
    for (int32 i = Start; i < Ids.Num(); ++i)
    {
        if (i > Start) Key += TEXT(",");
        Key += FString::FromInt(Ids[i]);
    }
    return Key;
}

TArray<FPCGCandidate> FPCGFastPredictor::MapToCandidates(
    const TMap<int32, float>& ScoreMap, int32 TopK)
{
    // 转为数组并按分数降序排序
    TArray<TPair<int32, float>> Sorted;
    Sorted.Reserve(ScoreMap.Num());
    for (auto& Pair : ScoreMap)
    {
        Sorted.Add({Pair.Key, Pair.Value});
    }
    Sorted.Sort([](const TPair<int32,float>& A, const TPair<int32,float>& B){
        return A.Value > B.Value;
    });

    TArray<FPCGCandidate> Results;
    int32 Count = FMath::Min(TopK, Sorted.Num());
    Results.Reserve(Count);

    for (int32 i = 0; i < Count; ++i)
    {
        FPCGCandidate C;
        C.NodeTypeId   = Sorted[i].Key;
        C.NodeTypeName = FString::FromInt(Sorted[i].Key); // 由 Engine 层填充真实名称
        C.Score        = Sorted[i].Value;
        C.Source       = EPCGCandidateSource::CreateNew;
        Results.Add(C);
    }
    return Results;
}

TArray<FPCGCandidate> FPCGFastPredictor::Query(
    const TArray<int32>& RecentNodeIds,
    EPCGPredictPinDirection Direction,
    int32 TopK) const
{
    if (!bInitialized)
    {
        return {};
    }

    // 按优先级查找：trigram → bigram → unigram → starter
    const TMap<int32, float>* BestMatch = nullptr;

    if (RecentNodeIds.Num() >= 3)
    {
        FString Key = MakeKey(RecentNodeIds, 3);
        BestMatch = TrigramTable.Find(Key);
    }
    if (!BestMatch && RecentNodeIds.Num() >= 2)
    {
        FString Key = MakeKey(RecentNodeIds, 2);
        BestMatch = BigramTable.Find(Key);
    }
    if (!BestMatch && RecentNodeIds.Num() >= 1)
    {
        FString Key = MakeKey(RecentNodeIds, 1);
        BestMatch = UnigramTable.Find(Key);
    }
    if (!BestMatch && StarterTable.Num() > 0)
    {
        // starter 直接转换
        return MapToCandidates(StarterTable, TopK);
    }

    if (!BestMatch)
    {
        return {};
    }

    return MapToCandidates(*BestMatch, TopK);
}
