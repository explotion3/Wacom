// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleHUDCommandBarPresenter.h"

#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleCommandBarWidget.h"
#include "UI/Battle/WacomBattleHUDPresentationCoordinator.h"
#include "UI/Battle/WacomBattleHUDRuntime.h"

#define LOCTEXT_NAMESPACE "WacomBattleHUDRuntime"

FWacomBattleHUDCommandBarPresenter::FWacomBattleHUDCommandBarPresenter(
	FWacomBattleHUDRuntime& InRuntime)
	: Runtime(InRuntime)
{
}

void FWacomBattleHUDCommandBarPresenter::RefreshFromSnapshot(
	const FBattleSnapshot& Snapshot)
{
	ApplyViewData(BuildViewData(&Snapshot));
}

void FWacomBattleHUDCommandBarPresenter::RefreshEmpty()
{
	ApplyViewData(BuildViewData(nullptr));
}

FWacomBattleCommandBarViewData FWacomBattleHUDCommandBarPresenter::BuildViewData(
	const FBattleSnapshot* Snapshot) const
{
	const bool bCanSubmit = Runtime.CanSubmitPlayerActionCommand();
	const EWacomBattleHUDTurnBoundaryCommand PendingCommand =
		Runtime.GetPresentationCoordinator().GetPendingTurnBoundaryCommand();

	FWacomBattleCommandBarViewData ViewData;
	ViewData.WaitValueText = Snapshot
		? FText::Format(
			LOCTEXT("BattleCommandBarWaitValueFormat", "Wait Value: {0}"),
			FText::AsNumber(Snapshot->CurrentWaitValue))
		: LOCTEXT("BattleCommandBarWaitValueUnavailable", "Wait Value: -");
	ViewData.PendingCommandText = Runtime.GetPendingTurnBoundaryCommandText();
	ViewData.Commands.Reserve(2);
	ViewData.Commands.Add(BuildCommandView(
		EWacomBattleCommandId::Wait,
		LOCTEXT("BattleCommandWaitLabel", "Wait"),
		LOCTEXT("BattleCommandWaitTooltip", "等待并推进战斗时间。"),
		10,
		bCanSubmit,
		PendingCommand == EWacomBattleHUDTurnBoundaryCommand::Wait,
		false));
	ViewData.Commands.Add(BuildCommandView(
		EWacomBattleCommandId::EndTurn,
		LOCTEXT("BattleCommandEndTurnLabel", "End Turn"),
		LOCTEXT("BattleCommandEndTurnTooltip", "结束当前玩家回合。"),
		20,
		bCanSubmit,
		PendingCommand == EWacomBattleHUDTurnBoundaryCommand::EndTurn,
		true));
	return ViewData;
}

FWacomBattleCommandButtonView FWacomBattleHUDCommandBarPresenter::BuildCommandView(
	EWacomBattleCommandId CommandId,
	const FText& DisplayText,
	const FText& ToolTipText,
	int32 SortOrder,
	bool bEnabled,
	bool bPending,
	bool bPrimary)
{
	FWacomBattleCommandButtonView View;
	View.CommandId = CommandId;
	View.DisplayText = DisplayText;
	View.ToolTipText = ToolTipText;
	View.SortOrder = SortOrder;
	View.bVisible = true;
	View.bEnabled = bEnabled;
	View.bPending = bPending;
	View.bPrimary = bPrimary;
	return View;
}

void FWacomBattleHUDCommandBarPresenter::ApplyViewData(
	const FWacomBattleCommandBarViewData& ViewData) const
{
	if (UBattleCommandBarWidget* CommandBar = Runtime.Host().GetCommandBar())
	{
		CommandBar->SetCommandBarViewData(ViewData);
	}
}

#undef LOCTEXT_NAMESPACE
