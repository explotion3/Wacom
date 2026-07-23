// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomWorldCardRenderExperimentPolicy.h"

#include "Engine/World.h"
#include "UI/Card/WacomWorldCardSurfaceMaterialAdapter.h"

namespace
{
	const FWacomWorldCardRenderExperimentModeConfig ModeConfigs[] =
	{
		{
			EWacomWorldCardRenderExperimentMode::EngineTransparent,
			TEXT("Engine Transparent"),
			EWidgetBlendMode::Transparent,
			false,
			0.0f
		},
		{
			EWacomWorldCardRenderExperimentMode::EngineMasked,
			TEXT("Engine Masked"),
			EWidgetBlendMode::Masked,
			false,
			0.0f
		},
		{
			EWacomWorldCardRenderExperimentMode::WacomMaskedRaw,
			TEXT("Wacom Masked Raw"),
			EWidgetBlendMode::Masked,
			true,
			0.0f
		},
		{
			EWacomWorldCardRenderExperimentMode::WacomMaskedExposure,
			TEXT("Wacom Masked Exposure"),
			EWidgetBlendMode::Masked,
			true,
			1.0f
		}
	};
}

TConstArrayView<FWacomWorldCardRenderExperimentModeConfig>
FWacomWorldCardRenderExperimentPolicy::GetModes()
{
	return MakeArrayView(ModeConfigs);
}

const TCHAR* FWacomWorldCardRenderExperimentPolicy::GetCardViewClassPath()
{
	return TEXT("/Game/Wacom/UI/Card/WBP_FirstPersonCardView.WBP_FirstPersonCardView_C");
}

const TCHAR* FWacomWorldCardRenderExperimentPolicy::GetWorldMaterialPath()
{
	return FWacomWorldCardSurfaceMaterialAdapter::GetMaterialPath();
}

bool FWacomWorldCardRenderExperimentPolicy::IsSupportedPIEWorld(const UWorld* World)
{
	return World && World->WorldType == EWorldType::PIE;
}
