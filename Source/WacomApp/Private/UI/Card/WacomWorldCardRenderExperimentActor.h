// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomWorldCardRenderExperimentActor.generated.h"

class APlayerController;
class UMaterialInstanceDynamic;
class USceneComponent;
class UTextRenderComponent;
class UWidgetComponent;
class UWacomCardView;

UCLASS(Transient, NotBlueprintable)
class AWacomWorldCardRenderExperimentActor final : public AActor
{
	GENERATED_BODY()

public:
	AWacomWorldCardRenderExperimentActor();

	bool InitializeExperiment(
		APlayerController& InPlayerController,
		TSubclassOf<UWacomCardView> CardViewClass,
		const FWacomCardViewData& CardViewData);

	bool SetExposureCompensationStrength(float Strength);
	FString BuildDebugSummary() const;
	int32 GetWorldCardCount() const;

protected:
	virtual void Tick(float DeltaSeconds) override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	bool UpdateCameraRelativeTransform();

	UPROPERTY()
	TObjectPtr<USceneComponent> SceneRoot = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UWidgetComponent>> WorldCardComponents;

	UPROPERTY()
	TArray<TObjectPtr<UTextRenderComponent>> ModeLabelComponents;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomCardView>> WorldCardWidgets;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInstanceDynamic> ExposureMaterialInstance = nullptr;

	UPROPERTY(Transient)
	TObjectPtr<APlayerController> OwningPlayerController = nullptr;

	float ExposureCompensationStrength = 1.0f;
};
