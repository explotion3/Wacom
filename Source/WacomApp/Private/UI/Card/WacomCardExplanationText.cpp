// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationText.h"

#include "Tags/WacomGameplayTags.h"

#define LOCTEXT_NAMESPACE "WacomCardExplanationText"

namespace WacomCardExplanationText
{
	FString GetDisplayTagLeafName(const FGameplayTag& Tag)
	{
		const FString TagText = Tag.IsValid() ? Tag.ToString() : FString();
		int32 DotIndex = INDEX_NONE;
		TagText.FindLastChar(TEXT('.'), DotIndex);
		return DotIndex == INDEX_NONE ? TagText : TagText.Mid(DotIndex + 1);
	}

	FText GetDisplayHandZoneName(const FGameplayTag& HandZoneTag)
	{
		if (HandZoneTag.MatchesTagExact(WacomTags::HandZone_Left))
		{
			return LOCTEXT("HandZoneLeft", "左手区");
		}
		if (HandZoneTag.MatchesTagExact(WacomTags::HandZone_Both))
		{
			return LOCTEXT("HandZoneBoth", "双手区");
		}
		if (HandZoneTag.MatchesTagExact(WacomTags::HandZone_Right))
		{
			return LOCTEXT("HandZoneRight", "右手区");
		}
		return FText::FromString(GetDisplayTagLeafName(HandZoneTag));
	}

	FText GetDisplayStatusName(const FGameplayTag& StatusTag)
	{
		if (StatusTag.MatchesTagExact(WacomTags::Status_Poison))
		{
			return LOCTEXT("StatusPoison", "中毒");
		}
		if (StatusTag.MatchesTagExact(WacomTags::Status_Slow))
		{
			return LOCTEXT("StatusSlow", "迟缓");
		}
		if (StatusTag.MatchesTagExact(WacomTags::Status_Freeze))
		{
			return LOCTEXT("StatusFreeze", "冻结");
		}
		if (StatusTag.MatchesTagExact(WacomTags::Status_Twilight))
		{
			return LOCTEXT("StatusTwilight", "暮气");
		}
		if (StatusTag.MatchesTagExact(WacomTags::Status_Stunned))
		{
			return LOCTEXT("StatusStunned", "眩晕");
		}
		if (StatusTag.MatchesTagExact(WacomTags::Status_Shield))
		{
			return LOCTEXT("StatusShield", "护盾");
		}
		return FText::FromString(GetDisplayTagLeafName(StatusTag));
	}
}

#undef LOCTEXT_NAMESPACE
