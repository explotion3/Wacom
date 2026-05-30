// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

class UCardDefinition;
struct FBattleState;

enum class EWacomHandCardTargetEligibilityReject : uint8
{
	None,
	NormalHandCardUnsupported,
	HandAnchorUnsupported,
	MissingRequiredTargetKeyword,
	BlockedTargetKeyword,
};

struct FWacomHandCardTargetEligibility
{
	bool bCanTarget = false;
	EWacomHandCardTargetEligibilityReject RejectReason =
		EWacomHandCardTargetEligibilityReject::None;
};

struct FWacomResolvedHandCardTargetFilter
{
	bool bAllowNormalHandCards = true;
	bool bAllowHandAnchors = true;
	bool bUsesExplicitFilter = false;
	bool bUsesSelectedZoneMoveFallback = false;
	FGameplayTagContainer RequiredTargetKeywords;
	FGameplayTagContainer BlockedTargetKeywords;
};

struct FHandCardTargetEligibility
{
	static FWacomResolvedHandCardTargetFilter ResolveFilter(const UCardDefinition& SourceDefinition);
	static FWacomHandCardTargetEligibility Validate(
		const FBattleState& State,
		const UCardDefinition& SourceDefinition,
		const FGuid& TargetCardInstanceId);
	static bool IsHandAnchor(const FBattleState& State, const FGuid& CardInstanceId);
};
