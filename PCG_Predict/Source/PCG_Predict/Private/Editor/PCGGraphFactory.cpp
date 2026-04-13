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
    if (!Graph || Intent.IsEmpty())
    {
        return;
    }

    // 使用 Description 字段存储意图（UPCGGraph 原生支持）
    // 格式: "[PCGPredict_Intent]茂密森林，高低错落"
    FString IntentTag = FString::Printf(TEXT("[PCGPredict_Intent]%s"), *Intent);
    Graph->Description = FText::FromString(IntentTag);

    UE_LOG(LogTemp, Log, TEXT("[PCGGraphFactory] Intent attached: %s"), *Intent);
}

FString FPCGGraphFactory::GetGraphIntent(const UPCGGraph* Graph)
{
    if (!Graph)
    {
        return FString();
    }

    FString Desc = Graph->Description.ToString();
    const FString Prefix = TEXT("[PCGPredict_Intent]");

    if (Desc.StartsWith(Prefix))
    {
        return Desc.RightChop(Prefix.Len());
    }

    return FString();
}

void FPCGGraphFactory::ClearGraphIntent(UPCGGraph* Graph)
{
    if (!Graph)
    {
        return;
    }

    FString Desc = Graph->Description.ToString();
    const FString Prefix = TEXT("[PCGPredict_Intent]");

    if (Desc.StartsWith(Prefix))
    {
        Graph->Description = FText::GetEmpty();
        UE_LOG(LogTemp, Log, TEXT("[PCGGraphFactory] Intent cleared"));
    }
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
