// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Enemies/EnemyPartDefinition.h"
#include "GameFramework/Actor.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"

UWacomBattleEnemyPartWorldTargetBridgeComponent::UWacomBattleEnemyPartWorldTargetBridgeComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::SetPartId(FName InPartId)
{
	PartId = InPartId;
}

bool UWacomBattleEnemyPartWorldTargetBridgeComponent::SyncFromBattleHUD(
	UBattleHUD& HUD,
	const FBattleSnapshot& Snapshot,
	const FBattleTargetSelectionView& TargetSelectionView)
{
	if (PartId.IsNone())
	{
		LastBindResult = TEXT("MissingPartId");
		ClearBattleBinding();
		return false;
	}

	const FEnemyPartSnapshot* MatchedPart = nullptr;
	for (const FEnemyPartSnapshot& Part : Snapshot.Enemy.Parts)
	{
		if (Part.Definition && Part.Definition->PartId == PartId)
		{
			MatchedPart = &Part;
			break;
		}
	}

	if (!MatchedPart || !MatchedPart->InstanceId.IsValid() || MatchedPart->bDestroyed)
	{
		LastBindResult = MatchedPart && MatchedPart->bDestroyed ? TEXT("PartDestroyed") : TEXT("NoMatchingPart");
		ClearBattleBinding();
		return false;
	}

	PartInstanceId = MatchedPart->InstanceId;
	bBoundToSnapshot = true;
	LastBindResult = TEXT("MatchedPartId");

	bool bNewTargetable = false;
	FName NewDisabledReason = NAME_None;
	for (const FBattleTargetablePartView& PartView : TargetSelectionView.TargetableParts)
	{
		if (PartView.PartInstanceId == PartInstanceId)
		{
			bNewTargetable = PartView.bTargetable;
			NewDisabledReason = PartView.DisabledReason;
			break;
		}
	}
	TargetDisabledReason = NewDisabledReason;
	ApplyTargetableAffordance(bNewTargetable);
	UpdateInteractionTargetComponent();
	RegisterWithBattleHUD(HUD);
	return true;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearBattleBinding()
{
	UnregisterFromBattleHUD();
	PartInstanceId.Invalidate();
	bBoundToSnapshot = false;
	TargetDisabledReason = NAME_None;
	ApplyTargetableAffordance(false);

	if (UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent())
	{
		TargetComponent->SetTargetId(FGuid());
	}
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::PlayBattlePresentationCue(
	const FWacomBattlePresentationTargetCue& Cue)
{
	if (Cue.CueKind == EWacomBattlePresentationTargetCueKind::BattleEvent
		&& Cue.SourceEventType != EBattleEventType::DamageDealt
		&& Cue.SourceEventType != EBattleEventType::EnemyPartHpEmptied)
	{
		return;
	}

	LastCueKind = WacomBattlePresentationTargetCueKindToName(Cue.CueKind);
	LastCueType = Cue.SourceEventType;
	LastCueAmount = Cue.Amount;
	++CuePlayCount;

	float ScaleMultiplier = DamagePulseScale;
	if (Cue.CueKind == EWacomBattlePresentationTargetCueKind::TargetConfirmed)
	{
		ScaleMultiplier = TargetConfirmPulseScale;
	}
	else if (Cue.SourceEventType == EBattleEventType::EnemyPartHpEmptied)
	{
		ScaleMultiplier = DestroyedPulseScale;
	}

	BeginScaleFeedback(ScaleMultiplier, Cue.Duration > 0.0f ? Cue.Duration : CueHoldSeconds);
}

FWacomBattleEnemyPartWorldTargetDebugView
UWacomBattleEnemyPartWorldTargetBridgeComponent::GetBattleWorldTargetDebugView() const
{
	FWacomBattleEnemyPartWorldTargetDebugView View;
	View.PartId = PartId;
	View.PartInstanceId = PartInstanceId;
	View.bBoundToSnapshot = bBoundToSnapshot;
	View.bRegisteredWithBattleHUD = bRegisteredWithBattleHUD;
	View.bTargetable = bTargetable;
	View.TargetDisabledReason = TargetDisabledReason;
	View.LastBindResult = LastBindResult;
	View.LastCueKind = LastCueKind;
	View.LastCueType = LastCueType;
	View.LastCueAmount = LastCueAmount;
	View.CuePlayCount = CuePlayCount;
	return View;
}

FString UWacomBattleEnemyPartWorldTargetBridgeComponent::GetBattleWorldTargetDebugSummary() const
{
	const FWacomBattleEnemyPartWorldTargetDebugView View = GetBattleWorldTargetDebugView();
	return FString::Printf(
		TEXT("BattleEnemyPartWorldTarget{Owner=%s PartId=%s PartInstanceId=%s Bound=%s Registered=%s Targetable=%s Disabled=%s LastBind=%s LastCue=%s CueType=%d CueAmount=%d CueCount=%d}"),
		*GetNameSafe(GetOwner()),
		*View.PartId.ToString(),
		*View.PartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.bBoundToSnapshot ? TEXT("true") : TEXT("false"),
		View.bRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
		View.bTargetable ? TEXT("true") : TEXT("false"),
		*View.TargetDisabledReason.ToString(),
		*View.LastBindResult.ToString(),
		*View.LastCueKind.ToString(),
		static_cast<int32>(View.LastCueType),
		View.LastCueAmount,
		View.CuePlayCount);
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::LogBattleWorldTargetDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattleEnemyPartWorldTarget] %s"), *GetBattleWorldTargetDebugSummary());
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearBattleBinding();
	StopFeedbackTimer();
	Super::EndPlay(EndPlayReason);
}

UWacomInteractionTargetComponent*
UWacomBattleEnemyPartWorldTargetBridgeComponent::ResolveInteractionTargetComponent() const
{
	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UWacomInteractionTargetComponent>() : nullptr;
}

UPrimitiveComponent*
UWacomBattleEnemyPartWorldTargetBridgeComponent::ResolveVisualTargetComponent() const
{
	if (VisualTargetComponent)
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	return Owner ? Owner->FindComponentByClass<UPrimitiveComponent>() : nullptr;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::RegisterWithBattleHUD(UBattleHUD& HUD)
{
	if (!PartInstanceId.IsValid())
	{
		return;
	}

	if (RegisteredBattleHUD.Get() != &HUD)
	{
		UnregisterFromBattleHUD();
	}
	else
	{
		HUD.UnregisterBattlePresentationTargetsForOwner(this);
		bRegisteredWithBattleHUD = false;
	}

	TWeakObjectPtr<UWacomBattleEnemyPartWorldTargetBridgeComponent> WeakThis = this;
	HUD.RegisterBattlePresentationTarget(
		PartInstanceId,
		this,
		[WeakThis](const FWacomBattlePresentationTargetCue& Cue)
		{
			if (UWacomBattleEnemyPartWorldTargetBridgeComponent* StrongThis = WeakThis.Get())
			{
				StrongThis->PlayBattlePresentationCue(Cue);
			}
		});

	RegisteredBattleHUD = &HUD;
	bRegisteredWithBattleHUD = true;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::UnregisterFromBattleHUD()
{
	if (UBattleHUD* HUD = RegisteredBattleHUD.Get())
	{
		HUD->UnregisterBattlePresentationTargetsForOwner(this);
	}
	RegisteredBattleHUD.Reset();
	bRegisteredWithBattleHUD = false;
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ApplyTargetableAffordance(bool bInTargetable)
{
	if (bTargetable == bInTargetable)
	{
		return;
	}

	bTargetable = bInTargetable;
	if (bTargetable)
	{
		BeginScaleFeedback(TargetableAffordanceScale, 0.0f);
	}
	else
	{
		RestoreBaseScaleIfNeeded();
	}
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::BeginScaleFeedback(float ScaleMultiplier, float HoldSeconds)
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

	if (HoldSeconds > 0.0f)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				FeedbackTimerHandle,
				this,
				&UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearScaleFeedback,
				HoldSeconds,
				false);
		}
	}
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::ClearScaleFeedback()
{
	StopFeedbackTimer();
	if (bTargetable)
	{
		BeginScaleFeedback(TargetableAffordanceScale, 0.0f);
		return;
	}
	RestoreBaseScaleIfNeeded();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::RestoreBaseScaleIfNeeded()
{
	StopFeedbackTimer();
	if (bHasCachedBaseScale)
	{
		if (UPrimitiveComponent* Primitive = CachedVisualTarget.Get())
		{
			Primitive->SetRelativeScale3D(CachedBaseScale);
		}
	}
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::StopFeedbackTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(FeedbackTimerHandle);
	}
	FeedbackTimerHandle = FTimerHandle();
}

void UWacomBattleEnemyPartWorldTargetBridgeComponent::UpdateInteractionTargetComponent()
{
	if (!bAutoConfigureInteractionTarget)
	{
		return;
	}

	if (UWacomInteractionTargetComponent* TargetComponent = ResolveInteractionTargetComponent())
	{
		TargetComponent->SetTargetId(PartInstanceId);
		TargetComponent->SetStableTargetId(PartId);
		TargetComponent->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
	}
}
