// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "RunState.h"
#include "UI/Foundation/WacomAppToastTypes.h"
#include "WacomRunEventPresentationBuilder.generated.h"

UENUM(BlueprintType)
enum class EWacomRunEventChoiceAvailabilityTone : uint8
{
	None,
	Ready,
	Requirement,
	Blocked,
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunEventChoiceRequirementItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	ERunEventChoiceRequirementKind Kind = ERunEventChoiceRequirementKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	bool bSatisfied = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FName DisabledReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	EWacomRunEventChoiceAvailabilityTone Tone = EWacomRunEventChoiceAvailabilityTone::None;
};

/** UI-only summary of why a RunEvent choice is ready, requires card payment, or is blocked. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunEventChoiceRequirementView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	bool bAvailable = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	bool bRequiresCardPayment = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FText RequirementText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FText BlockedReasonText;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FName PrimaryReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	int32 PaymentCandidateCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	int32 UnsatisfiedRequirementCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	TArray<FWacomRunEventChoiceRequirementItemView> RequirementItems;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	EWacomRunEventChoiceAvailabilityTone Tone = EWacomRunEventChoiceAvailabilityTone::None;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunEventChoiceConsequenceItemView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	ERunEventChoiceConsequenceKind Kind = ERunEventChoiceConsequenceKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	EWacomRunEventEffectType EffectType = EWacomRunEventEffectType::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FText Text;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	EWacomRunEventChoiceAvailabilityTone Tone = EWacomRunEventChoiceAvailabilityTone::None;
};

/** UI-only summary of what a RunEvent choice intends to do if submitted. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunEventChoiceConsequenceView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	FName ChoiceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|RunEvent")
	TArray<FWacomRunEventChoiceConsequenceItemView> ConsequenceItems;
};

/** Builds UI-only presentation data for lightweight RunEvent feedback. */
UCLASS()
class WACOMAPP_API UWacomRunEventPresentationBuilder : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static FText FormatDisabledReason(FName DisabledReason);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static FText FormatPressureName(EWacomPressureType PressureType);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static FWacomRunEventChoiceRequirementView BuildChoiceRequirementView(
		const FRunEventChoiceSnapshot& Choice);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static FWacomRunEventChoiceConsequenceView BuildChoiceConsequenceView(
		const FRunEventChoiceSnapshot& Choice);

	UFUNCTION(BlueprintPure, Category = "Wacom|RunEvent")
	static TArray<FWacomAppToastView> BuildToastViewsFromChoiceResult(const FRunEventChoiceResult& Result);
};
