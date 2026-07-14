// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartImpactStyle.h"

#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"

float UWacomBattleEnemyPartImpactStyle::ResolveDamageIntensity(int32 DamageAmount) const
{
	const float LowerBound = FMath::Min(DamageIntensityMin, DamageIntensityMax);
	const float UpperBound = FMath::Max(DamageIntensityMin, DamageIntensityMax);
	const float RawIntensity = DamageIntensityBase
		+ FMath::Sqrt(static_cast<float>(FMath::Max(0, DamageAmount)))
			* DamageIntensitySqrtScale;
	return FMath::Clamp(RawIntensity, LowerBound, UpperBound);
}

bool UWacomBattleEnemyPartImpactStyle::HasValidVisualAssets() const
{
	return IsValid(ImpactSystem) && IsValid(ImpactMaterialInstance);
}
