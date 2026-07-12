// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardPlayedDissolveStyle.generated.h"

/** Reusable theme preset for the first-person Played pixel-ash dissolve. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardPlayedDissolveStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ShowOnlyInnerProperties, ToolTip = "Played 原地像素灰烬消散使用的材质、颜色、节奏、灰烬和音效参数。"))
	FWacomFirstPersonCardPlayedDissolveStyleData Style;
};
