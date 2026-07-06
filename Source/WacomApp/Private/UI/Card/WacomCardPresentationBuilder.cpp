// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardPresentationBuilder.h"

#include "WacomCardFaceViewDataBuilder.h"
#include "WacomCardDetailDocumentBuilder.h"

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(const UCardDefinition* Card)
{
	return BuildCardViewData(Card, FWacomCardPresentationRuntimeContext());
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardFaceViewDataBuilder::BuildCardViewData(Card, RuntimeContext);
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(const UCardDefinition* Card)
{
	return BuildCardDetailViewData(Card, FWacomCardPresentationRuntimeContext());
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardDetailDocumentBuilder::BuildCardDetailViewData(Card, RuntimeContext);
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(const UCardDefinition* Card)
{
	return BuildEffectBadges(Card, FWacomCardPresentationRuntimeContext());
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardFaceViewDataBuilder::BuildEffectBadges(Card, RuntimeContext);
}
