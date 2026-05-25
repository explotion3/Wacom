// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattlePresentationTargetComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

UWacomBattlePresentationTargetComponent::UWacomBattlePresentationTargetComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UWacomBattlePresentationTargetComponent::SetPartId(FName InPartId)
{
	PartId = InPartId;
}

void UWacomBattlePresentationTargetComponent::SetPartInstanceId(const FGuid& InPartInstanceId)
{
	if (PartInstanceId == InPartInstanceId)
	{
		return;
	}

	UBattleHUD* HUDToReregister = RegisteredHUD.Get();
	const bool bWasRegistered = IsRegisteredWithBattleHUD();

	if (bWasRegistered)
	{
		UnregisterFromBattleHUD();
	}

	PartInstanceId = InPartInstanceId;

	if (bWasRegistered && HUDToReregister && PartInstanceId.IsValid())
	{
		RegisterWithBattleHUD(HUDToReregister);
	}
}

void UWacomBattlePresentationTargetComponent::SetVisualTargetComponent(UPrimitiveComponent* InVisualTargetComponent)
{
	if (VisualTargetComponent == InVisualTargetComponent)
	{
		return;
	}

	RestoreVisualFeedback();
	VisualTargetComponent = InVisualTargetComponent;
}

void UWacomBattlePresentationTargetComponent::SetClickTargetComponent(UPrimitiveComponent* InClickTargetComponent)
{
	if (ClickTargetComponent == InClickTargetComponent)
	{
		return;
	}

	UBattleHUD* HUDToReregister = RegisteredHUD.Get();
	const bool bWasRegistered = IsRegisteredWithBattleHUD();
	if (bWasRegistered)
	{
		UnregisterFromBattleHUD();
	}

	ClickTargetComponent = InClickTargetComponent;

	if (bWasRegistered && HUDToReregister && PartInstanceId.IsValid())
	{
		RegisterWithBattleHUD(HUDToReregister);
	}
}

bool UWacomBattlePresentationTargetComponent::RequestSceneTargetClick()
{
	UBattleHUD* HUD = RegisteredHUD.Get();
	if (!IsValid(HUD) || !PartInstanceId.IsValid() || !HUD->IsBattlePresentationTargetRegisteredForOwner(this))
	{
		return false;
	}

	HUD->OnEnemyPartClickedByUser(PartInstanceId);
	return true;
}

bool UWacomBattlePresentationTargetComponent::RegisterWithBattleHUD(UBattleHUD* InHUD)
{
	if (!PartInstanceId.IsValid() || !IsValid(InHUD))
	{
		return false;
	}

	UnregisterFromBattleHUD();

	TWeakObjectPtr<UWacomBattlePresentationTargetComponent> WeakThis(this);
	InHUD->RegisterBattlePresentationTarget(
		PartInstanceId,
		this,
		[WeakThis](const FWacomBattlePresentationTargetCue& Cue)
		{
			if (UWacomBattlePresentationTargetComponent* Component = WeakThis.Get())
			{
				Component->HandleBattlePresentationCue(Cue.SourceEventType, Cue.Amount);
			}
		});

	RegisteredHUD = InHUD;
	BindPIEClickTarget();
	return true;
}

void UWacomBattlePresentationTargetComponent::UnregisterFromBattleHUD()
{
	UnbindPIEClickTarget();
	if (UBattleHUD* HUD = RegisteredHUD.Get())
	{
		HUD->UnregisterBattlePresentationTargetsForOwner(this);
	}
	RegisteredHUD.Reset();
	RestoreVisualFeedback();
}

bool UWacomBattlePresentationTargetComponent::IsRegisteredWithBattleHUD() const
{
	UBattleHUD* HUD = RegisteredHUD.Get();
	return HUD && HUD->IsBattlePresentationTargetRegisteredForOwner(this);
}

void UWacomBattlePresentationTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromBattleHUD();
	RestoreVisualFeedback();
	Super::EndPlay(EndPlayReason);
}

void UWacomBattlePresentationTargetComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	UnregisterFromBattleHUD();
	RestoreVisualFeedback();
	Super::OnComponentDestroyed(bDestroyingHierarchy);
}

void UWacomBattlePresentationTargetComponent::NativeOnBattlePresentationCue(
	EBattleEventType /*SourceEventType*/,
	int32 /*Amount*/)
{
}

void UWacomBattlePresentationTargetComponent::HandleBattlePresentationCue(
	EBattleEventType SourceEventType,
	int32 Amount)
{
	LastCueType = SourceEventType;
	LastCueAmount = Amount;
	++CuePlayCount;
	PlayVisualFeedback(SourceEventType);
	NativeOnBattlePresentationCue(SourceEventType, Amount);
}

void UWacomBattlePresentationTargetComponent::BindPIEClickTarget()
{
	if (!bEnablePIECollisionClick)
	{
		return;
	}

	UPrimitiveComponent* Target = ResolveClickTargetComponent();
	if (!IsValid(Target))
	{
		return;
	}

	UnbindPIEClickTarget();

	BoundClickTarget = Target;
	if (!Target->OnClicked.IsAlreadyBound(this, &UWacomBattlePresentationTargetComponent::HandleClickTargetClicked))
	{
		Target->OnClicked.AddDynamic(this, &UWacomBattlePresentationTargetComponent::HandleClickTargetClicked);
	}

	if (bConfigurePIEClickCollision)
	{
		SaveAndConfigureClickTargetCollision(*Target);
	}

	if (UBattleHUD* HUD = RegisteredHUD.Get())
	{
		HUD->AcquirePlayerControllerClickEvents();
		bHasAcquiredPlayerControllerClickEvents = true;
	}
}

void UWacomBattlePresentationTargetComponent::UnbindPIEClickTarget()
{
	UPrimitiveComponent* Target = BoundClickTarget.Get();
	if (Target)
	{
		Target->OnClicked.RemoveDynamic(this, &UWacomBattlePresentationTargetComponent::HandleClickTargetClicked);
		RestoreClickTargetCollision(*Target);
	}
	else
	{
		bHasSavedClickTargetCollision = false;
		SavedClickTargetCollisionEnabled = ECollisionEnabled::NoCollision;
		SavedClickTargetVisibilityResponse = ECR_Block;
	}

	BoundClickTarget.Reset();

	if (bHasAcquiredPlayerControllerClickEvents)
	{
		if (UBattleHUD* HUD = RegisteredHUD.Get())
		{
			HUD->ReleasePlayerControllerClickEvents();
		}
		bHasAcquiredPlayerControllerClickEvents = false;
	}
}

void UWacomBattlePresentationTargetComponent::SaveAndConfigureClickTargetCollision(UPrimitiveComponent& Target)
{
	if (!bHasSavedClickTargetCollision)
	{
		SavedClickTargetCollisionEnabled = Target.GetCollisionEnabled();
		SavedClickTargetVisibilityResponse = Target.GetCollisionResponseToChannel(ECC_Visibility);
		bHasSavedClickTargetCollision = true;
	}

	if (Target.GetCollisionEnabled() == ECollisionEnabled::NoCollision)
	{
		Target.SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	else if (Target.GetCollisionEnabled() == ECollisionEnabled::PhysicsOnly)
	{
		Target.SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	Target.SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
}

void UWacomBattlePresentationTargetComponent::RestoreClickTargetCollision(UPrimitiveComponent& Target)
{
	if (!bHasSavedClickTargetCollision)
	{
		return;
	}

	Target.SetCollisionEnabled(SavedClickTargetCollisionEnabled);
	Target.SetCollisionResponseToChannel(ECC_Visibility, SavedClickTargetVisibilityResponse);
	bHasSavedClickTargetCollision = false;
	SavedClickTargetCollisionEnabled = ECollisionEnabled::NoCollision;
	SavedClickTargetVisibilityResponse = ECR_Block;
}

UPrimitiveComponent* UWacomBattlePresentationTargetComponent::ResolveClickTargetComponent() const
{
	if (IsValid(ClickTargetComponent))
	{
		return ClickTargetComponent;
	}

	if (IsValid(VisualTargetComponent))
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<UPrimitiveComponent>();
}

void UWacomBattlePresentationTargetComponent::HandleClickTargetClicked(
	UPrimitiveComponent* /*TouchedComponent*/,
	FKey /*ButtonPressed*/)
{
	RequestSceneTargetClick();
}

void UWacomBattlePresentationTargetComponent::PlayVisualFeedback(EBattleEventType SourceEventType)
{
	if (!bEnableVisualFeedback
		|| (SourceEventType != EBattleEventType::DamageDealt
			&& SourceEventType != EBattleEventType::EnemyPartHpEmptied))
	{
		return;
	}

	RestoreVisualFeedback();

	UPrimitiveComponent* Target = ResolveVisualTargetComponent();
	if (!IsValid(Target))
	{
		return;
	}

	ActiveVisualFeedbackTarget = Target;
	BaseVisualFeedbackScale = Target->GetRelativeScale3D();
	bVisualFeedbackActive = true;

	const float ScaleMultiplier = SourceEventType == EBattleEventType::EnemyPartHpEmptied
		? FMath::Max(1.0f, DestroyedPulseScale)
		: FMath::Max(1.0f, DamagePulseScale);
	Target->SetRelativeScale3D(BaseVisualFeedbackScale * ScaleMultiplier);

	const float HoldSeconds = SourceEventType == EBattleEventType::EnemyPartHpEmptied
		? FMath::Max(0.0f, DestroyedPulseSeconds)
		: FMath::Max(0.0f, DamagePulseSeconds);
	if (HoldSeconds <= 0.0f)
	{
		RestoreVisualFeedback();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			VisualFeedbackTimerHandle,
			this,
			&UWacomBattlePresentationTargetComponent::RestoreVisualFeedback,
			HoldSeconds,
			false);
	}
}

void UWacomBattlePresentationTargetComponent::RestoreVisualFeedback()
{
	StopVisualFeedbackTimer();

	if (UPrimitiveComponent* Target = ActiveVisualFeedbackTarget.Get())
	{
		Target->SetRelativeScale3D(BaseVisualFeedbackScale);
	}

	ActiveVisualFeedbackTarget.Reset();
	BaseVisualFeedbackScale = FVector::OneVector;
	bVisualFeedbackActive = false;
}

void UWacomBattlePresentationTargetComponent::StopVisualFeedbackTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisualFeedbackTimerHandle);
	}
	VisualFeedbackTimerHandle = FTimerHandle();
}

UPrimitiveComponent* UWacomBattlePresentationTargetComponent::ResolveVisualTargetComponent() const
{
	if (IsValid(VisualTargetComponent))
	{
		return VisualTargetComponent;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	return Owner->FindComponentByClass<UPrimitiveComponent>();
}
