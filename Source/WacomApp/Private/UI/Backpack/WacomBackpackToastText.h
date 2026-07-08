// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunStateTypes.h"

struct FWacomBackpackToastText
{
	static FText FormatZoneNameForToast(EZoneKind Zone);
	static FText FormatMoveFailureReasonForToast(FName DisabledReason);
	static FText FormatDeleteFailureReasonForToast(FName DisabledReason);
};
