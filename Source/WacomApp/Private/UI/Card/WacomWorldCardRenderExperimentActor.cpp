// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomWorldCardRenderExperimentActor.h"

#include "Components/SceneComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/WidgetComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomWorldCardRenderExperimentPolicy.h"
#include "UI/Card/WacomWorldCardSurfaceMaterialAdapter.h"

namespace
{
	constexpr float CameraDistanceCentimeters = 320.0f;
	constexpr float CameraUpOffsetCentimeters = 20.0f;
	constexpr float CardSpacingCentimeters = 92.0f;
	constexpr float CardWorldScale = 0.10f;
	const FIntPoint CardDrawSize(720, 976);
	const FVector2D CardPivot(0.5f, 0.5f);
}

AWacomWorldCardRenderExperimentActor::AWacomWorldCardRenderExperimentActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
	SetActorEnableCollision(false);

	SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
	SetRootComponent(SceneRoot);

	constexpr int32 ModeCount = 4;
	WorldCardComponents.Reserve(ModeCount);
	ModeLabelComponents.Reserve(ModeCount);
	for (int32 Index = 0; Index < ModeCount; ++Index)
	{
		UWidgetComponent* CardComponent = CreateDefaultSubobject<UWidgetComponent>(
			*FString::Printf(TEXT("WorldCard_%02d"), Index + 1));
		CardComponent->SetupAttachment(SceneRoot);
		WorldCardComponents.Add(CardComponent);

		UTextRenderComponent* LabelComponent = CreateDefaultSubobject<UTextRenderComponent>(
			*FString::Printf(TEXT("ModeLabel_%02d"), Index + 1));
		LabelComponent->SetupAttachment(SceneRoot);
		ModeLabelComponents.Add(LabelComponent);
	}
}

bool AWacomWorldCardRenderExperimentActor::InitializeExperiment(
	APlayerController& InPlayerController,
	TSubclassOf<UWacomCardView> CardViewClass,
	const FWacomCardViewData& CardViewData)
{
	const TConstArrayView<FWacomWorldCardRenderExperimentModeConfig> Modes =
		FWacomWorldCardRenderExperimentPolicy::GetModes();
	if (!CardViewClass
		|| Modes.Num() != WorldCardComponents.Num()
		|| Modes.Num() != ModeLabelComponents.Num())
	{
		return false;
	}

	OwningPlayerController = &InPlayerController;
	WorldCardWidgets.Reset(Modes.Num());
	ExposureMaterialInstance = nullptr;

	const float StartOffset =
		-0.5f * CardSpacingCentimeters * static_cast<float>(Modes.Num() - 1);
	for (int32 Index = 0; Index < Modes.Num(); ++Index)
	{
		UWidgetComponent* CardComponent = WorldCardComponents[Index];
		UTextRenderComponent* LabelComponent = ModeLabelComponents[Index];
		if (!CardComponent || !LabelComponent)
		{
			return false;
		}

		const FWacomWorldCardRenderExperimentModeConfig& Mode = Modes[Index];
		const float HorizontalOffset =
			StartOffset + CardSpacingCentimeters * static_cast<float>(Index);

		CardComponent->SetRelativeLocation(FVector(0.0f, HorizontalOffset, 0.0f));
		CardComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
		CardComponent->SetRelativeScale3D(FVector(CardWorldScale));
		CardComponent->SetWidgetSpace(EWidgetSpace::World);
		CardComponent->SetDrawSize(CardDrawSize);
		CardComponent->SetDrawAtDesiredSize(false);
		CardComponent->SetPivot(CardPivot);
		CardComponent->SetTwoSided(true);
		CardComponent->SetBlendMode(Mode.BlendMode);
		CardComponent->SetBackgroundColor(FLinearColor::Transparent);
		CardComponent->SetWindowFocusable(false);
		CardComponent->SetTickWhenOffscreen(true);
		CardComponent->SetManuallyRedraw(false);
		CardComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		CardComponent->SetCollisionResponseToAllChannels(ECR_Ignore);

		UWacomCardView* CardWidget =
			CreateWidget<UWacomCardView>(&InPlayerController, CardViewClass);
		if (!CardWidget)
		{
			return false;
		}
		CardWidget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		CardWidget->SetCardViewData(CardViewData);
		CardComponent->SetWidget(CardWidget);
		WorldCardWidgets.Add(CardWidget);

		if (Mode.bUseWacomMaterial)
		{
			if (!FWacomWorldCardSurfaceMaterialAdapter::Apply(
				*CardComponent,
				Mode.ExposureCompensationStrength))
			{
				return false;
			}
			UMaterialInstanceDynamic* MaterialInstance =
				CardComponent->GetMaterialInstance();
			if (!MaterialInstance)
			{
				return false;
			}
			MaterialInstance->SetScalarParameterValue(
				FWacomWorldCardSurfaceMaterialAdapter::GetExposureStrengthParameterName(),
				Mode.ExposureCompensationStrength);
			if (Mode.Mode ==
				EWacomWorldCardRenderExperimentMode::WacomMaskedExposure)
			{
				ExposureMaterialInstance = MaterialInstance;
				ExposureCompensationStrength =
					Mode.ExposureCompensationStrength;
			}
		}

		LabelComponent->SetRelativeLocation(
			FVector(-1.0f, HorizontalOffset, 56.0f));
		LabelComponent->SetRelativeRotation(FRotator(0.0f, 180.0f, 0.0f));
		LabelComponent->SetHorizontalAlignment(EHorizTextAligment::EHTA_Center);
		LabelComponent->SetVerticalAlignment(EVerticalTextAligment::EVRTA_TextCenter);
		LabelComponent->SetWorldSize(13.0f);
		LabelComponent->SetTextRenderColor(FColor::White);
		LabelComponent->SetText(FText::FromString(Mode.Label));
		LabelComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	return ExposureMaterialInstance != nullptr
		&& UpdateCameraRelativeTransform();
}

bool AWacomWorldCardRenderExperimentActor::SetExposureCompensationStrength(
	const float Strength)
{
	if (!ExposureMaterialInstance || Strength < 0.0f || Strength > 1.0f)
	{
		return false;
	}

	ExposureCompensationStrength = Strength;
	ExposureMaterialInstance->SetScalarParameterValue(
		FWacomWorldCardSurfaceMaterialAdapter::GetExposureStrengthParameterName(),
		ExposureCompensationStrength);
	return true;
}

FString AWacomWorldCardRenderExperimentActor::BuildDebugSummary() const
{
	return FString::Printf(
		TEXT("Actor=%s WorldCards=%d ExposureStrength=%.3f Player=%s"),
		*GetName(),
		GetWorldCardCount(),
		ExposureCompensationStrength,
		OwningPlayerController ? *OwningPlayerController->GetName() : TEXT("None"));
}

int32 AWacomWorldCardRenderExperimentActor::GetWorldCardCount() const
{
	int32 ValidCount = 0;
	for (const UWacomCardView* CardWidget : WorldCardWidgets)
	{
		ValidCount += IsValid(CardWidget) ? 1 : 0;
	}
	return ValidCount;
}

void AWacomWorldCardRenderExperimentActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!UpdateCameraRelativeTransform())
	{
		Destroy();
	}
}

void AWacomWorldCardRenderExperimentActor::EndPlay(
	const EEndPlayReason::Type EndPlayReason)
{
	ExposureMaterialInstance = nullptr;
	WorldCardWidgets.Reset();
	OwningPlayerController = nullptr;
	Super::EndPlay(EndPlayReason);
}

bool AWacomWorldCardRenderExperimentActor::UpdateCameraRelativeTransform()
{
	if (!IsValid(OwningPlayerController))
	{
		return false;
	}

	FVector ViewLocation = FVector::ZeroVector;
	FRotator ViewRotation = FRotator::ZeroRotator;
	OwningPlayerController->GetPlayerViewPoint(ViewLocation, ViewRotation);
	const FVector ViewUp = ViewRotation.RotateVector(FVector::UpVector);
	SetActorLocationAndRotation(
		ViewLocation
			+ ViewRotation.Vector() * CameraDistanceCentimeters
			+ ViewUp * CameraUpOffsetCentimeters,
		ViewRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);
	return true;
}
