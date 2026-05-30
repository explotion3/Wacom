// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "GameFramework/Actor.h"
#include "Tags/WacomGameplayTags.h"

UWacomRunWorldInteractionTargetBridgeComponent::UWacomRunWorldInteractionTargetBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomRunWorldInteractionTargetBridgeComponent::BeginPlay()
{
	Super::BeginPlay();
	RefreshRunWorldTargetBinding();
}

void UWacomRunWorldInteractionTargetBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearProbePreview();
	Super::EndPlay(EndPlayReason);
}

void UWacomRunWorldInteractionTargetBridgeComponent::SetRunTargetStableId(FName InStableId)
{
	RunTargetStableId = InStableId;
	RefreshRunWorldTargetBinding();
}

bool UWacomRunWorldInteractionTargetBridgeComponent::RefreshRunWorldTargetBinding()
{
	bInteractionTargetConfigured = false;

	if (!bAutoConfigureInteractionTarget)
	{
		LastConfigureResult = TEXT("AutoConfigureDisabled");
		LogDebugState(TEXT("ConfigureSkipped"));
		return false;
	}

	UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent();
	if (!TargetComponent)
	{
		LastConfigureResult = TEXT("MissingInteractionTargetComponent");
		LogDebugState(TEXT("ConfigureFailed"));
		return false;
	}

	RuntimeTargetId = TargetComponent->GetTargetId();
	if (!RuntimeTargetId.IsValid() && bAutoGenerateRuntimeTargetId)
	{
		RuntimeTargetId = FGuid::NewGuid();
	}

	if (!RuntimeTargetId.IsValid())
	{
		LastConfigureResult = TEXT("MissingRuntimeTargetId");
		LogDebugState(TEXT("ConfigureFailed"));
		return false;
	}

	TargetComponent->SetTargetId(RuntimeTargetId);
	TargetComponent->SetStableTargetId(RunTargetStableId);
	TargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Run_Object);
	bInteractionTargetConfigured = true;
	LastConfigureResult = TEXT("Configured");
	LogDebugState(TEXT("Configured"));
	return true;
}

void UWacomRunWorldInteractionTargetBridgeComponent::SetProbePreviewActive(bool bActive)
{
	if (!bActive)
	{
		ClearProbePreview();
		return;
	}

	bProbePreviewActive = true;
	BeginScaleFeedback(ProbePreviewScale);
	LogDebugState(TEXT("PreviewActive"));
}

void UWacomRunWorldInteractionTargetBridgeComponent::ClearProbePreview()
{
	if (!bProbePreviewActive)
	{
		return;
	}

	bProbePreviewActive = false;
	RestoreBaseScaleIfNeeded();
	LogDebugState(TEXT("PreviewCleared"));
}

FWacomRunWorldInteractionTargetDebugView
UWacomRunWorldInteractionTargetBridgeComponent::GetRunWorldTargetDebugView() const
{
	FWacomRunWorldInteractionTargetDebugView View;
	View.RunTargetStableId = RunTargetStableId;
	View.RuntimeTargetId = RuntimeTargetId;
	View.TargetTag = WacomTags::Interaction_Target_Run_Object;
	View.bHasInteractionTargetComponent = ResolveInteractionTargetComponent() != nullptr;
	View.bInteractionTargetConfigured = bInteractionTargetConfigured;
	View.bHasVisualTarget = ResolveVisualTargetComponent() != nullptr;
	View.bProbePreviewActive = bProbePreviewActive;
	View.LastConfigureResult = LastConfigureResult;
	return View;
}

FString UWacomRunWorldInteractionTargetBridgeComponent::GetRunWorldTargetDebugSummary() const
{
	const FWacomRunWorldInteractionTargetDebugView View = GetRunWorldTargetDebugView();
	return FString::Printf(
		TEXT("RunWorldInteractionTarget{Owner=%s StableId=%s RuntimeTargetId=%s TargetTag=%s HasTargetComponent=%s Configured=%s HasVisual=%s PreviewActive=%s LastConfigure=%s}"),
		*GetNameSafe(GetOwner()),
		*View.RunTargetStableId.ToString(),
		*View.RuntimeTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*View.TargetTag.ToString(),
		View.bHasInteractionTargetComponent ? TEXT("true") : TEXT("false"),
		View.bInteractionTargetConfigured ? TEXT("true") : TEXT("false"),
		View.bHasVisualTarget ? TEXT("true") : TEXT("false"),
		View.bProbePreviewActive ? TEXT("true") : TEXT("false"),
		*View.LastConfigureResult.ToString());
}

void UWacomRunWorldInteractionTargetBridgeComponent::LogRunWorldTargetDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomRunWorldInteractionTarget] %s"), *GetRunWorldTargetDebugSummary());
}

UWacomInteractionTargetComponent*
UWacomRunWorldInteractionTargetBridgeComponent::ResolveInteractionTargetComponent() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UWacomInteractionTargetComponent>() : nullptr;
}

UPrimitiveComponent*
UWacomRunWorldInteractionTargetBridgeComponent::ResolveVisualTargetComponent() const
{
	if (VisualTargetComponent)
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UPrimitiveComponent>() : nullptr;
}

void UWacomRunWorldInteractionTargetBridgeComponent::BeginScaleFeedback(float ScaleMultiplier)
{
	UPrimitiveComponent* Primitive = ResolveVisualTargetComponent();
	if (!Primitive)
	{
		return;
	}

	if (!bHasCachedBaseScale || CachedVisualTarget.Get() != Primitive)
	{
		CachedVisualTarget = Primitive;
		CachedBaseScale = Primitive->GetRelativeScale3D();
		bHasCachedBaseScale = true;
	}

	Primitive->SetRelativeScale3D(CachedBaseScale * FMath::Max(1.0f, ScaleMultiplier));
}

void UWacomRunWorldInteractionTargetBridgeComponent::RestoreBaseScaleIfNeeded()
{
	if (bHasCachedBaseScale)
	{
		if (UPrimitiveComponent* Primitive = CachedVisualTarget.Get())
		{
			Primitive->SetRelativeScale3D(CachedBaseScale);
		}
	}
}

void UWacomRunWorldInteractionTargetBridgeComponent::LogDebugState(const TCHAR* Prefix) const
{
	if (!bLogRunWorldTargetDebug)
	{
		return;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunWorldInteractionTarget] %s %s"),
		Prefix,
		*GetRunWorldTargetDebugSummary());
}
