// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomData : ModuleRules
{
	public WacomData(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			"WacomCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { });
	}
}
