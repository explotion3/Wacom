// Copyright Wacom. All Rights Reserved.

#include "WacomCardExplanationText.h"

#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalog.h"
#include "UI/Battle/WacomBattleStatusPresentationCatalogProvider.h"
#include "UI/Card/WacomCardExplanationLexicon.h"

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

	FText GetDisplayTagName(
		const FGameplayTag& Tag,
		const UWacomCardExplanationLexicon* Lexicon)
	{
		FText DisplayName;
		if (Lexicon && Lexicon->FindTagDisplayName(Tag, DisplayName))
		{
			return DisplayName;
		}
		return FText::FromString(GetDisplayTagLeafName(Tag));
	}

	FText GetDisplayHandZoneName(
		const FGameplayTag& HandZoneTag,
		const UWacomCardExplanationLexicon* Lexicon)
	{
		FText DisplayName;
		if (Lexicon && Lexicon->FindTagDisplayName(HandZoneTag, DisplayName))
		{
			return DisplayName;
		}
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

	FText GetDisplayStatusName(
		const FGameplayTag& StatusTag,
		const UWacomCardExplanationLexicon* /*Lexicon*/)
	{
		return WacomBattleStatusPresentationCatalogProvider::GetCatalog()
			.ResolveDisplayName(StatusTag);
	}

	FText ResolveNamedText(
		const UWacomCardExplanationLexicon* Lexicon,
		FName Key,
		const FText& Fallback)
	{
		FText Text;
		return Lexicon && Lexicon->FindNamedText(Key, Text) ? Text : Fallback;
	}

	FText FormatNamedText(
		const UWacomCardExplanationLexicon* Lexicon,
		FName Key,
		const FText& Fallback,
		const FFormatOrderedArguments& Arguments)
	{
		return FText::Format(ResolveNamedText(Lexicon, Key, Fallback), Arguments);
	}
}

#undef LOCTEXT_NAMESPACE
