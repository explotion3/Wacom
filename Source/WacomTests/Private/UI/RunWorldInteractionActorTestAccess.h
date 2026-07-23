// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#if WITH_AUTOMATION_TESTS

class AWacomGenericRunWorldClickableInteractableProbe;
class AWacomPlayerController;
class AWacomRunCardPickupClickProbe;
class AWacomRunEventTriggerClickProbe;
class AWacomRunKeyChestClickProbe;
class AWacomRunPickupClickProbe;
class AWacomRunRewardPickupClickProbe;
class AWacomRunWorldNonClickableInteractableProbe;
class AWacomShopTriggerClickProbe;

struct FWacomRunWorldInteractionActorTestAccess
{
	static void SyncClickTarget(AWacomRunEventTriggerClickProbe* Probe);
	static void SyncClickTarget(AWacomShopTriggerClickProbe* Probe);
	static void SyncClickTarget(AWacomRunPickupClickProbe* Probe);
	static void SyncClickTarget(AWacomRunCardPickupClickProbe* Probe);
	static void SyncClickTarget(AWacomRunRewardPickupClickProbe* Probe);
	static void SyncClickTarget(AWacomRunKeyChestClickProbe* Probe);
	static void SyncClickTarget(AWacomGenericRunWorldClickableInteractableProbe* Probe);

	static void EnsureRunSessionBinding(
		AWacomRunKeyChestClickProbe* Probe,
		AWacomPlayerController* PC);

	static int32 TryInteractCount(const AWacomRunEventTriggerClickProbe* Probe);
	static int32 TryInteractCount(const AWacomShopTriggerClickProbe* Probe);
	static int32 TryInteractCount(const AWacomGenericRunWorldClickableInteractableProbe* Probe);
	static int32 TryInteractCount(const AWacomRunWorldNonClickableInteractableProbe* Probe);

	static AWacomPlayerController* LastInteractingPlayerController(
		const AWacomRunEventTriggerClickProbe* Probe);
	static AWacomPlayerController* LastInteractingPlayerController(
		const AWacomShopTriggerClickProbe* Probe);
	static AWacomPlayerController* LastInteractingPlayerController(
		const AWacomGenericRunWorldClickableInteractableProbe* Probe);

	static void SetInteractResult(AWacomRunEventTriggerClickProbe* Probe, bool bResult);
	static void SetInteractResult(AWacomShopTriggerClickProbe* Probe, bool bResult);
	static void SetInteractResult(AWacomGenericRunWorldClickableInteractableProbe* Probe, bool bResult);

	static void SetCanInteract(AWacomRunWorldNonClickableInteractableProbe* Probe, bool bCanInteract);
	static void SetCanInteract(AWacomGenericRunWorldClickableInteractableProbe* Probe, bool bCanInteract);
	static void SetInteractPrompt(
		AWacomRunWorldNonClickableInteractableProbe* Probe,
		const FText& Prompt);
	static void SetInteractPrompt(
		AWacomGenericRunWorldClickableInteractableProbe* Probe,
		const FText& Prompt);
	static void SetGenericStableId(
		AWacomGenericRunWorldClickableInteractableProbe* Probe,
		FName StableId);
	static void SetGenericHoverPrompt(
		AWacomGenericRunWorldClickableInteractableProbe* Probe,
		const FText& Prompt);
	static void SetGenericDebugResult(
		AWacomGenericRunWorldClickableInteractableProbe* Probe,
		FName DebugResult);
};

#endif
