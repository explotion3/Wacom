// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

#include "BattleTargetValidationResult.generated.h"

UENUM(BlueprintType)
enum class EWacomBattleTargetRejectReason : uint8
{
	None UMETA(DisplayName = "None"),
	InvalidTarget UMETA(DisplayName = "InvalidTarget"),
	SourceCardInvalid UMETA(DisplayName = "SourceCardInvalid"),
	SourceCardNotInHand UMETA(DisplayName = "SourceCardNotInHand"),
	SourceCardMissingDefinition UMETA(DisplayName = "SourceCardMissingDefinition"),
	UnsupportedWorldTarget UMETA(DisplayName = "UnsupportedWorldTarget"),
	InvalidWorldTarget UMETA(DisplayName = "InvalidWorldTarget"),
	UnsupportedCardTarget UMETA(DisplayName = "UnsupportedCardTarget"),
	TargetCardInvalid UMETA(DisplayName = "TargetCardInvalid"),
	TargetCardNotInHand UMETA(DisplayName = "TargetCardNotInHand"),
	SelfTarget UMETA(DisplayName = "SelfTarget"),
	UnsupportedNormalHandCardTarget UMETA(DisplayName = "UnsupportedNormalHandCardTarget"),
	UnsupportedHandAnchorTarget UMETA(DisplayName = "UnsupportedHandAnchorTarget"),
	UnsupportedZoneTarget UMETA(DisplayName = "UnsupportedZoneTarget"),
};

USTRUCT(BlueprintType)
struct WACOMBATTLE_API FWacomBattleTargetValidationResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Target")
	bool bCanTarget = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Target")
	EWacomBattleTargetRejectReason RejectReason = EWacomBattleTargetRejectReason::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|Battle|Target")
	FString DebugSummary;
};
