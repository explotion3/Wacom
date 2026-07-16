// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardEffectBadgeFeedbackStyle.generated.h"

/** Reusable timing, material and optional audio preset for EffectBadge feedback. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardEffectBadgeFeedbackStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge", meta = (ShowOnlyInnerProperties, ToolTip = "EffectBadge 预览、数值变化、新增和移除使用的局部表现合同；颜色与像素外观在材质实例中调整。"))
	FWacomFirstPersonCardEffectBadgeFeedbackStyleData Style;
};
