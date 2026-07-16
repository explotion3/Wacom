// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "WacomFirstPersonCardAnchorComponent.generated.h"

class APlayerController;
class AWacomPlayerCharacter;
class UMaterialInterface;
class USoundBase;
class UWacomCardView;
class UWacomFirstPersonCardViewWidget;
class UWacomFirstPersonCardLayerWidget;
class UWacomFirstPersonCardHandTargetImpactStyle;
class UWacomFirstPersonCardDataRewriteStyle;
class UWacomFirstPersonCardDrawRevealStyle;
class UWacomFirstPersonCardPlayedDissolveStyle;
class UWacomFirstPersonCardPileTransferStyle;
class UWacomFirstPersonCardUseEffectStyle;
class UWacomFirstPersonCardSelectionStyle;
class UCardDefinition;
class FWacomFirstPersonCardLayerDelegateRouter;
class FWacomFirstPersonCardLayerOwner;
class FWacomFirstPersonCardAnchorRuntimeState;
struct FWacomFirstPersonCardLayerSlotView;
struct FWacomFirstPersonCardLayerTestAccess;
struct FWacomFirstPersonCardAccessibilityBridge;
struct FWacomFirstPersonCardPresentationScaleBridge;
struct FWacomLocalSettingsSnapshot;
enum class EWacomRuntimeSettingsChangeReason : uint8;

struct FWacomFirstPersonCardAnchorRuntimeStateDeleter
{
	void operator()(FWacomFirstPersonCardAnchorRuntimeState* State) const;
};

struct FWacomFirstPersonCardLayerOwnerDeleter
{
	void operator()(FWacomFirstPersonCardLayerOwner* Owner) const;
};

struct FWacomFirstPersonCardLayerDelegateRouterDeleter
{
	void operator()(FWacomFirstPersonCardLayerDelegateRouter* Router) const;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorTargetNative, const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&);

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomFirstPersonCardAnchorAutomationTestView
{
	float TargetPhysicalScale = 1.0f;
	float PresentationScale = 1.0f;
	float ResolvedHandCardRenderScale = 1.0f;
	float ResolvedCardSpacingPixels = 0.0f;
	float ResolvedHoverLiftPixels = 0.0f;
	float ResolvedDragPickupLiftPixels = 0.0f;
	float ResolvedDenyShakePixels = 0.0f;
	float ResolvedRetainedLiftPixels = 0.0f;
	FVector2D ResolvedDrawnEnterOffsetPixels = FVector2D::ZeroVector;
	float ResolvedDrawnEnterArcLiftPixels = 0.0f;
	FWacomFirstPersonCardPileTransferStyleData ResolvedPileTransferStyle;
	bool bHasValidAnchor = false;
	EWacomFirstPersonCardAnchorMode Mode = EWacomFirstPersonCardAnchorMode::Invalid;
	FTransform AnchorTransform = FTransform::Identity;
	FRotator LookOffsetUsed = FRotator::ZeroRotator;
	FRotator RawCursorLookOffset = FRotator::ZeroRotator;
	FRotator AppliedAnchorLookOffset = FRotator::ZeroRotator;
	float LookInfluenceYaw = 0.0f;
	float LookInfluencePitch = 0.0f;
	FName LastFallbackReason = NAME_None;
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	bool bLookOffsetAppliedToLayout = false;
	bool bLookResponsiveProjection = false;
	UWacomFirstPersonCardLayerWidget* CardLayerWidget = nullptr;
	int32 CardLayerConfigApplyCount = 0;
	FName PendingTransitionHintSourceId = NAME_None;
	TArray<FGuid> PendingTransitionHintCardIds;
	bool bHasPendingTransitionHintsForCurrentSource = false;
	bool bCanConsumePendingTransitionHintsForCurrentSource = false;
	bool bTransitionPresentationEnabledForCurrentSource = true;
	FName PresentationAnchorSourceId = NAME_None;
	FWacomFirstPersonCardPresentationAnchorSet PresentationAnchors;
};
#endif

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorDragNative, const FGuid&, const FWacomFirstPersonCardDragView&);
DECLARE_MULTICAST_DELEGATE_OneParam(FWacomFirstPersonCardLayerAnchorPointerNative, const FWacomFirstPersonCardPointerView&);
DECLARE_MULTICAST_DELEGATE(FWacomFirstPersonCardLayerAnchorPointerExitNative);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomFirstPersonCardLayerAnchorEnterTransitionStartedNative,
	const FWacomFirstPersonCardEnterTransitionStartedView&);
DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomFirstPersonCardLayerAnchorPileTransferProgressNative,
	const FWacomFirstPersonCardPileTransferProgressView&);

/**
 * Computes the first-person virtual card hand anchor used by the HUD-rendered
 * Battle / Run hand.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomFirstPersonCardAnchorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomFirstPersonCardAnchorComponent();
	virtual ~UWacomFirstPersonCardAnchorComponent() override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "600.0", Units = "cm", ToolTip = "第一人称锚点到虚拟手牌平面的距离，单位为 Unreal 厘米。"))
	float DistanceFromView = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (UIMin = "-240.0", UIMax = "120.0", Units = "cm", ToolTip = "虚拟手牌平面相对第一人称锚点的垂直偏移，单位为 Unreal 厘米；负值会让卡牌在画面中更低。"))
	float VerticalOffset = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (UIMin = "-240.0", UIMax = "240.0", Units = "cm", ToolTip = "虚拟手牌平面相对第一人称锚点的水平偏移，单位为 Unreal 厘米。"))
	float HorizontalOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", Units = "cm", ToolTip = "投影前相邻虚拟卡牌槽之间的距离，单位为 Unreal 厘米。"))
	float CardSpacing = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "每张卡牌相对第一人称手牌锚点增加的扇形偏航角，单位为度；角度越大，旋转锯齿风险越高。"))
	float FanYawDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "360.0", ToolTip = "Authored2D 模式下相邻卡牌的基础水平间距，单位为 UMG 布局像素。"))
	float AuthoredCardSpacingPixels = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "Authored2D 模式下整副手牌允许占用的最大宽度，单位为 UMG 布局像素；大于 0 时会自动压缩水平间距，0 表示不限制宽度。"))
	float AuthoredMaxHandWidthPixels = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (UIMin = "-600.0", UIMax = "600.0", ToolTip = "Authored2D 模式下整副手牌中心投影后的额外屏幕偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D AuthoredHandScreenOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "Authored2D 模式下中心卡牌额外上抬距离，单位为 UMG 布局像素；正值让中心卡牌更高，边缘卡牌逐渐减弱。"))
	float AuthoredCenterLiftPixels = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "6.0", ToolTip = "Authored2D 模式下边缘下坠曲线指数；数值越大，越靠近边缘的卡牌下坠越集中。"))
	float AuthoredDropCurveExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "6.0", ToolTip = "Authored2D 模式下扇形旋转曲线指数；1 表示线性，数值越大，中心卡牌更接近水平，边缘卡牌承担更多旋转。"))
	float AuthoredFanCurveExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ToolTip = "Authored2D 模式下是否让中心卡牌默认绘制在边缘卡牌之上；悬停和等待选目标的层级提升仍会优先生效。"))
	bool bAuthoredCenterCardsDrawOnTop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "Look Responsive Projected 中，共享鼠标镜头偏航偏移对手牌锚点的影响比例；BodyLocked 不使用该值影响手牌锚点。"))
	float LookInfluenceYaw = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "Look Responsive Projected 中，共享鼠标镜头俯仰偏移对手牌锚点的影响比例；BodyLocked 不使用该值影响手牌锚点。"))
	float LookInfluencePitch = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "第一人称卡牌锚点跟随目标位置和朝向的插值速度；设为 0 时立即贴合目标锚点。"))
	float FollowInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", ToolTip = "第一人称镜头 staging 期间以及 RunPath / BattleCamera / ViewStageBlend 交接帧的卡牌锚点跟随速度，单位为反秒；0 表示立即贴合目标镜头空间，避免 HUD 或 hand source 已刷新但锚点还在慢慢追。"))
	float CameraStageFollowInterpSpeed = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ToolTip = "第一人称卡牌层的投影模式。BodyLocked 是稳定默认风格：锁定布局基准但仍使用当前真实相机投影；Look Responsive Projected 会让鼠标镜头偏移参与手牌锚点计算以获得更强跟随感和视差。"))
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "投影点被限制在视口内时保留的屏幕安全边距，单位为 UMG 布局像素。"))
	float ProjectionPadding = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ToolTip = "第一人称手牌投影点的视口限制方式。HardClamp 会强制留在屏幕内；SoftClamp 允许离屏一段距离后柔性拉回；AllowOffscreen 完全允许离屏。"))
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "SoftClamp 模式下允许手牌锚点离开视口安全区域的距离，单位为 UMG 布局像素；数值越大，手牌越像真实空间物体。"))
	float SoftClampOffscreenAllowancePixels = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "SoftClamp 模式下超过离屏允许范围后逐步拉回的过渡距离，单位为 UMG 布局像素；0 表示越界后立即停在软边界。"))
	float SoftClampBlendRangePixels = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ToolTip = "是否对 Authored2D 的整副手牌中心做屏幕空间平滑；用于保留空间上下变化的同时减少移动时的高频抖动。"))
	bool bEnableAnchorScreenSmoothing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "60.0", ToolTip = "手牌中心屏幕空间平滑速度，单位为反秒；数值越低越稳但越滞后，0 表示立即贴合。"))
	float AnchorScreenSmoothingSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1200.0", ToolTip = "当手牌中心跳变距离超过该阈值时重置屏幕平滑，单位为 UMG 布局像素；用于切换场景、切换路线或传送时避免慢慢飘过去。"))
	float AnchorScreenSmoothingResetDistancePixels = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ToolTip = "是否启用第一人称卡牌槽的轻量视觉过渡；只影响 UMG 表现，不改变 hover、按下、拖拽或出牌流程。"))
	bool bEnableCardSlotMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽位置、角度和缩放追向目标布局的速度，单位为反秒；数值越高越跟手，0 表示立即贴合。"))
	float CardSlotMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽透明度追向目标透明度的速度，单位为反秒；数值越高淡入淡出越快，0 表示立即贴合。"))
	float CardSlotOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0", ToolTip = "卡牌槽位置、角度、缩放和透明度插值的缓动指数；1 表示保持当前线性手感，大于 1 会让每帧追踪更柔和，小于 1 会更快贴近目标。"))
	float CardSlotMotionEasePower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ToolTip = "是否单独覆盖悬浮卡牌的 motion profile。关闭时悬浮继续使用上方通用 CardSlotMotionSpeed / OpacitySpeed / EasePower。"))
	bool bOverrideHoverMotionProfile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideHoverMotionProfile", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "悬浮卡牌位置、角度和缩放追向目标表现的速度，单位为反秒；只在启用 Hover motion profile 覆盖时生效。"))
	float HoverMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideHoverMotionProfile", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "悬浮卡牌透明度追向目标透明度的速度，单位为反秒；只在启用 Hover motion profile 覆盖时生效。"))
	float HoverOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideHoverMotionProfile", ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0", ToolTip = "悬浮卡牌插值缓动指数；1 为线性，只在启用 Hover motion profile 覆盖时生效。"))
	float HoverMotionEasePower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ToolTip = "是否单独覆盖拖拽时当前手牌目标的运动参数；关闭时沿用通用槽位参数。"))
	bool bOverrideDragTargetFocusMotionProfile = false;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideDragTargetFocusMotionProfile", ToolTip = "手牌目标上浮、缩放追向目标值的速度，单位为反秒；推荐 18 到 32。"))
	float DragTargetFocusMotionSpeed = 26.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideDragTargetFocusMotionProfile", ToolTip = "手牌目标透明度追踪速度，单位为反秒；推荐 14 到 26。"))
	float DragTargetFocusOpacitySpeed = 18.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideDragTargetFocusMotionProfile", ToolTip = "手牌目标运动缓动指数；1 为线性，推荐 1 到 2。"))
	float DragTargetFocusMotionEasePower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ToolTip = "是否单独覆盖卡牌入场 / 离场的 motion profile。关闭时 Enter / Exit 继续使用通用 CardSlot motion 参数。"))
	bool bOverrideEnterExitMotionProfile = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideEnterExitMotionProfile", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌入场位置、角度和缩放追踪速度，单位为反秒；只在启用 Enter / Exit motion profile 覆盖时生效。"))
	float EnterMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideEnterExitMotionProfile", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌入场透明度追踪速度，单位为反秒；只在启用 Enter / Exit motion profile 覆盖时生效。"))
	float EnterOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideEnterExitMotionProfile", ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0", ToolTip = "卡牌入场插值缓动指数；1 为线性，只在启用 Enter / Exit motion profile 覆盖时生效。"))
	float EnterMotionEasePower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideEnterExitMotionProfile", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌离场位置、角度和缩放追踪速度，单位为反秒；只在启用 Enter / Exit motion profile 覆盖时生效。"))
	float ExitMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideEnterExitMotionProfile", ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌离场透明度追踪速度，单位为反秒；只在启用 Enter / Exit motion profile 覆盖时生效。"))
	float ExitOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (EditCondition = "bOverrideEnterExitMotionProfile", ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0", ToolTip = "卡牌离场插值缓动指数；1 为线性，只在启用 Enter / Exit motion profile 覆盖时生效。"))
	float ExitMotionEasePower = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "新卡牌进入时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotEnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "新卡牌进入时的起始透明度；0 表示从完全透明淡入。"))
	float CardSlotEnterOpacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌离开手牌时相对当前位置的结束偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "卡牌离开手牌时保留 outgoing widget 的时长，单位为秒；0 表示立即移除。"))
	float CardSlotExitDuration = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "兼容保留参数，单位为 UMG 布局像素；当前普通卡牌槽 reflow 不再根据距离阈值重置视觉过渡，真正需要瞬移的重同步后续应使用显式策略。"))
	float CardSlotMotionResetDistancePixels = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "是否启用事件感知的第一人称卡牌转场；只根据 BattleHUD 提供的表现 hint 改变入场 / 离场方向，不改变战斗规则或命令路径。"))
	bool bEnableEventAwareCardTransitions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "是否启用更可读的转场来源解析；开启后抽牌 / 获得 / 打出 / 弃置可从手牌锚点或视口锚点移动，关闭则完全回到 V0-Q 的相对卡槽偏移。"))
	bool bEnableReadableTransitionOrigins = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "抽牌进入手牌时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D DrawnCardEnterOffsetPixels = FVector2D(0.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "抽牌入场的来源模式；默认从整副手牌中心下方进入，便于看出新卡先进入手牌再展开到目标槽位。"))
	EWacomFirstPersonCardTransitionOriginMode DrawnCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "抽牌入场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下，单位为视口比例。"))
	FVector2D DrawnCardEnterViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "抽牌入场起点相对目标卡牌缩放的倍率；只影响转场视觉起点，不改变最终卡牌布局。"))
	float DrawnCardEnterScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "抽牌入场起点相对目标卡牌角度的额外偏移，单位为度；默认 0，避免额外增加旋转采样锯齿。"))
	float DrawnCardEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", Units = "s", ToolTip = "抽牌入场的固定播放时长，单位为秒；0 表示沿用普通入场速度追踪。"))
	float DrawnCardEnterDurationSeconds = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.2", Units = "s", ToolTip = "同一批抽牌中每张卡开始入场的错峰间隔，单位为秒；只影响表现顺序，不改变手牌顺序。"))
	float DrawnCardEnterStaggerSeconds = 0.075f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "抽牌入场中段向上的弧线抬升，单位为 UMG 布局像素；0 表示直线飞入。"))
	float DrawnCardEnterArcLiftPixels = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0", ToolTip = "抽牌入场固定播放的缓动指数；数值越大起落越柔和。"))
	float DrawnCardEnterEasePower = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "抽牌入场播放期间是否禁止该卡 hover / press / drag；用于避免发牌途中被交互状态打断。"))
	bool bBlockInteractionDuringDrawnCardEnter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场起点偏移，单位为 UMG 布局像素；默认从手牌中心上方进入。"))
	FVector2D GainedCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场来源模式；只影响表现。"))
	EWacomFirstPersonCardTransitionOriginMode GainedCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌使用视口锚点时的归一化位置。"))
	FVector2D GainedCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场起点缩放倍率；推荐 0.9 到 1.0。"))
	float GainedCardEnterScaleMultiplier = 0.96f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场起点角度偏移，单位为度。"))
	float GainedCardEnterAngleOffsetDegrees = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场时长，单位为秒；推荐 0.25 到 0.4。"))
	float GainedCardEnterDurationSeconds = 0.32f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "同批获得卡牌的错峰间隔，单位为秒；推荐 0.04 到 0.1。"))
	float GainedCardEnterStaggerSeconds = 0.075f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场弧线抬升，单位为 UMG 布局像素；推荐 24 到 64。"))
	float GainedCardEnterArcLiftPixels = 42.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场缓动指数；推荐 1.5 到 2.5。"))
	float GainedCardEnterEasePower = 2.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "获得卡牌入场播放期间是否阻止该卡交互。"))
	bool bBlockInteractionDuringGainedCardEnter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "左/右手牌生成入手时相对目标位置的起始偏移，单位为 UMG 布局像素；只影响 UI 表现，不改变抽牌或保留规则。"))
	FVector2D HandAnchorCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "左/右手牌生成入手的来源模式；默认从整副手牌中心上方进入，和普通抽牌方向区分开。"))
	EWacomFirstPersonCardTransitionOriginMode HandAnchorCardEnterOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "左/右手牌生成入手使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D HandAnchorCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "左/右手牌生成入手起点相对目标卡牌缩放的倍率；只影响转场视觉起点。"))
	float HandAnchorCardEnterScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "左/右手牌生成入手起点相对目标卡牌角度的额外偏移，单位为度。"))
	float HandAnchorCardEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", Units = "s", ToolTip = "左/右手牌生成入手的固定播放时长，单位为秒；0 表示沿用普通入场速度追踪。"))
	float HandAnchorCardEnterDurationSeconds = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.2", Units = "s", ToolTip = "同一批左/右手牌生成入手时每张卡开始的错峰间隔，单位为秒；只影响表现顺序。"))
	float HandAnchorCardEnterStaggerSeconds = 0.075f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "左/右手牌生成入手中段向上的弧线抬升，单位为 UMG 布局像素；0 表示直线飞入。"))
	float HandAnchorCardEnterArcLiftPixels = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.1", UIMin = "0.5", UIMax = "4.0", ToolTip = "左/右手牌生成入手固定播放的缓动指数；数值越大起落越柔和。"))
	float HandAnchorCardEnterEasePower = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "左/右手牌生成入手播放期间是否禁止该卡 hover / press / drag；用于避免生成途中被交互状态打断。"))
	bool bBlockInteractionDuringHandAnchorCardEnter = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (ToolTip = "是否启用语义入场音效；只在实际开始入场播放时请求一次。"))
	bool bEnableCardEnterSounds = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (EditCondition = "bEnableCardEnterSounds", ToolTip = "抽牌入场的 UI 2D 音效；留空表示静音。"))
	TSoftObjectPtr<USoundBase> DrawnCardEnterSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (EditCondition = "bEnableCardEnterSounds", ToolTip = "获得卡牌入场的 UI 2D 音效；留空表示静音。"))
	TSoftObjectPtr<USoundBase> GainedCardEnterSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (EditCondition = "bEnableCardEnterSounds", ToolTip = "Run 手牌入场的 UI 2D 音效；留空表示静音。"))
	TSoftObjectPtr<USoundBase> RunHandCardEnterSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (EditCondition = "bEnableCardEnterSounds", ToolTip = "左右手牌锚点入场的 UI 2D 音效；留空表示静音。"))
	TSoftObjectPtr<USoundBase> HandAnchorCardEnterSound;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (EditCondition = "bEnableCardEnterSounds", ToolTip = "入场音效音量倍率；1 为资产原始音量，推荐 0.5 到 1.2。"))
	float CardEnterSoundVolumeMultiplier = 1.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Transition Audio", meta = (EditCondition = "bEnableCardEnterSounds", ToolTip = "入场音效音高倍率；1 为资产原始音高，推荐 0.8 到 1.2。"))
	float CardEnterSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌被打出时相对当前位置的离场偏移，单位为 UMG 布局像素；默认向上离开手牌。"))
	FVector2D PlayedCardExitOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "卡牌被打出时的离场来源模式；默认从当前卡牌视觉位置向上离开。"))
	EWacomFirstPersonCardTransitionOriginMode PlayedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "打出离场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D PlayedCardExitViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "打出离场终点相对当前卡牌缩放的倍率；只影响离场 outgoing 视觉。"))
	float PlayedCardExitScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "打出离场终点相对当前卡牌角度的额外偏移，单位为度。"))
	float PlayedCardExitAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌被弃置时相对当前位置的离场偏移，单位为 UMG 布局像素；默认向下离开手牌。"))
	FVector2D DiscardedCardExitOffsetPixels = FVector2D(0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "卡牌被弃置时的离场来源模式；默认从当前卡牌视觉位置向下离开。"))
	EWacomFirstPersonCardTransitionOriginMode DiscardedCardExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "弃置离场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D DiscardedCardExitViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "弃置离场终点相对当前卡牌缩放的倍率；只影响离场 outgoing 视觉。"))
	float DiscardedCardExitScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "弃置离场终点相对当前卡牌角度的额外偏移，单位为度。"))
	float DiscardedCardExitAngleOffsetDegrees = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "0.0", UIMax = "0.2", Units = "s", ToolTip = "同一批弃牌中每张卡开始离场的错峰间隔，单位为秒；推荐 0.04-0.10，只影响表现顺序，不改变弃牌规则或手牌顺序。"))
	float DiscardedCardExitStaggerSeconds = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ToolTip = "是否在投影、边缘下坠、悬停上浮和等待选目标上浮后，把最终卡牌位置吸附到稳定网格；用于减少 UMG 旋转时的位置闪动。"))
	bool bEnableCardLayerPixelSnapping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "8.0", ToolTip = "开启像素对齐时使用的 UMG 布局网格大小；1.0 表示吸附到整数 UMG 布局单位。"))
	float CardLayerPixelSnapGrid = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ToolTip = "是否限制第一人称卡牌层的 UMG 渲染旋转角；高对比卡面被整体旋转时容易出现锯齿。"))
	bool bClampCardLayerRenderAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0", Units = "deg", ToolTip = "开启旋转限制时，每张第一人称卡牌允许的最大 UMG 渲染旋转角，单位为度。"))
	float MaxCardLayerRenderAngleDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "是否在第一人称卡牌层检测到槽位生命周期异常时输出简短日志；默认关闭，仅用于排查幽灵 Widget、outgoing 泄漏或重复槽位。"))
	bool bLogCardLayerMotionDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|01 Card View", meta = (ToolTip = "第一人称卡牌层使用的 Layer Widget 类；同时服务 Battle / Run runtime hand。空值时使用 C++ 默认层 Widget。"))
	TSubclassOf<UWacomFirstPersonCardLayerWidget> CardLayerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|01 Card View", meta = (ToolTip = "第一人称卡牌层使用的卡面包装 Widget 类；正式验证建议设置为 /Game/Wacom/UI/Card/WBP_FPCardView。为空时使用原生 UWacomFirstPersonCardViewWidget 调试视图。"))
	TSubclassOf<UWacomFirstPersonCardViewWidget> FirstPersonCardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "2.0", ToolTip = "第一人称 runtime hand 与开发预览中每张卡牌使用的 UMG 渲染缩放；只影响表现，不改变 Battle / Run 手牌数据。"))
	float HandCardRenderScale = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "260.0", ToolTip = "runtime hand 的 Authored2D 排布中，大手牌最外侧卡牌额外下坠的最大屏幕距离，单位为 UMG 布局像素；越靠近中心的卡牌下坠越少。"))
	float HandMaxEdgeDropPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ToolTip = "是否按当前手牌数量缩放最外侧卡牌下坠；开启后少牌使用较小下坠，大手牌逐渐过渡到 HandMaxEdgeDropPixels。"))
	bool bScaleEdgeDropByHandCount = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (EditCondition = "bScaleEdgeDropByHandCount", ClampMin = "0.0", UIMin = "0.0", UIMax = "260.0", ToolTip = "按手牌数量缩放边缘下坠时，小手牌使用的最外侧卡牌下坠距离，单位为 UMG 布局像素。"))
	float ShortHandEdgeDropPixels = 64.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (EditCondition = "bScaleEdgeDropByHandCount", ClampMin = "1", UIMin = "1", UIMax = "16", ToolTip = "手牌数量小于等于该值时，边缘下坠使用 ShortHandEdgeDropPixels。"))
	int32 EdgeDropScaleMinCardCount = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (EditCondition = "bScaleEdgeDropByHandCount", ClampMin = "1", UIMin = "1", UIMax = "32", ToolTip = "手牌数量大于等于该值时，边缘下坠使用 HandMaxEdgeDropPixels；中间数量平滑过渡。"))
	int32 EdgeDropScaleMaxCardCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|01 Card View", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称卡牌层 Widget 添加到 Viewport 时使用的层级；同时影响 Battle / Run runtime hand。"))
	int32 CardLayerZOrder = 9996;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "正在等待目标选择的卡牌额外上浮距离，单位为 UMG 布局像素。"))
	float PendingTargetingLiftPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "正在等待目标选择的卡牌额外渲染缩放倍率。"))
	float PendingTargetingScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "正在等待目标选择的卡牌额外增加的 ZOrder 层级；用于确保 pending 卡绘制在普通手牌之上。"))
	int32 PendingTargetingZOrderBoost = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ToolTip = "等待目标选择时，是否让 pending 卡牌的 UMG 渲染角度向 0 度轻微归正；只影响表现，不改变手牌顺序或出牌逻辑。"))
	bool bPendingTargetingStraightenAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "pending 卡牌角度向 0 度归正的混合比例；0 表示保持原扇形角度，1 表示完全归正。"))
	float PendingTargetingAngleBlend = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ToolTip = "进入 TargetSelect 且存在 pending 卡时，是否轻微弱化其他第一人称手牌；只降低透明度，不改变布局、输入或战斗规则。"))
	bool bEnableTargetSelectHandDeemphasis = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|08 Targeting State", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "TargetSelect 中非 pending 手牌的透明度倍率；会与不可用卡透明度相乘，范围 0 到 1。"))
	float TargetSelectNonPendingOpacityMultiplier = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "不可用卡牌在第一人称卡牌层上的整体透明度；卡面自身的 disabled overlay 仍由 FWacomCardViewData::bDisabled 控制。"))
	float DisabledRenderOpacity = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "兼容旧资产的默认交互开关；正式运行时由当前 first-person card layer source owner 调用 SetFirstPersonCardLayerInteractionEnabled 控制，不再只属于 BattleHand。"))
	bool bEnableBattleHandInteraction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "鼠标悬停的第一人称卡牌槽额外上浮距离，单位为 UMG 布局像素。"))
	float HoverLiftPixels = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "鼠标悬停的第一人称卡牌槽额外渲染缩放倍率。"))
	float HoverScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "鼠标悬停的第一人称卡牌槽额外增加的 ZOrder 层级。"))
	int32 HoverZOrderBoost = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "64.0", ToolTip = "第一人称手牌父层命中解析的悬停滞后距离，单位为 UMG 布局像素；用于避免鼠标在重叠卡牌分界线附近来回抖动。"))
	float HoverHitHysteresisPixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "是否启用 Hover / Drag 卡面透视倾斜；只影响 UMG 表现，不改变 296×420 的卡牌主体命中区域。"))
	bool bEnableCardFake3D = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "Hover 时卡面单轴最大倾角，单位为度；推荐 4 到 7 度，数值过大会影响读牌。"))
	float HoverCardFake3DMaxTiltDegrees = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "Drag 时由指针速度产生的单轴最大惯性倾角，单位为度；推荐 7 到 11 度。"))
	float DragCardFake3DMaxTiltDegrees = 9.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "按住但尚未拖拽时保留 Hover 倾角的倍率；推荐 0.2 到 0.5，0 表示按下立即压平。"))
	float PressedCardFake3DTiltMultiplier = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "传给 WBP Retainer Effect Material 的透视强度；推荐 0.08 到 0.18。材质未绑定时不会产生透视。"))
	float CardFake3DPerspectiveStrength = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "卡面倾斜和材质接触阴影追向交互目标的响应速度，单位为每秒；推荐 14 到 22，越高越跟手。"))
	float CardFake3DResponseSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "结束 Hover / Drag 后倾斜和材质接触阴影回到静止状态的速度，单位为每秒；推荐 10 到 18。"))
	float CardFake3DReturnSpeed = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "Drag 指针速度低通滤波速度，单位为每秒；推荐 12 到 20，用于抑制高频抖动。"))
	float CardFake3DDragVelocityFilterSpeed = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "Drag 达到最大倾角所需的指针速度，单位为 UMG 布局像素/秒；推荐 1000 到 1800。"))
	float CardFake3DVelocityForMaxTiltPixelsPerSecond = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "是否启用由 Retainer 实时卡面 Alpha 生成的材质接触阴影；它与卡面倾斜共用 Retainer，但可独立关闭。"))
	bool bEnableCardContactShadow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "Hover 状态传给材质接触阴影的抬升归一化值；0 表示紧贴轮廓，1 表示最软最淡端点。推荐 0.4 到 0.7。"))
	float CardHoverContactShadowLift = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "Drag 状态传给材质接触阴影的抬升归一化值；0 表示紧贴轮廓，1 表示最软最淡端点。推荐 0.85 到 1.0。"))
	float CardDragContactShadowLift = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "卡牌达到最大参考倾角时，实时 Alpha 接触阴影沿倾斜反方向增加的最大位移，单位为 UMG 逻辑像素；推荐 6 到 12，默认 10。0 会关闭倾斜位移，但保留静止与抬升阴影；不影响布局或命中区域。"))
	float CardContactShadowTiltOffsetPixels = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "实时 Alpha 接触阴影的不透明度倍率；1 使用材质原始强度，推荐 0.8 到 2.0，默认 1.5。数值越高阴影越清晰，但过高会形成生硬黑边；不影响卡牌布局、命中或倾斜幅度。"))
	float CardContactShadowOpacityMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "是否让卡面插画、实体边框和稀有度饰条根据 Hover / Drag 倾角产生分层 UV 视差。仅影响表现，不改变卡牌命中区域。"))
	bool bEnableCardSurfaceParallax = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "卡面分层 UV 视差的整体强度倍率；1 使用材质实例中的 authored 深度，推荐 0.65 到 1.35。0 会关闭材质层间位移，但不关闭外层 Fake-3D。"))
	float CardSurfaceParallaxStrength = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "实体出血装饰在最大参考倾角时的目标位移，单位为 UMG 布局像素；推荐 3 到 7。不会改变布局或命中区域。"))
	float AttachmentParallaxDepthPixels = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "实体出血装饰视差位移的安全上限，单位为 UMG 布局像素；推荐 5 到 10，用于避免装饰滑出 Retainer bleed。"))
	float AttachmentParallaxMaxOffsetPixels = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Card Depth", meta = (ToolTip = "是否弱化卡面分层视差运动。开启后内层材质和实体出血装饰保持零位移，但外层 Fake-3D 与接触阴影仍按各自开关工作。"))
	bool bReduceCardSurfaceParallaxMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Card Use Effect", meta = (ToolTip = "是否让普通使用并离开手牌的卡牌播放原地 Surface Effect；只改变 outgoing 表现，不改变规则结算。"))
	bool bEnableCardUseEffect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Card Use Effect", meta = (ToolTip = "普通使用卡牌的可复用播放预设；视觉颜色、菱形密度和波宽在预设引用的材质实例中调整。为空或资源无效时回退旧 Played 空间离场。"))
	TObjectPtr<UWacomFirstPersonCardUseEffectStyle> CardUseEffectStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Card Use Effect", meta = (ToolTip = "弱化普通使用效果：使用约 0.12 秒均匀淡出，不播放中心向外的菱形波，但仍允许一次性音效。"))
	bool bReduceCardUseEffectMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Card Use Effect", meta = (Units = "s", ToolTip = "普通使用 Surface Effect 总时长覆盖，单位为秒；负值表示使用 Style，默认 0.36，推荐 0.28 到 0.46。"))
	float CardUseEffectDurationOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|13 Card Exhausted Dissolve", meta = (DisplayName = "Enable Card Exhausted Dissolve", ToolTip = "是否让实际进入 Exhaust 的卡牌使用现有 PixelAsh / OrderedDither 消散；旧 C++ 属性名为 PlayedDissolve，仅为保护既有资产引用。"))
	bool bEnableCardPlayedDissolve = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|13 Card Exhausted Dissolve", meta = (DisplayName = "Card Exhausted Dissolve Style", ToolTip = "实际消耗卡牌使用的既有消散预设；为空或资源无效时安全回退弃牌方向空间离场。"))
	TObjectPtr<UWacomFirstPersonCardPlayedDissolveStyle> CardPlayedDissolveStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|13 Card Exhausted Dissolve", meta = (DisplayName = "Reduce Card Exhausted Dissolve Motion", ToolTip = "弱化消耗消散动态：使用约 0.12 秒均匀淡出，不播放方向前沿或残片漂移，但仍允许一次性音效。"))
	bool bReduceCardPlayedDissolveMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|13 Card Exhausted Dissolve", meta = (DisplayName = "Card Exhausted Dissolve Duration Override", Units = "s", ToolTip = "消耗消散总时长覆盖，单位为秒；负值表示使用 Style，推荐 0.32 到 0.48。"))
	float CardPlayedDissolveDurationOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|14 Card Pile Transfer", meta = (ToolTip = "是否在 Battle 抽牌堆耗尽、弃牌堆整体洗回时播放逐张像素牌印迁移；只改变表现，不改变洗牌或抽牌顺序。"))
	bool bEnableCardPileTransfer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|14 Card Pile Transfer", meta = (ToolTip = "是否让 Battle 中真正的 CardDiscarded 先在原位收束成牌印，再飞入弃牌堆；关闭后保留旧的空间弃牌离场。不会影响自然打出、Exhaust 或洗牌。"))
	bool bEnableCardDiscardGlyphTransfer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|14 Card Pile Transfer", meta = (ToolTip = "弃牌堆洗回抽牌堆的可复用牌印预设；材质实例控制牌印外观，Style 控制尺寸、弧线、时长和可选音效。为空时立即完成表现并继续后续抽牌。"))
	TObjectPtr<UWacomFirstPersonCardPileTransferStyle> CardPileTransferStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|14 Card Pile Transfer", meta = (ToolTip = "弱化洗回动态：使用约 0.18 秒的源牌堆静态牌印与目标收束闪光，不执行跨屏弧线飞行。"))
	bool bReduceCardPileTransferMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|15 Card Hand Target Impact", meta = (ToolTip = "是否让 Battle 中规则有效的手牌目标显示弱刻印预演，并在成功提交后播放一次像素压印反馈；只改变表现，不改变目标验证或命中区域。"))
	bool bEnableCardHandTargetImpact = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|15 Card Hand Target Impact", meta = (ToolTip = "手牌目标像素刻印的可复用预设；材质实例控制颜色和图案，DataAsset 控制节奏、实体运动与可选音效。为空或材质无效时立即沿用原离场。"))
	TObjectPtr<UWacomFirstPersonCardHandTargetImpactStyle> CardHandTargetImpactStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|15 Card Hand Target Impact", meta = (ToolTip = "弱化手牌目标刻印动态：预演变为静态弱刻印，成功提交只播放约 0.12 秒透明度闪现，不改变目标卡缩放或位移。"))
	bool bReduceCardHandTargetImpactMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|15 Card Hand Target Impact", meta = (Units = "s", ToolTip = "有效目标预演呼吸周期覆盖，单位为秒；负值表示使用 Style，默认 0.90，推荐 0.70 到 1.30。"))
	float CardHandTargetImpactPreviewPeriodOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|15 Card Hand Target Impact", meta = (Units = "s", ToolTip = "成功刻印完整播放时长覆盖，单位为秒；负值表示使用 Style，默认 0.29，推荐 0.24 到 0.38。"))
	float CardHandTargetImpactCommitDurationOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|16 Card Data Rewrite", meta = (ToolTip = "是否让 Battle 中真实可见的卡牌费用变化播放局部像素重写；只改变表现，不修改 RuntimeCost、命中区域或卡牌布局。"))
	bool bEnableCardDataRewrite = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|16 Card Data Rewrite", meta = (ToolTip = "费用数字消散重组的可复用预设；材质实例只作用于 CostDigitImage，DataAsset 控制时序、回弹、错峰与可选音效。为空或材质无效时费用立即刷新但不播放效果。"))
	TObjectPtr<UWacomFirstPersonCardDataRewriteStyle> CardDataRewriteStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|16 Card Data Rewrite", meta = (ToolTip = "弱化费用数字动态：使用约 0.12 秒旧、新数字交叉淡化与静态轮廓短闪，不播放逐格消散、重组或缩放回弹。"))
	bool bReduceCardDataRewriteMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|16 Card Data Rewrite", meta = (Units = "s", ToolTip = "费用数字消散重组总时长覆盖，单位为秒；负值表示使用 Style，默认 0.34，推荐 0.28 到 0.42。"))
	float CardDataRewriteDurationOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|98 Experimental Surface Effect", meta = (ToolTip = "实验性像素棱镜 Surface Effect 制作开关；当前拖拽流程不会激活它，默认关闭。后续只允许由明确的卡面数据更新或升级表现语义驱动。"))
	bool bEnableCardSelectionEffect = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|98 Experimental Surface Effect", meta = (ToolTip = "实验性像素棱镜预设；当前 Fake3D 实时材质不消费该资源，保留给未来卡面更新效果。"))
	TObjectPtr<UWacomFirstPersonCardSelectionStyle> CardSelectionStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|98 Experimental Surface Effect", meta = (ToolTip = "实验性像素棱镜的弱化动态标记；当前拖拽流程不读取该参数。"))
	bool bReduceCardSelectionMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|98 Experimental Surface Effect", meta = (ToolTip = "实验性像素棱镜进入时长覆盖，单位为秒；当前拖拽流程不读取该参数。"))
	float CardSelectionEnterDurationOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|98 Experimental Surface Effect", meta = (ToolTip = "实验性像素棱镜退出时长覆盖，单位为秒；当前拖拽流程不读取该参数。"))
	float CardSelectionExitDurationOverrideSeconds = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|17 Camera Look While UI", meta = (ToolTip = "Inspect 或拖拽第一人称卡牌时，是否让当前 Battle / Run 第一人称镜头持续跟随鼠标偏转。只影响镜头表现，不改变鼠标捕获、Inspect 滑选、目标校验或出牌结果。"))
	bool bAllowCameraLookDuringCardDrag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|17 Camera Look While UI", meta = (ToolTip = "Inspect / Drag 期间传给当前 first-person cursor look 的强度倍率；1 表示使用镜头自身 LookYawScale / LookPitchScale，0 表示不推动镜头。推荐 0.35 到 1.0，不影响卡牌布局。"))
	float CardDragCameraLookScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|17 Camera Look While UI", meta = (ToolTip = "Inspect / Drag 期间镜头追向鼠标的插值速度覆盖值，单位为每秒；小于 0 时沿用当前镜头 LookInterpSpeed，0 表示立即贴合。推荐 -1 或 8 到 18。"))
	float CardDragCameraLookInterpSpeedOverride = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|17 Camera Look While UI", meta = (ToolTip = "Hover 第一人称卡牌时，是否让当前 Battle / Run 第一人称镜头跟随鼠标轻微偏转。进入 Inspect / Drag 后由 Card Drag Camera Look 参数继续接管。"))
	bool bAllowCameraLookDuringCardPointer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|17 Camera Look While UI", meta = (ToolTip = "Hover 卡牌时传给当前第一人称 cursor look 的强度倍率；1 表示使用镜头自身的 LookYawScale / LookPitchScale，0 表示不推动镜头。推荐 0.35 到 1.0，不影响卡牌布局。"))
	float CardPointerCameraLookScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|17 Camera Look While UI", meta = (ToolTip = "Hover 卡牌时镜头追向鼠标的插值速度覆盖值，单位为每秒；小于 0 时沿用当前镜头的 LookInterpSpeed，0 表示立即贴合。推荐 -1 或 8 到 18。"))
	float CardPointerCameraLookInterpSpeedOverride = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|18 Card Draw Reveal", meta = (ToolTip = "是否让 Battle 中真实 Drawn 卡牌以牌背飞出抽牌堆，并在飞行中翻成正面；Run 入场、获得卡牌和手牌锚点入场不受影响。"))
	bool bEnableCardDrawReveal = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|18 Card Draw Reveal", meta = (ToolTip = "Battle 抽牌牌背翻面的可复用预设；材质实例控制牌背与边缘外观，DataAsset 控制归一化翻面阶段和落定反馈。为空或材质无效时沿用原 Drawn 入场。"))
	TObjectPtr<UWacomFirstPersonCardDrawRevealStyle> CardDrawRevealStyle;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|18 Card Draw Reveal", meta = (ToolTip = "弱化抽牌翻面动态：保留原 Drawn 飞行、计数和声音，只在飞行约 55% 到 75% 之间让牌背均匀交叉淡化到正面，不执行横向压缩或落定位移。"))
	bool bReduceCardDrawRevealMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "是否启用第一人称手牌的轻量交互反馈；只影响 hover、按下、确认和不可用点击的 UMG 表现，不改变出牌命令路径。"))
	bool bEnableCardInteractionFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "可打卡牌悬停时叠加的轻微颜色；通过 C++ overlay 表现，不要求修改卡面 WBP。"))
	FLinearColor PlayableHoverFeedbackColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.4", ToolTip = "可打卡牌悬停时颜色叠加的不透明度，范围 0 到 1；建议保持很低，避免盖住卡面。"))
	float PlayableHoverFeedbackOpacity = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.1", ToolTip = "左键按下可交互卡牌时额外乘上的缩放倍率；小于 1 会产生轻微按下感。"))
	float PressedFeedbackScale = 0.985f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "左键按下可交互卡牌时叠加的颜色。"))
	FLinearColor PressedFeedbackColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "左键按下可交互卡牌时颜色叠加的不透明度，范围 0 到 1。"))
	float PressedFeedbackOpacity = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "是否在卡牌首次进入正式拖拽时播放一次拾牌反馈；只影响局部缩放、上提和 2D 音效，不改变拖拽、瞄准或提交规则。"))
	bool bEnableCardDragPickupFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (Units = "s", ToolTip = "拾牌反馈总时长，单位为秒；推荐 0.10 到 0.18。只作用于 UMG RenderTransform，不阻塞输入。"))
	float CardDragPickupDurationSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (Units = "s", ToolTip = "拾牌反馈快速建立到峰值的时长，单位为秒；推荐 0.01 到 0.04，运行时不会超过总时长。"))
	float CardDragPickupRiseSeconds = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "拾牌瞬间额外上提距离，单位为 UMG 布局像素；推荐 8 到 18。它是短时局部偏移，不改变手牌布局或命中区域。"))
	float CardDragPickupLiftPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "拾牌瞬间额外缩放倍率；推荐 1.015 到 1.05，会与现有正式拖拽源卡缩放相乘，不改变命中区域。"))
	float CardDragPickupScaleMultiplier = 1.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "弱化拾牌动态：开启后取消额外上提和缩放，但仍保留现有正式拖拽姿态并播放拾牌音效。"))
	bool bReduceCardDragPickupMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "正式拖拽开始时播放的短促实体纸牌音，建议使用 80 到 140 毫秒的纸张摩擦加轻微卡边扣响；为空时静默跳过。使用硬引用以避免交互瞬间同步加载。"))
	TObjectPtr<USoundBase> CardDragPickupSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "拾牌音效音量倍率；推荐 0.6 到 1.1，只影响本地 2D UI 音效。"))
	float CardDragPickupSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "拾牌音效基础音高倍率；推荐 0.9 到 1.1。"))
	float CardDragPickupSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback|Drag Pickup", meta = (ToolTip = "每次拾牌音效相对基础音高的随机浮动比例；0.03 表示约正负 3%，推荐 0 到 0.06。"))
	float CardDragPickupSoundPitchVariation = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "有效点击释放后确认反馈保留的时长，单位为秒；不延迟出牌或目标选择流程。"))
	float ConfirmFeedbackDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "有效点击释放后确认反馈的不透明度，范围 0 到 1。"))
	float ConfirmFeedbackOpacity = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.6", Units = "s", ToolTip = "点击不可打卡牌时拒绝反馈保留的时长，单位为秒；只消费第一人称卡槽点击，不提交战斗命令。"))
	float DenyFeedbackDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "32.0", ToolTip = "点击不可打卡牌时的横向抖动幅度，单位为 UMG 布局像素。"))
	float DenyFeedbackShakePixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "点击不可打卡牌时叠加的拒绝反馈颜色。"))
	FLinearColor DenyFeedbackColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.6", ToolTip = "点击不可打卡牌时拒绝反馈的不透明度，范围 0 到 1。"))
	float DenyFeedbackOpacity = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "Pressed / Confirm / Commit / Deny 源卡交互反馈使用的 UI 材质；为空时优先使用 WBP InteractionFeedbackImage 自带材质，没有材质时 pressed/confirm/commit 退化为普通 tint，deny 只保留横向抖动。可手动指定 /Game/DreamMaterials/Card/M_FirstPersonCard_FeedbackEdge。"))
	TSoftObjectPtr<UMaterialInterface> InteractionFeedbackMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.16", ToolTip = "源卡交互反馈材质的四边高亮宽度，UV 单位；数值越大边框越粗。"))
	float InteractionFeedbackEdgeWidth = 0.048f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.12", ToolTip = "源卡交互反馈材质向内淡出的柔和宽度，UV 单位；数值越大边缘越软。"))
	float InteractionFeedbackEdgeSoftness = 0.024f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "源卡交互反馈材质的暗角强度；0 表示只显示边框，不显示暗角。"))
	float InteractionFeedbackVignetteStrength = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.2", ToolTip = "源卡交互反馈材质暗角开始出现的中心距离，UV 距离；数值越小暗角越靠近中心。"))
	float InteractionFeedbackVignetteRadius = 0.58f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.8", ToolTip = "源卡交互反馈材质暗角淡入柔和度；数值越大暗角过渡越缓。"))
	float InteractionFeedbackVignetteSoftness = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "是否在战斗卡牌成功提交后播放第一人称手牌 commit 脉冲；只影响 UMG 表现，不延迟或改变 BattleSession 命令。"))
	bool bEnablePlayCommitFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "成功提交出牌后 commit 反馈保留时长，单位为秒。"))
	float PlayCommitFeedbackDuration = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "成功提交出牌后 commit 反馈颜色叠加的不透明度，范围 0 到 1。"))
	float PlayCommitFeedbackOpacity = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "成功提交出牌后 commit 反馈使用的颜色；用于和普通点击确认区分。"))
	FLinearColor PlayCommitFeedbackColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.2", ToolTip = "成功提交出牌后 commit 反馈额外乘上的缩放倍率；只作用于视觉 slot。"))
	float PlayCommitFeedbackScale = 1.015f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "是否启用回合结束保留牌的纯运动反馈；不使用旧 Overlay 发光。"))
	bool bEnableRetainedFeedback = true;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "保留牌运动反馈时长，单位为秒；推荐 0.22 到 0.34。"))
	float RetainedFeedbackDuration = 0.28f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "多张保留牌的错峰间隔，单位为秒；推荐 0.03 到 0.07。"))
	float RetainedFeedbackStaggerSeconds = 0.045f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "保留反馈额外上浮距离，单位为 UMG 布局像素；推荐 8 到 18。"))
	float RetainedFeedbackLiftPixels = 12.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "保留反馈额外缩放倍率；推荐 1.015 到 1.04。"))
	float RetainedFeedbackScale = 1.025f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|10 Interaction Feedback", meta = (ToolTip = "保留反馈期间额外增加的绘制层级；推荐 100 到 300。"))
	int32 RetainedFeedbackZOrderBoost = 180;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "是否启用第一人称手牌按住读牌、拖出手牌或拉箭头提交。关闭后只保留 hover / 读牌表现，不再提交卡牌。"))
	bool bEnableFirstPersonCardDragCommit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "按住卡牌多久后进入 Inspect 读牌姿态，单位为秒；推荐 0.10 到 0.20 秒。快速松开不会出牌。"))
	float CardInspectHoldDelaySeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "按下后鼠标移动超过该距离才进入拖出/瞄准状态，单位为 UMG 布局像素。"))
	float CardDragStartThresholdPixels = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "Inspect 滑选区域在整副可交互手牌主体包围盒外额外扩展的 X/Y 距离，单位为 UMG 布局像素；区域内可切换 Inspect 卡或穿过牌缝，离开后才允许最后 Inspect 卡升级为拖拽。推荐 X=24‑48、Y=32‑64，不改变手牌视觉布局。"))
	FVector2D CardInspectScrubHandPaddingPixels = FVector2D(32.0f, 48.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "480.0", ToolTip = "Battle 无目标卡向上拖出超过该距离后进入可提交状态，单位为 UMG 布局像素；推荐 110 到 180，释放时才真正提交。"))
	float NoTargetCardDragOutCommitDistancePixels = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "Battle 无目标卡拖出提交的方向；当前正式合同只支持向上，避免横向整理或查看时误触。"))
	EWacomFirstPersonCardDragOutDirection NoTargetCardDragOutDirection =
		EWacomFirstPersonCardDragOutDirection::Up;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "Inspect 姿态中卡牌移动到的视口归一化位置；X/Y 为 0 到 1，默认靠近屏幕中央略偏上。"))
	FVector2D CardInspectScreenPosition = FVector2D(0.5f, 0.46f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "2.0", ToolTip = "Inspect 姿态中卡牌额外乘上的缩放倍率；推荐 1.10 到 1.25，只影响视觉，不扩大主体命中。"))
	float CardInspectScale = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "Inspect 姿态是否继续显示第一人称卡牌详情面板；只影响表现，不改变 Battle / Run 命令路径。"))
	bool bShowDetailDuringCardInspect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "有目标卡拖动时是否绘制从源卡到鼠标的 C++ UMG 箭头线。"))
	bool bEnableAimArrow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "是否输出第一人称卡牌拖拽/瞄准诊断日志；默认关闭，仅用于排查手势状态，不改变拖拽语义。"))
	bool bLogCardDragDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽指向手牌目标时的额外上浮距离，单位为 UMG 布局像素；推荐 12 到 24。"))
	float DragCardTargetFocusLiftPixels = 18.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽指向手牌目标时的额外缩放倍率；推荐 1.025 到 1.06。"))
	float DragCardTargetFocusScale = 1.045f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽指向手牌目标时额外增加的绘制层级；推荐 400 到 900。"))
	int32 DragCardTargetFocusZOrderBoost = 650;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Hand|99 Debug")
	void RefreshAnchor(float DeltaTime);

	void RefreshCardLayerNow(float DeltaTime = 0.0f);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	FTransform GetCurrentAnchorTransform() const { return CurrentAnchorTransform; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	FTransform ComputeCardTransform(int32 NumCards, int32 CardIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Hand|99 Debug")
	bool ProjectCardTransformToScreen(
		const FTransform& CardTransform,
		FWacomFirstPersonCardProjectedPoint& OutProjectedPoint,
		int32 PointIndex = -1) const;

	TArray<FWacomFirstPersonCardLayerSlotView> BuildActiveCardLayerSlotViews() const;

	void CommitRuntimeCardLayerFrame(
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);
	void ApplyRuntimeCardLayerSourceLifecycleFrame(
		const FWacomFirstPersonCardLayerSourceLifecycleFrame& Frame);

	bool HasRuntimeCardLayerPendingPresentationFrame(FName SourceId) const;
	bool HasActiveCardLayerPresentationPlayback() const;
	bool HasHandTargetImpactReachedPeak(const FGuid& CardInstanceId) const;
	void ForceSettleCardLayerPresentationPlayback();

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	bool HasRuntimeCardLayerData() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	FName GetRuntimeCardLayerSourceId() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	int32 GetRuntimeCardLayerCardCount() const;

	const TArray<FWacomCardViewData>& GetRuntimeCardLayerData() const;
	const TArray<FWacomFirstPersonCardLayerEntry>& GetRuntimeCardLayerEntries() const;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|09 Gesture")
	bool IsFirstPersonCardLayerInteractionEnabled() const
	{
		return bFirstPersonCardLayerInteractionEnabled;
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "兼容旧 Blueprint 的 BattleHand 命名 getter；新代码请使用 IsFirstPersonCardLayerInteractionEnabled。"))
	bool IsBattleHandInteractionEnabled() const
	{
		return IsFirstPersonCardLayerInteractionEnabled();
	}

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|07 Hover")
	FGuid GetHoveredCardInstanceId() const;

	/** 从当前悬停的第一人称手牌卡牌构建统一交互目标 handle。无悬停卡时返回无效 handle。 */
	FWacomInteractionTargetHandle BuildCardTargetHandle() const;
	void SetFirstPersonCardDragFeedbackTarget(
		const FWacomInteractionTargetHandle& TargetHandle,
		bool bValidTarget,
		EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
			EWacomFirstPersonCardDragTargetFeedbackState::None,
		const TOptional<FVector2D>& FeedbackTargetScreenPosition = TOptional<FVector2D>(),
		const FString& ResolvedIntentDebugSummary = FString(),
		const TArray<FWacomFirstPersonCardTargetAffordance>& CardTargetAffordances =
			TArray<FWacomFirstPersonCardTargetAffordance>());
	void CancelFirstPersonCardDragGesture(bool bBroadcastCancel);
	bool TryStartFirstPersonCardDragGesture(const FGuid& CardInstanceId);
	bool TryStartFirstPersonCardDragGesture(
		const FGuid& CardInstanceId,
		const TOptional<FVector2D>& InitialPointerWidgetPosition);
	bool UpdateFirstPersonCardDragPointer(const FVector2D& WidgetPosition);
	bool ReleaseFirstPersonCardDragGesture(const FVector2D& WidgetPosition);
	bool ReleaseFirstPersonCardDragGestureAtCurrentPointer();
	bool IsFirstPersonCardDragGestureActive() const;
	bool IsFirstPersonCardKeyboardShortcutDragGestureActive() const;

	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardHovered;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardUnhovered;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerHoveredCardLayoutUpdated;
	FWacomFirstPersonCardLayerAnchorTargetNative OnFirstPersonCardLayerCardTargetHovered;
	FWacomFirstPersonCardLayerAnchorTargetNative OnFirstPersonCardLayerCardTargetUnhovered;
	FWacomFirstPersonCardLayerAnchorTargetNative OnFirstPersonCardLayerHoveredCardTargetUpdated;
	FWacomFirstPersonCardLayerAnchorDragNative OnFirstPersonCardLayerDragStarted;
	FWacomFirstPersonCardLayerAnchorDragNative OnFirstPersonCardLayerDragUpdated;
	FWacomFirstPersonCardLayerAnchorDragNative OnFirstPersonCardLayerDragReleased;
	FWacomFirstPersonCardLayerAnchorDragNative OnFirstPersonCardLayerDragCancelled;
	FWacomFirstPersonCardLayerAnchorPointerNative OnFirstPersonCardLayerPointerMoved;
	FWacomFirstPersonCardLayerAnchorPointerExitNative OnFirstPersonCardLayerPointerLeft;
	FWacomFirstPersonCardLayerAnchorEnterTransitionStartedNative OnFirstPersonCardLayerEnterTransitionStarted;
	FWacomFirstPersonCardLayerAnchorPileTransferProgressNative OnFirstPersonCardLayerPileTransferProgress;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	virtual bool ResolveCameraTransformForAnchor(FTransform& OutCameraTransform) const;
	virtual bool ProjectWorldLocationForAnchor(const FVector& WorldLocation, FVector2D& OutScreenPosition) const;
	virtual bool ProjectWorldLocationToWidgetPositionForAnchor(
		const FVector& WorldLocation,
		FVector2D& OutWidgetPosition,
		FVector2D& OutRawScreenPosition) const;
	virtual bool GetViewportSizeForAnchor(FVector2D& OutViewportSize) const;
	virtual float GetViewportScaleForAnchor() const;
	virtual float GetAnchorSmoothingDeltaTimeForAnchor() const;
	virtual bool CanCreateCardLayerForAnchor(APlayerController* PlayerController) const;
	virtual UWacomFirstPersonCardLayerWidget* CreateCardLayerWidgetForAnchor(
		APlayerController* PlayerController,
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const;
	virtual void AddCardLayerWidgetToViewportForAnchor(
		UWacomFirstPersonCardLayerWidget* LayerWidget,
		int32 ZOrder) const;
	void UpdateCardLayer();

private:
#if WITH_AUTOMATION_TESTS
public:
	// Test-only layout fixtures. These do not create a PIE preview widget and are
	// intentionally excluded from reflected authoring API.
	int32 LayoutFixtureCardCount = 5;
	TArray<TSoftObjectPtr<UCardDefinition>> LayoutFixtureCardDefinitions;
	TArray<FWacomFirstPersonCardLayerSlotView> BuildLayoutFixtureCardSlotViews() const;
	FWacomFirstPersonCardAnchorAutomationTestView GetAutomationTestViewForTest() const;
	void SetCardLayerWidgetForTest(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void SetHoveredCardInstanceIdForTest(const FGuid& CardInstanceId);
	void ResetAnchorScreenSmoothingForTest() { ResetAnchorScreenSmoothing(); }
	void SetRuntimeCardLayerEntries(FName SourceId, const TArray<FWacomFirstPersonCardLayerEntry>& Entries);
	void SetRuntimeCardLayerPresentationFrame(
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);
	void SetRuntimeCardLayerPresentationFrame(
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& TransitionHints);
	void SetRuntimeCardLayerTransitionHints(
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints);
#endif
private:
	void SetRuntimeCardLayerTransitionPresentationEnabled(FName SourceId, bool bEnabled);
	void SetRuntimeCardLayerData(FName SourceId, const TArray<FWacomCardViewData>& Cards);
	void ClearRuntimeCardLayerData(FName SourceId);
	void ClearCardLayerVisualState();
	void SetFirstPersonCardLayerInteractionEnabled(bool bEnabled);

	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardLayerWidget> CardLayerWidget;

	FTransform CurrentAnchorTransform = FTransform::Identity;
	EWacomFirstPersonCardAnchorMode CurrentMode = EWacomFirstPersonCardAnchorMode::Invalid;
	FRotator CurrentLookOffsetUsed = FRotator::ZeroRotator;
	FRotator CurrentRawCursorLookOffset = FRotator::ZeroRotator;
	FName LastFallbackReason = NAME_None;
	bool bHasValidAnchor = false;
	bool bHasInitializedAnchor = false;
	bool bCurrentLookOffsetAppliedToLayout = false;
	mutable FVector2D SmoothedAnchorWidgetPosition = FVector2D::ZeroVector;
	mutable EWacomFirstPersonCardProjectionMode SmoothedAnchorProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	mutable EWacomFirstPersonCardViewportClampMode SmoothedAnchorViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
	mutable EWacomFirstPersonCardAnchorMode SmoothedAnchorMode = EWacomFirstPersonCardAnchorMode::Invalid;
	mutable FVector2D LastAnchorScreenSmoothingTargetWidgetPosition = FVector2D::ZeroVector;
	mutable uint64 LastAnchorScreenSmoothingFrame = 0;
	mutable bool bHasSmoothedAnchorWidgetPosition = false;
	mutable bool bLastAnchorScreenSmoothed = false;
	mutable float LastAnchorScreenSmoothingDistancePixels = 0.0f;
	mutable bool bHasResolvedCardLayoutConfigHash = false;
	mutable uint32 LastResolvedCardLayoutConfigHash = 0;
	mutable bool bHasResolvedCardBaseConfigHash = false;
	mutable uint32 LastResolvedCardBaseConfigHash = 0;

	mutable FWacomFirstPersonCardSlotMotionConfig CachedSlotMotionConfig;

	friend struct FWacomFirstPersonCardLayerTestAccess;
	friend struct FWacomFirstPersonCardAccessibilityBridge;
	friend struct FWacomFirstPersonCardPresentationScaleBridge;

	void ApplyRuntimeCardLayerPresentationFrame(
		const FWacomFirstPersonCardLayerPresentationFrame& Frame);
	mutable FWacomFirstPersonCardSlotVisualConfig CachedSlotVisualConfig;
	mutable FWacomFirstPersonCardSlotFeedbackConfig CachedSlotFeedbackConfig;
	mutable FWacomFirstPersonCardDragConfig CachedCardDragConfig;
	mutable FWacomFirstPersonCardPileTransferConfig CachedPileTransferConfig;
	mutable TObjectPtr<UClass> CachedCardViewClass;
	mutable uint32 CachedOwnerConfigHash = 0;
	mutable bool CachedInteractionEnabled = false;
	mutable bool CachedLogDiagnostics = false;
	mutable bool bHasCachedOwnerConfig = false;
	bool bFirstPersonCardLayerInteractionEnabled = false;
	FDelegateHandle RuntimeSettingsChangedHandle;
	float RuntimeDecorativeFlashIntensityScale = 1.0f;
	bool bRuntimeSimplifiedMotion = false;

	TUniquePtr<
		FWacomFirstPersonCardAnchorRuntimeState,
		FWacomFirstPersonCardAnchorRuntimeStateDeleter> RuntimeState;
	TUniquePtr<
		FWacomFirstPersonCardLayerOwner,
		FWacomFirstPersonCardLayerOwnerDeleter> CardLayerOwner;
	TUniquePtr<
		FWacomFirstPersonCardLayerDelegateRouter,
		FWacomFirstPersonCardLayerDelegateRouterDeleter> CardLayerDelegateRouter;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	bool ResolveBaseAnchor(FTransform& OutBaseTransform, EWacomFirstPersonCardAnchorMode& OutMode, FName& OutFallbackReason) const;
	void ConfigureTickPrerequisites();
	bool RefreshResolvedCardLayoutRuntimeState() const;
	void InvalidateResolvedCardLayoutRuntimeState(
		bool bPreservePresentationPlayback = false) const;
	void BindRuntimeSettings();
	void UnbindRuntimeSettings();
	void HandleRuntimeSettingsChanged(
		const FWacomLocalSettingsSnapshot& Snapshot,
		EWacomRuntimeSettingsChangeReason Reason);
	void ResetAnchorScreenSmoothing() const;
	void ApplyAnchorScreenSmoothing(FWacomFirstPersonCardProjectedPoint& AnchorPoint) const;
	static TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntriesFromData(
		const TArray<FWacomCardViewData>& CardData);
	TArray<FWacomFirstPersonCardLayerSlotView> BuildCardSlotViewsFromEntries(
		const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries) const;
	void RemoveCardLayer();
	void ConfigureCardLayerDelegateRouter();
	static FString AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode);
	static FString ProjectionModeToString(EWacomFirstPersonCardProjectionMode Mode);
	static FString ViewportClampModeToString(EWacomFirstPersonCardViewportClampMode Mode);
};
