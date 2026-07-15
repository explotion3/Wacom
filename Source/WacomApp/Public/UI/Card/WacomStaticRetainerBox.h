// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/RetainerBox.h"
#include "WacomStaticRetainerBox.generated.h"

/** Retainer preset that redraws on invalidation/request only, never on a frame phase. */
UCLASS()
class WACOMAPP_API UWacomStaticRetainerBox : public URetainerBox
{
	GENERATED_BODY()

public:
	UWacomStaticRetainerBox(const FObjectInitializer& ObjectInitializer);
};
