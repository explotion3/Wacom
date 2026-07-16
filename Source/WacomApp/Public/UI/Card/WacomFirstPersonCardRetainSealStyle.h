// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardRetainSealStyle.generated.h"

/** Reusable Retainer material preset for end-turn retained-card seals. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardRetainSealStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Retain Seal", meta = (ShowOnlyInnerProperties, ToolTip = "回合结束保留牌像素封存使用的材质实例；实体运动和时长由 FirstPersonCardAnchorComponent 统一制作。"))
	FWacomFirstPersonCardRetainSealStyleData Style;
};
