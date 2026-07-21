// Copyright Wacom. All Rights Reserved.

#include "Snapshots/BattlePileInspectionSnapshotBuilder.h"

#include "Cards/CardDefinition.h"
#include "Core/BattleRules.h"
#include "Core/BattleState.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"

namespace WacomBattlePileInspectionSnapshotBuilderPrivate
{
	FBattlePileCardSnapshot BuildCard(const FRuntimeCardInstance& Card)
	{
		FBattlePileCardSnapshot Out;
		Out.InstanceId = Card.InstanceId;
		Out.Definition = Card.Definition;
		Out.Location = Card.Location;
		Out.RuntimeCost = FBattleRules::ComputeRuntimeCost(Card);
		Out.StatusStacks = Card.StatusStacks;
		Out.TemporaryKeywords = Card.TemporaryKeywords;
		return Out;
	}

	FBattlePileInspectionSectionSnapshot BuildSection(
		const FBattleState& State,
		ECardLocation Location,
		const TArray<FGuid>& CardIds,
		bool bHideOrder)
	{
		FBattlePileInspectionSectionSnapshot Out;
		Out.Location = Location;
		Out.Count = CardIds.Num();
		Out.bOrderHidden = bHideOrder;
		Out.Cards.Reserve(CardIds.Num());
		for (const FGuid& CardId : CardIds)
		{
			if (const FRuntimeCardInstance* Card = FBattleRules::FindCard(State, CardId))
			{
				Out.Cards.Add(BuildCard(*Card));
			}
		}

		if (bHideOrder)
		{
			Out.Cards.Sort([](const FBattlePileCardSnapshot& A, const FBattlePileCardSnapshot& B)
			{
				const FString APath = A.Definition ? A.Definition->GetPathName() : FString();
				const FString BPath = B.Definition ? B.Definition->GetPathName() : FString();
				const int32 PathCompare = APath.Compare(BPath, ESearchCase::CaseSensitive);
				return PathCompare != 0
					? PathCompare < 0
					: A.InstanceId.ToString(EGuidFormats::Digits)
						< B.InstanceId.ToString(EGuidFormats::Digits);
			});
		}
		return Out;
	}
}

FBattlePileInspectionSnapshot FBattlePileInspectionSnapshotBuilder::Build(
	const FBattleState& State)
{
	using namespace WacomBattlePileInspectionSnapshotBuilderPrivate;
	FBattlePileInspectionSnapshot Out;
	Out.BattleVersion = State.StateVersion;
	Out.Sections.Reserve(4);
	Out.Sections.Add(BuildSection(State, ECardLocation::Draw, State.Cards.DrawPile, true));
	Out.Sections.Add(BuildSection(State, ECardLocation::Discard, State.Cards.DiscardPile, false));
	Out.Sections.Add(BuildSection(State, ECardLocation::Played, State.Cards.PlayedPile, false));
	Out.Sections.Add(BuildSection(State, ECardLocation::Exhaust, State.Cards.ExhaustPile, false));
	return Out;
}
