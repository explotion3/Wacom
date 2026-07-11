// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardSelectionStyle.generated.h"

/** Reusable theme preset for first-person source-card selection presentation. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardSelectionStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ShowOnlyInnerProperties, ToolTip = "像素棱镜选中效果的颜色、节奏、像素网格和轮廓参数。"))
	FWacomFirstPersonCardSelectionStyleData Style;
};
