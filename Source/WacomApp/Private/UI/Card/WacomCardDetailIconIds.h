// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomCardPresentationTypes.h"

namespace WacomCardDetailIconIds
{
	FString ToString(EWacomCardDetailIcon Icon);
	EWacomCardDetailIcon FromString(const FString& Id);
}
