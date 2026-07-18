// Copyright Wacom. All Rights Reserved.

#pragma once

#include "../BackpackScreenTestAccess.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/PanelWidget.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"

namespace WacomBackpackScreenSpecFixture
{

	inline FWacomCardViewEffectBadge MakeCardViewEffectBadgeForTest(
		EWacomCardViewEffectBadgeKind Kind,
		int32 Value)
	{
		FWacomCardViewEffectBadge Badge;
		Badge.Kind = Kind;
		Badge.Value = Value;
		return Badge;
	}

	inline FWacomCardDetailBlock MakeCardDetailTextBlockForTest(
		FName BlockId,
		EWacomCardDetailBlockKind Kind,
		const FString& Text)
	{
		FWacomCardDetailRun Run;
		Run.StableId = FName(*FString::Printf(TEXT("%s.Text"), *BlockId.ToString()));
		Run.Kind = EWacomCardDetailRunKind::Text;
		Run.Text = FText::FromString(Text);

		FWacomCardDetailBlock Block;
		Block.BlockId = BlockId;
		Block.Kind = Kind;
		Block.Runs.Add(Run);
		return Block;
	}

	inline FString JoinCardDetailSectionTextForTest(
		const FWacomCardDetailViewData& Data,
		EWacomCardDetailSectionKind SectionKind)
	{
		FString Text;
		for (const FWacomCardDetailSection& Section : Data.Sections)
		{
			if (Section.Kind != SectionKind)
			{
				continue;
			}

			const FString SectionText =
				UWacomCardDetailPlainTextRenderer::RenderSectionPlainText(Section).ToString();
			if (!SectionText.IsEmpty())
			{
				if (!Text.IsEmpty())
				{
					Text += TEXT("\n");
				}
				Text += SectionText;
			}
		}
		return Text;
	}

	inline const UWacomCardEffectBadgeWidget* GetSingleSlotBadgeForTest(const UPanelWidget* Slot)
	{
		if (!Slot || Slot->GetChildrenCount() != 1)
		{
			return nullptr;
		}

		return Cast<UWacomCardEffectBadgeWidget>(Slot->GetChildAt(0));
	}

	inline UCardDefinition* MakeBackpackUiCardForTest(
		UObject* Outer,
		FName CardId,
		int32 Capacity = 0,
		bool bTypeB = false)
	{
		UCardDefinition* Card = NewObject<UCardDefinition>(Outer);
		Card->CardId = CardId;
		Card->DisplayName = FText::FromName(CardId);
		Card->BaseCost = 1;
		Card->Physique.Capacity = Capacity;
		if (bTypeB)
		{
			Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_WeaponDamagePlus3;
		}
		return Card;
	}

	inline UCharacterDefinition* MakeBackpackUiCharacterForTest(
		UObject* Outer,
		const TArray<UCardDefinition*>& StarterDeck)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Backpack.UI.Character");
		Character->DisplayName = FText::FromString(TEXT("背包 UI 测试角色"));
		for (UCardDefinition* Card : StarterDeck)
		{
			Character->StarterDeck.Add(Card);
		}
		return Character;
	}

	inline UWacomBackpackScreen* MakeBackpackUiScreenForTest(UObject* Outer, URunSession* Run)
	{
		return FWacomBackpackScreenTestAccess::Create(Outer, Run);
	}

}
