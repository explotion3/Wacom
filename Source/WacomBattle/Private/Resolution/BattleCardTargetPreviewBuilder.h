// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Resolution/BattleCardTargetPreview.h"

struct FBattleState;
struct FWacomInteractionTargetHandle;

class FBattleCardTargetPreviewBuilder
{
public:
	static FBattleCardTargetPreview Build(
		const FBattleState& State,
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target);
};
