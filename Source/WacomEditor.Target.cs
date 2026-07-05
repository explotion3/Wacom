// Copyright Wacom. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class WacomEditorTarget : TargetRules
{
	public WacomEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_8;

		ExtraModuleNames.AddRange(new string[]
		{
			"WacomCore",
			"WacomData",
			"WacomBattle",
			"WacomRun",
			"WacomApp",
			"WacomEditor",
			"WacomTests"
		});
	}
}
