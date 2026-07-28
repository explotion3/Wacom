// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Commandlets/Commandlet.h"
#include "WacomBuildFireWriteCardContentCommandlet.generated.h"

/**
 * Deterministic FireWrite card-content seeder and inspector.
 *
 * Default invocation is inspect-only. Mutating modes must be requested
 * explicitly with -SeedMissing, -MigrateLegacyUpgrade and/or
 * -WriteExplanationTemplates. -SyncSeedDefaults is the explicit,
 * allowlisted repair path for existing FireWrite cards whose formal
 * editable properties no longer match the frozen design seed.
 * -SyncExplanationLexiconDefaults only restores the approved common
 * damage/heal/shield/draw/status-effect sentence templates.
 */
UCLASS()
class UWacomBuildFireWriteCardContentCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UWacomBuildFireWriteCardContentCommandlet();
	virtual int32 Main(const FString& Params) override;
};
