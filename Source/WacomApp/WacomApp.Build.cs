// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomApp : ModuleRules
{
	public WacomApp(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayTags",
			"UMG",
			"CommonUI",
			"CommonInput",
			"DeveloperSettings",
			"FieldNotification",
			"ModelViewViewModel",
			"WacomCore",
			"WacomData",
			"WacomBattle",
			"WacomRun",
			"Paper2D",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Slate",
			"SlateCore"
		});
	}
}
