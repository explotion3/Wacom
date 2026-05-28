// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/WacomEnums.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomFirstPersonCardAnchorComponent.generated.h"

class APlayerController;
class AWacomPlayerCharacter;
class UCardDefinition;
class UWacomCardView;
class UWacomFirstPersonCardAnchorDebugWidget;
class UWacomFirstPersonCardLayerWidget;
struct FWacomFirstPersonCardLayerSlotView;

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorInteractionNative, const FGuid&, const FWacomFirstPersonCardLayerSlotView&);

UENUM(BlueprintType)
enum class EWacomFirstPersonCardAnchorMode : uint8
{
	Invalid,
	BattleCamera,
	RunTunnel,
	CameraFallback
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardProjectionMode : uint8
{
	BodyLocked UMETA(DisplayName = "Body Locked Layout", ToolTip = "卡牌 3D 布局锁在身体、战斗基准或 Run Tunnel spline 上，但仍使用当前真实相机投影。"),
	LegacyWorldProjected UMETA(DisplayName = "Legacy World Projected", ToolTip = "旧投影对照路径：共享鼠标镜头偏移会参与卡牌布局，然后再使用当前真实相机投影。")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardLayoutMode : uint8
{
	Authored2D UMETA(DisplayName = "Authored 2D", ToolTip = "默认布局：只投影整副手牌中心点，再用美术可控的 2D 参数排列每张卡牌。"),
	LegacyProjectedFan2D UMETA(DisplayName = "Legacy Projected Fan 2D", ToolTip = "旧布局对照：每张卡牌先生成 3D 槽位，再分别投影到 UMG。")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardViewportClampMode : uint8
{
	HardClampToViewport UMETA(DisplayName = "Hard Clamp To Viewport", ToolTip = "硬限制到视口安全区域内，复现旧行为。"),
	SoftClampToViewport UMETA(DisplayName = "Soft Clamp To Viewport", ToolTip = "默认限制方式：允许手牌锚点离开视口一段距离，超过软范围后再平滑拉回。"),
	AllowOffscreen UMETA(DisplayName = "Allow Offscreen", ToolTip = "不限制到视口内；只要世界点投影成功，就允许 UMG 坐标位于屏幕外。")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardSlotTransitionKind : uint8
{
	Default UMETA(DisplayName = "Default", ToolTip = "默认槽位转场；使用通用入场或离场偏移。"),
	Drawn UMETA(DisplayName = "Drawn", ToolTip = "抽牌进入手牌；默认从手牌下方进入。"),
	Gained UMETA(DisplayName = "Gained", ToolTip = "战斗中获得卡牌进入手牌；默认从战斗空间方向进入。"),
	Played UMETA(DisplayName = "Played", ToolTip = "卡牌被打出离开手牌；默认向上离开。"),
	Discarded UMETA(DisplayName = "Discarded", ToolTip = "卡牌被弃置离开手牌；默认向下离开。")
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerTransitionHint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardSlotTransitionKind TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerMotionDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 InputSlotCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 ActiveSlotCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 OutgoingSlotCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 RootCanvasChildCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 MotionTickSlotCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 DuplicateKeyCount = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 CreatedThisUpdate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 ReusedThisUpdate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 RemovedThisUpdate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 OutgoingStartedThisUpdate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 OutgoingFinishedThisUpdate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	int32 UntrackedChildRemovedThisUpdate = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	bool bHadInvariantViolation = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardProjectedPoint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 Index = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D RawScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D WidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D UnclampedWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D SnappedWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardLayoutMode LayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D AnchorWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D UnsmoothedAnchorWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D SmoothedAnchorWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D AuthoredLayoutOffset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float NormalizedHandOffset = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ViewportScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float OffscreenDistancePixels = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float AnchorScreenSmoothingDistancePixels = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bProjected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bClamped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bOutsideViewport = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPixelSnapped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bAnchorScreenSmoothed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBodyLockedLayout = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bCurrentCameraProjection = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bLookOffsetAppliedToLayout = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardAnchorDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bHasValidAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardAnchorMode Mode = EWacomFirstPersonCardAnchorMode::Invalid;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FTransform AnchorTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardLayoutMode LayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FRotator LookOffsetUsed = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TArray<FWacomFirstPersonCardProjectedPoint> ProjectedPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FName LastFallbackReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBodyLockedLayout = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bCurrentCameraProjection = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bLookOffsetAppliedToLayout = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bAnchorScreenSmoothed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardLayerMotionDebugView LayerMotionDebugView;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomCardViewData CardViewData;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EHandZone Zone = EHandZone::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsHandAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsPlayable = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsPendingTargeting = false;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerSlotView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 Index = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardLayerEntry Entry;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D RawScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D WidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D UnclampedWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D SnappedWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardLayoutMode LayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D AnchorWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D UnsmoothedAnchorWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D SmoothedAnchorWidgetPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D AuthoredLayoutOffset = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float NormalizedHandOffset = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RenderOpacity = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 ZOrder = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ViewportScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float OffscreenDistancePixels = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float AnchorScreenSmoothingDistancePixels = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bProjected = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bClamped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bOutsideViewport = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPixelSnapped = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsHovered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bAnchorScreenSmoothed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBodyLockedLayout = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bCurrentCameraProjection = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bLookOffsetAppliedToLayout = false;
};

/**
 * Computes the first-person virtual card hand anchor used by future HUD-rendered
 * cards. V0-B can draw a non-interactive static card layer for PIE validation.
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent))
class WACOMAPP_API UWacomFirstPersonCardAnchorComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UWacomFirstPersonCardAnchorComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "600.0", Units = "cm", ToolTip = "第一人称锚点到虚拟手牌平面的距离，单位为 Unreal 厘米。"))
	float DistanceFromView = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-240.0", UIMax = "120.0", Units = "cm", ToolTip = "虚拟手牌平面相对第一人称锚点的垂直偏移，单位为 Unreal 厘米；负值会让卡牌在画面中更低。"))
	float VerticalOffset = -70.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-240.0", UIMax = "240.0", Units = "cm", ToolTip = "虚拟手牌平面相对第一人称锚点的水平偏移，单位为 Unreal 厘米。"))
	float HorizontalOffset = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "120.0", Units = "cm", ToolTip = "投影前相邻虚拟卡牌槽之间的距离，单位为 Unreal 厘米。"))
	float CardSpacing = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Layout", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "每张卡牌相对第一人称手牌锚点增加的扇形偏航角，单位为度；角度越大，旋转锯齿风险越高。"))
	float FanYawDegrees = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (ToolTip = "第一人称卡牌层的手牌排布方式。Authored2D 只投影手牌中心点，再用 2D 参数排卡；LegacyProjectedFan2D 保留旧的每张卡牌 3D 槽位投影，用于对照。"))
	EWacomFirstPersonCardLayoutMode CardLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "360.0", ToolTip = "Authored2D 模式下相邻卡牌的基础水平间距，单位为 UMG 布局像素。"))
	float AuthoredCardSpacingPixels = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "Authored2D 模式下整副手牌允许占用的最大宽度，单位为 UMG 布局像素；大于 0 时会自动压缩水平间距，0 表示不限制宽度。"))
	float AuthoredMaxHandWidthPixels = 720.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (UIMin = "-600.0", UIMax = "600.0", ToolTip = "Authored2D 模式下整副手牌中心投影后的额外屏幕偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D AuthoredHandScreenOffset = FVector2D::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "Authored2D 模式下中心卡牌额外上抬距离，单位为 UMG 布局像素；正值让中心卡牌更高，边缘卡牌逐渐减弱。"))
	float AuthoredCenterLiftPixels = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "6.0", ToolTip = "Authored2D 模式下边缘下坠曲线指数；数值越大，越靠近边缘的卡牌下坠越集中。"))
	float AuthoredDropCurveExponent = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "6.0", ToolTip = "Authored2D 模式下扇形旋转曲线指数；1 表示线性，数值越大，中心卡牌更接近水平，边缘卡牌承担更多旋转。"))
	float AuthoredFanCurveExponent = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Authored Layout", meta = (ToolTip = "Authored2D 模式下是否让中心卡牌默认绘制在边缘卡牌之上；悬停和等待选目标的层级提升仍会优先生效。"))
	bool bAuthoredCenterCardsDrawOnTop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "共享鼠标镜头偏航偏移对卡牌锚点的影响比例；数值越低，卡牌越像跟随角色身体而不是镜头。"))
	float LookInfluenceYaw = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "共享鼠标镜头俯仰偏移对卡牌锚点的影响比例；数值越低，卡牌越像跟随角色身体而不是镜头。"))
	float LookInfluencePitch = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Look", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "第一人称卡牌锚点跟随目标位置和朝向的插值速度；设为 0 时立即贴合目标锚点。"))
	float FollowInterpSpeed = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ToolTip = "第一人称卡牌层的投影模式。BodyLocked 会把卡牌 3D 布局锁在战斗基准朝向或 Run Tunnel spline 基准朝向上，但仍使用当前真实相机投影，保留第一人称空间感；LegacyWorldProjected 保留旧的 LookInfluence 影响布局路径，用于调试对照。"))
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "200.0", ToolTip = "投影点被限制在视口内时保留的屏幕安全边距，单位为 UMG 布局像素。"))
	float ProjectionPadding = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ToolTip = "第一人称手牌投影点的视口限制方式。HardClamp 会强制留在屏幕内；SoftClamp 允许离屏一段距离后柔性拉回；AllowOffscreen 完全允许离屏。"))
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "SoftClamp 模式下允许手牌锚点离开视口安全区域的距离，单位为 UMG 布局像素；数值越大，手牌越像真实空间物体。"))
	float SoftClampOffscreenAllowancePixels = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1000.0", ToolTip = "SoftClamp 模式下超过离屏允许范围后逐步拉回的过渡距离，单位为 UMG 布局像素；0 表示越界后立即停在软边界。"))
	float SoftClampBlendRangePixels = 240.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Motion Stability", meta = (ToolTip = "是否对 Authored2D 的整副手牌中心做屏幕空间平滑；用于保留空间上下变化的同时减少移动时的高频抖动。"))
	bool bEnableAnchorScreenSmoothing = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Motion Stability", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "60.0", ToolTip = "手牌中心屏幕空间平滑速度，单位为反秒；数值越低越稳但越滞后，0 表示立即贴合。"))
	float AnchorScreenSmoothingSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Motion Stability", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1200.0", ToolTip = "当手牌中心跳变距离超过该阈值时重置屏幕平滑，单位为 UMG 布局像素；用于切换场景、切换路线或传送时避免慢慢飘过去。"))
	float AnchorScreenSmoothingResetDistancePixels = 320.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ToolTip = "是否启用第一人称卡牌槽的轻量视觉过渡；只影响 UMG 表现，不改变 hover/click/出牌流程。"))
	bool bEnableCardSlotMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽位置、角度和缩放追向目标布局的速度，单位为反秒；数值越高越跟手，0 表示立即贴合。"))
	float CardSlotMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽透明度追向目标透明度的速度，单位为反秒；数值越高淡入淡出越快，0 表示立即贴合。"))
	float CardSlotOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "新卡牌进入时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotEnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "新卡牌进入时的起始透明度；0 表示从完全透明淡入。"))
	float CardSlotEnterOpacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌离开手牌时相对当前位置的结束偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "卡牌离开手牌时保留 outgoing widget 的时长，单位为秒；0 表示立即移除。"))
	float CardSlotExitDuration = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "当卡牌槽目标位置跳变超过该距离时重置视觉过渡，单位为 UMG 布局像素；用于传送、切段或窗口变化时避免慢漂。"))
	float CardSlotMotionResetDistancePixels = 420.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (ToolTip = "是否启用事件感知的第一人称卡牌转场；只根据 BattleHUD 提供的表现 hint 改变入场 / 离场方向，不改变战斗规则或命令路径。"))
	bool bEnableEventAwareCardTransitions = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "抽牌进入手牌时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D DrawnCardEnterOffsetPixels = FVector2D(0.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "战斗中获得卡牌进入手牌时相对目标位置的起始偏移，单位为 UMG 布局像素；默认从上方 / 战斗空间方向进入。"))
	FVector2D GainedCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌被打出时相对当前位置的离场偏移，单位为 UMG 布局像素；默认向上离开手牌。"))
	FVector2D PlayedCardExitOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌被弃置时相对当前位置的离场偏移，单位为 UMG 布局像素；默认向下离开手牌。"))
	FVector2D DiscardedCardExitOffsetPixels = FVector2D(0.0f, 120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ToolTip = "是否在投影、边缘下坠、悬停上浮和等待选目标上浮后，把最终卡牌位置吸附到稳定网格；用于减少 UMG 旋转时的位置闪动。"))
	bool bEnableCardLayerPixelSnapping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "8.0", ToolTip = "开启像素对齐时使用的 UMG 布局网格大小；1.0 表示吸附到整数 UMG 布局单位。"))
	float CardLayerPixelSnapGrid = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ToolTip = "是否限制第一人称卡牌层的 UMG 渲染旋转角；高对比卡面被整体旋转时容易出现锯齿。"))
	bool bClampCardLayerRenderAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0", Units = "deg", ToolTip = "开启旋转限制时，每张第一人称卡牌允许的最大 UMG 渲染旋转角，单位为度。"))
	float MaxCardLayerRenderAngleDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "是否绘制 5 个非交互 HUD 调试点，用于验证第一人称卡牌锚点投影位置；仅开发调试使用，默认关闭。"))
	bool bDrawDebugProjection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称卡牌锚点调试 Widget 的视口层级。"))
	int32 DebugWidgetZOrder = 9998;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "是否在第一人称卡牌层检测到槽位生命周期异常时输出简短日志；默认关闭，仅用于排查幽灵 Widget、outgoing 泄漏或重复槽位。"))
	bool bLogCardLayerMotionDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "是否从第一人称卡牌锚点绘制非交互 HUD/UMG 静态预览卡牌层；仅原型验证使用，默认关闭。"))
	bool bDrawStaticCardLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "非交互第一人称静态卡牌层使用的 Widget 类；为空时使用 C++ 默认层 Widget。"))
	TSubclassOf<UWacomFirstPersonCardLayerWidget> StaticCardLayerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|View", meta = (ToolTip = "第一人称卡牌层使用的卡面 Widget 类；正式验证建议设置为 /Game/Wacom/UI/Card/WBP_FirstPersonCardView。为空时仅使用 UWacomCardView 作为测试兜底。"))
	TSubclassOf<UWacomCardView> FirstPersonCardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ToolTip = "静态预览卡牌层使用的可选卡牌定义；为空时生成占位卡牌数据，便于 PIE 直接验证。"))
	TArray<TSoftObjectPtr<UCardDefinition>> StaticPreviewCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "2.0", ToolTip = "第一人称卡牌层中每张卡牌使用的 UMG 渲染缩放。"))
	float StaticCardRenderScale = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "260.0", ToolTip = "最外侧卡牌额外下坠的屏幕距离，单位为 UMG 布局像素；越靠近中心的卡牌下坠越少，用于形成手牌弧线。"))
	float StaticCardEdgeDropPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称静态卡牌层 Widget 的视口层级。"))
	int32 StaticCardLayerZOrder = 9996;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Static Layer", meta = (ClampMin = "0", UIMin = "0", UIMax = "12", ToolTip = "静态预览卡牌定义为空时绘制的占位卡牌数量。"))
	int32 StaticCardCountFallback = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "正在等待目标选择的卡牌额外上浮距离，单位为 UMG 布局像素。"))
	float PendingTargetingLiftPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "正在等待目标选择的卡牌额外渲染缩放倍率。"))
	float PendingTargetingScale = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "正在等待目标选择的卡牌额外增加的 ZOrder 层级；用于确保 pending 卡绘制在普通手牌之上。"))
	int32 PendingTargetingZOrderBoost = 1200;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ToolTip = "等待目标选择时，是否让 pending 卡牌的 UMG 渲染角度向 0 度轻微归正；只影响表现，不改变手牌顺序或出牌逻辑。"))
	bool bPendingTargetingStraightenAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "pending 卡牌角度向 0 度归正的混合比例；0 表示保持原扇形角度，1 表示完全归正。"))
	float PendingTargetingAngleBlend = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ToolTip = "进入 TargetSelect 且存在 pending 卡时，是否轻微弱化其他第一人称手牌；只降低透明度，不改变布局、输入或战斗规则。"))
	bool bEnableTargetSelectHandDeemphasis = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "TargetSelect 中非 pending 手牌的透明度倍率；会与不可用卡透明度相乘，范围 0 到 1。"))
	float TargetSelectNonPendingOpacityMultiplier = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "手牌锚点卡牌使用的渲染缩放倍率。"))
	float HandAnchorScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Visual States", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "不可用卡牌在第一人称卡牌层上的整体透明度；卡面自身的 disabled overlay 仍由 FWacomCardViewData::bDisabled 控制。"))
	float DisabledRenderOpacity = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ToolTip = "是否启用第一人称战斗手牌层的 hover/click 处理；原型开关，战斗中由 BattleHUD 控制，默认关闭。"))
	bool bEnableBattleHandInteractionPrototype = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "鼠标悬停的第一人称卡牌槽额外上浮距离，单位为 UMG 布局像素。"))
	float HoverLiftPixels = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "鼠标悬停的第一人称卡牌槽额外渲染缩放倍率。"))
	float HoverScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Prototype", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "鼠标悬停的第一人称卡牌槽额外增加的 ZOrder 层级。"))
	int32 HoverZOrderBoost = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ToolTip = "是否启用第一人称手牌的轻量交互反馈；只影响 hover、按下、确认和不可用点击的 UMG 表现，不改变出牌命令路径。"))
	bool bEnableCardInteractionFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ToolTip = "可打卡牌悬停时叠加的轻微颜色；通过 C++ overlay 表现，不要求修改卡面 WBP。"))
	FLinearColor PlayableHoverFeedbackColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.4", ToolTip = "可打卡牌悬停时颜色叠加的不透明度，范围 0 到 1；建议保持很低，避免盖住卡面。"))
	float PlayableHoverFeedbackOpacity = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.1", ToolTip = "左键按下可交互卡牌时额外乘上的缩放倍率；小于 1 会产生轻微按下感。"))
	float PressedFeedbackScale = 0.985f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ToolTip = "左键按下可交互卡牌时叠加的颜色。"))
	FLinearColor PressedFeedbackColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "左键按下可交互卡牌时颜色叠加的不透明度，范围 0 到 1。"))
	float PressedFeedbackOpacity = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "有效点击释放后确认反馈保留的时长，单位为秒；不延迟出牌或目标选择流程。"))
	float ConfirmFeedbackDuration = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "有效点击释放后确认反馈的不透明度，范围 0 到 1。"))
	float ConfirmFeedbackOpacity = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.6", Units = "s", ToolTip = "点击不可打卡牌时拒绝反馈保留的时长，单位为秒；只消费第一人称卡槽点击，不提交战斗命令。"))
	float DenyFeedbackDuration = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "32.0", ToolTip = "点击不可打卡牌时的横向抖动幅度，单位为 UMG 布局像素。"))
	float DenyFeedbackShakePixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ToolTip = "点击不可打卡牌时叠加的拒绝反馈颜色。"))
	FLinearColor DenyFeedbackColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Card Layer|Interaction Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.6", ToolTip = "点击不可打卡牌时拒绝反馈的不透明度，范围 0 到 1。"))
	float DenyFeedbackOpacity = 0.18f;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	void RefreshAnchor(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FTransform GetCurrentAnchorTransform() const { return CurrentAnchorTransform; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FTransform ComputeCardTransform(int32 NumCards, int32 CardIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	bool ProjectCardTransformToScreen(
		const FTransform& CardTransform,
		FWacomFirstPersonCardProjectedPoint& OutProjectedPoint,
		int32 PointIndex = -1) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardAnchorDebugView GetFirstPersonCardAnchorDebugView(int32 NumDebugCards = 5) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Card Layer")
	TArray<FWacomFirstPersonCardLayerSlotView> BuildStaticCardSlotViews() const;

	TArray<FWacomFirstPersonCardLayerSlotView> BuildActiveCardLayerSlotViews() const;

	void SetRuntimeCardLayerEntries(FName SourceId, const TArray<FWacomFirstPersonCardLayerEntry>& Entries);
	void SetRuntimeCardLayerTransitionHints(
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints);
	void SetRuntimeCardLayerData(FName SourceId, const TArray<FWacomCardViewData>& Cards);
	void ClearRuntimeCardLayerData(FName SourceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool HasRuntimeCardLayerData() const { return bHasRuntimeCardLayerData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FName GetRuntimeCardLayerSourceId() const { return RuntimeCardLayerSourceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	int32 GetRuntimeCardLayerCardCount() const { return RuntimeCardLayerEntries.Num(); }

	const TArray<FWacomCardViewData>& GetRuntimeCardLayerData() const { return RuntimeCardLayerData; }
	const TArray<FWacomFirstPersonCardLayerEntry>& GetRuntimeCardLayerEntries() const { return RuntimeCardLayerEntries; }

	void SetBattleHandInteractionPrototypeEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsStaticCardLayerWidgetActive() const { return StaticCardLayerWidget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	bool IsBattleHandInteractionPrototypeEnabled() const { return bEnableBattleHandInteractionPrototype; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FGuid GetHoveredCardInstanceId() const { return HoveredCardInstanceId; }

	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardClicked;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardHovered;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardUnhovered;
	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerHoveredCardLayoutUpdated;

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Card Layer")
	FString GetDebugSummary() const;

#if WITH_AUTOMATION_TESTS
	UWacomFirstPersonCardLayerWidget* GetStaticCardLayerWidgetForTest() const { return StaticCardLayerWidget; }
	void SetHoveredCardInstanceIdForTest(const FGuid& CardInstanceId) { HoveredCardInstanceId = CardInstanceId; }
	void ResetAnchorScreenSmoothingForTest() { ResetAnchorScreenSmoothing(); }
#endif

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
	virtual bool CanCreateStaticCardLayerForAnchor(APlayerController* PlayerController) const;
	virtual UWacomFirstPersonCardLayerWidget* CreateStaticCardLayerWidgetForAnchor(
		APlayerController* PlayerController,
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const;
	virtual void AddStaticCardLayerWidgetToViewportForAnchor(
		UWacomFirstPersonCardLayerWidget* LayerWidget,
		int32 ZOrder) const;
	void UpdateStaticCardLayer();

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardAnchorDebugWidget> DebugWidget;

	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardLayerWidget> StaticCardLayerWidget;

	FTransform CurrentAnchorTransform = FTransform::Identity;
	EWacomFirstPersonCardAnchorMode CurrentMode = EWacomFirstPersonCardAnchorMode::Invalid;
	FRotator CurrentLookOffsetUsed = FRotator::ZeroRotator;
	FName LastFallbackReason = NAME_None;
	bool bHasValidAnchor = false;
	bool bHasInitializedAnchor = false;
	bool bCurrentLookOffsetAppliedToLayout = false;
	mutable FVector2D SmoothedAnchorWidgetPosition = FVector2D::ZeroVector;
	mutable EWacomFirstPersonCardLayoutMode SmoothedAnchorLayoutMode = EWacomFirstPersonCardLayoutMode::Authored2D;
	mutable EWacomFirstPersonCardProjectionMode SmoothedAnchorProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;
	mutable EWacomFirstPersonCardViewportClampMode SmoothedAnchorViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;
	mutable EWacomFirstPersonCardAnchorMode SmoothedAnchorMode = EWacomFirstPersonCardAnchorMode::Invalid;
	mutable FVector2D LastAnchorScreenSmoothingTargetWidgetPosition = FVector2D::ZeroVector;
	mutable uint64 LastAnchorScreenSmoothingFrame = 0;
	mutable bool bHasSmoothedAnchorWidgetPosition = false;
	mutable bool bLastAnchorScreenSmoothed = false;
	mutable float LastAnchorScreenSmoothingDistancePixels = 0.0f;

	UPROPERTY(Transient)
	TArray<FWacomCardViewData> RuntimeCardLayerData;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerEntry> RuntimeCardLayerEntries;

	UPROPERTY(Transient)
	TArray<FWacomFirstPersonCardLayerTransitionHint> RuntimeCardLayerTransitionHints;

	bool bHasRuntimeCardLayerData = false;
	FName RuntimeCardLayerSourceId = NAME_None;
	FName RuntimeCardLayerTransitionHintSourceId = NAME_None;
	FGuid HoveredCardInstanceId;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	bool ResolveBaseAnchor(FTransform& OutBaseTransform, EWacomFirstPersonCardAnchorMode& OutMode, FName& OutFallbackReason) const;
	FWacomCardViewData BuildStaticCardViewData(int32 CardIndex) const;
	void ConfigureTickPrerequisites();
	void ResetAnchorScreenSmoothing() const;
	void ApplyAnchorScreenSmoothing(FWacomFirstPersonCardProjectedPoint& AnchorPoint) const;
	FVector2D ApplyViewportClampToWidgetPosition(
		FVector2D UnclampedPosition,
		FVector2D WidgetViewportSize,
		bool& bOutClamped,
		bool& bOutOutsideViewport,
		float& OutOffscreenDistancePixels) const;
	TArray<FWacomFirstPersonCardLayerEntry> BuildStaticCardLayerEntries() const;
	FVector2D SnapCardLayerPosition(FVector2D Position, bool& bOutPixelSnapped) const;
	float ClampCardLayerRenderAngle(float AngleDegrees) const;
	static TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntriesFromData(
		const TArray<FWacomCardViewData>& CardData);
	TArray<FWacomFirstPersonCardLayerSlotView> BuildCardSlotViewsFromEntries(
		const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries) const;
	void UpdateDebugWidget();
	void RemoveDebugWidget();
	void RemoveStaticCardLayer();
	void BindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void UnbindStaticCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void HandleLayerCardClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerHoveredCardSlotUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	static FString AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode);
	static FString ProjectionModeToString(EWacomFirstPersonCardProjectionMode Mode);
	static FString LayoutModeToString(EWacomFirstPersonCardLayoutMode Mode);
	static FString ViewportClampModeToString(EWacomFirstPersonCardViewportClampMode Mode);
};
