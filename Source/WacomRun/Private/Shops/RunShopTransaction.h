// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

class UCardDefinition;

/**
 * 商店访问、购买和卡牌强化事务的私有 helper。
 *
 * 只操作 FRunState，不广播、不访问 UI、不消耗行动点。
 * URunSession 负责将首次购买与行动点、地图活动合并为一次 working-state 事务。
 */
struct FRunShopTransaction
{
	using FAcquireCardCallback = TFunctionRef<bool(UCardDefinition*)>;

	static bool BeginVisit(FRunState& State, const FRunShopVisitRequest& Request);
	static bool BeginVisit(FRunState& State, FName ShopId, const TArray<FRunShopOfferInput>& Offers);
	static bool EndVisit(FRunState& State);
	static FRunShopSnapshot BuildSnapshot(const FRunState& State);

	static bool PurchaseOffer(FRunState& State, FGuid OfferId, FAcquireCardCallback AcquireCard);
	static bool PurchaseCard(FRunState& State, UCardDefinition* Card, int32 Price, FAcquireCardCallback AcquireCard);
	static FRunShopCardUpgradeResult UpgradeOwnedCard(
		FRunState& State,
		const FRunShopCardUpgradeCommand& Command);

private:
	static FRunShopState BuildShopStateFromRequest(const FRunShopVisitRequest& Request);
};
