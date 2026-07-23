// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomWorldCardSurfaceMaterialAdapter.h"

#include "Components/WidgetComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace
{
	constexpr const TCHAR* WorldCardSurfaceMaterialPath =
		TEXT("/Game/Wacom/UI/Card/World/M_WorldCardSurface.M_WorldCardSurface");
	const FName ExposureStrengthParameterName(TEXT("ExposureCompensationStrength"));
	constexpr float ProductionExposureStrength = 1.0f;
}

const TCHAR* FWacomWorldCardSurfaceMaterialAdapter::GetMaterialPath()
{
	return WorldCardSurfaceMaterialPath;
}

FName FWacomWorldCardSurfaceMaterialAdapter::GetExposureStrengthParameterName()
{
	return ExposureStrengthParameterName;
}

float FWacomWorldCardSurfaceMaterialAdapter::GetProductionExposureStrength()
{
	return ProductionExposureStrength;
}

UMaterialInterface* FWacomWorldCardSurfaceMaterialAdapter::ResolveMaterial()
{
	return LoadObject<UMaterialInterface>(nullptr, WorldCardSurfaceMaterialPath);
}

bool FWacomWorldCardSurfaceMaterialAdapter::Apply(
	UWidgetComponent& Component,
	const float ExposureStrength)
{
	return ApplyResolvedMaterial(
		Component,
		ResolveMaterial(),
		ExposureStrength);
}

bool FWacomWorldCardSurfaceMaterialAdapter::ApplyResolvedMaterial(
	UWidgetComponent& Component,
	UMaterialInterface* Material,
	const float ExposureStrength)
{
	if (!Material
		|| Material->GetPathName() != WorldCardSurfaceMaterialPath
		|| !FMath::IsFinite(ExposureStrength)
		|| ExposureStrength < 0.0f
		|| ExposureStrength > 1.0f)
	{
		return false;
	}

	Component.SetBlendMode(EWidgetBlendMode::Masked);
	Component.SetMaterial(0, Material);
	UMaterialInstanceDynamic* MaterialInstance = Component.GetMaterialInstance();
	if (!MaterialInstance)
	{
		return false;
	}
	MaterialInstance->SetScalarParameterValue(
		ExposureStrengthParameterName,
		ExposureStrength);
	return true;
}
