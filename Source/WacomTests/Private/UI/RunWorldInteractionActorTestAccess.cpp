// Copyright Wacom. All Rights Reserved.

#include "UI/RunWorldInteractionActorTestAccess.h"

#if WITH_AUTOMATION_TESTS

#include "UI/WacomShopRunEventTestProbes.h"

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomRunEventTriggerClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomShopTriggerClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomBattleTriggerClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomRunPickupClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomRunCardPickupClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomRunRewardPickupClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomRunKeyChestClickProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::SyncClickTarget(
	AWacomGenericRunWorldClickableInteractableProbe* Probe)
{
	if (Probe)
	{
		Probe->SyncClickTargetForTest();
	}
}

void FWacomRunWorldInteractionActorTestAccess::EnsureRunSessionBinding(
	AWacomRunKeyChestClickProbe* Probe,
	AWacomPlayerController* PC)
{
	if (Probe)
	{
		Probe->EnsureRunSessionBindingForTest(PC);
	}
}

int32 FWacomRunWorldInteractionActorTestAccess::TryInteractCount(
	const AWacomRunEventTriggerClickProbe* Probe)
{
	return Probe ? Probe->TryInteractCountForTest : 0;
}

int32 FWacomRunWorldInteractionActorTestAccess::TryInteractCount(
	const AWacomShopTriggerClickProbe* Probe)
{
	return Probe ? Probe->TryInteractCountForTest : 0;
}

int32 FWacomRunWorldInteractionActorTestAccess::TryInteractCount(
	const AWacomBattleTriggerClickProbe* Probe)
{
	return Probe ? Probe->TryInteractCountForTest : 0;
}

int32 FWacomRunWorldInteractionActorTestAccess::TryInteractCount(
	const AWacomGenericRunWorldClickableInteractableProbe* Probe)
{
	return Probe ? Probe->TryInteractCountForTest : 0;
}

int32 FWacomRunWorldInteractionActorTestAccess::TryInteractCount(
	const AWacomRunWorldNonClickableInteractableProbe* Probe)
{
	return Probe ? Probe->TryInteractCountForTest : 0;
}

AWacomPlayerController*
FWacomRunWorldInteractionActorTestAccess::LastInteractingPlayerController(
	const AWacomRunEventTriggerClickProbe* Probe)
{
	return Probe ? Probe->GetLastInteractingPlayerControllerForTest() : nullptr;
}

AWacomPlayerController*
FWacomRunWorldInteractionActorTestAccess::LastInteractingPlayerController(
	const AWacomShopTriggerClickProbe* Probe)
{
	return Probe ? Probe->GetLastInteractingPlayerControllerForTest() : nullptr;
}

AWacomPlayerController*
FWacomRunWorldInteractionActorTestAccess::LastInteractingPlayerController(
	const AWacomBattleTriggerClickProbe* Probe)
{
	return Probe ? Probe->GetLastInteractingPlayerControllerForTest() : nullptr;
}

AWacomPlayerController*
FWacomRunWorldInteractionActorTestAccess::LastInteractingPlayerController(
	const AWacomGenericRunWorldClickableInteractableProbe* Probe)
{
	return Probe ? Probe->GetLastInteractingPlayerControllerForTest() : nullptr;
}

void FWacomRunWorldInteractionActorTestAccess::SetInteractResult(
	AWacomRunEventTriggerClickProbe* Probe,
	bool bResult)
{
	if (Probe)
	{
		Probe->bInteractResultForTest = bResult;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetInteractResult(
	AWacomShopTriggerClickProbe* Probe,
	bool bResult)
{
	if (Probe)
	{
		Probe->bInteractResultForTest = bResult;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetInteractResult(
	AWacomBattleTriggerClickProbe* Probe,
	bool bResult)
{
	if (Probe)
	{
		Probe->bInteractResultForTest = bResult;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetInteractResult(
	AWacomGenericRunWorldClickableInteractableProbe* Probe,
	bool bResult)
{
	if (Probe)
	{
		Probe->bInteractResultForTest = bResult;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetCanInteract(
	AWacomRunWorldNonClickableInteractableProbe* Probe,
	bool bCanInteract)
{
	if (Probe)
	{
		Probe->bCanInteractForTest = bCanInteract;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetCanInteract(
	AWacomGenericRunWorldClickableInteractableProbe* Probe,
	bool bCanInteract)
{
	if (Probe)
	{
		Probe->bCanInteractForTest = bCanInteract;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetInteractPrompt(
	AWacomRunWorldNonClickableInteractableProbe* Probe,
	const FText& Prompt)
{
	if (Probe)
	{
		Probe->PromptForTest = Prompt;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetInteractPrompt(
	AWacomGenericRunWorldClickableInteractableProbe* Probe,
	const FText& Prompt)
{
	if (Probe)
	{
		Probe->InteractPromptForTest = Prompt;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetGenericStableId(
	AWacomGenericRunWorldClickableInteractableProbe* Probe,
	FName StableId)
{
	if (Probe)
	{
		Probe->StableIdForTest = StableId;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetGenericHoverPrompt(
	AWacomGenericRunWorldClickableInteractableProbe* Probe,
	const FText& Prompt)
{
	if (Probe)
	{
		Probe->HoverPromptForTest = Prompt;
	}
}

void FWacomRunWorldInteractionActorTestAccess::SetGenericDebugResult(
	AWacomGenericRunWorldClickableInteractableProbe* Probe,
	FName DebugResult)
{
	if (Probe)
	{
		Probe->LastDebugResultForTest = DebugResult;
	}
}

#endif
