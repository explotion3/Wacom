// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

class UCardDefinition;
class URunSession;
class UWacomAppToastSubsystem;
class UWacomBackpackScreen;
class UWacomCardDragOperation;

class FWacomBackpackCommandFlow
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

private:
	static FText GetCardDisplayName(const UCardDefinition* Card);
	static UWacomAppToastSubsystem* GetToastSubsystem(const UObject* Context);
	static void ShowWarningToast(const UObject* Context, const FText& Message);
	static void ShowMoveFailureToast(UWacomAppToastSubsystem* ToastSubsystem, FName DisabledReason);
};
