// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCombatActivityStyle.h"

#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"

namespace
{
	bool IsCardFlowEvent(EBattleEventType Type)
	{
		return Type == EBattleEventType::CardGained
			|| Type == EBattleEventType::CardDiscarded
			|| Type == EBattleEventType::CardExhausted
			|| Type == EBattleEventType::CardRuntimeCostChanged
			|| Type == EBattleEventType::CardStatusChanged;
	}
}

const FSlateBrush* UWacomBattleCombatActivityStyle::ResolveTagIcon(FGameplayTag Tag) const
{
	if (!Tag.IsValid())
	{
		return nullptr;
	}
	for (const FWacomBattleCombatActivityTagIconEntry& Entry : TagIcons)
	{
		if (Entry.Tag == Tag)
		{
			return &Entry.IconBrush;
		}
	}
	return nullptr;
}

FSlateBrush UWacomBattleCombatActivityStyle::ResolveActivityIconBrush(
	const FWacomBattleCombatActivityRowView& Row) const
{
	if (Row.IconTag.IsValid())
	{
		if (const FSlateBrush* TagBrush = ResolveTagIcon(Row.IconTag))
		{
			return *TagBrush;
		}
	}
	if (Row.IconKey == TEXT("Player"))
	{
		return PlayerPortraitBrush;
	}
	if (Row.IconKey == TEXT("Intent") && EnemyIntentStyle)
	{
		if (const FSlateBrush* IntentBrush = EnemyIntentStyle->ResolveIntentIcon(Row.IntentId))
		{
			return *IntentBrush;
		}
	}
	if (Row.SourceEventType == EBattleEventType::DamageDealt)
	{
		return DamageIconBrush;
	}
	if (IsCardFlowEvent(Row.SourceEventType))
	{
		return CardFlowIconBrush;
	}
	if (Row.IconKey == TEXT("Wait"))
	{
		return WaitIconBrush;
	}
	if (Row.IconKey == TEXT("TurnStart"))
	{
		return TurnIconBrush;
	}
	if (Row.IconKey == TEXT("System"))
	{
		return SystemIconBrush;
	}
	return FallbackIconBrush;
}
