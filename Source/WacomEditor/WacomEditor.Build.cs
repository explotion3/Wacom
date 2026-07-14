// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomEditor : ModuleRules
{
	public WacomEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		bUseUnity = false;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"WacomCore",
			"WacomData",
			"WacomBattle"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"UnrealEd",
			"UMG",
			"UMGEditor",
			"AssetTools",
			"AssetRegistry",
			"EditorStyle",
			"Slate",
			"SlateCore",
			"UMG",
			"UMGEditor",
			"CommonUI",
			"ToolMenus",
			"DataValidation",
			"EnhancedInput",
			"InputCore",
			"WacomApp"
		});
	}
}
