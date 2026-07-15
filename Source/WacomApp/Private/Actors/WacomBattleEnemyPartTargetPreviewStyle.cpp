// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"

#include "Materials/MaterialInterface.h"
#include "NiagaraSystem.h"

bool UWacomBattleEnemyPartTargetPreviewStyle::HasValidVisualAssets() const
{
	return IsValid(PreviewSystem) && IsValid(PreviewMaterialInstance);
}
