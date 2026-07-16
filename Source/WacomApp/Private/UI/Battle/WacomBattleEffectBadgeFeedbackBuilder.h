// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleCardPresentationHelper.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

namespace WacomBattleEffectBadgeFeedback
{
	inline TArray<FWacomFirstPersonCardEffectBadgeChange> BuildVisibleValueChanges(
		const FHandCardSnapshot& PreviousCard,
		const FHandCardSnapshot& NextCard,
		int32 EventSequence)
	{
		const FWacomCardViewData PreviousView =
			WacomBattleCardPresentation::BuildCardViewData(PreviousCard);
		const FWacomCardViewData NextView =
			WacomBattleCardPresentation::BuildCardViewData(NextCard);
		TArray<FWacomFirstPersonCardEffectBadgeChange> Result;
		for (const FWacomCardViewEffectBadge& NextBadge : NextView.EffectBadges)
		{
			if (NextBadge.PresentationKey.IsNone())
			{
				continue;
			}
			const FWacomCardViewEffectBadge* PreviousBadge =
				PreviousView.EffectBadges.FindByPredicate([&NextBadge](
					const FWacomCardViewEffectBadge& Candidate)
				{
					return Candidate.PresentationKey == NextBadge.PresentationKey;
				});
			if (!PreviousBadge || PreviousBadge->Value == NextBadge.Value)
			{
				continue;
			}

			FWacomFirstPersonCardEffectBadgeChange& Change = Result.AddDefaulted_GetRef();
			Change.PresentationKey = NextBadge.PresentationKey;
			Change.BadgeKind = NextBadge.Kind;
			Change.ChangeKind = EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged;
			Change.Direction = NextBadge.Value > PreviousBadge->Value
				? EWacomFirstPersonCardEffectBadgeValueDirection::Increase
				: EWacomFirstPersonCardEffectBadgeValueDirection::Decrease;
			Change.OldValue = PreviousBadge->Value;
			Change.NewValue = NextBadge.Value;
			Change.Seed = static_cast<int32>(HashCombineFast(
				GetTypeHash(NextCard.InstanceId),
				HashCombineFast(
					GetTypeHash(NextBadge.PresentationKey),
					GetTypeHash(EventSequence))) & 0x7FFFFFFFu);
		}
		Result.Sort([](
			const FWacomFirstPersonCardEffectBadgeChange& A,
			const FWacomFirstPersonCardEffectBadgeChange& B)
		{
			return A.PresentationKey.LexicalLess(B.PresentationKey);
		});
		return Result;
	}
}
