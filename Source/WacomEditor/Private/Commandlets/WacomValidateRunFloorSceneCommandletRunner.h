// Copyright Wacom. All Rights Reserved.

#pragma once

class UWorld;
struct FWacomRunSceneBindingValidationReport;

namespace Wacom::Editor
{
	int32 ClassifyRunFloorSceneValidation(
		const UWorld* World,
		bool bHasMapArgument,
		bool bMapLoaded,
		FWacomRunSceneBindingValidationReport* OutReport = nullptr);
}
