// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class WacomTarget : TargetRules
{
	public WacomTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

		ExtraModuleNames.AddRange(new string[]
		{
			"WacomCore",
			"WacomData",
			"WacomBattle",
			"WacomRun",
			"WacomApp"
		});
	}
}
