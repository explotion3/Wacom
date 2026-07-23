// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "WacomWorldShopOfferAnchorComponent.generated.h"

/** World Shop 中一个稳定的商品摆放槽。组件只描述表现位置，不持有 Offer 或 Run 状态。 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomWorldShopOfferAnchorComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UWacomWorldShopOfferAnchorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop",
		meta = (ToolTip = "槽位稳定标识。Host 内必须唯一；用于刷新时保持同一个世界卡牌身份。"))
	FName SlotId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop",
		meta = (ToolTip = "商品排序序号。数值较小的槽优先；Host 内必须唯一。"))
	int32 SlotOrder = 0;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|World Shop",
		meta = (ToolTip = "是否允许当前槽参与商品投影。关闭后不计入 Host 容量。"))
	bool bEnabledForOffers = true;
};
