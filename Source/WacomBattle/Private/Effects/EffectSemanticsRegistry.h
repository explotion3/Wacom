// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Core/BattleOperationAdapter.h"

struct FEffectContext;

using FBattleEffectHandler = bool (*)(FEffectContext&);

/** Single private fact record for execution and authoring support of one effect tag. */
struct FBattleEffectSemantics
{
	FBattleEffectHandler Handler = nullptr;
	EBattleOperationDeterminism PreviewDeterminism = EBattleOperationDeterminism::Unknown;
	bool bSupportedCardEffect = false;
	bool bSupportedEnemyIntentEffect = false;
};

class FBattleEffectSemanticsRegistry
{
public:
	static const FBattleEffectSemantics* Find(const FGameplayTag& EffectType);
};
