// Copyright Wacom. All Rights Reserved.

#include "Components/WacomBattleCameraLookComponent.h"

#include "Components/WacomCursorLookDriverComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"

UWacomBattleCameraLookComponent::UWacomBattleCameraLookComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UWacomBattleCameraLookComponent::BeginPlay()
{
	Super::BeginPlay();
	SetComponentTickEnabled(false);
}

void UWacomBattleCameraLookComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	DeactivateBattleCameraLook();
	Super::EndPlay(EndPlayReason);
}

void UWacomBattleCameraLookComponent::TickComponent(
	float DeltaTime,
	ELevelTick TickType,
	FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bBattleCameraLookActive)
	{
		return;
	}

	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!PC || !Driver || !Character)
	{
		return;
	}

	UpdateCursorLookOffset(DeltaTime);
	const FRotator LookOffset = Driver->GetCurrentLookOffset();
	PC->SetControlRotation(FRotator(
		BaseBattleRotation.Pitch + LookOffset.Pitch,
		BaseBattleRotation.Yaw + LookOffset.Yaw,
		0.0f));
	Character->SetActorRotation(BaseActorRotation, ETeleportType::TeleportPhysics);
}

bool UWacomBattleCameraLookComponent::ActivateBattleCameraLook()
{
	bUseRuntimeCursorLookProfile = false;
	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!PC || !Driver || !Character)
	{
		return false;
	}

	return ActivateBattleCameraLookInternal(
		*PC,
		*Driver,
		*Character,
		PC->GetControlRotation(),
		Character->GetActorRotation(),
		/*bResetCursorLookOffset*/true);
}

bool UWacomBattleCameraLookComponent::ActivateBattleCameraLookWithProfile(
	const FWacomCursorLookProfile& RuntimeProfile)
{
	bUseRuntimeCursorLookProfile = true;
	RuntimeCursorLookProfile = RuntimeProfile.Sanitized();
	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return PC && Driver && Character
		&& ActivateBattleCameraLookInternal(
			*PC, *Driver, *Character, PC->GetControlRotation(), Character->GetActorRotation(), true);
}

bool UWacomBattleCameraLookComponent::ActivateBattleCameraLookFromBaseRotation(
	FRotator InBaseBattleRotation,
	FRotator InBaseActorRotation,
	bool bPreserveCurrentCursorLookOffset)
{
	bUseRuntimeCursorLookProfile = false;
	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	if (!PC || !Driver || !Character)
	{
		return false;
	}

	return ActivateBattleCameraLookInternal(
		*PC,
		*Driver,
		*Character,
		InBaseBattleRotation,
		InBaseActorRotation,
		/*bResetCursorLookOffset*/!bPreserveCurrentCursorLookOffset);
}

bool UWacomBattleCameraLookComponent::ActivateBattleCameraLookFromBaseRotationWithProfile(
	FRotator InBaseBattleRotation,
	FRotator InBaseActorRotation,
	bool bPreserveCurrentCursorLookOffset,
	const FWacomCursorLookProfile& RuntimeProfile)
{
	bUseRuntimeCursorLookProfile = true;
	RuntimeCursorLookProfile = RuntimeProfile.Sanitized();
	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return PC && Driver && Character
		&& ActivateBattleCameraLookInternal(
			*PC,
			*Driver,
			*Character,
			InBaseBattleRotation,
			InBaseActorRotation,
			!bPreserveCurrentCursorLookOffset);
}

FWacomCursorLookProfile UWacomBattleCameraLookComponent::GetAuthoredCursorLookProfile() const
{
	FWacomCursorLookProfile Profile;
	Profile.YawClampDegrees = YawClampDegrees;
	Profile.PitchClampDegrees = PitchClampDegrees;
	Profile.LookYawScale = LookYawScale;
	Profile.LookPitchScale = LookPitchScale;
	Profile.LookInterpSpeed = LookInterpSpeed;
	return Profile.Sanitized();
}

bool UWacomBattleCameraLookComponent::ActivateBattleCameraLookInternal(
	APlayerController& PlayerController,
	UWacomCursorLookDriverComponent& Driver,
	AWacomPlayerCharacter& Character,
	FRotator InBaseBattleRotation,
	FRotator InBaseActorRotation,
	bool bResetCursorLookOffset)
{
	if (bBattleCameraLookActive)
	{
		return true;
	}

	BaseBattleRotation = InBaseBattleRotation.GetNormalized();
	BaseActorRotation = InBaseActorRotation.GetNormalized();
	bSavedUseControllerRotationYaw = Character.bUseControllerRotationYaw;
	bSavedUseControllerRotationPitch = Character.bUseControllerRotationPitch;
	bSavedUseControllerRotationRoll = Character.bUseControllerRotationRoll;
	bHasSavedRotationPolicy = true;
	Character.bUseControllerRotationYaw = false;
	Character.bUseControllerRotationPitch = false;
	Character.bUseControllerRotationRoll = false;
	if (bResetCursorLookOffset)
	{
		Driver.ResetLookOffset();
	}
	else
	{
		const FRotator LookOffset = Driver.GetCurrentLookOffset();
		PlayerController.SetControlRotation(FRotator(
			BaseBattleRotation.Pitch + LookOffset.Pitch,
			BaseBattleRotation.Yaw + LookOffset.Yaw,
			0.0f));
		Character.SetActorRotation(BaseActorRotation, ETeleportType::TeleportPhysics);
	}
	bBattleCameraLookActive = true;
	SetComponentTickEnabled(true);
	return true;
}

void UWacomBattleCameraLookComponent::DeactivateBattleCameraLook()
{
	if (!bBattleCameraLookActive)
	{
		return;
	}

	bBattleCameraLookActive = false;
	bUseRuntimeCursorLookProfile = false;
	ClearCursorLookOverride();
	SetComponentTickEnabled(false);
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	if (APlayerController* PC = GetOwnerPlayerController())
	{
		PC->SetControlRotation(BaseBattleRotation);
	}
	if (AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		Character->SetActorRotation(BaseActorRotation, ETeleportType::TeleportPhysics);
	}
	RestoreOwnerRotationPolicy();
}

void UWacomBattleCameraLookComponent::DeactivateBattleCameraLookPreservingView()
{
	if (!bBattleCameraLookActive)
	{
		return;
	}

	bBattleCameraLookActive = false;
	bUseRuntimeCursorLookProfile = false;
	ClearCursorLookOverride();
	SetComponentTickEnabled(false);
	if (UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver())
	{
		Driver->ResetLookOffset();
	}
	RestoreOwnerRotationPolicy();
}

void UWacomBattleCameraLookComponent::UpdateCursorLookOffset(float DeltaTime)
{
	APlayerController* PC = GetOwnerPlayerController();
	UWacomCursorLookDriverComponent* Driver = GetCursorLookDriver();
	if (!PC || !Driver)
	{
		return;
	}

	const FWacomCursorLookProfile Profile = bUseRuntimeCursorLookProfile
		? RuntimeCursorLookProfile
		: GetAuthoredCursorLookProfile();
	if (bHasCursorLookOverride)
	{
		const float Scale = FMath::Max(0.0f, CursorLookOverrideScale);
		const float InterpSpeed = CursorLookOverrideInterpSpeed >= 0.0f
			? CursorLookOverrideInterpSpeed
			: Profile.LookInterpSpeed;
		Driver->UpdateFromNormalizedCursor(
			CursorLookOverrideNormalized,
			DeltaTime,
			Profile.YawClampDegrees,
			Profile.PitchClampDegrees,
			Profile.LookYawScale * Scale,
			Profile.LookPitchScale * Scale,
			InterpSpeed);
		return;
	}

	Driver->UpdateFromPlayerCursor(
		PC,
		DeltaTime,
		Profile);
}

void UWacomBattleCameraLookComponent::SetCursorLookOverrideNormalized(
	FVector2D NormalizedCursor,
	float Scale,
	float InterpSpeedOverride)
{
	CursorLookOverrideNormalized = FVector2D(
		FMath::Clamp(NormalizedCursor.X, -1.0f, 1.0f),
		FMath::Clamp(NormalizedCursor.Y, -1.0f, 1.0f));
	CursorLookOverrideScale = FMath::Max(0.0f, Scale);
	CursorLookOverrideInterpSpeed = InterpSpeedOverride;
	bHasCursorLookOverride = true;
}

void UWacomBattleCameraLookComponent::ClearCursorLookOverride()
{
	bHasCursorLookOverride = false;
	CursorLookOverrideNormalized = FVector2D::ZeroVector;
	CursorLookOverrideScale = 1.0f;
	CursorLookOverrideInterpSpeed = -1.0f;
}

AWacomPlayerCharacter* UWacomBattleCameraLookComponent::GetOwnerCharacter() const
{
	return Cast<AWacomPlayerCharacter>(GetOwner());
}

APlayerController* UWacomBattleCameraLookComponent::GetOwnerPlayerController() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Cast<APlayerController>(Character->GetController()) : nullptr;
}

UWacomCursorLookDriverComponent* UWacomBattleCameraLookComponent::GetCursorLookDriver() const
{
	const AWacomPlayerCharacter* Character = GetOwnerCharacter();
	return Character ? Character->GetCursorLookDriverComponent() : nullptr;
}

void UWacomBattleCameraLookComponent::RestoreOwnerRotationPolicy()
{
	if (!bHasSavedRotationPolicy)
	{
		return;
	}

	if (AWacomPlayerCharacter* Character = GetOwnerCharacter())
	{
		Character->bUseControllerRotationYaw = bSavedUseControllerRotationYaw;
		Character->bUseControllerRotationPitch = bSavedUseControllerRotationPitch;
		Character->bUseControllerRotationRoll = bSavedUseControllerRotationRoll;
	}
	bHasSavedRotationPolicy = false;
}
