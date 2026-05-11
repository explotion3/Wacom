// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomEditor : ModuleRules
{
	public WacomEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

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
			"AssetTools",
			"AssetRegistry",
			"EditorStyle",
			"Slate",
			"SlateCore",
			"ToolMenus",
			"DataValidation",
			"EnhancedInput",
			"InputCore"
		});
	}
}
