// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Runtime callbacks shared by simple-host and multi-part enemy action playback. */
struct WACOMAPP_API FWacomBattleEnemyActionPlaybackCallbacks
{
	TFunction<void()> OnImpact;
	TFunction<void()> OnCompleted;

	void CompleteImmediately()
	{
		if (OnImpact)
		{
			TFunction<void()> Callback = MoveTemp(OnImpact);
			Callback();
		}
		if (OnCompleted)
		{
			TFunction<void()> Callback = MoveTemp(OnCompleted);
			Callback();
		}
	}
};
