// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

class UCardDefinition;

/**
 * 商店访问和购买事务的私有 helper。
 *
 * 只操作 FRunState，不广播、不访问 UI、不直接消耗节点。
 * URunSession 仍负责 public API、节点消耗和 NotifyRunStateChanged。
 */
struct FRunShopTransaction
{
	using FAcquireCardCallback = TFunctionRef<bool(UCardDefinition*)>;

	static bool BeginVisit(FRunState& State, FName ShopId, const TArray<FRunShopOfferInput>& Offers);
	static bool EndVisit(FRunState& State);
	static FRunShopSnapshot BuildSnapshot(const FRunState& State);

	static bool PurchaseOffer(FRunState& State, FGuid OfferId, FAcquireCardCallback AcquireCard);
	static bool PurchaseCard(FRunState& State, UCardDefinition* Card, int32 Price, FAcquireCardCallback AcquireCard);

private:
	static FRunShopState BuildShopStateFromInputs(const TArray<FRunShopOfferInput>& Inputs);
};
