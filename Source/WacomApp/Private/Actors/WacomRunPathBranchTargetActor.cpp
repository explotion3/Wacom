// Copyright Wacom. All Rights Reserved.

#include "Actors/WacomRunPathBranchTargetActor.h"

#include "Components/BoxComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "Settings/WacomPresentationAccessibilityPolicy.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName TintParameter(TEXT("Tint"));
	const FName GlowStrengthParameter(TEXT("GlowStrength"));
	const FName PulseStrengthParameter(TEXT("PulseStrength"));
	const FName AttentionPulseParameter(TEXT("AttentionPulse"));

	void ConfigureVisualMesh(UStaticMeshComponent& Component)
	{
		Component.SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Component.SetGenerateOverlapEvents(false);
		Component.SetCastShadow(false);
		Component.bReceivesDecals = false;
	}
}

AWacomRunPathBranchTargetActor::AWacomRunPathBranchTargetActor()
{
	PrimaryActorTick.bCanEverTick = false;
	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	ClickBounds = CreateDefaultSubobject<UBoxComponent>(TEXT("ClickBounds"));
	ClickBounds->SetupAttachment(SceneRoot);
	ClickBounds->SetRelativeLocation(FVector(75.0f, 0.0f, -45.0f));
	ClickBounds->SetBoxExtent(FVector(125.0f, 95.0f, 95.0f));
	ClickBounds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ClickBounds->SetCollisionObjectType(ECC_WorldDynamic);
	ClickBounds->SetCollisionResponseToAllChannels(ECR_Ignore);
	ClickBounds->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	ClickBounds->SetGenerateOverlapEvents(false);

	VisualRoot = CreateDefaultSubobject<USceneComponent>(TEXT("VisualRoot"));
	VisualRoot->SetupAttachment(SceneRoot);

	LeftPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("LeftPost"));
	LeftPost->SetupAttachment(VisualRoot);
	LeftPost->SetRelativeLocation(FVector(0.0f, -75.0f, -25.0f));
	LeftPost->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.1f));
	ConfigureVisualMesh(*LeftPost);

	RightPost = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("RightPost"));
	RightPost->SetupAttachment(VisualRoot);
	RightPost->SetRelativeLocation(FVector(0.0f, 75.0f, -25.0f));
	RightPost->SetRelativeScale3D(FVector(0.18f, 0.18f, 1.1f));
	ConfigureVisualMesh(*RightPost);

	GroundGuide = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GroundGuide"));
	GroundGuide->SetupAttachment(VisualRoot);
	GroundGuide->SetRelativeLocation(FVector(80.0f, 0.0f, -118.0f));
	GroundGuide->SetRelativeScale3D(FVector(1.6f, 0.12f, 0.025f));
	ConfigureVisualMesh(*GroundGuide);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(
		TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		LeftPost->SetStaticMesh(CubeMesh.Object);
		RightPost->SetStaticMesh(CubeMesh.Object);
		GroundGuide->SetStaticMesh(CubeMesh.Object);
	}

	EntranceMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(
		TEXT("/Game/DreamMaterials/World/M_WacomRunBranchEntrance.M_WacomRunBranchEntrance")));

	VisualRoot->SetVisibility(false, true);
	VisualRoot->SetHiddenInGame(true, true);
}

void AWacomRunPathBranchTargetActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyEntranceMaterial();
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWacomSettingsSubsystem* Settings =
			GameInstance->GetSubsystem<UWacomSettingsSubsystem>())
		{
			RuntimeSettingsChangedHandle =
				Settings->OnRuntimeSettingsChangedNative().AddUObject(
					this,
					&AWacomRunPathBranchTargetActor::HandleRuntimeSettingsChanged);
			HandleRuntimeSettingsChanged(
				Settings->GetCurrentSnapshot(),
				EWacomRuntimeSettingsChangeReason::Startup);
		}
	}
	ApplyPresentationState();
}

void AWacomRunPathBranchTargetActor::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(AttentionPulseTimerHandle);
	}
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWacomSettingsSubsystem* Settings =
			GameInstance->GetSubsystem<UWacomSettingsSubsystem>();
			Settings && RuntimeSettingsChangedHandle.IsValid())
		{
			Settings->OnRuntimeSettingsChangedNative().Remove(
				RuntimeSettingsChangedHandle);
		}
	}
	RuntimeSettingsChangedHandle.Reset();
	Super::EndPlay(EndPlayReason);
}

void AWacomRunPathBranchTargetActor::SetPresentationState(
	const EWacomRunPathBranchPresentationState NewState)
{
	if (PresentationState == NewState)
	{
		return;
	}
	PresentationState = NewState;
	ApplyPresentationState();
	BP_OnBranchPresentationStateChanged(PresentationState);
}

void AWacomRunPathBranchTargetActor::PlayAttentionPulse()
{
	if (PresentationState == EWacomRunPathBranchPresentationState::Hidden)
	{
		return;
	}
	for (UStaticMeshComponent* Mesh : { LeftPost, RightPost, GroundGuide })
	{
		if (Mesh)
		{
			Mesh->SetScalarParameterValueOnMaterials(
				AttentionPulseParameter,
				DecorativeFlashScale);
		}
	}
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().SetTimer(
			AttentionPulseTimerHandle,
			this,
			&AWacomRunPathBranchTargetActor::ClearAttentionPulse,
			0.35f,
			false);
	}
}

bool AWacomRunPathBranchTargetActor::RequestBranch() const
{
	if (EdgeId.IsNone()
		|| PresentationState == EWacomRunPathBranchPresentationState::Hidden)
	{
		return false;
	}
	BranchRequestedNative.Broadcast(EdgeId);
	return true;
}

void AWacomRunPathBranchTargetActor::ApplyPresentationState()
{
	const bool bVisible =
		PresentationState != EWacomRunPathBranchPresentationState::Hidden;
	if (VisualRoot)
	{
		VisualRoot->SetVisibility(bVisible, true);
		VisualRoot->SetHiddenInGame(!bVisible, true);
	}
	if (ClickBounds)
	{
		ClickBounds->SetCollisionEnabled(
			bVisible ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
	}

	const bool bFocused =
		PresentationState == EWacomRunPathBranchPresentationState::Focused;
	const FLinearColor Tint = bFocused ? FocusedColor : AvailableColor;
	const float GlowStrength = bFocused
		? FocusedGlowStrength
		: AvailableGlowStrength;
	for (UStaticMeshComponent* Mesh : { LeftPost, RightPost, GroundGuide })
	{
		if (!Mesh)
		{
			continue;
		}
		Mesh->SetVectorParameterValueOnMaterials(
			TintParameter,
			FVector(Tint.R, Tint.G, Tint.B));
		Mesh->SetScalarParameterValueOnMaterials(
			GlowStrengthParameter,
			GlowStrength);
		Mesh->SetScalarParameterValueOnMaterials(
			PulseStrengthParameter,
			DecorativeFlashScale);
		Mesh->SetScalarParameterValueOnMaterials(AttentionPulseParameter, 0.0f);
	}
}

void AWacomRunPathBranchTargetActor::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	const EWacomRuntimeSettingsChangeReason Reason)
{
	(void)Reason;
	DecorativeFlashScale =
		FWacomPresentationAccessibilityPolicy::GetDecorativeFlashIntensityScale(
			Snapshot.FlashEffectMode);
	ApplyPresentationState();
}

void AWacomRunPathBranchTargetActor::ClearAttentionPulse()
{
	for (UStaticMeshComponent* Mesh : { LeftPost, RightPost, GroundGuide })
	{
		if (Mesh)
		{
			Mesh->SetScalarParameterValueOnMaterials(AttentionPulseParameter, 0.0f);
		}
	}
}

void AWacomRunPathBranchTargetActor::ApplyEntranceMaterial()
{
	UMaterialInterface* Material = EntranceMaterial.LoadSynchronous();
	if (!Material)
	{
		return;
	}
	for (UStaticMeshComponent* Mesh : { LeftPost, RightPost, GroundGuide })
	{
		if (Mesh)
		{
			Mesh->SetMaterial(0, Material);
		}
	}
}
