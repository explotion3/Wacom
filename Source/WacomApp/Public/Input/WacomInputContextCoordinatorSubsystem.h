// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/LocalPlayerSubsystem.h"
#include "WacomInputContextCoordinatorSubsystem.generated.h"

class APlayerController;
class UInputMappingContext;

UENUM(BlueprintType)
enum class EWacomInputFlowContext : uint8
{
	MainMenu,
	Exploration,
	Battle,
};

UENUM(BlueprintType)
enum class EWacomExplorationInputProfile : uint8
{
	FreeLook,
	RunTunnel,
};

/**
 * Local-player owner for Wacom gameplay input context.
 *
 * GameMode / PlayerController / prototype components declare intent here; this
 * subsystem applies CommonUI input config, cursor capture, mapping contexts,
 * and PC click/mouse-over event leases from one place.
 */
UCLASS()
class WACOMAPP_API UWacomInputContextCoordinatorSubsystem : public ULocalPlayerSubsystem
{
	GENERATED_BODY()

public:
	void InitializeForPlayerController(APlayerController* InPlayerController);

	void SetFlowContext(EWacomInputFlowContext NewContext);
	EWacomInputFlowContext GetFlowContext() const { return FlowContext; }

	void SetExplorationProfile(EWacomExplorationInputProfile NewProfile);
	EWacomExplorationInputProfile GetExplorationProfile() const { return ExplorationProfile; }

	void SetMappingContexts(UInputMappingContext* InExplorationMappingContext, UInputMappingContext* InBattleMappingContext);
	void ApplyCurrentInputContext();

	void AcquirePlayerControllerInteractionEvents(UObject* Owner, bool bClickEvents, bool bMouseOverEvents);
	void ReleasePlayerControllerInteractionEvents(UObject* Owner, bool bClickEvents, bool bMouseOverEvents);
	void ReleasePlayerControllerInteractionEvents(UObject* Owner);
	void ReleaseAllPlayerControllerInteractionEvents();

	bool IsClickEventsLeasedForTest() const { return InteractionLeases.Num() > 0 && ActiveClickLeaseCount > 0; }
	bool IsMouseOverEventsLeasedForTest() const { return InteractionLeases.Num() > 0 && ActiveMouseOverLeaseCount > 0; }
	bool ShouldShowMouseCursorForCurrentContextForTest() const;

private:
	struct FInteractionEventLease
	{
		TWeakObjectPtr<UObject> Owner;
		int32 ClickEventLeaseCount = 0;
		int32 MouseOverEventLeaseCount = 0;
	};

	UPROPERTY(Transient)
	TWeakObjectPtr<APlayerController> PlayerController;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> ExplorationMappingContext = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<UInputMappingContext> BattleMappingContext = nullptr;

	EWacomInputFlowContext FlowContext = EWacomInputFlowContext::Exploration;
	EWacomExplorationInputProfile ExplorationProfile = EWacomExplorationInputProfile::FreeLook;
	bool bApplyingInputContext = false;
	bool bExplorationMappingActive = false;
	bool bBattleMappingActive = false;

	TArray<FInteractionEventLease> InteractionLeases;
	bool bHasSavedInteractionEventState = false;
	bool bSavedClickEvents = false;
	bool bSavedMouseOverEvents = false;
	int32 ActiveClickLeaseCount = 0;
	int32 ActiveMouseOverLeaseCount = 0;

	APlayerController* ResolvePlayerController() const;
	void ApplyCommonUIInputConfig();
	void ApplyMappingContexts();
	void AddMappingContext(UInputMappingContext* MappingContext, int32 Priority);
	void RemoveMappingContext(UInputMappingContext* MappingContext);
	void RecomputeInteractionEventLeases();
};
