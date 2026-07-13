// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardHandTargetImpactStyle.generated.h"

/** Reusable playback preset for successful Battle hand-card target impacts. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardHandTargetImpactStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact", meta = (ShowOnlyInnerProperties, ToolTip = "手牌目标像素刻印的材质、预演节奏、实体压印运动与可选音效合同。"))
	FWacomFirstPersonCardHandTargetImpactStyleData Style;
};
