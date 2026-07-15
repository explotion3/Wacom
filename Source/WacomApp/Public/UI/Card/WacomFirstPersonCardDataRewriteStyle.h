// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardDataRewriteStyle.generated.h"

/** Reusable timing and material preset for explicit card-face data rewrites. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardDataRewriteStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite", meta = (ShowOnlyInnerProperties, ToolTip = "卡面数据重写使用的材质实例、时序与可选音效合同；视觉颜色和像素外观在材质实例中调整。"))
	FWacomFirstPersonCardDataRewriteStyleData Style;
};
