// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Settings/WacomLocalSettingsTypes.h"

class UWacomGameUserSettings;

/** Single App-private source of truth for the project-balanced local-settings profile. */
struct WACOMAPP_API FWacomLocalSettingsDefaults
{
	static FWacomLocalSettingsSnapshot Build(const UWacomGameUserSettings* Settings);
};
