// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

/**
 * PCG Predict 模块
 * 提供 PCG Graph 智能预测功能
 */
class FPCG_PredictModule : public IModuleInterface
{
public:
  /** IModuleInterface implementation */
  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};
