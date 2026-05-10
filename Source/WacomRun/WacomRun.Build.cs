// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomRun : ModuleRules
{
	public WacomRun(ReadOnlyTargetRules Target) : base(Target)
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

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
