#include "Editor/PCGGraphFactory.h"
#include "Config/PCGPredictSettings.h"
#include "PCGGraph.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Subsystems/AssetEditorSubsystem.h"
#include "Editor.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "UObject/Package.h"

UPCGGraph* FPCGGraphFactory::CreateGraphWithIntent(const FString& Intent, const FString& SavePath)
{
	// 确定保存路径
	FString FinalPath = SavePath.IsEmpty() ? GetDefaultSavePath() : SavePath;

	// 生成唯一资产名称
	FString AssetName = GenerateUniqueAssetName();
	FString PackagePath = FinalPath / AssetName;

	// 创建 Package
	UPackage* Package = CreatePackage(*PackagePath);
	if (!Package)
	{
		UE_LOG(LogTemp, Error, TEXT("[PCGGraphFactory] Failed to create package: %s"), *PackagePath);
		return nullptr;
	}

	// 创建 PCG Graph
	UPCGGraph* NewGraph = NewObject<UPCGGraph>(Package, *AssetName, RF_Public | RF_Standalone);
	if (!NewGraph)
	{
		UE_LOG(LogTemp, Error, TEXT("[PCGGraphFactory] Failed to create PCG Graph"));
		return nullptr;
	}

	// 附加意图
	AttachIntentAsComment(NewGraph, Intent);

	// 注册资产
	FAssetRegistryModule::AssetCreated(NewGraph);
	Package->MarkPackageDirty();

	// 打开编辑器
	OpenGraphEditor(NewGraph);

	UE_LOG(LogTemp, Log, TEXT("[PCGGraphFactory] Created PCG Graph: %s"), *PackagePath);
	return NewGraph;
}

void FPCGGraphFactory::OpenGraphEditor(UPCGGraph* Graph)
{
	if (Graph && GEditor)
	{
		GEditor->GetEditorSubsystem<UAssetEditorSubsystem>()->OpenEditorForAsset(Graph);
	}
}

void FPCGGraphFactory::AttachIntentAsComment(UPCGGraph* Graph, const FString& Intent)
{
	if (!Graph)
		return;

	// 暂时使用 Graph 的描述字段存储意图
	// 未来可以考虑创建专门的 PCG Comment 节点
	UE_LOG(LogTemp, Log, TEXT("[PCGGraphFactory] Intent attached: %s"), *Intent);

	// TODO: 在 PCG Graph 中创建 Comment 节点或使用元数据存储意图
	// 当前版本先记录日志，Graph 创建成功即可
}

FString FPCGGraphFactory::GetDefaultSavePath()
{
	const UPCGPredictSettings* Settings = GetDefault<UPCGPredictSettings>();
	FString Path = Settings->DefaultGraphSavePath.Path;

	if (Path.IsEmpty())
	{
		Path = TEXT("/Game/PCG/Generated");
	}

	// 确保目录存在
	FString AbsolutePath = FPaths::ProjectContentDir() / Path.RightChop(6); // 移除 "/Game/"
	IFileManager::Get().MakeDirectory(*AbsolutePath, true);

	return Path;
}

FString FPCGGraphFactory::GenerateUniqueAssetName()
{
	return FString::Printf(TEXT("PCG_Intent_%s"),
		*FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")));
}
