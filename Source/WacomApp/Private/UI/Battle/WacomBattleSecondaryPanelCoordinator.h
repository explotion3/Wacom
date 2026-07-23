// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "UI/Battle/WacomBattleCardPileDetailsTypes.h"

class FWacomBattleHUDRuntime;
class UCommonActivatableWidget;
class UWacomBattleCardPileDetailsScreen;
class UWacomBattleCombatLogDetailsScreen;
class UWacomBattleSecondaryPanelScreenBase;
class UWacomGameUIManagerSubsystem;

enum class EWacomBattleSecondaryPanelKind : uint8
{
	None,
	CombatLog,
	CardPile
};

/** App-private owner for Battle GameMenu secondary panels and their command gate. */
class FWacomBattleSecondaryPanelCoordinator
	: public TSharedFromThis<FWacomBattleSecondaryPanelCoordinator>
{
public:
	explicit FWacomBattleSecondaryPanelCoordinator(FWacomBattleHUDRuntime& InRuntime);
	~FWacomBattleSecondaryPanelCoordinator();

	bool RequestOpenCombatLogDetails();
	bool RequestOpenCardPileDetails(EWacomBattlePileDetailsTab InitialTab);
	void Shutdown(bool bResetBattlePreference);

	bool IsOpenOrPending() const { return bPushPending || ActiveScreen.IsValid(); }
	bool IsShowingCombatLogDetails() const { return bShowCombatLogDetails; }

private:
	bool BeginPush(EWacomBattleSecondaryPanelKind Kind, FGameplayTag WidgetTag, TSubclassOf<UCommonActivatableWidget> FallbackClass);
	bool AttachPushedScreen(UCommonActivatableWidget& Pushed, FName& OutFailureReason);
	void AttachCombatLogScreen(UWacomBattleCombatLogDetailsScreen& Screen);
	void AttachCardPileScreen(UWacomBattleCardPileDetailsScreen& Screen);
	void HandleScreenClosed();
	void HandleDetailsModeChanged(bool bShowDetails);
	void HandlePushCompleted(bool bSucceeded, FName FailureReason);
	void ReleaseCommandGate();
	void ReleaseViewportDepthLease();
	bool IsCurrentRequest(int32 RequestGeneration) const;

	FWacomBattleHUDRuntime& Runtime;
	TWeakObjectPtr<UWacomBattleSecondaryPanelScreenBase> ActiveScreen;
	FBattlePileInspectionSnapshot PendingPileSnapshot;
	EWacomBattlePileDetailsTab PendingPileTab = EWacomBattlePileDetailsTab::Draw;
	EWacomBattleSecondaryPanelKind PendingKind = EWacomBattleSecondaryPanelKind::None;
	TWeakObjectPtr<UWacomGameUIManagerSubsystem> ViewportDepthLeaseOwner;
	uint64 ViewportDepthLeaseId = 0;
	int32 Generation = 0;
	bool bPushPending = false;
	bool bShowCombatLogDetails = false;
	bool bCardPileHandHidden = false;
	bool bShuttingDown = false;
};
