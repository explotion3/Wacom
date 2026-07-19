// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class FWacomBattleHUDRuntime;
class UWacomBattleCombatLogDetailsScreen;

/** App-private owner for Battle GameMenu secondary panels and their command gate. */
class FWacomBattleSecondaryPanelCoordinator
	: public TSharedFromThis<FWacomBattleSecondaryPanelCoordinator>
{
public:
	explicit FWacomBattleSecondaryPanelCoordinator(FWacomBattleHUDRuntime& InRuntime);
	~FWacomBattleSecondaryPanelCoordinator();

	bool RequestOpenCombatLogDetails();
	void Shutdown(bool bResetBattlePreference);

	bool IsOpenOrPending() const { return bPushPending || ActiveScreen.IsValid(); }
	bool IsShowingCombatLogDetails() const { return bShowCombatLogDetails; }

private:
	void AttachScreen(UWacomBattleCombatLogDetailsScreen& Screen);
	void HandleScreenClosed();
	void HandleDetailsModeChanged(bool bShowDetails);
	void HandlePushCompleted(bool bSucceeded, FName FailureReason);
	void ReleaseCommandGate();
	bool IsCurrentRequest(int32 RequestGeneration) const;

	FWacomBattleHUDRuntime& Runtime;
	TWeakObjectPtr<UWacomBattleCombatLogDetailsScreen> ActiveScreen;
	int32 Generation = 0;
	bool bPushPending = false;
	bool bShowCombatLogDetails = false;
	bool bShuttingDown = false;
};
