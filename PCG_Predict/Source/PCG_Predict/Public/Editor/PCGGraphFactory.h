#pragma once

#include "CoreMinimal.h"

class UPCGGraph;

/**
 * PCG Graph 工厂类
 * 负责创建带意图的 PCG Graph 资产
 */
class PCG_PREDICT_API FPCGGraphFactory
{
public:
	/**
	 * 创建带意图的 PCG Graph
	 * @param Intent 用户意图描述
	 * @param SavePath 保存路径（可选，使用配置默认路径）
	 * @return 创建的 Graph，失败返回 nullptr
	 */
	static UPCGGraph* CreateGraphWithIntent(const FString& Intent, const FString& SavePath = TEXT(""));

	/**
	 * 打开 PCG Graph 编辑器
	 */
	static void OpenGraphEditor(UPCGGraph* Graph);

	/**
	 * 将意图附加为 Comment 节点
	 */
	static void AttachIntentAsComment(UPCGGraph* Graph, const FString& Intent);

private:
	static FString GetDefaultSavePath();
	static FString GenerateUniqueAssetName();
};
