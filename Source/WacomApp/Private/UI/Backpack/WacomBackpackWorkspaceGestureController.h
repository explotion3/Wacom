// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "WacomBackpackWorkspaceTypes.h"

struct FPointerEvent;
class UWacomBackpackZonePileWidget;

struct FWacomBackpackPendingCardPress
{
	FGuid InstanceId;
	FVector2D LocalPosition = FVector2D::ZeroVector;
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	bool bControlDown = false;
	bool bActive = false;

	void Reset() { *this = FWacomBackpackPendingCardPress(); }
};

struct FWacomBackpackPendingPilePress
{
	TWeakObjectPtr<UWacomBackpackZonePileWidget> Pile;
	FVector2D LocalPosition = FVector2D::ZeroVector;
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	FVector2D PileStartPosition = FVector2D::ZeroVector;
	bool bControlDown = false;
	bool bOnDragHandle = false;
	bool bActive = false;

	void Reset() { *this = FWacomBackpackPendingPilePress(); }
};

struct FWacomBackpackPileMoveVisualSnapshot
{
	TWeakObjectPtr<UWacomBackpackZonePileWidget> Pile;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid OwnerInstanceId;
	FVector2D CanvasPosition = FVector2D::ZeroVector;
	int32 ZOrder = 0;
	bool bValid = false;

	void Reset() { *this = FWacomBackpackPileMoveVisualSnapshot(); }
};

struct FWacomBackpackPendingMarqueePress
{
	FWacomBackpackZoneKey SourceZone;
	FVector2D LocalPosition = FVector2D::ZeroVector;
	FVector2D ScreenPosition = FVector2D::ZeroVector;
	bool bControlDown = false;
	bool bActive = false;

	void Reset() { *this = FWacomBackpackPendingMarqueePress(); }
};

/**
 * Workspace 的 Slate 手势状态。阈值只使用 FSlateApplication 的屏幕空间策略，
 * 不再把逻辑像素常量混入卡牌、牌堆或框选语义。
 */
class WACOMAPP_API FWacomBackpackWorkspaceGestureController
{
public:
	void BeginCardPress(
		FGuid InstanceId,
		FVector2D LocalPosition,
		FVector2D ScreenPosition,
		bool bControlDown);
	void BeginPilePress(
		UWacomBackpackZonePileWidget& Pile,
		FVector2D LocalPosition,
		FVector2D ScreenPosition,
		FVector2D PileStartPosition,
		bool bControlDown,
		bool bOnDragHandle);
	void BeginMarqueePress(
		const FWacomBackpackZoneKey& SourceZone,
		FVector2D LocalPosition,
		FVector2D ScreenPosition,
		bool bControlDown);
	bool HasCardDragThreshold(const FPointerEvent& Event) const;
	bool HasPileDragThreshold(const FPointerEvent& Event) const;
	bool HasMarqueeDragThreshold(const FPointerEvent& Event) const;
	bool HasPendingCardPress() const { return CardPress.bActive; }
	bool HasPendingPilePress() const { return PilePress.bActive; }
	bool HasPendingMarqueePress() const { return MarqueePress.bActive; }
	bool HasAnyPendingPress() const
	{
		return HasPendingCardPress()
			|| HasPendingPilePress()
			|| HasPendingMarqueePress();
	}
	bool HasPileMoveSnapshot() const { return PileMoveSnapshot.bValid; }

	const FWacomBackpackPendingCardPress& GetCardPress() const
	{
		return CardPress;
	}
	const FWacomBackpackPendingPilePress& GetPilePress() const
	{
		return PilePress;
	}
	const FWacomBackpackPendingMarqueePress& GetMarqueePress() const
	{
		return MarqueePress;
	}
	const FWacomBackpackPileMoveVisualSnapshot& GetPileMoveSnapshot() const
	{
		return PileMoveSnapshot;
	}

	void ClearCardPress() { CardPress.Reset(); }
	void ClearPilePress() { PilePress.Reset(); }
	void ClearMarqueePress() { MarqueePress.Reset(); }
	void SetPileMoveSnapshot(
		const FWacomBackpackPileMoveVisualSnapshot& Snapshot)
	{
		PileMoveSnapshot = Snapshot;
	}
	void ClearPileMoveSnapshot() { PileMoveSnapshot.Reset(); }
	void ResetPendingPresses();
	void Reset();

private:
	static bool HasDragThreshold(const FPointerEvent& Event, FVector2D ScreenOrigin);

	FWacomBackpackPendingCardPress CardPress;
	FWacomBackpackPendingPilePress PilePress;
	FWacomBackpackPendingMarqueePress MarqueePress;
	FWacomBackpackPileMoveVisualSnapshot PileMoveSnapshot;
};
