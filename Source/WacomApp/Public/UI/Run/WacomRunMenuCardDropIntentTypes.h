// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "WacomRunMenuCardDropIntentTypes.generated.h"

UENUM(BlueprintType)
enum class EWacomRunMenuCardDropIntentKind : uint8
{
	None UMETA(DisplayName = "None"),
	ProbeZoneTarget UMETA(DisplayName = "Probe Zone Target"),
	PayOwnedCardToZone UMETA(DisplayName = "Pay Owned Card To Zone"),
	Reject UMETA(DisplayName = "Reject")
};

UENUM(BlueprintType)
enum class EWacomRunMenuCardDropRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	NotInExploration UMETA(DisplayName = "Not In Exploration"),
	MissingGameMenu UMETA(DisplayName = "Missing Game Menu"),
	MissingMenuLease UMETA(DisplayName = "Missing Menu Lease"),
	MissingSession UMETA(DisplayName = "Missing Session"),
	InvalidSourceCard UMETA(DisplayName = "Invalid Source Card"),
	MissingZoneTarget UMETA(DisplayName = "Missing Zone Target"),
	UnsupportedTargetKind UMETA(DisplayName = "Unsupported Target Kind"),
	MenuNotFound UMETA(DisplayName = "Menu Not Found"),
	MenuDoesNotAccept UMETA(DisplayName = "Menu Does Not Accept"),
	CardNotOwned UMETA(DisplayName = "Card Not Owned"),
	RunValidationFailed UMETA(DisplayName = "Run Validation Failed"),
	SubmitFailed UMETA(DisplayName = "Submit Failed")
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomRunMenuCardDropResolveResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	EWacomRunMenuCardDropIntentKind IntentKind =
		EWacomRunMenuCardDropIntentKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	EWacomRunMenuCardDropRejectReason RejectReason =
		EWacomRunMenuCardDropRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	bool bCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	bool bSubmitted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FGuid SourceCardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FWacomInteractionTargetHandle TargetHandle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FName ZoneId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FName LeaseId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FName LeaseSourceId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FName RunValidationReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Run|Menu Card Drop")
	FString DebugSummary;
};
