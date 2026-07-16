// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardGainRevealStyle.generated.h"

/** Reusable material and normalized timing preset for explicit Gained card reveals. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardGainRevealStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal", meta = (ShowOnlyInnerProperties, ToolTip = "获得卡牌正面像素结晶的材质实例和归一化阶段；Gained Enter Transition 仍是唯一总时长。"))
	FWacomFirstPersonCardGainRevealStyleData Style;
};
