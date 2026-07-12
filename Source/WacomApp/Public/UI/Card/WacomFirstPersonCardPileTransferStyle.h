// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardPileTransferStyle.generated.h"

/** Reusable playback/audio preset for discard-to-draw pixel glyph migration. */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomFirstPersonCardPileTransferStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ShowOnlyInnerProperties, ToolTip = "弃牌堆洗回抽牌堆时的牌印材质、运动节奏和可选音效；不保存规则顺序或 UI 锚点。"))
	FWacomFirstPersonCardPileTransferStyleData Style;
};
