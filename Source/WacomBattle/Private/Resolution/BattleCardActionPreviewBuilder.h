// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Resolution/BattleCardActionPreview.h"

struct FBattleState;
struct FWacomInteractionTargetHandle;

class FBattleCardActionPreviewBuilder
{
public:
	static FBattleCardActionPreview Build(
		const FBattleState& State,
		const FGuid& CardInstanceId,
		const FWacomInteractionTargetHandle& Target);
};
