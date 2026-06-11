// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"
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
DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorTargetNative, const FWacomInteractionTargetHandle&, const FWacomFirstPersonCardLayerSlotView&);

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
	BodyLocked UMETA(DisplayName = "Body Locked Layout", ToolTip = "稳定默认投影：手牌锚点锁在身体、战斗基准或 Run Tunnel spline 上；鼠标镜头偏移只通过当前真实相机影响最终投影。"),
	LegacyWorldProjected UMETA(DisplayName = "Look Responsive Projected", ToolTip = "Look Responsive 投影风格：共享鼠标镜头偏移会先参与手牌锚点计算，然后再使用当前真实相机投影；适合需要更强跟随感或空间视差的手牌表现。")
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

UENUM(BlueprintType)
enum class EWacomFirstPersonCardTransitionOriginMode : uint8
{
	SlotOffset UMETA(DisplayName = "Slot Offset", ToolTip = "保留旧行为：从目标卡槽或当前视觉卡槽按偏移进入 / 离开。"),
	HandAnchorOffset UMETA(DisplayName = "Hand Anchor Offset", ToolTip = "从整副手牌中心锚点按偏移进入 / 离开，适合抽牌或战斗中获得卡牌。"),
	ViewportAnchor UMETA(DisplayName = "Viewport Anchor", ToolTip = "从视口归一化锚点按偏移进入 / 离开，适合模拟牌堆、战斗空间或弃牌区方向。")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardDragOutDirection : uint8
{
	Up UMETA(DisplayName = "Up", ToolTip = "向上拖出手牌一定距离后释放提交。")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardGestureState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	Pressed UMETA(DisplayName = "Pressed"),
	Inspecting UMETA(DisplayName = "Inspecting"),
	DraggingNoTargetCard UMETA(DisplayName = "Dragging No Target Card"),
	AimingTargetedCard UMETA(DisplayName = "Aiming Targeted Card"),
	ArmedForCommit UMETA(DisplayName = "Armed For Commit"),
	Cancelled UMETA(DisplayName = "Cancelled")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardDragTargetFeedbackState : uint8
{
	None UMETA(DisplayName = "None"),
	Invalid UMETA(DisplayName = "Invalid"),
	ValidWorldTarget UMETA(DisplayName = "Valid World Target"),
	ValidCardTarget UMETA(DisplayName = "Valid Card Target"),
	InvalidCardTarget UMETA(DisplayName = "Invalid Card Target"),
	CardProbe UMETA(DisplayName = "Card Probe"),
	ZoneProbe UMETA(DisplayName = "Zone Probe"),
	CommitReady UMETA(DisplayName = "Commit Ready")
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardTargetAffordance
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	EWacomFirstPersonCardDragTargetFeedbackState FeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bCanSubmit = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "Card target affordance 的调试摘要；只用于排查拖卡目标反馈，不参与战斗规则。"))
	FString DebugSummary;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerTransitionHint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardSlotTransitionKind TransitionKind = EWacomFirstPersonCardSlotTransitionKind::Default;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPlayCommitFeedback = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bHasPlayedExitTargetWidgetPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitTargetWidgetPosition = FVector2D::ZeroVector;
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
struct WACOMAPP_API FWacomFirstPersonCardDragConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bEnableFirstPersonCardDragCommit = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bEnableClickToPlayCard = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float CardInspectHoldDelaySeconds = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float CardDragStartThresholdPixels = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float HoverHitHysteresisPixels = 16.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float NoTargetCardDragOutCommitDistancePixels = 140.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	EWacomFirstPersonCardDragOutDirection NoTargetCardDragOutDirection = EWacomFirstPersonCardDragOutDirection::Up;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D CardInspectScreenPosition = FVector2D(0.5f, 0.46f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float CardInspectScale = 1.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bShowDetailDuringCardInspect = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bEnableAimArrow = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bLogCardDragDiagnostics = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bAllowCameraLookDuringCardDrag = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float CardDragCameraLookScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float CardDragCameraLookInterpSpeedOverride = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	bool bAllowCameraLookDuringCardPointer = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	float CardPointerCameraLookScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	float CardPointerCameraLookInterpSpeedOverride = -1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bEnableDragTargetFeedback = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FLinearColor DragValidTargetColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FLinearColor DragInvalidTargetColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FLinearColor DragCardProbeTargetColor = FLinearColor(0.45f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float DragTargetFeedbackOpacity = 0.16f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bSnapAimArrowToValidWorldTarget = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float DragAimArrowSnapBlend = 0.85f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float DragCommitReadyScale = 1.035f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float DragCardTargetProbeScale = 1.025f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float DragCardTargetFocusLiftPixels = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float DragCardTargetFocusScale = 1.045f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	int32 DragCardTargetFocusZOrderBoost = 650;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float SelectedSourceLiftPixels = 36.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float SelectedSourceScale = 1.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	int32 SelectedSourceZOrderBoost = 1200;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bSelectedSourceStraightenAngle = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float SelectedSourceAngleBlend = 0.75f;
};

#if WITH_AUTOMATION_TESTS
struct WACOMAPP_API FWacomFirstPersonCardAnchorAutomationTestView
{
	UWacomFirstPersonCardLayerWidget* CardLayerWidget = nullptr;
	int32 CardLayerConfigApplyCount = 0;
};
#endif

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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardGestureState GestureState = EWacomFirstPersonCardGestureState::Idle;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardAnchorDebugView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "当前 Anchor 是否解析成功。"))
	bool bHasValidAnchor = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "当前 Anchor 模式。"))
	EWacomFirstPersonCardAnchorMode Mode = EWacomFirstPersonCardAnchorMode::Invalid;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "当前 Anchor 的世界变换。"))
	FTransform AnchorTransform = FTransform::Identity;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	EWacomFirstPersonCardProjectionMode ProjectionMode = EWacomFirstPersonCardProjectionMode::BodyLocked;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	EWacomFirstPersonCardViewportClampMode ViewportClampMode = EWacomFirstPersonCardViewportClampMode::SoftClampToViewport;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	FRotator LookOffsetUsed = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "CursorLookDriver 当前提供的原始鼠标镜头偏移；BodyLocked 下仍可有值，但不会参与手牌锚点。"))
	FRotator RawCursorLookOffset = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "Look Responsive 投影中实际应用到手牌锚点的偏移，等价于 LookOffsetUsed。"))
	FRotator AppliedAnchorLookOffset = FRotator::ZeroRotator;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "当前 ProjectionMode 是否为 Look Responsive 投影风格。"))
	bool bLookResponsiveProjection = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "当前 resolved config 中鼠标镜头偏航偏移对手牌锚点的影响比例。"))
	float LookInfluenceYaw = 0.25f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug", meta = (ToolTip = "当前 resolved config 中鼠标镜头俯仰偏移对手牌锚点的影响比例。"))
	float LookInfluencePitch = 0.15f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	TArray<FWacomFirstPersonCardProjectedPoint> ProjectedPoints;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	FName LastFallbackReason = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	bool bBodyLockedLayout = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	bool bCurrentCameraProjection = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	bool bLookOffsetAppliedToLayout = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
	bool bAnchorScreenSmoothed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Debug")
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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	ECardTargetMode TargetMode = ECardTargetMode::None;
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
	FVector2D InputHitCenter = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InputHitScale = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InputHitAngleDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 InputHitOrder = INDEX_NONE;

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
	bool bBodyBottomViewportAdjusted = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bIsHovered = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bHasPendingTargetingCardInHand = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bAnchorScreenSmoothed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBodyLockedLayout = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bCurrentCameraProjection = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bLookOffsetAppliedToLayout = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardGestureState GestureState = EWacomFirstPersonCardGestureState::Idle;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDragView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	EWacomFirstPersonCardGestureState GestureState = EWacomFirstPersonCardGestureState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FWacomFirstPersonCardLayerSlotView SourceSlotView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D PressScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D CurrentScreenPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FWacomInteractionTargetHandle CurrentTarget;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bCommitArmed = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bTargetValid = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bHasPointerViewportPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D PointerViewportPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D PointerNormalizedViewportPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	EWacomFirstPersonCardDragTargetFeedbackState TargetFeedbackState =
		EWacomFirstPersonCardDragTargetFeedbackState::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	bool bHasFeedbackTargetScreenPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D FeedbackTargetScreenPosition = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardPointerView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	FWacomFirstPersonCardLayerSlotView SlotView;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	bool bHasPointerViewportPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	FVector2D PointerViewportPosition = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pointer")
	FVector2D PointerNormalizedViewportPosition = FVector2D::ZeroVector;
};

DECLARE_MULTICAST_DELEGATE_TwoParams(FWacomFirstPersonCardLayerAnchorDragNative, const FGuid&, const FWacomFirstPersonCardDragView&);
DECLARE_MULTICAST_DELEGATE_OneParam(FWacomFirstPersonCardLayerAnchorPointerNative, const FWacomFirstPersonCardPointerView&);
DECLARE_MULTICAST_DELEGATE(FWacomFirstPersonCardLayerAnchorPointerExitNative);

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ToolTip = "Authored2D 模式下是否限制每张卡牌主体底部留在视口内。开启后手牌中心仍可柔性离屏，但最终卡牌主体不会因为贴近屏幕底部而裁掉 TypeName / 类型文字。"))
	bool bKeepAuthoredCardBodyBottomInViewport = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (EditCondition = "bKeepAuthoredCardBodyBottomInViewport", ClampMin = "0.0", UIMin = "0.0", UIMax = "96.0", ToolTip = "Authored2D 卡牌主体底部与视口底边之间保留的最小距离，单位为 UMG 布局像素；用于避免第一人称手牌底部类型文字被屏幕边缘裁掉。"))
	float AuthoredCardBodyBottomViewportPaddingPixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "Look Responsive Projected 中，共享鼠标镜头偏航偏移对手牌锚点的影响比例；BodyLocked 不使用该值影响手牌锚点。"))
	float LookInfluenceYaw = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (UIMin = "0.0", UIMax = "1.0", ToolTip = "Look Responsive Projected 中，共享鼠标镜头俯仰偏移对手牌锚点的影响比例；BodyLocked 不使用该值影响手牌锚点。"))
	float LookInfluencePitch = 0.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|02 Anchor World Position", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "30.0", ToolTip = "第一人称卡牌锚点跟随目标位置和朝向的插值速度；设为 0 时立即贴合目标锚点。"))
	float FollowInterpSpeed = 12.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ToolTip = "是否启用第一人称卡牌槽的轻量视觉过渡；只影响 UMG 表现，不改变 hover/click/出牌流程。"))
	bool bEnableCardSlotMotion = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽位置、角度和缩放追向目标布局的速度，单位为反秒；数值越高越跟手，0 表示立即贴合。"))
	float CardSlotMotionSpeed = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "卡牌槽透明度追向目标透明度的速度，单位为反秒；数值越高淡入淡出越快，0 表示立即贴合。"))
	float CardSlotOpacitySpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "新卡牌进入时相对目标位置的起始偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotEnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "新卡牌进入时的起始透明度；0 表示从完全透明淡入。"))
	float CardSlotEnterOpacity = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "卡牌离开手牌时相对当前位置的结束偏移，单位为 UMG 布局像素；X 正值向右，Y 正值向下。"))
	FVector2D CardSlotExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "卡牌离开手牌时保留 outgoing widget 的时长，单位为秒；0 表示立即移除。"))
	float CardSlotExitDuration = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|05 Slot Motion", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "1600.0", ToolTip = "当卡牌槽目标位置跳变超过该距离时重置视觉过渡，单位为 UMG 布局像素；用于传送、切段或窗口变化时避免慢漂。"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-240.0", UIMax = "240.0", ToolTip = "战斗中获得卡牌进入手牌时相对目标位置的起始偏移，单位为 UMG 布局像素；默认从上方 / 战斗空间方向进入。"))
	FVector2D GainedCardEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ToolTip = "战斗中获得卡牌入场的来源模式；默认从整副手牌中心上方进入，强调来自战斗空间的奖励。"))
	EWacomFirstPersonCardTransitionOriginMode GainedCardEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "获得卡牌入场使用 ViewportAnchor 时的归一化视口锚点；(0,0) 为左上，(1,1) 为右下。"))
	FVector2D GainedCardEnterViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "获得卡牌入场起点相对目标卡牌缩放的倍率；只影响转场视觉起点。"))
	float GainedCardEnterScaleMultiplier = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|06 Transition Motion", meta = (UIMin = "-30.0", UIMax = "30.0", Units = "deg", ToolTip = "获得卡牌入场起点相对目标卡牌角度的额外偏移，单位为度。"))
	float GainedCardEnterAngleOffsetDegrees = 0.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ToolTip = "是否在投影、边缘下坠、悬停上浮和等待选目标上浮后，把最终卡牌位置吸附到稳定网格；用于减少 UMG 旋转时的位置闪动。"))
	bool bEnableCardLayerPixelSnapping = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.01", UIMin = "0.25", UIMax = "8.0", ToolTip = "开启像素对齐时使用的 UMG 布局网格大小；1.0 表示吸附到整数 UMG 布局单位。"))
	float CardLayerPixelSnapGrid = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ToolTip = "是否限制第一人称卡牌层的 UMG 渲染旋转角；高对比卡面被整体旋转时容易出现锯齿。"))
	bool bClampCardLayerRenderAngle = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|03 Projection", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "20.0", Units = "deg", ToolTip = "开启旋转限制时，每张第一人称卡牌允许的最大 UMG 渲染旋转角，单位为度。"))
	float MaxCardLayerRenderAngleDegrees = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "是否绘制 5 个非交互 HUD 调试点，用于验证第一人称卡牌锚点投影位置；仅开发调试使用，默认关闭。"))
	bool bDrawDebugProjection = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|99 Debug", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称卡牌锚点调试 Widget 的视口层级。"))
	int32 DebugWidgetZOrder = 9998;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "是否在第一人称卡牌层检测到槽位生命周期异常时输出简短日志；默认关闭，仅用于排查幽灵 Widget、outgoing 泄漏或重复槽位。"))
	bool bLogCardLayerMotionDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|90 Prototype Preview", meta = (ToolTip = "是否从第一人称卡牌锚点绘制非交互 HUD/UMG 静态预览卡牌层；仅用于 PIE / 开发验证，不是 Battle / Run runtime hand 数据源，默认关闭。"))
	bool bDrawPreviewCardLayer = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|01 Card View", meta = (ToolTip = "第一人称卡牌层使用的 Layer Widget 类；同时服务 Battle / Run runtime hand 与 PIE 预览。空值时使用 C++ 默认层 Widget。"))
	TSubclassOf<UWacomFirstPersonCardLayerWidget> CardLayerWidgetClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|01 Card View", meta = (ToolTip = "第一人称卡牌层使用的卡面 Widget 类；正式验证建议设置为 /Game/Wacom/UI/Card/WBP_FirstPersonCardView。为空时仅使用 UWacomCardView 作为测试兜底。"))
	TSubclassOf<UWacomCardView> FirstPersonCardViewClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|90 Prototype Preview", meta = (ToolTip = "静态预览卡牌层使用的可选卡牌定义；仅用于 PIE / 开发验证，空值时生成占位卡牌数据。"))
	TArray<TSoftObjectPtr<UCardDefinition>> PreviewCardDefinitions;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.01", UIMin = "0.1", UIMax = "2.0", ToolTip = "第一人称 runtime hand 与静态预览中每张卡牌使用的 UMG 渲染缩放；只影响表现，不改变 Battle / Run 手牌数据。"))
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|01 Card View", meta = (ClampMin = "0", UIMin = "0", UIMax = "20000", ToolTip = "第一人称卡牌层 Widget 添加到 Viewport 时使用的层级；同时影响 Battle / Run runtime hand 与 PIE 预览。"))
	int32 CardLayerZOrder = 9996;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|90 Prototype Preview", meta = (ClampMin = "0", UIMin = "0", UIMax = "12", ToolTip = "静态预览卡牌定义为空时绘制的占位卡牌数量；仅用于 PIE / 开发验证。"))
	int32 PreviewCardCountFallback = 5;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|98 Legacy", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "旧兼容：左右手锚点牌现在按普通卡牌表现，此字段不作为新调参入口，仅保留旧资产序列化兼容。"))
	float HandAnchorScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|04 Hand Shape", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "不可用卡牌在第一人称卡牌层上的整体透明度；卡面自身的 disabled overlay 仍由 FWacomCardViewData::bDisabled 控制。"))
	float DisabledRenderOpacity = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "是否启用第一人称战斗手牌层的 hover/click/drag 处理；战斗中由 BattleHUD 控制，默认关闭。"))
	bool bEnableBattleHandInteraction = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "160.0", ToolTip = "鼠标悬停的第一人称卡牌槽额外上浮距离，单位为 UMG 布局像素。"))
	float HoverLiftPixels = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "1.5", ToolTip = "鼠标悬停的第一人称卡牌槽额外渲染缩放倍率。"))
	float HoverScale = 1.06f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0", UIMin = "0", UIMax = "5000", ToolTip = "鼠标悬停的第一人称卡牌槽额外增加的 ZOrder 层级。"))
	int32 HoverZOrderBoost = 500;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|07 Hover", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "64.0", ToolTip = "第一人称手牌父层命中解析的悬停滞后距离，单位为 UMG 布局像素；用于避免鼠标在重叠卡牌分界线附近来回抖动。"))
	float HoverHitHysteresisPixels = 16.0f;

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

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "是否启用第一人称手牌按住读牌、拖出手牌或拉箭头提交。关闭后恢复纯 hover/click 行为。"))
	bool bEnableFirstPersonCardDragCommit = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "是否保留轻点出牌。开启时，按下后在读牌延迟内释放仍走现有点击出牌/TargetSelect；关闭时必须拖出或拉箭头释放才提交。"))
	bool bEnableClickToPlayCard = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "0.5", Units = "s", ToolTip = "按住卡牌多久后进入读牌姿态，单位为秒。仅用于区分轻点出牌和按住读牌。"))
	float CardInspectHoldDelaySeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "80.0", ToolTip = "按下后鼠标移动超过该距离才进入拖出/瞄准状态，单位为 UMG 布局像素。"))
	float CardDragStartThresholdPixels = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "480.0", ToolTip = "无目标卡向上拖出超过该距离后进入可提交状态，单位为 UMG 布局像素；释放时才真正提交。"))
	float NoTargetCardDragOutCommitDistancePixels = 140.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "无目标卡拖出提交的方向。本轮默认只支持向上，避免横向整理或查看时误触。"))
	EWacomFirstPersonCardDragOutDirection NoTargetCardDragOutDirection = EWacomFirstPersonCardDragOutDirection::Up;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "读牌姿态中卡牌移动到的视口归一化位置，X/Y 范围 0 到 1。"))
	FVector2D CardInspectScreenPosition = FVector2D(0.5f, 0.46f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ClampMin = "0.01", UIMin = "0.5", UIMax = "2.0", ToolTip = "读牌姿态中卡牌额外乘上的缩放倍率。"))
	float CardInspectScale = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "读牌姿态是否显示第一人称卡牌详情面板；只影响表现，不改变战斗命令路径。"))
	bool bShowDetailDuringCardInspect = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|09 Gesture", meta = (ToolTip = "有目标卡拖动时是否绘制从源卡到鼠标的 C++ UMG 箭头线。"))
	bool bEnableAimArrow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "是否输出第一人称卡牌拖拽/瞄准诊断日志；默认关闭，仅用于排查手势状态，不改变拖拽语义。"))
	bool bLogCardDragDiagnostics = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Camera Look While UI", meta = (ToolTip = "拖拽第一人称卡牌时是否继续让当前第一人称镜头根据拖拽指针轻微偏转。开启后保留 UI 鼠标捕获，Battle 使用 BattleCameraLook，探索使用 RunTunnel cursor look 作为临时输入。"))
	bool bAllowCameraLookDuringCardDrag = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Camera Look While UI", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", ToolTip = "拖拽期间传给当前 first-person cursor look 的强度倍率；1 表示沿用当前镜头自身 LookYawScale / LookPitchScale，0 表示拖拽期间不推动镜头偏转。"))
	float CardDragCameraLookScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Camera Look While UI", meta = (UIMin = "-1.0", UIMax = "60.0", ToolTip = "拖拽期间当前 first-person cursor look 追向拖拽指针的插值速度覆盖值；小于 0 时沿用当前镜头自身 LookInterpSpeed，0 表示立即贴合。"))
	float CardDragCameraLookInterpSpeedOverride = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Camera Look While UI", meta = (ToolTip = "鼠标悬浮或拖拽第一人称卡牌时，是否继续让当前第一人称镜头根据卡牌指针位置偏转。开启后即使 UMG 处理 mouse move，Battle / Run 也会收到临时 cursor look 输入。"))
	bool bAllowCameraLookDuringCardPointer = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Camera Look While UI", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "2.0", ToolTip = "悬浮或拖拽第一人称卡牌期间传给当前 first-person cursor look 的强度倍率；1 表示沿用当前镜头自身 LookYawScale / LookPitchScale，0 表示不推动镜头偏转。"))
	float CardPointerCameraLookScale = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|12 Camera Look While UI", meta = (UIMin = "-1.0", UIMax = "60.0", ToolTip = "悬浮或拖拽第一人称卡牌期间当前 first-person cursor look 追向卡牌指针的插值速度覆盖值；小于 0 时沿用当前镜头自身 LookInterpSpeed，0 表示立即贴合。"))
	float CardPointerCameraLookInterpSpeedOverride = -1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽第一人称卡牌时是否显示释放目标反馈。开启后箭头、源卡和被指向卡槽会根据当前目标合法性显示轻量提示。"))
	bool bEnableDragTargetFeedback = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽瞄准到合法 World 目标时使用的确认颜色。"))
	FLinearColor DragValidTargetColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽瞄准到非法目标或空目标时使用的拒绝颜色。"))
	FLinearColor DragInvalidTargetColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽指向另一张第一人称卡牌时使用的探测颜色；本轮只表示识别到 Card target，不提交卡对卡规则。"))
	FLinearColor DragCardProbeTargetColor = FLinearColor(0.45f, 0.75f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "0.5", ToolTip = "拖拽目标反馈叠加颜色的不透明度，范围 0 到 1。"))
	float DragTargetFeedbackOpacity = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ToolTip = "拖拽瞄准到合法 World 目标时，箭头终点是否轻微吸附到目标屏幕位置；拿不到目标位置时仍跟随鼠标。"))
	bool bSnapAimArrowToValidWorldTarget = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0", UIMin = "0.0", UIMax = "1.0", ToolTip = "合法 World 目标吸附时，箭头终点从鼠标位置朝目标屏幕位置混合的比例；0 表示不吸附，1 表示完全贴到目标。"))
	float DragAimArrowSnapBlend = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.2", ToolTip = "无目标卡拖出达到提交阈值时源卡额外乘上的缩放倍率，用于表示松手会提交。"))
	float DragCommitReadyScale = 1.035f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0.01", UIMin = "0.9", UIMax = "1.2", ToolTip = "拖拽指向另一张第一人称卡牌时，被指向卡槽额外乘上的缩放倍率；只表示 Card target probe。"))
	float DragCardTargetProbeScale = 1.025f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0.0", UIMin = "0.0", UIMax = "48.0", ToolTip = "拖拽指向另一张第一人称卡牌时，被指向卡槽额外上浮距离，单位为 UMG 布局像素；只影响拖拽目标 focus 视觉，不触发普通 hover。"))
	float DragCardTargetFocusLiftPixels = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0.01", UIMin = "1.0", UIMax = "1.12", ToolTip = "拖拽指向另一张第一人称卡牌时，被指向卡槽额外乘上的 focus 缩放倍率；不改变稳定命中范围。"))
	float DragCardTargetFocusScale = 1.045f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|First Person Hand|11 Drag Target Feedback", meta = (ClampMin = "0", UIMin = "0", UIMax = "1400", ToolTip = "拖拽指向另一张第一人称卡牌时，被指向卡槽额外增加的 ZOrder 层级；用于确保目标卡显示在相邻手牌之上。"))
	int32 DragCardTargetFocusZOrderBoost = 650;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Hand|99 Debug")
	void RefreshAnchor(float DeltaTime);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	FTransform GetCurrentAnchorTransform() const { return CurrentAnchorTransform; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	FTransform ComputeCardTransform(int32 NumCards, int32 CardIndex) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Hand|99 Debug")
	bool ProjectCardTransformToScreen(
		const FTransform& CardTransform,
		FWacomFirstPersonCardProjectedPoint& OutProjectedPoint,
		int32 PointIndex = -1) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "构建第一人称卡牌 Anchor 的投影调试快照；用于 PIE / 蓝图排查布局，不改变手牌或战斗状态。"))
	FWacomFirstPersonCardAnchorDebugView GetFirstPersonCardAnchorDebugView(int32 NumDebugCards = 5) const;

	UFUNCTION(BlueprintCallable, Category = "Wacom|First Person Hand|90 Prototype Preview")
	TArray<FWacomFirstPersonCardLayerSlotView> BuildPreviewCardSlotViews() const;

	TArray<FWacomFirstPersonCardLayerSlotView> BuildActiveCardLayerSlotViews() const;

	void SetRuntimeCardLayerEntries(FName SourceId, const TArray<FWacomFirstPersonCardLayerEntry>& Entries);
	void SetRuntimeCardLayerTransitionHints(
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints);
	void SetRuntimeCardLayerData(FName SourceId, const TArray<FWacomCardViewData>& Cards);
	void ClearRuntimeCardLayerData(FName SourceId);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	bool HasRuntimeCardLayerData() const { return bHasRuntimeCardLayerData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	FName GetRuntimeCardLayerSourceId() const { return RuntimeCardLayerSourceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug")
	int32 GetRuntimeCardLayerCardCount() const { return RuntimeCardLayerEntries.Num(); }

	const TArray<FWacomCardViewData>& GetRuntimeCardLayerData() const { return RuntimeCardLayerData; }
	const TArray<FWacomFirstPersonCardLayerEntry>& GetRuntimeCardLayerEntries() const { return RuntimeCardLayerEntries; }

	void SetBattleHandInteractionEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|90 Prototype Preview")
	bool IsCardLayerWidgetActive() const { return CardLayerWidget != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|09 Gesture")
	bool IsBattleHandInteractionEnabled() const { return bEnableBattleHandInteraction; }

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|07 Hover")
	FGuid GetHoveredCardInstanceId() const { return HoveredCardInstanceId; }

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

	FWacomFirstPersonCardLayerAnchorInteractionNative OnFirstPersonCardLayerCardClicked;
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

	UFUNCTION(BlueprintPure, Category = "Wacom|First Person Hand|99 Debug", meta = (ToolTip = "获取第一人称卡牌 Anchor 的单行调试摘要；用于排查 anchor、投影、runtime source 和手势状态。"))
	FString GetDebugSummary() const;

#if WITH_AUTOMATION_TESTS
	FWacomFirstPersonCardAnchorAutomationTestView GetAutomationTestViewForTest() const;
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
	virtual bool CanCreateCardLayerForAnchor(APlayerController* PlayerController) const;
	virtual UWacomFirstPersonCardLayerWidget* CreateCardLayerWidgetForAnchor(
		APlayerController* PlayerController,
		TSubclassOf<UWacomFirstPersonCardLayerWidget> LayerClass) const;
	virtual void AddCardLayerWidgetToViewportForAnchor(
		UWacomFirstPersonCardLayerWidget* LayerWidget,
		int32 ZOrder) const;
	void UpdateCardLayer();

private:
	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardAnchorDebugWidget> DebugWidget;

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
	bool bHasAppliedCardLayerConfig = false;
	uint32 LastAppliedCardLayerConfigHash = 0;
	TWeakObjectPtr<UClass> LastAppliedCardLayerCardViewClass;
	bool bLastAppliedCardLayerLogDiagnostics = false;
	bool bLastAppliedCardLayerInteractionEnabled = false;
#if WITH_AUTOMATION_TESTS
	int32 CardLayerConfigApplyCountForTest = 0;
#endif

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
	FWacomInteractionTargetHandle HoveredCardTargetHandle;

	AWacomPlayerCharacter* GetOwnerCharacter() const;
	APlayerController* GetOwnerPlayerController() const;
	bool ResolveBaseAnchor(FTransform& OutBaseTransform, EWacomFirstPersonCardAnchorMode& OutMode, FName& OutFallbackReason) const;
	FWacomCardViewData BuildPreviewCardViewData(int32 CardIndex) const;
	void ConfigureTickPrerequisites();
	void RefreshResolvedCardLayoutRuntimeState() const;
	void InvalidateResolvedCardLayoutRuntimeState() const;
	void ResetAnchorScreenSmoothing() const;
	void ApplyAnchorScreenSmoothing(FWacomFirstPersonCardProjectedPoint& AnchorPoint) const;
	TArray<FWacomFirstPersonCardLayerEntry> BuildPreviewCardLayerEntries() const;
	static TArray<FWacomFirstPersonCardLayerEntry> BuildCardLayerEntriesFromData(
		const TArray<FWacomCardViewData>& CardData);
	TArray<FWacomFirstPersonCardLayerSlotView> BuildCardSlotViewsFromEntries(
		const TArray<FWacomFirstPersonCardLayerEntry>& CardEntries) const;
	void UpdateDebugWidget();
	void RemoveDebugWidget();
	void RemoveCardLayer();
	void BindCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void UnbindCardLayerWidget(UWacomFirstPersonCardLayerWidget* LayerWidget);
	void HandleLayerCardClicked(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardHovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardUnhovered(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerHoveredCardSlotUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardTargetHovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerCardTargetUnhovered(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerHoveredCardTargetUpdated(const FWacomInteractionTargetHandle& CardTargetHandle, const FWacomFirstPersonCardLayerSlotView& SlotView);
	void HandleLayerDragStarted(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleLayerDragUpdated(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleLayerDragReleased(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleLayerDragCancelled(const FGuid& CardInstanceId, const FWacomFirstPersonCardDragView& DragView);
	void HandleLayerPointerMoved(const FWacomFirstPersonCardPointerView& PointerView);
	void HandleLayerPointerLeft();
	static FString AnchorModeToString(EWacomFirstPersonCardAnchorMode Mode);
	static FString ProjectionModeToString(EWacomFirstPersonCardProjectionMode Mode);
	static FString ViewportClampModeToString(EWacomFirstPersonCardViewportClampMode Mode);
};
