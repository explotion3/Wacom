// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Styling/SlateBrush.h"

/** One localized, passive row derived from an authoritative Intent effect fact. */
struct WACOMAPP_API FWacomBattleIntentEffectRowViewData
{
	FGameplayTag EffectType;
	FText TargetText;
	FText EffectText;
	FText CoreRuleText;
	FSlateBrush IconBrush;
	FLinearColor Tint = FLinearColor::White;
	int32 RepeatCount = 1;
};

/** Passive presentation model used by the head-up and enemy-inspection Intent tooltips. */
struct WACOMAPP_API FWacomBattleIntentPresentationViewData
{
	FName IntentId = NAME_None;
	FText IntentDisplayName;
	FText HeaderMetaText;
	FSlateBrush IntentIconBrush;
	TArray<FWacomBattleIntentEffectRowViewData> EffectRows;
	int32 HiddenEffectRowCount = 0;

	bool HasIntent() const
	{
		return !IntentId.IsNone();
	}
};
