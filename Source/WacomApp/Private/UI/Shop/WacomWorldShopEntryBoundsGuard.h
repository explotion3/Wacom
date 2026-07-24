// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class AActor;
class UPrimitiveComponent;

/**
 * World Shop 活动期间临时释放正式商店入口 ClickBounds 的 Visibility 阻挡。
 *
 * 正式 Host Owner 就是组合式 Shop Actor；兼容旧 Host ChildActor，并在活动结束
 * 时恢复进入前的碰撞状态。
 */
class WACOMAPP_API FWacomWorldShopEntryBoundsGuard
{
public:
	~FWacomWorldShopEntryBoundsGuard();

	bool SuppressForHost(AActor* HostOwner);
	void Restore();
	bool IsSuppressing() const { return EntryBounds.IsValid(); }

private:
	TWeakObjectPtr<UPrimitiveComponent> EntryBounds;
	ECollisionEnabled::Type PreviousCollisionEnabled = ECollisionEnabled::NoCollision;
};
