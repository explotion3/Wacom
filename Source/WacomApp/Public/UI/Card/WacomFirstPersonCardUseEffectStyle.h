// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardUseEffectStyle.generated.h"

/** Reusable playback preset for a normal Played card-use surface effect. */
UCLASS(BlueprintType)
class WACOMAPP_API UWacomFirstPersonCardUseEffectStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ShowOnlyInnerProperties, ToolTip = "普通使用卡牌 Surface Effect 的播放合同；视觉参数保留在引用的材质实例中。"))
	FWacomFirstPersonCardUseEffectStyleData Style;
};
