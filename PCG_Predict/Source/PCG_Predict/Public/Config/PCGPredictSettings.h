#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "PCGPredictSettings.generated.h"

/**
 * PCG Predict 插件配置
 * 可在 Editor Preferences -> Plugins -> PCG Predict 中修改
 */
UCLASS(config=EditorPerProjectUserSettings, meta=(DisplayName="PCG Predict"))
class PCG_PREDICT_API UPCGPredictSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	/** 是否启用 Pin 悬停自动预测 */
	UPROPERTY(config, EditAnywhere, Category="Prediction")
	bool bEnablePinHoverPrediction = true;

	/** PCG Graph 默认保存路径 */
	UPROPERTY(config, EditAnywhere, Category="Graph Creation", meta=(ContentDir))
	FDirectoryPath DefaultGraphSavePath;

	/** 预测候选数量 */
	UPROPERTY(config, EditAnywhere, Category="Prediction", meta=(ClampMin=1, ClampMax=10))
	int32 MaxCandidates = 5;

	/** 悬停检测间隔（帧数） */
	UPROPERTY(config, EditAnywhere, Category="Performance", meta=(ClampMin=1, ClampMax=60))
	int32 HoverDetectionInterval = 10;

	// UDeveloperSettings interface
	virtual FName GetCategoryName() const override { return TEXT("Plugins"); }
};
