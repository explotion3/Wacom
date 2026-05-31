// Copyright Wacom. All Rights Reserved.

#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/ShapeComponent.h"
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

	UPrimitiveComponent* Primitive = ResolveVisualTargetComponent();
	if (!Primitive)
	{
		bProbePreviewActive = false;
		LastPreviewResult = TEXT("MissingVisualTarget");
		LogDebugState(TEXT("PreviewFailed"));
		return;
	}

	if (CachedVisualTarget.IsValid() && CachedVisualTarget.Get() != Primitive)
	{
		RestoreProbeVisualFeedbackIfNeeded();
	}

	bProbePreviewActive = true;
	BeginProbeVisualFeedback(Primitive);
	LogDebugState(TEXT("PreviewActive"));
}

void UWacomRunWorldInteractionTargetBridgeComponent::ClearProbePreview()
{
	if (!bProbePreviewActive)
	{
		return;
	}

	bProbePreviewActive = false;
	RestoreProbeVisualFeedbackIfNeeded();
	LastPreviewResult = TEXT("Cleared");
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
	const UPrimitiveComponent* VisualTarget = ResolveVisualTargetComponent();
	View.bHasVisualTarget = VisualTarget != nullptr;
	View.bHasRenderableVisualTarget = IsRenderableProbeVisualTarget(VisualTarget);
	View.VisualTargetName = VisualTarget ? VisualTarget->GetFName() : NAME_None;
	View.bProbePreviewActive = bProbePreviewActive;
	View.bProbeScaleSignalEnabled = bEnableProbeScaleSignal;
	View.bProbeCustomDepthSignalEnabled = bEnableProbeCustomDepthSignal;
	View.ProbeCustomDepthStencilValue = FMath::Clamp(ProbeCustomDepthStencilValue, 0, 255);
	View.LastConfigureResult = LastConfigureResult;
	View.LastPreviewResult = LastPreviewResult;
	return View;
}

FString UWacomRunWorldInteractionTargetBridgeComponent::GetRunWorldTargetDebugSummary() const
{
	const FWacomRunWorldInteractionTargetDebugView View = GetRunWorldTargetDebugView();
	return FString::Printf(
		TEXT("RunWorldInteractionTarget{Owner=%s StableId=%s RuntimeTargetId=%s TargetTag=%s HasTargetComponent=%s Configured=%s HasVisual=%s HasRenderableVisual=%s VisualTarget=%s PreviewActive=%s ScaleSignal=%s CustomDepthSignal=%s Stencil=%d LastConfigure=%s LastPreview=%s}"),
		*GetNameSafe(GetOwner()),
		*View.RunTargetStableId.ToString(),
		*View.RuntimeTargetId.ToString(EGuidFormats::DigitsWithHyphens),
		*View.TargetTag.ToString(),
		View.bHasInteractionTargetComponent ? TEXT("true") : TEXT("false"),
		View.bInteractionTargetConfigured ? TEXT("true") : TEXT("false"),
		View.bHasVisualTarget ? TEXT("true") : TEXT("false"),
		View.bHasRenderableVisualTarget ? TEXT("true") : TEXT("false"),
		*View.VisualTargetName.ToString(),
		View.bProbePreviewActive ? TEXT("true") : TEXT("false"),
		View.bProbeScaleSignalEnabled ? TEXT("true") : TEXT("false"),
		View.bProbeCustomDepthSignalEnabled ? TEXT("true") : TEXT("false"),
		View.ProbeCustomDepthStencilValue,
		*View.LastConfigureResult.ToString(),
		*View.LastPreviewResult.ToString());
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
	if (IsRenderableProbeVisualTarget(VisualTargetComponent))
	{
		return VisualTargetComponent;
	}

	if (UPrimitiveComponent* RenderableOwnerPrimitive = ResolveRenderableOwnerPrimitive())
	{
		return RenderableOwnerPrimitive;
	}

	if (VisualTargetComponent)
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UPrimitiveComponent>() : nullptr;
}

UPrimitiveComponent*
UWacomRunWorldInteractionTargetBridgeComponent::ResolveRenderableOwnerPrimitive() const
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	TArray<UPrimitiveComponent*> Components;
	Owner->GetComponents<UPrimitiveComponent>(Components);
	for (UPrimitiveComponent* Component : Components)
	{
		if (Component && Component != VisualTargetComponent && IsRenderableProbeVisualTarget(Component))
		{
			return Component;
		}
	}

	return nullptr;
}

bool UWacomRunWorldInteractionTargetBridgeComponent::IsRenderableProbeVisualTarget(
	const UPrimitiveComponent* Primitive) const
{
	return Primitive
		&& Primitive->IsVisible()
		&& !Primitive->IsA<UShapeComponent>();
}

void UWacomRunWorldInteractionTargetBridgeComponent::BeginProbeVisualFeedback(
	UPrimitiveComponent* Primitive)
{
	if (!Primitive)
	{
		LastPreviewResult = TEXT("MissingVisualTarget");
		return;
	}

	if (!bHasCachedBaseScale || CachedVisualTarget.Get() != Primitive)
	{
		CachedVisualTarget = Primitive;
		CachedBaseScale = Primitive->GetRelativeScale3D();
		bHasCachedBaseScale = true;
		bCachedVisualWasRenderable = IsRenderableProbeVisualTarget(Primitive);
	}

	if (!bHasCachedCustomDepth || CachedVisualTarget.Get() != Primitive)
	{
		CachedVisualTarget = Primitive;
		bCachedRenderCustomDepth = Primitive->bRenderCustomDepth;
		CachedCustomDepthStencilValue = Primitive->CustomDepthStencilValue;
		bHasCachedCustomDepth = true;
		bCachedVisualWasRenderable = IsRenderableProbeVisualTarget(Primitive);
	}

	if (bEnableProbeScaleSignal)
	{
		Primitive->SetRelativeScale3D(CachedBaseScale * FMath::Max(1.0f, ProbePreviewScale));
	}

	if (bEnableProbeCustomDepthSignal)
	{
		Primitive->SetRenderCustomDepth(true);
		Primitive->SetCustomDepthStencilValue(FMath::Clamp(ProbeCustomDepthStencilValue, 0, 255));
	}

	LastPreviewResult = bCachedVisualWasRenderable
		? FName(TEXT("Applied"))
		: FName(TEXT("AppliedNonRenderableFallback"));
}

void UWacomRunWorldInteractionTargetBridgeComponent::RestoreProbeVisualFeedbackIfNeeded()
{
	UPrimitiveComponent* Primitive = CachedVisualTarget.Get();
	if (!Primitive)
	{
		bHasCachedBaseScale = false;
		bHasCachedCustomDepth = false;
		return;
	}

	if (bHasCachedBaseScale)
	{
		Primitive->SetRelativeScale3D(CachedBaseScale);
	}

	if (bHasCachedCustomDepth)
	{
		Primitive->SetRenderCustomDepth(bCachedRenderCustomDepth);
		Primitive->SetCustomDepthStencilValue(CachedCustomDepthStencilValue);
	}

	bHasCachedBaseScale = false;
	bHasCachedCustomDepth = false;
	bCachedVisualWasRenderable = false;
	CachedVisualTarget.Reset();
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
