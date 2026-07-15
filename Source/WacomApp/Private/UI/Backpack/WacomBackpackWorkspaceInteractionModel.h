// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBackpackWorkspaceTypes.h"

/** 活动工作台中可参与命中/选择/携带的一张卡。 */
struct FWacomBackpackWorkspaceCardHitRecord
{
	FGuid InstanceId;
	FVector2D CardCenter = FVector2D::ZeroVector;
	int32 LayerRank = 0;
	bool bMovable = true;
};

/** 指针释放只产生意图；Screen 成功提交后再调用 CommitReleasedCards。 */
struct FWacomBackpackWorkspaceReleaseIntent
{
	TArray<FGuid> InstanceIds;
	bool bConsumedByInitialReleaseGuard = false;
	bool bReleaseAll = false;
};

/** 不依赖 Widget/Run 的背包选择、框选和持续携带状态机。 */
class WACOMAPP_API FWacomBackpackWorkspaceInteractionModel
{
public:
	void ReconcileCards(
		const FWacomBackpackZoneKey& ActiveZone,
		TConstArrayView<FWacomBackpackWorkspaceCardHitRecord> Cards);

	void ClickCard(FGuid InstanceId, bool bControlDown);
	void ClickBlank();
	void BeginMarquee(FVector2D Start, bool bControlDown);
	void UpdateMarquee(FVector2D Current);
	void CompleteMarquee();
	void SelectAllMovable();

	bool BeginCarry(FGuid DraggedInstanceId, FVector2D PointerPosition, uint64 SourceStorageRevision);
	void UpdateCarryPointer(FVector2D PointerPosition);
	void StepCurrentByWheel(float WheelDelta);
	/** 新的释放手势已经按下；即使起手 PointerUp 丢失，也不能再吞掉这次手势的释放。 */
	void NotifyReleaseGestureStarted();
	FWacomBackpackWorkspaceReleaseIntent BuildReleaseIntent(bool bReleaseAll);
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
	bool IsSelected(FGuid InstanceId) const { return Selection.OrderedSelectedInstanceIds.Contains(InstanceId); }
	bool IsMovable(FGuid InstanceId) const;

private:
	TArray<FWacomBackpackWorkspaceCardHitRecord> AvailableCards;
	FWacomBackpackZoneKey ActiveZone;
	FWacomBackpackWorkspaceSelectionState Selection;
	FWacomBackpackWorkspaceCarryState Carry;
	TArray<FGuid> MarqueeStartSelection;
	bool bMouseCaptured = false;

	const FWacomBackpackWorkspaceCardHitRecord* FindCard(FGuid InstanceId) const;
	void ReplaceSelection(TConstArrayView<FGuid> InstanceIds);
	void NormalizeSelection();
};
