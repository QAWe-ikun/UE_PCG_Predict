#pragma once

#include "CoreMinimal.h"
#include "Core/PCGPredictionTypes.h"

/**
 * PCGFastPredictor
 * 基于 n-gram 统计查找表的快速节点预测器。
 *
 * 工作流程：
 *   1. Initialize() 时一次性加载 models/fast_lookup.json 到内存
 *   2. Query() 时按 trigram → bigram → unigram → starter 优先级查表
 *   3. 全程只做 TMap::Find + 数组排序，< 1ms
 *
 * 查找表格式（由 Python src/build_fast_lookup.py 生成）：
 *   {
 *     "unigram":  {"87": {"36": 0.45, "100": 0.20}},
 *     "bigram":   {"87,36": {"100": 0.72}},
 *     "trigram":  {"87,36,100": {"7": 0.80}},
 *     "starter":  {"87": 0.35}
 *   }
 */
class PCG_PREDICT_API FPCGFastPredictor
{
public:
    /**
     * 加载查找表 JSON 文件。
     * @param LookupTablePath  fast_lookup.json 的绝对路径
     * @return 加载成功返回 true
     */
    bool Initialize(const FString& LookupTablePath);

    bool IsValid() const { return bInitialized; }

    /**
     * 查询下一个节点候选。
     * @param RecentNodeIds  最近操作的节点 type_id 列表（按时间顺序，最新在末尾）
     * @param Direction      预测方向（Output=预测下游, Input=预测上游）
     * @param TopK           返回候选数量
     * @return               按 Score 降序排列的候选列表
     */
    TArray<FPCGCandidate> Query(
        const TArray<int32>& RecentNodeIds,
        EPCGPredictPinDirection Direction,
        int32 TopK = 10) const;

private:
    bool bInitialized = false;

    // 三级查找表：key = "id1,id2,..." → {node_type_id → probability}
    TMap<FString, TMap<int32, float>> UnigramTable;
    TMap<FString, TMap<int32, float>> BigramTable;
    TMap<FString, TMap<int32, float>> TrigramTable;
    TMap<int32, float>                StarterTable;

    /** 从 JSON 文件加载所有表 */
    bool LoadFromJson(const FString& JsonPath);

    /** 解析单个候选 map：{"node_id_str": probability} */
    static TMap<int32, float> ParseCandidateMap(
        const TSharedPtr<class FJsonObject>& Obj);

    /** 构建 n-gram key，取 RecentNodeIds 末尾 NGram 个 */
    static FString MakeKey(const TArray<int32>& Ids, int32 NGram);

    /** 将 TMap<int32,float> 转换为排序后的 FPCGCandidate 数组 */
    static TArray<FPCGCandidate> MapToCandidates(
        const TMap<int32, float>& ScoreMap, int32 TopK);
};
