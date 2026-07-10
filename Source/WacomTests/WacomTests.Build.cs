// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomTests : ModuleRules
{
	public WacomTests(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Automation test framework lives inside Core; no extra module required.
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"WacomCore",
			"WacomData",
			"WacomBattle",
			"WacomRun",
			"WacomApp",
			"WacomEditor",
			"UMG",
			"CommonUI",
			"EnhancedInput",
			"ModelViewViewModel",
			"FieldNotification",
			"Paper2D"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"InputCore",
			"Slate",
			"SlateCore"
		});
	}
}
