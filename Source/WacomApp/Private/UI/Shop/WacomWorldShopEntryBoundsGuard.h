// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AWacomWorldShopHostActor;
class UPrimitiveComponent;

/**
 * World Shop 活动期间临时释放正式商店入口 ClickBounds 的 Visibility 阻挡。
 *
 * 正式 Host 是组合式 Shop Actor 的 ChildActor；活动结束时恢复进入前的碰撞状态。
 */
class WACOMAPP_API FWacomWorldShopEntryBoundsGuard
{
public:
	~FWacomWorldShopEntryBoundsGuard();

	bool SuppressForHost(AWacomWorldShopHostActor* Host);
	void Restore();
	bool IsSuppressing() const { return EntryBounds.IsValid(); }

private:
	TWeakObjectPtr<UPrimitiveComponent> EntryBounds;
	ECollisionEnabled::Type PreviousCollisionEnabled = ECollisionEnabled::NoCollision;
};
