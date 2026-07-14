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

TArray<FWacomBackpackZoneRackEntryView> UWacomBackpackScreenPresenter::BuildZoneRackEntries(
	const FRunBackpackStorageSnapshot& Snapshot,
	EZoneKind ActiveZone,
	FGuid ActiveZoneOwnerInstanceId)
{
	const FGuid NormalizedActiveOwner = ActiveZone == EZoneKind::SpecialZone
		? ActiveZoneOwnerInstanceId
		: FGuid();
	auto IsActive = [ActiveZone, NormalizedActiveOwner](EZoneKind Zone, FGuid OwnerInstanceId)
	{
		const FGuid NormalizedOwner = Zone == EZoneKind::SpecialZone ? OwnerInstanceId : FGuid();
		return Zone == ActiveZone && NormalizedOwner == NormalizedActiveOwner;
	};

	TArray<FWacomBackpackZoneRackEntryView> Entries;
	FWacomBackpackZoneRackEntryView FluxEntry;
	FluxEntry.Zone = EZoneKind::Backpack;
	FluxEntry.Title = LOCTEXT("FluxRackTitle", "通量内容");
	FluxEntry.CardCount = Snapshot.FluxContentCount;
	FluxEntry.Capacity = Snapshot.FluxCapacity;
	FluxEntry.bHasCapacity = true;
	FluxEntry.bActive = IsActive(FluxEntry.Zone, FGuid());
	Entries.Add(MoveTemp(FluxEntry));

	FWacomBackpackZoneRackEntryView BattleEntry;
	BattleEntry.Zone = EZoneKind::BattleDeck;
	BattleEntry.Title = LOCTEXT("BattleRackTitle", "备战区");
	BattleEntry.CardCount = Snapshot.BattleDeckPhysicalCount;
	BattleEntry.Capacity = Snapshot.BattleDeckCapacity;
	BattleEntry.bHasCapacity = true;
	BattleEntry.bActive = IsActive(BattleEntry.Zone, FGuid());
	Entries.Add(MoveTemp(BattleEntry));

	for (const FRunSpecialStorageView& SpecialZone : Snapshot.SpecialZones)
	{
		if (!SpecialZone.OwnerCard.Instance.InstanceId.IsValid())
		{
			continue;
		}
		FWacomBackpackZoneRackEntryView SpecialEntry;
		SpecialEntry.Zone = EZoneKind::SpecialZone;
		SpecialEntry.OwnerInstanceId = SpecialZone.OwnerCard.Instance.InstanceId;
		SpecialEntry.Title = SpecialZone.OwnerCard.Instance.Definition
			? SpecialZone.OwnerCard.Instance.Definition->DisplayName
			: LOCTEXT("UnknownSpecialRackTitle", "特殊存放区");
		SpecialEntry.CardCount = SpecialZone.ContentCards.Num();
		SpecialEntry.Capacity = SpecialZone.Capacity;
		SpecialEntry.bHasCapacity = true;
		SpecialEntry.bActive = IsActive(SpecialEntry.Zone, SpecialEntry.OwnerInstanceId);
		Entries.Add(MoveTemp(SpecialEntry));
	}

	if (Snapshot.BurdenCount > 0)
	{
		FWacomBackpackZoneRackEntryView BurdenEntry;
		BurdenEntry.Zone = EZoneKind::BurdenZone;
		BurdenEntry.Title = LOCTEXT("BurdenRackTitle", "负重区");
		BurdenEntry.CardCount = Snapshot.BurdenCount;
		BurdenEntry.bActive = IsActive(BurdenEntry.Zone, FGuid());
		Entries.Add(MoveTemp(BurdenEntry));
	}
	return Entries;
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
