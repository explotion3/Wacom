// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattlePresentationTargetComponent.h"

#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Actor.h"
#include "Engine/World.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"

namespace
{
	FString GetObjectDebugName(const UObject* Object)
	{
		return IsValid(Object) ? Object->GetName() : TEXT("None");
	}

	bool CanCollisionReceiveVisibilityQuery(ECollisionEnabled::Type CollisionEnabled)
	{
		return CollisionEnabled == ECollisionEnabled::QueryOnly
			|| CollisionEnabled == ECollisionEnabled::QueryAndPhysics;
	}

	FString CollisionEnabledToString(ECollisionEnabled::Type CollisionEnabled)
	{
		return StaticEnum<ECollisionEnabled::Type>()
			? StaticEnum<ECollisionEnabled::Type>()->GetNameStringByValue(static_cast<int64>(CollisionEnabled))
			: FString::FromInt(static_cast<int32>(CollisionEnabled));
	}

	FString CollisionResponseToString(ECollisionResponse Response)
	{
		return StaticEnum<ECollisionResponse>()
			? StaticEnum<ECollisionResponse>()->GetNameStringByValue(static_cast<int64>(Response))
			: FString::FromInt(static_cast<int32>(Response));
	}
}

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

	StopAllVisualPresentation();
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
	if (!PartInstanceId.IsValid())
	{
		MarkClickResult(TEXT("InvalidPartInstanceId"));
		return false;
	}

	UBattleHUD* HUD = RegisteredHUD.Get();
	if (!IsValid(HUD))
	{
		MarkClickResult(TEXT("NoRegisteredHUD"));
		return false;
	}
	if (!HUD->IsBattlePresentationTargetRegisteredForOwner(this))
	{
		MarkClickResult(TEXT("NotRegisteredInHUD"));
		return false;
	}

	HUD->OnEnemyPartClickedByUser(PartInstanceId);
	MarkClickResult(TEXT("Forwarded"));
	return true;
}

bool UWacomBattlePresentationTargetComponent::RegisterWithBattleHUD(UBattleHUD* InHUD)
{
	if (!PartInstanceId.IsValid())
	{
		MarkRegistrationResult(TEXT("InvalidPartInstanceId"));
		return false;
	}
	if (!IsValid(InHUD))
	{
		MarkRegistrationResult(TEXT("InvalidHUD"));
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
	MarkRegistrationResult(TEXT("Registered"));
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
	StopAllVisualPresentation();
	MarkRegistrationResult(TEXT("Unregistered"));
}

bool UWacomBattlePresentationTargetComponent::IsRegisteredWithBattleHUD() const
{
	UBattleHUD* HUD = RegisteredHUD.Get();
	return HUD && HUD->IsBattlePresentationTargetRegisteredForOwner(this);
}

void UWacomBattlePresentationTargetComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnregisterFromBattleHUD();
	StopAllVisualPresentation();
	Super::EndPlay(EndPlayReason);
}

void UWacomBattlePresentationTargetComponent::OnComponentDestroyed(bool bDestroyingHierarchy)
{
	UnregisterFromBattleHUD();
	StopAllVisualPresentation();
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
	LogDebugStateChange(TEXT("Cue"), SourceEventType == EBattleEventType::EnemyPartHpEmptied
		? TEXT("EnemyPartHpEmptied")
		: SourceEventType == EBattleEventType::DamageDealt
			? TEXT("DamageDealt")
			: TEXT("OtherCue"));
}

FWacomBattlePresentationTargetDebugView UWacomBattlePresentationTargetComponent::GetBattlePresentationTargetDebugView() const
{
	FWacomBattlePresentationTargetDebugView View;
	View.PartId = PartId;
	View.PartInstanceId = PartInstanceId;
	View.bIsRegisteredWithBattleHUD = IsRegisteredWithBattleHUD();
	View.RegisteredHUDName = GetObjectDebugName(RegisteredHUD.Get());
	View.ResolvedVisualTargetName = GetObjectDebugName(ResolveVisualTargetComponent());
	View.ResolvedClickTargetName = GetObjectDebugName(ResolveClickTargetComponent());
	View.BoundClickTargetName = GetObjectDebugName(BoundClickTarget.Get());

	if (UPrimitiveComponent* ClickTarget = ResolveClickTargetComponent())
	{
		View.ClickTargetCollisionEnabled = ClickTarget->GetCollisionEnabled();
		View.ClickTargetVisibilityResponse = ClickTarget->GetCollisionResponseToChannel(ECC_Visibility);
		View.bClickTargetBlocksVisibility =
			CanCollisionReceiveVisibilityQuery(ClickTarget->GetCollisionEnabled())
			&& ClickTarget->GetCollisionResponseToChannel(ECC_Visibility) == ECR_Block;
	}

	View.LastCueType = LastCueType;
	View.LastCueAmount = LastCueAmount;
	View.CuePlayCount = CuePlayCount;
	View.bVisualFeedbackActive = bVisualFeedbackActive;
	View.bTargetSelectionAffordanceActive = bTargetSelectionAffordanceActive;
	View.bTargetSelectionTargetable = bTargetSelectionTargetable;
	View.TargetSelectionDisabledReason = TargetSelectionDisabledReason;
	View.LastRegistrationResult = LastRegistrationResult;
	View.LastAutoBindResult = LastAutoBindResult;
	View.LastClickResult = LastClickResult;
	return View;
}

FString UWacomBattlePresentationTargetComponent::GetBattlePresentationTargetDebugSummary() const
{
	const FWacomBattlePresentationTargetDebugView View = GetBattlePresentationTargetDebugView();
	return FString::Printf(
		TEXT("PartId=%s PartInstanceId=%s Registered=%s HUD=%s Visual=%s Click=%s BoundClick=%s Collision=%s Visibility=%s BlocksVisibility=%s LastRegistration=%s LastAutoBind=%s LastClick=%s LastCue=%s Amount=%d Count=%d VisualActive=%s TargetAffordance=%s Targetable=%s TargetDisabledReason=%s"),
		*View.PartId.ToString(),
		*View.PartInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
		View.bIsRegisteredWithBattleHUD ? TEXT("true") : TEXT("false"),
		*View.RegisteredHUDName,
		*View.ResolvedVisualTargetName,
		*View.ResolvedClickTargetName,
		*View.BoundClickTargetName,
		*CollisionEnabledToString(View.ClickTargetCollisionEnabled.GetValue()),
		*CollisionResponseToString(View.ClickTargetVisibilityResponse.GetValue()),
		View.bClickTargetBlocksVisibility ? TEXT("true") : TEXT("false"),
		*View.LastRegistrationResult.ToString(),
		*View.LastAutoBindResult.ToString(),
		*View.LastClickResult.ToString(),
		*UEnum::GetValueAsString(View.LastCueType),
		View.LastCueAmount,
		View.CuePlayCount,
		View.bVisualFeedbackActive ? TEXT("true") : TEXT("false"),
		View.bTargetSelectionAffordanceActive ? TEXT("true") : TEXT("false"),
		View.bTargetSelectionTargetable ? TEXT("true") : TEXT("false"),
		*View.TargetSelectionDisabledReason.ToString());
}

void UWacomBattlePresentationTargetComponent::LogBattlePresentationTargetDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomBattlePresentationTarget] %s :: %s"),
		*GetObjectDebugName(this),
		*GetBattlePresentationTargetDebugSummary());
}

bool UWacomBattlePresentationTargetComponent::ValidateBattlePresentationTargetAuthoring(
	TArray<FString>& OutWarnings) const
{
	OutWarnings.Reset();

	if (PartId.IsNone() && !PartInstanceId.IsValid())
	{
		OutWarnings.Add(TEXT("Set either PartId for BattleHUD auto-binding or PartInstanceId for manual registration."));
	}

	if (!ResolveVisualTargetComponent())
	{
		OutWarnings.Add(TEXT("No visual target primitive was found. Set VisualTargetComponent or add a PrimitiveComponent to the owner."));
	}

	UPrimitiveComponent* ClickTarget = ResolveClickTargetComponent();
	if (!ClickTarget)
	{
		OutWarnings.Add(TEXT("No click target primitive was found. Set ClickTargetComponent, VisualTargetComponent, or add a PrimitiveComponent to the owner."));
	}
	else if (!CanCollisionReceiveVisibilityQuery(ClickTarget->GetCollisionEnabled())
		|| ClickTarget->GetCollisionResponseToChannel(ECC_Visibility) != ECR_Block)
	{
		OutWarnings.Add(FString::Printf(
			TEXT("Click target %s is not currently queryable and blocking Visibility. Current collision=%s visibility=%s."),
			*ClickTarget->GetName(),
			*CollisionEnabledToString(ClickTarget->GetCollisionEnabled()),
			*CollisionResponseToString(ClickTarget->GetCollisionResponseToChannel(ECC_Visibility))));
	}

	return OutWarnings.IsEmpty();
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

	if (!EnsureManagedVisualTarget())
	{
		return;
	}

	bVisualFeedbackActive = true;

	ActiveVisualFeedbackScaleMultiplier = SourceEventType == EBattleEventType::EnemyPartHpEmptied
		? FMath::Max(1.0f, DestroyedPulseScale)
		: FMath::Max(1.0f, DamagePulseScale);
	ApplyCurrentVisualScale();

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
	bVisualFeedbackActive = false;
	ActiveVisualFeedbackScaleMultiplier = 1.0f;
	ApplyCurrentVisualScale();
	RestoreManagedVisualScaleIfIdle();
}

bool UWacomBattlePresentationTargetComponent::EnsureManagedVisualTarget()
{
	if (UPrimitiveComponent* CurrentTarget = ActiveVisualFeedbackTarget.Get())
	{
		if (CurrentTarget == ResolveVisualTargetComponent())
		{
			return true;
		}

		RestoreManagedVisualScale();
	}

	UPrimitiveComponent* Target = ResolveVisualTargetComponent();
	if (!IsValid(Target))
	{
		return false;
	}

	ActiveVisualFeedbackTarget = Target;
	BaseVisualFeedbackScale = Target->GetRelativeScale3D();
	bHasBaseVisualFeedbackScale = true;
	return true;
}

void UWacomBattlePresentationTargetComponent::ApplyCurrentVisualScale()
{
	UPrimitiveComponent* Target = ActiveVisualFeedbackTarget.Get();
	if (!Target || !bHasBaseVisualFeedbackScale)
	{
		return;
	}

	float ScaleMultiplier = 1.0f;
	if (bVisualFeedbackActive)
	{
		ScaleMultiplier = ActiveVisualFeedbackScaleMultiplier;
	}
	else if (bTargetSelectionAffordanceActive)
	{
		ScaleMultiplier = bTargetSelectionAffordancePulseHigh
			? FMath::Max(1.0f, TargetSelectionAffordancePulseScale)
			: FMath::Max(1.0f, TargetSelectionAffordanceScale);
	}

	Target->SetRelativeScale3D(BaseVisualFeedbackScale * ScaleMultiplier);
}

void UWacomBattlePresentationTargetComponent::RestoreManagedVisualScaleIfIdle()
{
	if (!bVisualFeedbackActive && !bTargetSelectionAffordanceActive)
	{
		RestoreManagedVisualScale();
	}
}

void UWacomBattlePresentationTargetComponent::RestoreManagedVisualScale()
{
	StopVisualFeedbackTimer();
	StopTargetSelectionAffordanceTimer();

	if (UPrimitiveComponent* Target = ActiveVisualFeedbackTarget.Get())
	{
		if (bHasBaseVisualFeedbackScale)
		{
			Target->SetRelativeScale3D(BaseVisualFeedbackScale);
		}
	}

	ActiveVisualFeedbackTarget.Reset();
	BaseVisualFeedbackScale = FVector::OneVector;
	bHasBaseVisualFeedbackScale = false;
	bVisualFeedbackActive = false;
	ActiveVisualFeedbackScaleMultiplier = 1.0f;
	bTargetSelectionAffordanceActive = false;
	bTargetSelectionAffordancePulseHigh = false;
}

void UWacomBattlePresentationTargetComponent::StopVisualFeedbackTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(VisualFeedbackTimerHandle);
	}
	VisualFeedbackTimerHandle = FTimerHandle();
}

void UWacomBattlePresentationTargetComponent::SetTargetSelectionAffordance(
	bool bTargetable,
	FName DisabledReason)
{
	if (bTargetable
		&& bTargetSelectionTargetable
		&& bTargetSelectionAffordanceActive
		&& TargetSelectionDisabledReason.IsNone())
	{
		if (EnsureManagedVisualTarget())
		{
			ApplyCurrentVisualScale();
		}
		return;
	}

	if (!bTargetable
		&& !bTargetSelectionTargetable
		&& !bTargetSelectionAffordanceActive
		&& TargetSelectionDisabledReason == DisabledReason)
	{
		return;
	}

	bTargetSelectionTargetable = bTargetable;
	TargetSelectionDisabledReason = bTargetable ? NAME_None : DisabledReason;

	if (!bEnableTargetSelectionAffordance || !bTargetable)
	{
		StopTargetSelectionAffordance();
		LogDebugStateChange(TEXT("TargetSelectionAffordance"), TargetSelectionDisabledReason);
		return;
	}

	StartTargetSelectionAffordance();
	LogDebugStateChange(TEXT("TargetSelectionAffordance"), TEXT("Targetable"));
}

void UWacomBattlePresentationTargetComponent::StartTargetSelectionAffordance()
{
	if (!EnsureManagedVisualTarget())
	{
		bTargetSelectionAffordanceActive = false;
		return;
	}

	bTargetSelectionAffordanceActive = true;
	bTargetSelectionAffordancePulseHigh = false;
	ApplyCurrentVisualScale();

	const float PulseSeconds = FMath::Max(0.0f, TargetSelectionAffordancePulseSeconds);
	if (PulseSeconds <= 0.0f)
	{
		StopTargetSelectionAffordanceTimer();
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TargetSelectionAffordanceTimerHandle,
			this,
			&UWacomBattlePresentationTargetComponent::AdvanceTargetSelectionAffordancePulse,
			PulseSeconds,
			true);
	}
}

void UWacomBattlePresentationTargetComponent::StopTargetSelectionAffordanceTimer()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TargetSelectionAffordanceTimerHandle);
	}
	TargetSelectionAffordanceTimerHandle = FTimerHandle();
}

void UWacomBattlePresentationTargetComponent::StopTargetSelectionAffordance()
{
	StopTargetSelectionAffordanceTimer();
	bTargetSelectionAffordanceActive = false;
	bTargetSelectionAffordancePulseHigh = false;
	ApplyCurrentVisualScale();
	RestoreManagedVisualScaleIfIdle();
}

void UWacomBattlePresentationTargetComponent::AdvanceTargetSelectionAffordancePulse()
{
	if (!bTargetSelectionAffordanceActive)
	{
		StopTargetSelectionAffordanceTimer();
		RestoreManagedVisualScaleIfIdle();
		return;
	}

	bTargetSelectionAffordancePulseHigh = !bTargetSelectionAffordancePulseHigh;
	ApplyCurrentVisualScale();
}

void UWacomBattlePresentationTargetComponent::StopAllVisualPresentation()
{
	RestoreManagedVisualScale();
	bTargetSelectionTargetable = false;
	TargetSelectionDisabledReason = TEXT("NotTargetSelecting");
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

void UWacomBattlePresentationTargetComponent::MarkRegistrationResult(FName Result)
{
	LastRegistrationResult = Result;
	LogDebugStateChange(TEXT("Registration"), Result);
}

void UWacomBattlePresentationTargetComponent::MarkAutoBindResult(FName Result)
{
	LastAutoBindResult = Result;
	LogDebugStateChange(TEXT("AutoBind"), Result);
}

void UWacomBattlePresentationTargetComponent::MarkClickResult(FName Result)
{
	LastClickResult = Result;
	LogDebugStateChange(TEXT("Click"), Result);
}

void UWacomBattlePresentationTargetComponent::LogDebugStateChange(const TCHAR* EventName, FName Result) const
{
	if (!bLogDebugStateChanges)
	{
		return;
	}

	UE_LOG(LogTemp, Display, TEXT("[WacomBattlePresentationTarget] %s %s=%s :: %s"),
		*GetObjectDebugName(this),
		EventName,
		*Result.ToString(),
		*GetBattlePresentationTargetDebugSummary());
}
