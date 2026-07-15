// Copyright Wacom. All Rights Reserved.

#include "GameFramework/WacomRunWorldInteractionRouter.h"

#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/ActorComponent.h"
#include "Engine/HitResult.h"
#include "GameFramework/Actor.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomInteractionTargetHitResolver.h"
#include "Interaction/WacomWorldInteractableContractHelpers.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"

namespace
{
	FString GetRunWorldDebugObjectName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("None");
	}

	bool IsAcceptedRunWorldTargetHandle(const FWacomInteractionTargetHandle& Handle)
	{
		return Handle.IsValid()
			&& Handle.TargetKind == EWacomInteractionTargetKind::World
			&& Handle.TargetTag == WacomTags::Interaction_Target_Run_Object
			&& Handle.WorldTargetId.IsValid();
	}
}

FWacomRunWorldInteractionRouter::FWacomRunWorldInteractionRouter(
	AWacomPlayerController& InPlayerController)
	: PlayerController(InPlayerController)
{
}

bool FWacomRunWorldInteractionRouter::TryProbeSceneInteractionTarget(
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	if (!PlayerController.IsInExplorationFlow())
	{
		return false;
	}

	FHitResult HitResult;
	if (!PlayerController.BuildRunSceneClickHitResult(HitResult))
	{
		return false;
	}

	OutHandle = WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(HitResult);
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		FVector2D ScreenPosition = FVector2D::ZeroVector;
		if (PlayerController.ProjectWorldLocationToScreen(HitResult.Location, ScreenPosition))
		{
			const float ViewportScale =
				FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(&PlayerController));
			OutHandle.ScreenPosition = ScreenPosition / ViewportScale;
		}
	}

	const bool bAccepted = IsAcceptedRunWorldTargetHandle(OutHandle);
	if (!bAccepted)
	{
		OutHandle = FWacomInteractionTargetHandle();
	}
	return bAccepted;
}

bool FWacomRunWorldInteractionRouter::TryProbeSceneInteractionTargetAtWidgetPosition(
	const FVector2D& WidgetPosition,
	FWacomInteractionTargetHandle& OutHandle) const
{
	OutHandle = FWacomInteractionTargetHandle();

	if (!PlayerController.IsInExplorationFlow())
	{
		return false;
	}

	FHitResult HitResult;
	if (!PlayerController.BuildRunSceneInteractionTargetHitResultAtWidgetPosition(
		WidgetPosition,
		HitResult))
	{
		return false;
	}

	OutHandle = WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(HitResult);
	if (OutHandle.IsValid())
	{
		OutHandle.WorldLocation = HitResult.Location;
		OutHandle.ScreenPosition = WidgetPosition;
	}

	const bool bAccepted = IsAcceptedRunWorldTargetHandle(OutHandle);
	if (!bAccepted)
	{
		OutHandle = FWacomInteractionTargetHandle();
	}
	return bAccepted;
}

bool FWacomRunWorldInteractionRouter::TryRouteInteractableClick()
{
	if (!PlayerController.bEnableRunWorldInteractableClick)
	{
		return false;
	}
	if (!PlayerController.IsInExplorationFlow())
	{
		return false;
	}
	if (PlayerController.HasActiveRunGameMenuOrTransitionSuppression()
		|| PlayerController.ShouldHandleRunFirstPersonMenuDropProbe())
	{
		if (PlayerController.bLogRunWorldInteractableClick)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableClick] NoRoute reason=BlockedByMenuOrDrag"));
		}
		return false;
	}

	FWacomInteractionTargetHandle Handle;
	if (!TryProbeSceneInteractionTarget(Handle))
	{
		if (PlayerController.bLogRunWorldInteractableClick)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableClick] NoRoute reason=NoRunWorldTarget"));
		}
		return false;
	}

	AActor* TargetActor = nullptr;
	UWacomRunWorldInteractionTargetBridgeComponent* TargetBridge = nullptr;
	FName RejectReason = NAME_None;
	if (!ResolveClickableInteractableFromHandle(
		Handle,
		TargetActor,
		TargetBridge,
		RejectReason))
	{
		if (PlayerController.bLogRunWorldInteractableClick)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableClick] NoRoute reason=%s actor=%s bridge=%s handle=%s"),
				*RejectReason.ToString(),
				*GetRunWorldDebugObjectName(TargetActor),
				*GetRunWorldDebugObjectName(TargetBridge),
				*Handle.ToString());
		}
		return false;
	}

	const bool bRouted =
		WacomWorldInteractableContractHelpers::TryInteractWithActor(TargetActor, &PlayerController);
	if (PlayerController.bLogRunWorldInteractableClick)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[WacomRunWorldInteractableClick] Route actor=%s handle=%s result=%s"),
			*GetRunWorldDebugObjectName(TargetActor),
			*Handle.ToString(),
			bRouted ? TEXT("true") : TEXT("false"));
	}
	return bRouted;
}

void FWacomRunWorldInteractionRouter::UpdateTargetProbePreview()
{
	if (!PlayerController.IsInExplorationFlow())
	{
		ClearTargetProbePreview();
		ClearHoverPrompt(TEXT("NotInExploration"));
		return;
	}

	FWacomInteractionTargetHandle Handle;
	const bool bHasRunTarget = TryProbeSceneInteractionTarget(Handle);
	AActor* InteractableActor = nullptr;
	UWacomRunWorldInteractionTargetBridgeComponent* NewBridge = nullptr;
	FName ResolveRejectReason = bHasRunTarget
		? FName(TEXT("NotResolved"))
		: FName(TEXT("NoRunWorldTarget"));
	const bool bResolvedClickable = bHasRunTarget
		&& ResolveClickableInteractableFromHandle(
			Handle,
			InteractableActor,
			NewBridge,
			ResolveRejectReason);
	const bool bCanShowRunWorldHover = CanShowHoverPrompt();
	if (!PlayerController.bEnableRunWorldTargetProbePreview
		|| !bCanShowRunWorldHover
		|| !bResolvedClickable)
	{
		NewBridge = nullptr;
	}

	UWacomRunWorldInteractionTargetBridgeComponent* OldBridge = PreviewedRunWorldTargetBridge.Get();
	if (OldBridge != NewBridge)
	{
		if (OldBridge)
		{
			OldBridge->ClearProbePreview();
		}

		PreviewedRunWorldTargetBridge = NewBridge;
		if (NewBridge)
		{
			NewBridge->SetProbePreviewActive(true);
			if (PlayerController.bLogRunWorldTargetProbePreview)
			{
				UE_LOG(LogTemp, Display,
					TEXT("[WacomRunWorldTargetProbe] Preview handle=%s bridge=%s"),
					*Handle.ToString(),
					*GetRunWorldDebugObjectName(NewBridge));
			}
		}
		else if (PlayerController.bLogRunWorldTargetProbePreview)
		{
			UE_LOG(LogTemp, Display, TEXT("[WacomRunWorldTargetProbe] Preview cleared"));
		}
	}

	if (!PlayerController.bEnableRunWorldInteractableHoverPrompt)
	{
		ClearHoverPrompt(TEXT("Disabled"));
		return;
	}
	if (!bCanShowRunWorldHover)
	{
		ClearHoverPrompt(TEXT("BlockedByMenuOrDrag"));
		return;
	}
	if (!bHasRunTarget)
	{
		ClearHoverPrompt(TEXT("NoRunWorldTarget"));
		return;
	}

	if (!bResolvedClickable)
	{
		ClearHoverPrompt(ResolveRejectReason);
		return;
	}

	UpdateHoverPrompt(Handle, InteractableActor);
}

void FWacomRunWorldInteractionRouter::ClearTargetProbePreview()
{
	if (UWacomRunWorldInteractionTargetBridgeComponent* Bridge = PreviewedRunWorldTargetBridge.Get())
	{
		Bridge->ClearProbePreview();
	}
	PreviewedRunWorldTargetBridge.Reset();
}

bool FWacomRunWorldInteractionRouter::CanShowHoverPrompt() const
{
	if (!PlayerController.bEnableRunWorldInteractableHoverPrompt
		|| !PlayerController.IsInExplorationFlow())
	{
		return false;
	}

	return !PlayerController.HasActiveRunGameMenuOrTransitionSuppression()
		&& !PlayerController.ShouldHandleRunFirstPersonMenuDropProbe();
}

AActor* FWacomRunWorldInteractionRouter::ResolveSourceActorFromHandle(
	const FWacomInteractionTargetHandle& Handle) const
{
	UObject* SourceObject = Handle.SourceObject.Get();
	if (!SourceObject)
	{
		return nullptr;
	}

	if (const UActorComponent* SourceComponent = Cast<UActorComponent>(SourceObject))
	{
		return SourceComponent->GetOwner();
	}
	return Cast<AActor>(SourceObject);
}

bool FWacomRunWorldInteractionRouter::ResolveClickableInteractableFromHandle(
	const FWacomInteractionTargetHandle& Handle,
	AActor*& OutInteractableActor,
	UWacomRunWorldInteractionTargetBridgeComponent*& OutBridge,
	FName& OutRejectReason) const
{
	OutInteractableActor = nullptr;
	OutBridge = nullptr;
	OutRejectReason = NAME_None;

	if (!Handle.IsValid())
	{
		OutRejectReason = TEXT("InvalidHandle");
		return false;
	}
	if (Handle.TargetKind != EWacomInteractionTargetKind::World)
	{
		OutRejectReason = TEXT("WrongTargetKind");
		return false;
	}
	if (Handle.TargetTag != WacomTags::Interaction_Target_Run_Object)
	{
		OutRejectReason = TEXT("WrongTargetTag");
		return false;
	}
	if (!Handle.WorldTargetId.IsValid())
	{
		OutRejectReason = TEXT("MissingWorldTargetId");
		return false;
	}

	OutInteractableActor = ResolveSourceActorFromHandle(Handle);
	if (!OutInteractableActor)
	{
		OutRejectReason = TEXT("MissingSourceActor");
		return false;
	}
	if (!WacomWorldInteractableContractHelpers::IsAvailableAtBoundRunMapNode(
		OutInteractableActor,
		&PlayerController,
		&OutRejectReason))
	{
		return false;
	}

	OutBridge = ResolveTargetBridgeFromHandle(Handle);
	if (!WacomWorldInteractableContractHelpers::IsWorldInteractableActor(OutInteractableActor))
	{
		OutRejectReason = TEXT("MissingWorldInteractableContract");
		return false;
	}
	if (!WacomWorldInteractableContractHelpers::IsRunWorldClickableInteractableActor(OutInteractableActor))
	{
		OutRejectReason = TEXT("MissingClickableContract");
		return false;
	}
	if (!OutBridge)
	{
		OutRejectReason = TEXT("MissingRunWorldBridge");
		return false;
	}

	OutRejectReason = TEXT("Ok");
	return true;
}

UWacomRunWorldInteractionTargetBridgeComponent*
FWacomRunWorldInteractionRouter::ResolveTargetBridgeFromHandle(
	const FWacomInteractionTargetHandle& Handle) const
{
	UObject* SourceObject = Handle.SourceObject.Get();
	if (!SourceObject)
	{
		return nullptr;
	}

	if (UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		Cast<UWacomRunWorldInteractionTargetBridgeComponent>(SourceObject))
	{
		return Bridge;
	}

	if (const UActorComponent* SourceComponent = Cast<UActorComponent>(SourceObject))
	{
		AActor* SourceOwner = SourceComponent->GetOwner();
		return SourceOwner
			? SourceOwner->FindComponentByClass<UWacomRunWorldInteractionTargetBridgeComponent>()
			: nullptr;
	}

	if (const AActor* SourceActor = Cast<AActor>(SourceObject))
	{
		return SourceActor->FindComponentByClass<UWacomRunWorldInteractionTargetBridgeComponent>();
	}

	return nullptr;
}

void FWacomRunWorldInteractionRouter::UpdateHoverPrompt(
	const FWacomInteractionTargetHandle& Handle,
	AActor* InteractableActor)
{
	if (!InteractableActor)
	{
		ClearHoverPrompt(TEXT("MissingInteractableActor"));
		return;
	}

	if (!WacomWorldInteractableContractHelpers::IsRunWorldClickableInteractableActor(InteractableActor))
	{
		ClearHoverPrompt(TEXT("MissingClickableContract"));
		return;
	}

	const FText NewPrompt =
		WacomWorldInteractableContractHelpers::GetRunWorldClickHoverPromptFromActor(
			InteractableActor,
			&PlayerController);
	const FWacomRunWorldClickableInteractableDebugView TriggerDebug =
		WacomWorldInteractableContractHelpers::GetRunWorldClickableDebugViewFromActor(
			InteractableActor,
			&PlayerController);
	const FName NewReason =
		WacomWorldInteractableContractHelpers::BuildRunWorldClickableHoverReason(TriggerDebug);

	const bool bChanged =
		HoveredRunWorldInteractableActor.Get() != InteractableActor
		|| !HoveredRunWorldInteractablePrompt.EqualTo(NewPrompt)
		|| HoveredRunWorldInteractableHandle.StableTargetId != Handle.StableTargetId
		|| LastRunWorldInteractableHoverReason != NewReason;

	HoveredRunWorldInteractableActor = InteractableActor;
	HoveredRunWorldInteractableHandle = Handle;
	HoveredRunWorldInteractablePrompt = NewPrompt;
	LastRunWorldInteractableHoverReason = NewReason;

	if (bChanged)
	{
		PlayerController.RefreshInteractToast();
		if (PlayerController.bLogRunWorldInteractableHoverPrompt)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableHover] Hover %s"),
				*BuildHoverDebugSummary());
		}
	}
}

void FWacomRunWorldInteractionRouter::ClearHoverPrompt(FName Reason)
{
	const bool bHadHover =
		HoveredRunWorldInteractableActor.IsValid()
		|| !HoveredRunWorldInteractablePrompt.IsEmpty()
		|| HoveredRunWorldInteractableHandle.IsValid();
	HoveredRunWorldInteractableActor.Reset();
	HoveredRunWorldInteractableHandle = FWacomInteractionTargetHandle();
	HoveredRunWorldInteractablePrompt = FText::GetEmpty();
	LastRunWorldInteractableHoverReason = Reason.IsNone()
		? FName(TEXT("Cleared"))
		: Reason;

	if (bHadHover)
	{
		PlayerController.RefreshInteractToast();
		if (PlayerController.bLogRunWorldInteractableHoverPrompt)
		{
			UE_LOG(LogTemp, Display,
				TEXT("[WacomRunWorldInteractableHover] Cleared reason=%s"),
				*LastRunWorldInteractableHoverReason.ToString());
		}
	}
}

FString FWacomRunWorldInteractionRouter::BuildHoverDebugSummary() const
{
	AActor* HoverActor = HoveredRunWorldInteractableActor.Get();
	FWacomRunWorldClickableInteractableDebugView TriggerDebug;
	if (WacomWorldInteractableContractHelpers::IsRunWorldClickableInteractableActor(HoverActor))
	{
		TriggerDebug =
			WacomWorldInteractableContractHelpers::GetRunWorldClickableDebugViewFromActor(
				HoverActor,
				&PlayerController);
	}
	else
	{
		TriggerDebug.ActorName = GetRunWorldDebugObjectName(HoverActor);
		TriggerDebug.StableId = HoveredRunWorldInteractableHandle.StableTargetId;
		TriggerDebug.bHasStableId = !TriggerDebug.StableId.IsNone();
		TriggerDebug.bImplementsWorldInteractable =
			WacomWorldInteractableContractHelpers::IsWorldInteractableActor(HoverActor);
		TriggerDebug.bImplementsClickableContract =
			WacomWorldInteractableContractHelpers::IsRunWorldClickableInteractableActor(HoverActor);
		TriggerDebug.HoverPrompt = HoveredRunWorldInteractablePrompt.ToString();
		TriggerDebug.RejectReason = LastRunWorldInteractableHoverReason;
		TriggerDebug.LastDebugResult = LastRunWorldInteractableHoverReason;
	}

	return FString::Printf(
		TEXT("RunWorldInteractableHover{Actor=%s StableId=%s Prompt=%s CanInteract=%s Completed=%s Reason=%s Target=%s Debug=%s}"),
		*GetRunWorldDebugObjectName(HoverActor),
		*HoveredRunWorldInteractableHandle.StableTargetId.ToString(),
		*HoveredRunWorldInteractablePrompt.ToString(),
		TriggerDebug.bCanInteract ? TEXT("true") : TEXT("false"),
		TriggerDebug.bIsCompleted ? TEXT("true") : TEXT("false"),
		*LastRunWorldInteractableHoverReason.ToString(),
		*HoveredRunWorldInteractableHandle.ToString(),
		*FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(TriggerDebug));
}

FText FWacomRunWorldInteractionRouter::GetHoverPrompt() const
{
	return HoveredRunWorldInteractableActor.IsValid()
		? HoveredRunWorldInteractablePrompt
		: FText::GetEmpty();
}
