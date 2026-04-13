// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class FPCGToolbarExtension;
class FPCGEditorExtension;
class FPCGPinHoverIntegration;

class FPCGPredictModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;

    FPCGPinHoverIntegration* GetPinHoverIntegration() { return PinHoverIntegration.Get(); }
    FPCGEditorExtension* GetEditorExtension() { return EditorExtension.Get(); }

private:
    TSharedPtr<FPCGToolbarExtension> ToolbarExtension;
    TSharedPtr<FPCGEditorExtension> EditorExtension;
    TSharedPtr<FPCGPinHoverIntegration> PinHoverIntegration;
};
