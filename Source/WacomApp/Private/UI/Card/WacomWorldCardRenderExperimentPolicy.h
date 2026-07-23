// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WidgetComponent.h"

enum class EWacomWorldCardRenderExperimentMode : uint8
{
	EngineTransparent,
	EngineMasked,
	WacomMaskedRaw,
	WacomMaskedExposure
};

struct FWacomWorldCardRenderExperimentModeConfig
{
	EWacomWorldCardRenderExperimentMode Mode =
		EWacomWorldCardRenderExperimentMode::EngineTransparent;
	const TCHAR* Label = TEXT("");
	EWidgetBlendMode BlendMode = EWidgetBlendMode::Transparent;
	bool bUseWacomMaterial = false;
	float ExposureCompensationStrength = 0.0f;
};

/**
 * Private, deterministic policy shared by the transient PIE harness and its
 * contract tests. It deliberately contains no Run or Shop state.
 */
class WACOMAPP_API FWacomWorldCardRenderExperimentPolicy
{
public:
	static TConstArrayView<FWacomWorldCardRenderExperimentModeConfig> GetModes();
	static const TCHAR* GetCardViewClassPath();
	static const TCHAR* GetWorldMaterialPath();
	static bool IsSupportedPIEWorld(const UWorld* World);
};
