// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomBattle : ModuleRules
{
	public WacomBattle(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"WacomCore",
			"WacomData"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
