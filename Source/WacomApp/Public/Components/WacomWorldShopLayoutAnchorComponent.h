// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Components/WacomWorldShopOfferAnchorComponent.h"
#include "WacomWorldShopLayoutAnchorComponent.generated.h"

/**
 * 正式 World Shop 的可视化商品槽。
 *
 * 组件本身就是正式商店运行时使用的真实 Anchor；WacomEditor 仅为它绘制
 * 无碰撞卡牌线框，不创建 BodyInstance 或第二套隐藏 Anchor。
 */
UCLASS(ClassGroup = (Wacom), NotBlueprintable)
class WACOMAPP_API UWacomWorldShopLayoutAnchorComponent
	: public UWacomWorldShopOfferAnchorComponent
{
	GENERATED_BODY()

public:
	UWacomWorldShopLayoutAnchorComponent();

#if WITH_EDITOR
	virtual bool CanEditChange(const FProperty* InProperty) const override;
#endif

	FName GetSlotId() const { return SlotId; }
	int32 GetSlotOrder() const { return SlotOrder; }

	/** 返回完整 WidgetComponent 渲染/命中平面的绝对世界宽高，单位厘米。 */
	FVector2D GetCardPreviewSizeCm() const;

	/** 返回玩家可见卡面与价格框的绝对世界宽高，单位厘米。 */
	FVector2D GetVisibleProductPreviewSizeCm() const;

	/** 返回可见价格框的绝对世界高度，单位厘米。 */
	float GetVisibleFooterPreviewHeightCm() const;

	/** 仅由正式组合 Actor 在构造默认子对象时设置稳定槽身份。 */
	void ConfigureSlot(FName InSlotId, int32 InSlotOrder);
};
