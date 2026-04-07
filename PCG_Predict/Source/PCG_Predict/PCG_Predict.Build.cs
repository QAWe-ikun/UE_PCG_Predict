// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PCG_Predict : ModuleRules
{
	public PCG_Predict(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(
			new string[] {
				// ... add public include paths required here ...
			}
			);


		PrivateIncludePaths.AddRange(
			new string[] {
				// ... add other private include paths required here ...
			}
			);


		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
				"CoreUObject",
				"Engine",
				"PCG",
				"Projects",
				"InputCore",
				"SlateCore",
				"Json",
				"JsonUtilities",
				"DeveloperSettings"
			}
			);


		PrivateDependencyModuleNames.AddRange(
			new string[]
			{
				"UnrealEd",
				"Slate",
				"SlateCore",
				"LevelEditor",
				"ApplicationCore",
				"UMG",
				"SlateReflector",
				"PCGEditor",
				"GraphEditor",
				"PCG",
				"CoreUObject",
				"StructUtilsEditor"
			}
			);

		// 添加 PCGEditor 私有路径以访问 UPCGEditorGraph
		PrivateIncludePaths.AddRange(
			new string[] {
				System.IO.Path.Combine(EngineDirectory, "Plugins/PCG/Source/PCGEditor/Private")
			}
		);


		DynamicallyLoadedModuleNames.AddRange(
			new string[]
			{
				// ... add any modules that your module loads dynamically here ...
			}
			);
	}
}
