// Copyright Wacom. All Rights Reserved.

#pragma once

class APlayerController;

/** Shared App-private entry point used by Main Menu and Pause Menu. */
struct FWacomSettingsScreenFlow
{
	static bool Open(APlayerController& PlayerController);
};
