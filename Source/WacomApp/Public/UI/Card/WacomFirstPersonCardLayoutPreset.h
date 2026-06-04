// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Engine/DataAsset.h"
#include "WacomFirstPersonCardLayoutPreset.generated.h"

/**
 * Reusable presentation preset for the first-person card layer.
 *
 * This asset only owns UI tuning values. Card face assets, debug toggles,
 * viewport ZOrder, and BattleHUD presentation mode remain on their owners.
 */
UCLASS(BlueprintType)
class WACOMAPP_API UWacomFirstPersonCardLayoutPreset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ToolTip = "第一人称卡牌层的手牌排布方式。Authored2D 是正式默认路径：只投影手牌中心点，再用 2D 参数排卡；LegacyProjectedFan2D 只保留为 PIE / debug 对照旧的每张卡牌 3D 槽位投影。"))
	EWacomFirstPersonCardLayoutMode CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "360.0", ToolTip = "Authored2D 模式下相邻卡牌的基础水平间距，单位为 UMG 布局像素。"))
	float AuthoredCardSpacingPixels = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "Authored2D 模式下整副手牌允许占用的最大宽度，单位为 UMG 布局像素；大于 0 时会自动压缩水平间距，0 表示不限制宽度。"))
	float AuthoredMaxHandWidthPixels = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (UIMin = "-600.0", UIMax = "600.0", ToolTip = "Authored2D 模式下整副手牌中心投影后的额外屏幕偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D AuthoredHandScreenOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "Authored2D 模式下中心卡牌额外上抬距离，单位为 UMG 布局像素；正值让中心卡牌更高，边缘卡牌逐渐减弱。"))
	float AuthoredCenterLiftPixels = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "6.0", ToolTip = "Authored2D 模式下边缘下坠曲线指数；数值越大，越靠近边缘的卡牌下坠越集中。"))
	float AuthoredDropCurveExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "6.0", ToolTip = "Authored2D 模式下扇形旋转曲线指数；1 表示线性，数值越大，中心卡牌更接近水平，边缘卡牌承担更多旋转。"))
	float AuthoredFanCurveExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ToolTip = "Authored2D 模式下是否让中心卡牌默认绘制在边缘卡牌之上；悬停和等待选目标的层级提升仍会优先生效。"))
	bool bAuthoredCenterCardsDrawOnTop = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (ToolTip = "Authored2D 模式下是否限制每张卡牌主体底部留在视口内。开启后手牌中心仍可柔性离屏，但最终卡牌主体不会因为贴近屏幕底部而裁掉 TypeName / 类型文字。"))
	bool bKeepAuthoredCardBodyBottomInViewport = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Authored Layout", meta = (EditCondition = "bKeepAuthoredCardBodyBottomInViewport", ClampMin = "0.0", UIMin = "0.0", UIMax = "96.0", ToolTip = "Authored2D 卡牌主体底部与视口底边之间保留的最小距离，单位为 UMG 布局像素；用于避免第一人称手牌底部类型文字被屏幕边缘裁掉。"))
	float AuthoredCardBodyBottomViewportPaddingPixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ToolTip = "第一人称卡牌层的投影模式。BodyLocked 是正式默认路径：锁定布局基准但仍使用当前真实相机投影；LegacyWorldProjected 只保留为旧 LookInfluence 漂移和视差实验的 PIE / debug 对照。"))
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "投影点被限制在视口内时保留的屏幕安全边距，单位为 UMG 布局像素。"))
	float ProjectionPadding = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ToolTip = "第一人称手牌投影点的视口限制方式。HardClamp 会强制留在屏幕内；SoftClamp 允许离屏一段距离后柔性拉回；AllowOffscreen 完全允许离屏。"))
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "SoftClamp 模式下允许手牌锚点离开视口安全区域的距离，单位为 UMG 布局像素；数值越大，手牌越像真实空间物体。"))
	float SoftClampOffscreenAllowancePixels = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "SoftClamp 模式下超过离屏允许范围后逐步拉回的过渡距离，单位为 UMG 布局像素；0 表示越界后立即停在软边界。"))
	float SoftClampBlendRangePixels = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ToolTip = "是否在投影、下坠、悬停和等待选目标位移后，把最终卡牌位置吸附到稳定网格。"))
	bool bEnableCardLayerPixelSnapping = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "8.0", ToolTip = "开启像素对齐时使用的 UMG 布局网格大小；1.0 表示吸附到整数 UMG 布局单位。"))
	float CardLayerPixelSnapGrid = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ToolTip = "是否限制第一人称卡牌层的 UMG 渲染旋转角；高对比卡面被整体旋转时容易出现锯齿。"))
	bool bClampCardLayerRenderAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0", Units = "deg", ToolTip = "开启旋转限制时，每张第一人称卡牌允许的最大 UMG 渲染旋转角，单位为度。"))
	float MaxCardLayerRenderAngleDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Scale And Fan", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "2.0", ToolTip = "第一人称卡牌层中每张卡牌使用的 UMG 渲染缩放。"))
	float StaticCardRenderScale = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Scale And Fan", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "260.0", ToolTip = "最外侧卡牌额外下坠的屏幕距离，单位为 UMG 布局像素；越靠近中心的卡牌下坠越少。"))
	float StaticCardEdgeDropPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Scale And Fan", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "每张卡牌相对手牌中心增加的扇形旋转角，单位为度；角度越大，旋转锯齿风险越高。"))
	float FanYawDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Motion Stability", meta = (ToolTip = "是否对 Authored2D 的整副手牌中心做屏幕空间平滑；用于保留空间上下变化的同时减少移动时的高频抖动。"))
	bool bEnableAnchorScreenSmoothing = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Motion Stability", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "60.0", ToolTip = "手牌中心屏幕空间平滑速度，单位为反秒；数值越低越稳但越滞后，0 表示立即贴合。"))
	float AnchorScreenSmoothingSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Motion Stability", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1200.0", ToolTip = "当手牌中心跳变距离超过该阈值时重置屏幕平滑，单位为 UMG 布局像素。"))
	float AnchorScreenSmoothingResetDistancePixels = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "是否启用第一人称卡牌槽的轻量视觉过渡；只影响 UMG 表现，不改变 hover/click/出牌流程。"))
	bool bEnableCardSlotMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽位置、角度和缩放追向目标布局的速度，单位为反秒；数值越高越跟手，0 表示立即贴合。"))
	float CardSlotMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽透明度追向目标透明度的速度，单位为反秒；数值越高淡入淡出越快，0 表示立即贴合。"))
	float CardSlotOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "新卡牌进入时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotEnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "新卡牌进入时的起始透明度；0 表示从完全透明淡入。"))
	float CardSlotEnterOpacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌离开手牌时相对当前位置的结束偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "卡牌离开手牌时保留 outgoing widget 的时长，单位为秒；0 表示立即移除。"))
	float CardSlotExitDuration = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "当卡牌槽目标位置跳变超过该距离时重置视觉过渡，单位为 UMG 布局像素。"))
	float CardSlotMotionResetDistancePixels = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "是否启用事件感知的第一人称卡牌转场；只根据表现 hint 改变入场 / 离场方向。"))
	bool bEnableEventAwareCardTransitions = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "是否启用更可读的转场来源解析；开启后抽牌 / 获得 / 打出 / 弃置可从手牌锚点或视口锚点移动，关闭则完全回到 V0-Q 的相对卡槽偏移。"))
	bool bEnableReadableTransitionOrigins = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "抽牌进入手牌时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D DrawnCardEnterOffsetPixels = FVector2D(0.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "抽牌入场的来源模式；默认从整副手牌中心下方进入，便于看出新卡先进入手牌再展开到目标槽位。"))
	EWacomFirstPersonCardTransitionOriginMode DrawnCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "抽牌入场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D DrawnCardEnterViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "抽牌入场起点相对目标卡牌缩放的倍率；只影响转场视觉起点，不改变最终卡牌布局。"))
	float DrawnCardEnterScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "抽牌入场起点相对目标卡牌角度的额外偏移，单位为度。"))
	float DrawnCardEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "战斗中获得卡牌进入手牌时相对目标位置的起始偏移，单位为 UMG 布局像素；默认从上方 / 战斗空间方向进入。"))
	FVector2D GainedCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "战斗中获得卡牌入场的来源模式；默认从整副手牌中心上方进入，强调来自战斗空间的奖励。"))
	EWacomFirstPersonCardTransitionOriginMode GainedCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "获得卡牌入场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D GainedCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "获得卡牌入场起点相对目标卡牌缩放的倍率；只影响转场视觉起点。"))
	float GainedCardEnterScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "获得卡牌入场起点相对目标卡牌角度的额外偏移，单位为度。"))
	float GainedCardEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌被打出时相对当前位置的离场偏移，单位为 UMG 布局像素；默认向上离开手牌。"))
	FVector2D PlayedCardExitOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "卡牌被打出时的离场来源模式；默认从当前卡牌视觉位置向上离开。"))
	EWacomFirstPersonCardTransitionOriginMode PlayedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "打出离场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D PlayedCardExitViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "打出离场终点相对当前卡牌缩放的倍率；只影响离场 outgoing 视觉。"))
	float PlayedCardExitScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "打出离场终点相对当前卡牌角度的额外偏移，单位为度。"))
	float PlayedCardExitAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌被弃置时相对当前位置的离场偏移，单位为 UMG 布局像素；默认向下离开手牌。"))
	FVector2D DiscardedCardExitOffsetPixels = FVector2D(0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ToolTip = "卡牌被弃置时的离场来源模式；默认从当前卡牌视觉位置向下离开。"))
	EWacomFirstPersonCardTransitionOriginMode DiscardedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "弃置离场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D DiscardedCardExitViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "弃置离场终点相对当前卡牌缩放的倍率；只影响离场 outgoing 视觉。"))
	float DiscardedCardExitScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Slot Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "弃置离场终点相对当前卡牌角度的额外偏移，单位为度。"))
	float DiscardedCardExitAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "正在等待目标选择的卡牌额外上浮距离，单位为 UMG 布局像素。"))
	float PendingTargetingLiftPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "正在等待目标选择的卡牌额外渲染缩放倍率。"))
	float PendingTargetingScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "正在等待目标选择的卡牌额外增加的 ZOrder 层级；用于确保 pending 卡绘制在普通手牌之上。"))
	int32 PendingTargetingZOrderBoost = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ToolTip = "等待目标选择时，是否让 pending 卡牌的 UMG 渲染角度向 0 度轻微归正；只影响表现，不改变手牌顺序或出牌逻辑。"))
	bool bPendingTargetingStraightenAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "pending 卡牌角度向 0 度归正的混合比例；0 表示保持原扇形角度，1 表示完全归正。"))
	float PendingTargetingAngleBlend = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ToolTip = "进入 TargetSelect 且存在 pending 卡时，是否轻微弱化其他第一人称手牌；只降低透明度，不改变布局、输入或战斗规则。"))
	bool bEnableTargetSelectHandDeemphasis = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "TargetSelect 中非 pending 手牌的透明度倍率；会与不可用卡透明度相乘，范围 0 到 1。"))
	float TargetSelectNonPendingOpacityMultiplier = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "手牌锚点卡牌使用的渲染缩放倍率。"))
	float HandAnchorScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "不可用卡牌在第一人称卡牌层上的整体透明度；卡面自身的 disabled overlay 仍由卡面数据控制。"))
	float DisabledRenderOpacity = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "鼠标悬停可打卡牌槽的额外上浮距离，单位为 UMG 布局像素。"))
	float HoverLiftPixels = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "鼠标悬停可打卡牌槽的额外渲染缩放倍率。"))
	float HoverScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Visual States", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "鼠标悬停可打卡牌槽额外增加的 ZOrder 层级。"))
	int32 HoverZOrderBoost = 500;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ToolTip = "是否启用第一人称手牌的轻量交互反馈；只影响 hover、按下、确认和不可用点击的 UMG 表现。"))
	bool bEnableCardInteractionFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ToolTip = "可打卡牌悬停时叠加的轻微颜色；通过 C++ overlay 表现，不要求修改卡面 WBP。"))
	FLinearColor PlayableHoverFeedbackColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.4", ToolTip = "可打卡牌悬停时颜色叠加的不透明度，范围 0 到 1；建议保持很低，避免盖住卡面。"))
	float PlayableHoverFeedbackOpacity = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.1", ToolTip = "左键按下可交互卡牌时额外乘上的缩放倍率；小于 1 会产生轻微按下感。"))
	float PressedFeedbackScale = 0.985f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ToolTip = "左键按下可交互卡牌时叠加的颜色。"))
	FLinearColor PressedFeedbackColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "左键按下可交互卡牌时颜色叠加的不透明度，范围 0 到 1。"))
	float PressedFeedbackOpacity = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "有效点击释放后确认反馈保留的时长，单位为秒；不延迟出牌或目标选择流程。"))
	float ConfirmFeedbackDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "有效点击释放后确认反馈的不透明度，范围 0 到 1。"))
	float ConfirmFeedbackOpacity = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.6", Units = "s", ToolTip = "点击不可打卡牌时拒绝反馈保留的时长，单位为秒；只消费第一人称卡槽点击，不提交战斗命令。"))
	float DenyFeedbackDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "32.0", ToolTip = "点击不可打卡牌时的横向抖动幅度，单位为 UMG 布局像素。"))
	float DenyFeedbackShakePixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ToolTip = "点击不可打卡牌时叠加的拒绝反馈颜色。"))
	FLinearColor DenyFeedbackColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Preset|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.6", ToolTip = "点击不可打卡牌时拒绝反馈的不透明度，范围 0 到 1。"))
	float DenyFeedbackOpacity = 0.18f;
};
