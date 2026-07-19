// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomBuildCombatActivityUICommandlet.generated.h"

/** Deterministically builds/audits the BattleHUD combat-activity WBP assets. */
UCLASS()
class UWacomBuildCombatActivityUICommandlet final : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildCombatActivityUICommandlet();
	virtual int32 Main(const FString& Params) override;
};
