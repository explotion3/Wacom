// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

struct FEffectCondition;

namespace WacomCardExplanationConditionRenderer
{
	void AppendConditionRuns(
		FWacomCardDetailBlock& Block,
		const FEffectCondition& Condition,
		const FString& StableIdPrefix,
		int32& RunIndex);
}
