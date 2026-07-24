// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardPresentationBuilder.h"

#include "WacomCardFaceViewDataBuilder.h"
#include "WacomCardDetailDocumentBuilder.h"

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(const UCardDefinition* Card)
{
	return BuildCardViewData(
		Card,
		EWacomCardFaceContext::Battle,
		FWacomCardPresentationRuntimeContext());
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewDataForFace(
	const UCardDefinition* Card,
	EWacomCardFaceContext FaceContext)
{
	return BuildCardViewData(Card, FaceContext, FWacomCardPresentationRuntimeContext());
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return BuildCardViewData(Card, EWacomCardFaceContext::Battle, RuntimeContext);
}

FWacomCardViewData UWacomCardPresentationBuilder::BuildCardViewData(
	const UCardDefinition* Card,
	EWacomCardFaceContext FaceContext,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardFaceViewDataBuilder::BuildCardViewData(
		Card,
		FaceContext,
		RuntimeContext);
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(const UCardDefinition* Card)
{
	return BuildCardDetailViewData(
		Card,
		EWacomCardFaceContext::Battle,
		FWacomCardPresentationRuntimeContext());
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewDataForFace(
	const UCardDefinition* Card,
	EWacomCardFaceContext FaceContext)
{
	return BuildCardDetailViewData(Card, FaceContext, FWacomCardPresentationRuntimeContext());
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return BuildCardDetailViewData(Card, EWacomCardFaceContext::Battle, RuntimeContext);
}

FWacomCardDetailViewData UWacomCardPresentationBuilder::BuildCardDetailViewData(
	const UCardDefinition* Card,
	EWacomCardFaceContext FaceContext,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardDetailDocumentBuilder::BuildCardDetailViewData(
		Card,
		FaceContext,
		RuntimeContext);
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(const UCardDefinition* Card)
{
	return BuildEffectBadges(
		Card,
		EWacomCardFaceContext::Battle,
		FWacomCardPresentationRuntimeContext());
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(
	const UCardDefinition* Card,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return BuildEffectBadges(Card, EWacomCardFaceContext::Battle, RuntimeContext);
}

TArray<FWacomCardViewEffectBadge> UWacomCardPresentationBuilder::BuildEffectBadges(
	const UCardDefinition* Card,
	EWacomCardFaceContext FaceContext,
	const FWacomCardPresentationRuntimeContext& RuntimeContext)
{
	return WacomCardFaceViewDataBuilder::BuildEffectBadges(
		Card,
		FaceContext,
		RuntimeContext);
}
