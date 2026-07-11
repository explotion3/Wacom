// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"

class UBattleSession;
class FWacomBattleHUDRuntime;
struct FBattleInitializationResult;
struct FBattleResolution;

struct FWacomBattlePlayCardCommitPresentation
{
	FGuid CardInstanceId;
	FBattlePartSlotIdentity TargetPartIdentity;
	TOptional<FVector2D> TargetWidgetPosition;
};

struct FWacomBattleCommandPresentationContext
{
	TWeakObjectPtr<UBattleSession> SourceSession;
	FWacomBattleCombatLogCommandContext CombatLogContext;
	FBattleSnapshot PreCommandSnapshot;
	TOptional<FWacomBattlePlayCardCommitPresentation> PlayCardCommit;
};

/** Single BattleHUD seam for applying initialization and command results. */
class FWacomBattleHUDResultApplicator
{
public:
	explicit FWacomBattleHUDResultApplicator(FWacomBattleHUDRuntime& InRuntime);

	void BeginBattleEntryPresentation();
	void AttachInitializedBattleSession(UBattleSession* Session, FBattleInitializationResult Initialization);
	void ReleaseBattleEntryPresentation();
	void ApplyCommandResolution(
		const FWacomBattleCommandPresentationContext& Context,
		const FBattleResolution& Resolution);
	void HandleSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession);

private:
	void CancelEntryPresentation();
	bool ValidateCommandResolution(
		UBattleSession* Session,
		const FWacomBattleCommandPresentationContext& Context,
		const FBattleResolution& Resolution) const;

	FWacomBattleHUDRuntime& Runtime;
	uint64 PresentationGeneration = 0;
	TWeakObjectPtr<UBattleSession> BoundSession;
	FBattleSnapshot PendingInitializationSnapshot;
	int32 LastAppliedStateVersion = INDEX_NONE;
	bool bEntryPresentationActive = false;
	bool bInitializationApplied = false;
	bool bBindingSessionInternally = false;
};
