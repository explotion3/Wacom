// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleCardPresentationHelper.h"

#include "Snapshots/HandSnapshot.h"
#include "UI/Card/WacomCardPresentationBuilder.h"

namespace WacomBattleCardPresentation
{
	FWacomCardPresentationRuntimeContext BuildRuntimeContext(const FHandCardSnapshot& CardSnapshot)
	{
		FWacomCardPresentationRuntimeContext Context;
		Context.bHasRuntimeCost = true;
		Context.RuntimeCost = CardSnapshot.RuntimeCost;
		Context.bHasPlayableState = true;
		Context.bIsPlayable = CardSnapshot.bIsPlayable;
		return Context;
	}

	FWacomCardViewData BuildCardViewData(const FHandCardSnapshot& CardSnapshot)
	{
		return UWacomCardPresentationBuilder::BuildCardViewData(
			CardSnapshot.Definition,
			BuildRuntimeContext(CardSnapshot));
	}

	FWacomCardDetailViewData BuildCardDetailViewData(const FHandCardSnapshot& CardSnapshot)
	{
		return UWacomCardPresentationBuilder::BuildCardDetailViewData(
			CardSnapshot.Definition,
			BuildRuntimeContext(CardSnapshot));
	}
}
