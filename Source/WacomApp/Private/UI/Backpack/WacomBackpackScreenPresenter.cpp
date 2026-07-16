// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreenPresenter.h"

#include "Cards/CardDefinition.h"
#include "Components/SlateWrapperTypes.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"

#define LOCTEXT_NAMESPACE "WacomBackpackScreenPresenter"

FWacomBackpackWorkspaceCardVisualState UWacomBackpackScreenPresenter::BuildWorkspaceCardVisualState(
	const UWacomBackpackWorkspaceStyle* Style,
	bool bSelected,
	bool bCurrent,
	bool bReadOnly,
	bool bValidTarget,
	bool bRejectedTarget)
{
	const UWacomBackpackWorkspaceStyle* ResolvedStyle = Style
		? Style
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	FWacomBackpackWorkspaceCardVisualState State;
	State.FeedbackMaterial = ResolvedStyle->CardFeedbackMaterial;
	State.Opacity = bReadOnly ? 0.72f : 1.0f;
	if (bRejectedTarget)
	{
		State.Tint = ResolvedStyle->RejectedTargetColor;
		State.FeedbackOpacity = ResolvedStyle->CardStateOverlayOpacity;
	}
	else if (bValidTarget)
	{
		State.Tint = ResolvedStyle->ValidTargetColor;
		State.FeedbackOpacity = ResolvedStyle->CardStateOverlayOpacity;
	}
	else if (bSelected)
	{
		State.Tint = ResolvedStyle->SelectionColor;
		State.FeedbackOpacity = ResolvedStyle->CardStateOverlayOpacity;
	}
	return State;
}

FText UWacomBackpackScreenPresenter::BuildBatchDeleteSummaryText(int32 CardCount, int32 TotalGoldReward)
{
	return FText::Format(
		LOCTEXT("BatchDeleteSummary", "永久销毁 {0} 张卡牌，获得 {1} 金币"),
		FText::AsNumber(FMath::Max(0, CardCount)),
		FText::AsNumber(FMath::Max(0, TotalGoldReward)));
}

TArray<FWacomBackpackZonePileView> UWacomBackpackScreenPresenter::BuildWorkspacePileViews(
	const FRunBackpackStorageSnapshot& Snapshot,
	EZoneKind ExpandedZone,
	FGuid ExpandedOwnerInstanceId,
	bool bHasExpandedPile)
{
	auto IsExpanded = [=](EZoneKind Zone, FGuid Owner)
	{
		return bHasExpandedPile
			&& Zone == ExpandedZone
			&& (Zone != EZoneKind::SpecialZone || Owner == ExpandedOwnerInstanceId);
	};
	TArray<FWacomBackpackZonePileView> Result;
	FWacomBackpackZonePileView Battle;
	Battle.Zone = EZoneKind::BattleDeck;
	Battle.Title = LOCTEXT("WorkspaceBattlePile", "备战区");
	Battle.CardCount = Snapshot.BattleDeckPhysicalCards.Num();
	Battle.Capacity = Snapshot.BattleDeckCapacity;
	Battle.ProjectedCount = Snapshot.BattleDeckProjectedCards.Num();
	Battle.bHasCapacity = true;
	Battle.bExpanded = IsExpanded(Battle.Zone, FGuid());
	Result.Add(MoveTemp(Battle));

	for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
	{
		const FGuid OwnerId = Special.OwnerCard.Instance.InstanceId;
		if (!OwnerId.IsValid())
		{
			continue;
		}
		FWacomBackpackZonePileView Pile;
		Pile.Zone = EZoneKind::SpecialZone;
		Pile.OwnerInstanceId = OwnerId;
		Pile.Title = Special.OwnerCard.Instance.Definition
			? Special.OwnerCard.Instance.Definition->DisplayName
			: LOCTEXT("UnknownSpecialPile", "特殊存放区");
		Pile.CardCount = Special.ContentCards.Num();
		Pile.Capacity = Special.Capacity;
		Pile.bHasCapacity = true;
		Pile.bExpanded = IsExpanded(Pile.Zone, OwnerId);
		Result.Add(MoveTemp(Pile));
	}

	if (!Snapshot.BurdenCards.IsEmpty())
	{
		FWacomBackpackZonePileView Burden;
		Burden.Zone = EZoneKind::BurdenZone;
		Burden.Title = LOCTEXT("WorkspaceBurdenPile", "负重区");
		Burden.CardCount = Snapshot.BurdenCards.Num();
		Burden.bMovable = false;
		Burden.bWarning = true;
		Burden.bExpanded = IsExpanded(Burden.Zone, FGuid());
		Result.Add(MoveTemp(Burden));
	}
	return Result;
}

FText UWacomBackpackScreenPresenter::BuildBattleDeckTitleText(int32 Count, int32 Capacity)
{
	return FText::Format(
		LOCTEXT("BattleDeckTitleFmt", "[ 备战区 ]   {0} / {1}"),
		FText::AsNumber(Count),
		FText::AsNumber(Capacity));
}

FText UWacomBackpackScreenPresenter::BuildBackpackTitleText()
{
	return LOCTEXT("BackpackTitle", "[ 背包区 ]");
}

FText UWacomBackpackScreenPresenter::BuildGoldText(int32 Gold)
{
	return FText::Format(
		LOCTEXT("GoldFmt", "金币：{0}"),
		FText::AsNumber(Gold));
}

FText UWacomBackpackScreenPresenter::BuildFluxContentTitleText(int32 Count, int32 Capacity)
{
	return FText::Format(
		LOCTEXT("FluxContentTitleFmt", "[ 通量内容 ]   {0} / {1}"),
		FText::AsNumber(Count),
		FText::AsNumber(Capacity));
}

FText UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(const FText& OwnerName, int32 CardCount, int32 Capacity)
{
	return FText::Format(
		LOCTEXT("SpecialZoneTitleFmt", "[ 特殊存放区 ] {0}   {1} / {2}"),
		OwnerName,
		FText::AsNumber(CardCount),
		FText::AsNumber(Capacity));
}

FText UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(int32 CardCount)
{
	return FText::Format(
		LOCTEXT("BurdenZoneTitleFmt", "[ 负重区 ] {0}"),
		FText::AsNumber(CardCount));
}

ESlateVisibility UWacomBackpackScreenPresenter::GetBurdenZoneVisibility(int32 CardCount)
{
	return CardCount > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed;
}

ESlateVisibility UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone)
{
	return OwnerZone == EZoneKind::BattleDeck ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed;
}

FText UWacomBackpackScreenPresenter::BuildProjectedFromBadgeText(const FText& OwnerName)
{
	return OwnerName.IsEmpty()
		? FText::GetEmpty()
		: FText::Format(LOCTEXT("ProjectedFromBadgeFmt", "来自 {0}"), OwnerName);
}

FText UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(
	const FRunStorageCardView& ProjectedCard,
	const FRunBackpackStorageSnapshot& Snapshot)
{
	const FRunSpecialStorageView* OwnerSpecialView = Snapshot.SpecialZones.FindByPredicate(
		[&ProjectedCard](const FRunSpecialStorageView& SpecialView)
		{
			return SpecialView.OwnerCard.Instance.InstanceId == ProjectedCard.ZoneOwnerInstanceId;
		});
	if (!OwnerSpecialView || !OwnerSpecialView->OwnerCard.Instance.Definition)
	{
		return FText::GetEmpty();
	}

	return BuildProjectedFromBadgeText(OwnerSpecialView->OwnerCard.Instance.Definition->DisplayName);
}

FWacomCardDetailViewData UWacomBackpackScreenPresenter::BuildCardDetailViewData(const UCardDefinition* Card)
{
	return UWacomCardPresentationBuilder::BuildCardDetailViewData(Card);
}

FVector2D UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
	FVector2D AnchorPosition,
	FVector2D AnchorSize,
	FVector2D LayerSize,
	FVector2D PanelSize,
	float Padding)
{
	const float MaxX = FMath::Max(0.f, LayerSize.X - PanelSize.X);
	const float MaxY = FMath::Max(0.f, LayerSize.Y - PanelSize.Y);

	float X = AnchorPosition.X + AnchorSize.X + Padding;
	if (X + PanelSize.X > LayerSize.X)
	{
		X = AnchorPosition.X - PanelSize.X - Padding;
	}

	return FVector2D(
		FMath::Clamp(X, 0.f, MaxX),
		FMath::Clamp(AnchorPosition.Y, 0.f, MaxY));
}

#undef LOCTEXT_NAMESPACE
