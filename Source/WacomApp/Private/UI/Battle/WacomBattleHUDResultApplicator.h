// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Runtime/BattlePartSlotIdentity.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"

class UBattleSession;
class FWacomBattleHUDRuntime;
class FWacomFirstPersonCardPresentationPrewarmController;
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
	~FWacomBattleHUDResultApplicator();

	void BeginBattleEntryPresentation();
	void AttachInitializedBattleSession(UBattleSession* Session, FBattleInitializationResult Initialization);
	void ReleaseBattleEntryPresentation();
	void Tick(float DeltaTime);
	void ApplyCommandResolution(
		const FWacomBattleCommandPresentationContext& Context,
		const FBattleResolution& Resolution);
	void HandleSessionChanged(UBattleSession* OldSession, UBattleSession* NewSession);
	int32 GetCardPresentationPrewarmState() const;
	uint32 GetCardPresentationPrewarmGeneration() const;
	int32 GetCardPresentationRequiredAssetCount() const;
	int32 GetCardPresentationOptionalAssetCount() const;
	float GetCardPresentationPrewarmElapsedSeconds() const;
	bool IsEntryWaitingForCamera() const;
	bool IsEntryWaitingForCardPresentationPrewarm() const;

private:
	void CancelEntryPresentation();
	void BeginCardPresentationPrewarm();
	void TryReleaseBattleEntryPresentation();
	void ResetCardPresentationPrewarm();
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
	bool bInitialTurnActivityPresented = false;
	bool bBindingSessionInternally = false;
	bool bCameraStageReady = false;
	bool bPrewarmGateReady = false;
	uint32 PrewarmGeneration = 0;
	TUniquePtr<FWacomFirstPersonCardPresentationPrewarmController> CardPresentationPrewarm;
};
