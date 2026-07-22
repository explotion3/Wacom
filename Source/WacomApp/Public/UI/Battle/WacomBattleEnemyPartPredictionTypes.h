// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "WacomBattleEnemyPartPredictionTypes.generated.h"

UENUM(BlueprintType)
enum class EWacomBattleEnemyPartPredictionMode : uint8
{
	Hidden,
	HoverInitiative,
	CardPrediction,
	Rejected,
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartPredictionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bVisible = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	EWacomBattleEnemyPartPredictionMode Mode = EWacomBattleEnemyPartPredictionMode::Hidden;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	int32 CurrentInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	int32 PredictedInitiative = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bHasSourceCard = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	int32 SourceCardRuntimeCost = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bSourceCardSwift = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bPerfectReleaseCandidate = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bActionRisk = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bWillSkipActionDueToStun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bHasResistanceComparison = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	int32 ResistancePlayerPeakDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	int32 ResistanceEnemyPeakDamage = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	bool bResistanceWillStun = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	FName RejectReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	FText MainText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Scene Enemy|Prediction")
	FText DetailText;
};
