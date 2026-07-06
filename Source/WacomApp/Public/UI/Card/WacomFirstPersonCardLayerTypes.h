// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomFirstPersonCardLayerTypes.generated.h"

UENUM(BlueprintType)
enum class EWacomFirstPersonCardAnchorMode : uint8
{
	Invalid = 0,
	BattleCamera = 1,
	RunTunnel = 2,
	CameraFallback = 3,
	ViewStageBlend = 4
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
	RunHandEntered UMETA(DisplayName = "Run Hand Entered", ToolTip = "Run 探索期默认手牌进入第一人称手牌层；UI 表现语义，不属于战斗抽牌事件。"),
	Gained UMETA(DisplayName = "Gained", ToolTip = "战斗中获得卡牌进入手牌；默认从战斗空间方向进入。"),
	HandAnchorEntered UMETA(DisplayName = "Hand Anchor Entered", ToolTip = "左/右手牌生成入手；由 UI 表现层在普通抽牌后触发，不属于普通抽牌事件。"),
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

enum class EWacomFirstPersonCardGestureSource : uint8
{
	None,
	MousePress,
	KeyboardShortcut
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

UENUM(BlueprintType)
enum class EWacomFirstPersonCardInteractionFeedbackKind : uint8
{
	None UMETA(DisplayName = "None"),
	Pressed UMETA(DisplayName = "Pressed"),
	Confirm UMETA(DisplayName = "Confirm"),
	Commit UMETA(DisplayName = "Commit"),
	Retained UMETA(DisplayName = "Retained"),
	Deny UMETA(DisplayName = "Deny")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardLayerFeedbackKind : uint8
{
	None UMETA(DisplayName = "None"),
	Retained UMETA(DisplayName = "Retained")
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardInteractionFeedbackView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	EWacomFirstPersonCardInteractionFeedbackKind Kind =
		EWacomFirstPersonCardInteractionFeedbackKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	TSoftObjectPtr<UMaterialInterface> Material;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	FLinearColor Color = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float Opacity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float Pulse = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float EdgeWidth = 0.048f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float EdgeSoftness = 0.024f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float VignetteStrength = 0.22f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float VignetteRadius = 0.58f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Feedback")
	float VignetteSoftness = 0.28f;
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
	int32 SequenceIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 SequenceCount = 1;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPlayCommitFeedback = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bHasPlayedExitTargetWidgetPosition = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitTargetWidgetPosition = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardLayerFeedbackHint
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FGuid CardInstanceId;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardLayerFeedbackKind FeedbackKind =
		EWacomFirstPersonCardLayerFeedbackKind::None;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 SequenceIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 SequenceCount = 1;
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
	float CardInspectHoldDelaySeconds = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	float CardDragStartThresholdPixels = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Drag")
	FVector2D CardInspectScrubHandPaddingPixels = FVector2D(32.0f, 48.0f);

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

enum class EWacomFirstPersonCardLayerFrameCommitMode : uint8
{
	StateRefresh,
	PresentationFrame,
	PreviewOverlay,
	Suppressed
};

struct WACOMAPP_API FWacomFirstPersonCardLayerPresentationFrame
{
	FName SourceId = NAME_None;
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	TArray<FWacomFirstPersonCardLayerTransitionHint> TransitionHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints;
	EWacomFirstPersonCardLayerFrameCommitMode CommitMode =
		EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;

	// True when this frame should atomically replace pending presentation hints.
	bool bApplyAsPresentationFrame = false;

	bool HasPresentationHints() const
	{
		return !TransitionHints.IsEmpty() || !FeedbackHints.IsEmpty();
	}

	bool ShouldApplyAsPresentationFrame() const
	{
		return CommitMode == EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame
			|| CommitMode == EWacomFirstPersonCardLayerFrameCommitMode::Suppressed
			|| bApplyAsPresentationFrame
			|| HasPresentationHints();
	}

	void Reset()
	{
		SourceId = NAME_None;
		Entries.Reset();
		TransitionHints.Reset();
		FeedbackHints.Reset();
		CommitMode = EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
		bApplyAsPresentationFrame = false;
	}
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

	EWacomFirstPersonCardGestureSource GestureSource = EWacomFirstPersonCardGestureSource::None;

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

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardTransitionMotionProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode OriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D OffsetPixels = FVector2D::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ViewportAnchor = FVector2D(0.5f, 0.5f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ScaleMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float AngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float StartDelaySeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DurationSeconds = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ArcLiftPixels = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EasePower = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBlockInteractionDuringPlayback = true;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotVisualConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HoverLiftPixels = 28.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HoverScale = 1.06f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 HoverZOrderBoost = 500;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PendingTargetingLiftPixels = 36.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PendingTargetingScale = 1.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 PendingTargetingZOrderBoost = 1200;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bPendingTargetingStraightenAngle = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PendingTargetingAngleBlend = 0.75f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableTargetSelectHandDeemphasis = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float TargetSelectNonPendingOpacityMultiplier = 0.88f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragCardTargetFocusLiftPixels = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragCardTargetFocusScale = 1.045f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 DragCardTargetFocusZOrderBoost = 650;
};

struct WACOMAPP_API FWacomFirstPersonCardSlotVisualState
{
	bool bPendingSource = false;
	bool bTargetSelectDeemphasized = false;
	bool bHovered = false;
	bool bCardDragTargetFocusActive = false;
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardMotionIntent : uint8
{
	Layout,
	Hover,
	Pending,
	DragTargetFocus,
	Enter,
	Exit
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardMotionProfile
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float MotionSpeed = 26.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float OpacitySpeed = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EasePower = 1.0f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotMotionConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float MotionSpeed = 26.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float OpacitySpeed = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EasePower = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardMotionProfile LayoutMotionProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardMotionProfile HoverMotionProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardMotionProfile PendingMotionProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardMotionProfile DragTargetFocusMotionProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardMotionProfile EnterMotionProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardMotionProfile ExitMotionProfile;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D EnterOffsetPixels = FVector2D(0.0f, 48.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EnterOpacity = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D ExitOffsetPixels = FVector2D(0.0f, 36.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ExitDuration = 0.16f;

	// Legacy compatibility field. Ordinary slot reflow no longer uses distance-threshold snapping.
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ResetDistancePixels = 420.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableEventAwareTransitions = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableReadableTransitionOrigins = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DrawnEnterOffsetPixels = FVector2D(0.0f, 96.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode DrawnEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DrawnEnterViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterDurationSeconds = 0.32f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterStaggerSeconds = 0.075f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterArcLiftPixels = 42.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DrawnEnterEasePower = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBlockInteractionDuringDrawnEnter = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D GainedEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode GainedEnterOriginMode = EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D GainedEnterViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterDurationSeconds = 0.32f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterStaggerSeconds = 0.075f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterArcLiftPixels = 42.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float GainedEnterEasePower = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBlockInteractionDuringGainedEnter = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D HandAnchorEnterOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode HandAnchorEnterOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D HandAnchorEnterViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HandAnchorEnterScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HandAnchorEnterAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HandAnchorEnterDurationSeconds = 0.32f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HandAnchorEnterStaggerSeconds = 0.075f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HandAnchorEnterArcLiftPixels = 42.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float HandAnchorEnterEasePower = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bBlockInteractionDuringHandAnchorEnter = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitOffsetPixels = FVector2D(0.0f, -120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode PlayedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D PlayedExitViewportAnchor = FVector2D(0.5f, 0.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayedExitScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayedExitAngleOffsetDegrees = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DiscardedExitOffsetPixels = FVector2D(0.0f, 120.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardTransitionOriginMode DiscardedExitOriginMode = EWacomFirstPersonCardTransitionOriginMode::SlotOffset;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FVector2D DiscardedExitViewportAnchor = FVector2D(0.5f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DiscardedExitScaleMultiplier = 0.96f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DiscardedExitAngleOffsetDegrees = 0.0f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotFeedbackConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor PlayableHoverColor = FLinearColor(1.0f, 0.92f, 0.45f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayableHoverOpacity = 0.06f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedScale = 0.985f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor PressedColor = FLinearColor::White;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedOpacity = 0.10f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ConfirmDuration = 0.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ConfirmOpacity = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyDuration = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyShakePixels = 8.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor DenyColor = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyOpacity = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TSoftObjectPtr<UMaterialInterface> InteractionFeedbackMaterial;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InteractionFeedbackEdgeWidth = 0.048f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InteractionFeedbackEdgeSoftness = 0.024f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InteractionFeedbackVignetteStrength = 0.22f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InteractionFeedbackVignetteRadius = 0.58f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InteractionFeedbackVignetteSoftness = 0.28f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnablePlayCommitFeedback = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitDuration = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitOpacity = 0.16f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor PlayCommitColor = FLinearColor(0.75f, 1.0f, 0.55f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitScale = 1.015f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableRetainedFeedback = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RetainedFeedbackDuration = 0.28f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RetainedFeedbackStaggerSeconds = 0.045f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RetainedFeedbackLiftPixels = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RetainedFeedbackScale = 1.025f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor RetainedFeedbackColor = FLinearColor(1.0f, 0.84f, 0.34f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RetainedFeedbackOpacity = 0.18f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 RetainedFeedbackZOrderBoost = 180;
};
