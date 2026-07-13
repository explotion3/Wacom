// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"
#include "Deck/RunDeckBatchTypes.h"
#include "WacomBackpackWorkspaceTypes.h"

class UCardDefinition;
class URunSession;
class UWacomAppToastSubsystem;
class UWacomBackpackScreen;
class UWacomCardDragOperation;
struct FWacomBackpackWorkspaceStateStore;

class WACOMAPP_API FWacomBackpackCommandFlow
{
public:
	static FRunDeckOperationValidation ValidateZoneDropPreview(
		URunSession* Run,
		const UWacomCardDragOperation& CardOp,
		EZoneKind TargetZone,
		FGuid TargetZoneOwnerInstanceId);

	static FRunDeckOperationValidation ValidateDeleteDropPreview(
		URunSession* Run,
		const UWacomCardDragOperation& CardOp);

	static FGuid ResolveDeleteRequestInstanceId(const UWacomCardDragOperation& CardOp);
	static FText BuildMoveZoneNameText(EZoneKind Zone);
	static FText BuildMoveFailureToastText(FName DisabledReason);
	static FText BuildDeleteFailureToastText(FName DisabledReason);
	static FText BuildBattleEnabledFailureToastText(FName DisabledReason);

	static bool HandleZoneDropRequested(
		UWacomBackpackScreen& Screen,
		URunSession* Run,
		const UWacomCardDragOperation& CardOp,
		EZoneKind TargetZone,
		FGuid TargetZoneOwnerInstanceId);

	static bool HandleDeleteDropRequested(
		UWacomBackpackScreen& Screen,
		URunSession* Run,
		const UWacomCardDragOperation& CardOp);

	static bool HandleBattleEnabledToggle(
		UWacomBackpackScreen& Screen,
		URunSession* Run,
		FGuid InstanceId);

	/** 纯布局整理：不调用 Run move API，不改变卡牌物理顺序或 storage revision。 */
	static void ArrangeAll(
		FWacomBackpackWorkspaceStateStore& StateStore,
		const FWacomBackpackZoneKey& ZoneKey);

	/** 同区放回牌匣等价于清除这些卡的手动布局；跨区意图必须走 Run batch command。 */
	static bool CollectSameZone(
		FWacomBackpackWorkspaceStateStore& StateStore,
		const FWacomBackpackZoneKey& SourceZone,
		const FWacomBackpackZoneKey& TargetZone,
		TConstArrayView<FGuid> InstanceIds);

	static FRunDeckBatchMoveRequest BuildBatchMoveRequest(
		const FWacomBackpackWorkspaceCarryState& Carry,
		const FWacomBackpackZoneKey& TargetZone,
		TConstArrayView<FGuid> InstanceIds);

	static FRunDeckBatchOperationResult SubmitBatchMove(
		UWacomBackpackScreen& Screen,
		URunSession* Run,
		const FRunDeckBatchMoveRequest& Request);

	static FRunDeckBatchDeleteRequest BuildBatchDeleteRequest(
		const FWacomBackpackWorkspaceCarryState& Carry,
		TConstArrayView<FGuid> InstanceIds);
	static FRunDeckBatchDeletePreview PreviewBatchDelete(
		URunSession* Run,
		const FRunDeckBatchDeleteRequest& Request);
	static FRunDeckBatchOperationResult SubmitBatchDelete(
		UWacomBackpackScreen& Screen,
		URunSession* Run,
		const FRunDeckBatchDeleteRequest& Request);

private:
	static FText GetCardDisplayName(const UCardDefinition* Card);
	static UWacomAppToastSubsystem* GetToastSubsystem(const UObject* Context);
	static void ShowWarningToast(const UObject* Context, const FText& Message);
	static void ShowMoveFailureToast(UWacomAppToastSubsystem* ToastSubsystem, FName DisabledReason);
};
