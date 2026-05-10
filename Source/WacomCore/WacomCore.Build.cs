// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;

public class WacomCore : ModuleRules
{
	public WacomCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"GameplayTags"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"Engine"    // NativeGameplayTags 依赖 UObject/反射基础设施，需要 Engine
		});
	}
}
