// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattleCommandBarTypes.h"

class FWacomBattleHUDRuntime;
struct FBattleSnapshot;

class FWacomBattleHUDCommandBarPresenter
{
public:
	explicit FWacomBattleHUDCommandBarPresenter(FWacomBattleHUDRuntime& InRuntime);

	void RefreshFromSnapshot(const FBattleSnapshot& Snapshot);
	void RefreshEmpty();

private:
	FWacomBattleHUDRuntime& Runtime;

	FWacomBattleCommandBarViewData BuildViewData(const FBattleSnapshot* Snapshot) const;

	static FWacomBattleCommandButtonView BuildCommandView(
		EWacomBattleCommandId CommandId,
		const FText& DisplayText,
		const FText& ToolTipText,
		int32 SortOrder,
		bool bEnabled,
		bool bPending,
		bool bPrimary);

	void ApplyViewData(const FWacomBattleCommandBarViewData& ViewData) const;
};
