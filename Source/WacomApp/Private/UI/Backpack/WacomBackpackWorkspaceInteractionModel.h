// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBackpackWorkspaceTypes.h"

/** 活动工作台中可参与命中/选择/携带的一张卡。 */
struct FWacomBackpackWorkspaceCardHitRecord
{
	FGuid InstanceId;
	FWacomBackpackZoneKey SourceZone;
	FVector2D CardCenter = FVector2D::ZeroVector;
	FVector2D CardSize = FVector2D::ZeroVector;
	float AngleDegrees = 0.0f;
	int32 LayerRank = 0;
	bool bMovable = true;

	FWacomBackpackWorkspaceCardHitRecord() = default;
	FWacomBackpackWorkspaceCardHitRecord(
		FGuid InInstanceId,
		FVector2D InCardCenter,
		int32 InLayerRank,
		bool bInMovable)
		: InstanceId(InInstanceId)
		, CardCenter(InCardCenter)
		, LayerRank(InLayerRank)
		, bMovable(bInMovable)
	{
	}
	FWacomBackpackWorkspaceCardHitRecord(
		FGuid InInstanceId,
		const FWacomBackpackZoneKey& InSourceZone,
		FVector2D InCardCenter,
		int32 InLayerRank,
		bool bInMovable)
		: InstanceId(InInstanceId)
		, SourceZone(InSourceZone)
		, CardCenter(InCardCenter)
		, LayerRank(InLayerRank)
		, bMovable(bInMovable)
	{
	}
};

/** 释放目标来源。Pointer 保持传统精确坐标；其余值是无鼠标语义目标。 */
enum class EWacomBackpackWorkspaceReleaseTargetKind : uint8
{
	Pointer,
	Flux,
	Pile,
	Delete
};

/** 释放只产生意图；Screen 成功提交后再调用 CommitReleasedCards。 */
struct FWacomBackpackWorkspaceReleaseIntent
{
	TArray<FGuid> InstanceIds;
	bool bConsumedByInitialReleaseGuard = false;
	bool bReleaseAll = false;
	EWacomBackpackWorkspaceReleaseTargetKind TargetKind =
		EWacomBackpackWorkspaceReleaseTargetKind::Pointer;
	FWacomBackpackZoneKey TargetZone;
};

/** 不依赖 Widget/Run 的背包选择、框选和持续携带状态机。 */
class WACOMAPP_API FWacomBackpackWorkspaceInteractionModel
{
public:
	void ReconcileCards(TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards);
	void ReconcileCards(
		const FWacomBackpackZoneKey& ActiveZone,
		TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards);
	/** 只更新既有显示身份的表现命中位置；不改变可移动性、选择或携带状态。 */
	void UpdateCardHitLayouts(TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards);

	void ClickCard(FGuid InstanceId, bool bControlDown);
	void ClickBlank();
	void BeginMarquee(FVector2D Start, bool bControlDown);
	void BeginMarquee(const FWacomBackpackZoneKey& SourceZone, FVector2D Start, bool bControlDown);
	void UpdateMarquee(FVector2D Current);
	void CompleteMarquee();
	void SelectAllMovable();
	void SelectAllMovable(const FWacomBackpackZoneKey& SourceZone);
	void SetCardPressActive(bool bActive);

	bool BeginPileMove(
		const FWacomBackpackZoneKey& Zone,
		FVector2D PointerStart,
		FVector2D PileStart);
	void UpdatePileMove(FVector2D PointerPosition);
	FWacomBackpackWorkspacePileMoveState CompletePileMove();

	bool BeginCarry(FGuid DraggedInstanceId, FVector2D PointerPosition, uint64 SourceStorageRevision);
	void UpdateCarryPointer(FVector2D PointerPosition);
	void StepCurrentByWheel(float WheelDelta);
	/** 新的释放手势已经按下；即使起手 PointerUp 丢失，也不能再吞掉这次手势的释放。 */
	void NotifyReleaseGestureStarted();
	FWacomBackpackWorkspaceReleaseIntent BuildReleaseIntent(
		bool bReleaseAll,
		EWacomBackpackWorkspaceReleaseTargetKind TargetKind =
			EWacomBackpackWorkspaceReleaseTargetKind::Pointer,
		const FWacomBackpackZoneKey& TargetZone = FWacomBackpackZoneKey());
	void CommitReleasedCards(TConstArrayView<FGuid> ReleasedInstanceIds);
	void UpdateCarrySourceStorageRevision(uint64 SourceStorageRevision);
	void SetCarryInputSuspended(bool bSuspended);
	void CancelTransientState();
	void RestoreCarry(const FWacomBackpackWorkspaceCarryState& CarrySnapshot);

	const FWacomBackpackWorkspaceSelectionState& GetSelection() const { return Selection; }
	const FWacomBackpackWorkspaceCarryState& GetCarry() const { return Carry; }
	bool IsCarrying() const { return !Carry.RemainingInstanceIds.IsEmpty(); }
	bool IsMarqueeActive() const { return Selection.bMarqueeActive; }
	bool IsMouseCaptured() const { return bMouseCaptured; }
	bool IsPileMoving() const { return PileMove.bActive; }
	EWacomBackpackWorkspaceInteractionMode GetMode() const { return Mode; }
	const FWacomBackpackWorkspacePileMoveState& GetPileMove() const { return PileMove; }
	bool IsSelected(FGuid InstanceId) const { return Selection.OrderedSelectedInstanceIds.Contains(InstanceId); }
	bool IsMovable(FGuid InstanceId) const;

private:
	TArray<FWacomBackpackWorkspaceCardHitRecord> AvailableCards;
	FWacomBackpackWorkspaceSelectionState Selection;
	FWacomBackpackWorkspaceCarryState Carry;
	FWacomBackpackWorkspacePileMoveState PileMove;
	EWacomBackpackWorkspaceInteractionMode Mode = EWacomBackpackWorkspaceInteractionMode::Idle;
	TArray<FGuid> MarqueeStartSelection;
	bool bMouseCaptured = false;

	const FWacomBackpackWorkspaceCardHitRecord* FindCard(FGuid InstanceId) const;
	void ReplaceSelection(TConstArrayView<FGuid> InstanceIds);
	void NormalizeSelection();
};
