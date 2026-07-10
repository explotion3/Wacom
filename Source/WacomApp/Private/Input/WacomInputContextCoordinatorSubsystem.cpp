// Copyright Wacom. All Rights Reserved.

#include "Input/WacomInputContextCoordinatorSubsystem.h"

#include "EnhancedInputSubsystems.h"
#include "GameFramework/PlayerController.h"
#include "Input/CommonUIActionRouterBase.h"
#include "Input/CommonUIInputTypes.h"

namespace
{
	FUIInputConfig BuildInputConfig(EWacomInputFlowContext FlowContext)
	{
		switch (FlowContext)
		{
		case EWacomInputFlowContext::MainMenu:
			return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
		case EWacomInputFlowContext::Battle:
			return FUIInputConfig(ECommonInputMode::All, EMouseCaptureMode::NoCapture);
		case EWacomInputFlowContext::Exploration:
		default:
			return FUIInputConfig(
				ECommonInputMode::All,
				EMouseCaptureMode::NoCapture,
				/*bInHideCursorDuringViewportCapture*/ false);
		}
	}
}

void UWacomInputContextCoordinatorSubsystem::InitializeForPlayerController(APlayerController* InPlayerController)
{
	PlayerController = InPlayerController;
	ApplyCurrentInputContext();
	RecomputeInteractionEventLeases();
}

void UWacomInputContextCoordinatorSubsystem::SetFlowContext(EWacomInputFlowContext NewContext)
{
	if (FlowContext == NewContext)
	{
		return;
	}

	FlowContext = NewContext;
	ApplyCurrentInputContext();
}

void UWacomInputContextCoordinatorSubsystem::SetMappingContexts(
	UInputMappingContext* InExplorationMappingContext,
	UInputMappingContext* InBattleMappingContext)
{
	const bool bExplorationMappingChanged = ExplorationMappingContext != InExplorationMappingContext;
	const bool bBattleMappingChanged = BattleMappingContext != InBattleMappingContext;

	if (bExplorationMappingChanged && bExplorationMappingActive)
	{
		RemoveMappingContext(ExplorationMappingContext);
		bExplorationMappingActive = false;
	}
	if (bBattleMappingChanged && bBattleMappingActive)
	{
		RemoveMappingContext(BattleMappingContext);
		bBattleMappingActive = false;
	}

	ExplorationMappingContext = InExplorationMappingContext;
	BattleMappingContext = InBattleMappingContext;
	ApplyMappingContexts();
}

void UWacomInputContextCoordinatorSubsystem::ApplyCurrentInputContext()
{
	if (bApplyingInputContext)
	{
		return;
	}

	TGuardValue<bool> Guard(bApplyingInputContext, true);
	ApplyCommonUIInputConfig();
	ApplyMappingContexts();
}

void UWacomInputContextCoordinatorSubsystem::AcquirePlayerControllerInteractionEvents(
	UObject* Owner,
	bool bClickEvents,
	bool bMouseOverEvents)
{
	if (!Owner || (!bClickEvents && !bMouseOverEvents))
	{
		return;
	}

	for (FInteractionEventLease& Lease : InteractionLeases)
	{
		if (Lease.Owner.Get() == Owner)
		{
			if (bClickEvents)
			{
				++Lease.ClickEventLeaseCount;
			}
			if (bMouseOverEvents)
			{
				++Lease.MouseOverEventLeaseCount;
			}
			RecomputeInteractionEventLeases();
			return;
		}
	}

	FInteractionEventLease Lease;
	Lease.Owner = Owner;
	Lease.ClickEventLeaseCount = bClickEvents ? 1 : 0;
	Lease.MouseOverEventLeaseCount = bMouseOverEvents ? 1 : 0;
	InteractionLeases.Add(Lease);
	RecomputeInteractionEventLeases();
}

void UWacomInputContextCoordinatorSubsystem::ReleasePlayerControllerInteractionEvents(
	UObject* Owner,
	bool bClickEvents,
	bool bMouseOverEvents)
{
	if (!Owner || (!bClickEvents && !bMouseOverEvents))
	{
		return;
	}

	for (FInteractionEventLease& Lease : InteractionLeases)
	{
		if (Lease.Owner.Get() == Owner)
		{
			if (bClickEvents && Lease.ClickEventLeaseCount > 0)
			{
				--Lease.ClickEventLeaseCount;
			}
			if (bMouseOverEvents && Lease.MouseOverEventLeaseCount > 0)
			{
				--Lease.MouseOverEventLeaseCount;
			}
			break;
		}
	}

	InteractionLeases.RemoveAllSwap(
		[](const FInteractionEventLease& Lease)
		{
			return !Lease.Owner.IsValid()
				|| (Lease.ClickEventLeaseCount <= 0 && Lease.MouseOverEventLeaseCount <= 0);
		});
	RecomputeInteractionEventLeases();
}

void UWacomInputContextCoordinatorSubsystem::ReleasePlayerControllerInteractionEvents(UObject* Owner)
{
	if (!Owner)
	{
		return;
	}

	const int32 Removed = InteractionLeases.RemoveAllSwap(
		[Owner](const FInteractionEventLease& Lease)
		{
			return !Lease.Owner.IsValid() || Lease.Owner.Get() == Owner;
		});

	if (Removed > 0)
	{
		RecomputeInteractionEventLeases();
	}
}

void UWacomInputContextCoordinatorSubsystem::ReleaseAllPlayerControllerInteractionEvents()
{
	InteractionLeases.Reset();
	RecomputeInteractionEventLeases();
}

bool UWacomInputContextCoordinatorSubsystem::ShouldShowMouseCursorForCurrentContextForTest() const
{
	const FUIInputConfig Config = BuildInputConfig(FlowContext);
	return Config.GetMouseCaptureMode() == EMouseCaptureMode::NoCapture;
}

APlayerController* UWacomInputContextCoordinatorSubsystem::ResolvePlayerController() const
{
	if (APlayerController* PC = PlayerController.Get())
	{
		return PC;
	}

	const ULocalPlayer* LocalPlayer = GetLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetPlayerController(GetWorld()) : nullptr;
}

void UWacomInputContextCoordinatorSubsystem::ApplyCommonUIInputConfig()
{
	APlayerController* PC = ResolvePlayerController();
	if (!PC)
	{
		return;
	}

	const FUIInputConfig Config = BuildInputConfig(FlowContext);
	if (UCommonUIActionRouterBase* ActionRouter = ULocalPlayer::GetSubsystem<UCommonUIActionRouterBase>(GetLocalPlayer()))
	{
		ActionRouter->SetActiveUIInputConfig(Config, this);
	}
	else
	{
		PC->SetShowMouseCursor(Config.GetMouseCaptureMode() == EMouseCaptureMode::NoCapture);
	}
}

void UWacomInputContextCoordinatorSubsystem::ApplyMappingContexts()
{
	switch (FlowContext)
	{
	case EWacomInputFlowContext::MainMenu:
		if (bExplorationMappingActive)
		{
			RemoveMappingContext(ExplorationMappingContext);
			bExplorationMappingActive = false;
		}
		if (bBattleMappingActive)
		{
			RemoveMappingContext(BattleMappingContext);
			bBattleMappingActive = false;
		}
		break;

	case EWacomInputFlowContext::Battle:
		if (bExplorationMappingActive)
		{
			RemoveMappingContext(ExplorationMappingContext);
			bExplorationMappingActive = false;
		}
		if (!bBattleMappingActive)
		{
			AddMappingContext(BattleMappingContext, 1);
			bBattleMappingActive = BattleMappingContext != nullptr;
		}
		break;

	case EWacomInputFlowContext::Exploration:
	default:
		if (bBattleMappingActive)
		{
			RemoveMappingContext(BattleMappingContext);
			bBattleMappingActive = false;
		}
		if (!bExplorationMappingActive)
		{
			AddMappingContext(ExplorationMappingContext, 0);
			bExplorationMappingActive = ExplorationMappingContext != nullptr;
		}
		break;
	}
}

void UWacomInputContextCoordinatorSubsystem::AddMappingContext(UInputMappingContext* MappingContext, int32 Priority)
{
	if (!MappingContext)
	{
		return;
	}

#if WITH_AUTOMATION_TESTS
	if (MappingOperationObserverForTest)
	{
		MappingOperationObserverForTest(true, MappingContext, Priority);
	}
#endif

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->AddMappingContext(MappingContext, Priority);
	}
}

void UWacomInputContextCoordinatorSubsystem::RemoveMappingContext(UInputMappingContext* MappingContext)
{
	if (!MappingContext)
	{
		return;
	}

#if WITH_AUTOMATION_TESTS
	if (MappingOperationObserverForTest)
	{
		MappingOperationObserverForTest(false, MappingContext, INDEX_NONE);
	}
#endif

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
		ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		Subsystem->RemoveMappingContext(MappingContext);
	}
}

void UWacomInputContextCoordinatorSubsystem::RecomputeInteractionEventLeases()
{
	APlayerController* PC = ResolvePlayerController();
	if (!PC)
	{
		return;
	}

	InteractionLeases.RemoveAllSwap(
		[](const FInteractionEventLease& Lease)
		{
			return !Lease.Owner.IsValid();
		});

	bool bNeedsClickEvents = false;
	bool bNeedsMouseOverEvents = false;
	for (const FInteractionEventLease& Lease : InteractionLeases)
	{
		bNeedsClickEvents |= Lease.ClickEventLeaseCount > 0;
		bNeedsMouseOverEvents |= Lease.MouseOverEventLeaseCount > 0;
	}

	if (!bHasSavedInteractionEventState && (bNeedsClickEvents || bNeedsMouseOverEvents))
	{
		bSavedClickEvents = PC->bEnableClickEvents;
		bSavedMouseOverEvents = PC->bEnableMouseOverEvents;
		bHasSavedInteractionEventState = true;
	}

	ActiveClickLeaseCount = bNeedsClickEvents ? 1 : 0;
	ActiveMouseOverLeaseCount = bNeedsMouseOverEvents ? 1 : 0;

	if (bHasSavedInteractionEventState)
	{
		PC->bEnableClickEvents = bNeedsClickEvents || bSavedClickEvents;
		PC->bEnableMouseOverEvents = bNeedsMouseOverEvents || bSavedMouseOverEvents;

		if (!bNeedsClickEvents && !bNeedsMouseOverEvents)
		{
			PC->bEnableClickEvents = bSavedClickEvents;
			PC->bEnableMouseOverEvents = bSavedMouseOverEvents;
			bHasSavedInteractionEventState = false;
			bSavedClickEvents = false;
			bSavedMouseOverEvents = false;
			ActiveClickLeaseCount = 0;
			ActiveMouseOverLeaseCount = 0;
		}
	}
}
