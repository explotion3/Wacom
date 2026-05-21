// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreenPresenter.h"

#include "Cards/CardDefinition.h"
#include "Components/SlateWrapperTypes.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

#define LOCTEXT_NAMESPACE "WacomBackpackScreenPresenter"

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
