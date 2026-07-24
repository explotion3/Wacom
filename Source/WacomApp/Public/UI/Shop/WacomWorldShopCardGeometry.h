// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * World Shop 世界商品的统一几何合同。
 *
 * RenderPlane 对应 WidgetComponent 的完整渲染/命中平面；VisibleProduct
 * 对应玩家实际看到的卡面与价格框，不包含两侧 overscan 和上下透明留白。
 * CardWorldScale 始终表示 Slate 渲染像素到世界厘米的绝对换算，不继承
 * Host 或地图实例的非均匀缩放。
 */
struct WACOMAPP_API FWacomWorldShopCardGeometry
{
	static constexpr int32 RenderDrawWidth = 720;
	static constexpr int32 RenderDrawHeight = 976;

	static constexpr float LogicalDesignWidth = 360.0f;
	static constexpr float LogicalDesignHeight = 488.0f;
	static constexpr float CardFaceWidth = 296.0f;
	static constexpr float CardFaceHeight = 420.0f;
	static constexpr float PriceFooterWidth = 296.0f;
	static constexpr float PriceFooterHeight = 52.0f;
	static constexpr float TopPadding = 8.0f;
	static constexpr float BottomPadding = 8.0f;

	static FIntPoint GetRenderDrawSize()
	{
		return FIntPoint(RenderDrawWidth, RenderDrawHeight);
	}

	static FVector2D GetRenderPlaneSizeCm(const float WorldScale)
	{
		return FVector2D(
			static_cast<float>(RenderDrawWidth) * WorldScale,
			static_cast<float>(RenderDrawHeight) * WorldScale);
	}

	static FVector2D GetVisibleProductSizeCm(const float WorldScale)
	{
		const float HorizontalLogicalToRenderScale =
			static_cast<float>(RenderDrawWidth) / LogicalDesignWidth;
		const float VerticalLogicalToRenderScale =
			static_cast<float>(RenderDrawHeight) / LogicalDesignHeight;
		return FVector2D(
			CardFaceWidth
				* HorizontalLogicalToRenderScale
				* WorldScale,
			(CardFaceHeight + PriceFooterHeight)
				* VerticalLogicalToRenderScale
				* WorldScale);
	}

	static float GetVisibleFooterHeightCm(const float WorldScale)
	{
		const float LogicalToRenderScale =
			static_cast<float>(RenderDrawHeight) / LogicalDesignHeight;
		return PriceFooterHeight * LogicalToRenderScale * WorldScale;
	}
};
