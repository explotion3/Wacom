// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardDrawRevealStyle.generated.h"

/** Reusable material and normalized timing preset for Battle Drawn card reveals. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardDrawRevealStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal", meta = (ShowOnlyInnerProperties, ToolTip = "Battle 抽牌牌背翻面的材质实例、归一化阶段与落定反馈合同；抽牌 Enter Transition 仍是唯一总时长。"))
	FWacomFirstPersonCardDrawRevealStyleData Style;
};
