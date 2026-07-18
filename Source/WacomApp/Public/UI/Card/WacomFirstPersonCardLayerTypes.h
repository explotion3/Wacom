// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Materials/MaterialInterface.h"
#include "Types/WacomEnums.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomFirstPersonCardLayerTypes.generated.h"

class USoundBase;
class UMaterialInstance;
class UTexture2D;

UENUM(BlueprintType)
enum class EWacomFirstPersonCardAnchorMode : uint8
{
	Invalid = 0,
	BattleCamera = 1,
	RunPath = 2,
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
	Gained = 3 UMETA(DisplayName = "Gained", ToolTip = "战斗中获得卡牌进入手牌；使用独立的获得牌入场运动。"),
	HandAnchorEntered = 4 UMETA(DisplayName = "Hand Anchor Entered", ToolTip = "左/右手牌生成入手；由 UI 表现层在普通抽牌后触发，不属于普通抽牌事件。"),
	Played = 5 UMETA(DisplayName = "Played", ToolTip = "卡牌被打出离开手牌；默认向上离开。"),
	Discarded = 6 UMETA(DisplayName = "Discarded", ToolTip = "卡牌被弃置离开手牌；默认向下离开。"),
	Exhausted = 7 UMETA(DisplayName = "Exhausted", ToolTip = "卡牌实际进入消耗区；使用独立消耗 Surface Effect，失效时按弃牌方向离场。")
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
	Idle = 0 UMETA(DisplayName = "Idle"),
	Pressed = 1 UMETA(DisplayName = "Pressed"),
	Inspecting = 2 UMETA(DisplayName = "Inspecting"),
	DraggingNoTargetCard = 3 UMETA(DisplayName = "Dragging No Target Card"),
	AimingTargetedCard = 4 UMETA(DisplayName = "Aiming Targeted Card"),
	ArmedForCommit = 5 UMETA(DisplayName = "Armed For Commit"),
	Cancelled = 6 UMETA(DisplayName = "Cancelled")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardInteractionIntent : uint8
{
	CommitNoTarget = 0 UMETA(DisplayName = "Commit No Target", ToolTip = "Battle 无目标卡交互：向上拖出手牌达到阈值后，由上层在释放时提交。"),
	AimWorldTarget = 1 UMETA(DisplayName = "Aim World Target", ToolTip = "世界目标交互：拖拽进入瞄准态，由上层解析世界目标并验证。"),
	AimCardTarget = 2 UMETA(DisplayName = "Aim Card Target", ToolTip = "手牌目标交互：拖拽进入瞄准态，由上层解析目标手牌并验证。"),
	InspectOnly = 3 UMETA(DisplayName = "Inspect Only", ToolTip = "仅允许悬停和长按读牌，不允许升级为正式拖拽或提交。"),
	DragToDropTarget = 4 UMETA(DisplayName = "Drag To Drop Target", ToolTip = "Run / App 投放交互：拖拽源卡到菜单区域或世界投放目标，由上层 adapter 解析目标和提交规则。")
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

enum class EWacomFirstPersonCardInteractionCueKind : uint8
{
	None,
	InvalidPreview,
	Deny
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardLayerFeedbackKind : uint8
{
	None UMETA(DisplayName = "None"),
	Retained UMETA(DisplayName = "Retained"),
	CardUseReform UMETA(DisplayName = "Card Use Reform"),
	HandTargetImpact UMETA(DisplayName = "Hand Target Impact"),
	CardDataRewrite UMETA(DisplayName = "Card Data Rewrite"),
	CardUseReformOut UMETA(DisplayName = "Card Use Reform Out"),
	CardUseReformIn UMETA(DisplayName = "Card Use Reform In"),
	RetainedRelease UMETA(DisplayName = "Retained Release"),
	EffectBadgeChange UMETA(DisplayName = "Effect Badge Change")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardEffectBadgeChangeKind : uint8
{
	ValueChanged UMETA(DisplayName = "Value Changed"),
	Added UMETA(DisplayName = "Added"),
	Removed UMETA(DisplayName = "Removed")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardEffectBadgeValueDirection : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Increase UMETA(DisplayName = "Increase"),
	Decrease UMETA(DisplayName = "Decrease")
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardEffectBadgeChange
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	FName PresentationKey;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	EWacomCardViewEffectBadgeKind BadgeKind = EWacomCardViewEffectBadgeKind::Generic;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	EWacomFirstPersonCardEffectBadgeChangeKind ChangeKind =
		EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	EWacomFirstPersonCardEffectBadgeValueDirection Direction =
		EWacomFirstPersonCardEffectBadgeValueDirection::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	int32 OldValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	int32 NewValue = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	int32 Seed = 0;
};

UENUM(BlueprintType, meta = (Bitflags))
enum class EWacomFirstPersonCardDataRewriteField : uint8
{
	None = 0 UMETA(Hidden),
	Cost = 1 << 0 UMETA(DisplayName = "Cost")
};
ENUM_CLASS_FLAGS(EWacomFirstPersonCardDataRewriteField)

UENUM(BlueprintType)
enum class EWacomFirstPersonCardDataRewriteTone : uint8
{
	Neutral UMETA(DisplayName = "Neutral"),
	Beneficial UMETA(DisplayName = "Beneficial"),
	Detrimental UMETA(DisplayName = "Detrimental")
};

struct WACOMAPP_API FWacomFirstPersonCardInteractionCueView
{
	EWacomFirstPersonCardInteractionCueKind Kind =
		EWacomFirstPersonCardInteractionCueKind::None;
	FLinearColor Color = FLinearColor(1.0f, 0.12f, 0.08f, 1.0f);
	FLinearColor AccentColor = FLinearColor(0.18f, 0.32f, 0.48f, 1.0f);
	float Amount = 0.0f;
	float Progress = 0.0f;
	float CornerInsetPixels = 8.0f;
	float CornerLengthPixels = 14.0f;
	float CornerThicknessPixels = 3.0f;
	float TightenPixels = 0.0f;
	float CrackLengthPixels = 30.0f;
	float CrackThicknessPixels = 3.0f;
	FVector2D Direction = FVector2D(0.0f, -1.0f);
	int32 Seed = 0;
	bool bReducedMotion = false;
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

/** Theme-neutral authoring values for the first-person source-card selection material. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSelectionStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "像素选中扫光和内侧硬边的主色；默认暖象牙金。"))
	FLinearColor PrimaryColor = FLinearColor(1.0f, 0.88f, 0.48f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "像素选中外侧硬边和折射过渡的次色；默认低饱和蓝。"))
	FLinearColor SecondaryColor = FLinearColor(0.50f, 0.68f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "像素亮点尖峰颜色；默认紫红，仅占少量高亮。"))
	FLinearColor AccentColor = FLinearColor(0.86f, 0.34f, 0.92f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "选中进入扫光时长，单位为秒；推荐 0.24 到 0.42。"))
	float EnterDurationSeconds = 0.34f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "选中退出淡出时长，单位为秒；推荐 0.10 到 0.22。"))
	float ExitDurationSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "保持选中时弱扫光的循环周期，单位为秒；推荐 2 到 4。"))
	float SustainPeriodSeconds = 2.6f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "弱扫光相对进入扫光的强度，推荐 0.18 到 0.38。"))
	float SustainIntensity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "横向像素网格列数；当前卡面推荐 80 到 112，默认 96。"))
	float GridColumns = 96.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "扫光角度，单位为度；推荐 35 到 50。"))
	float SweepAngleDegrees = 42.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "扫光宽度，UV 单位；推荐 0.10 到 0.20。"))
	float SweepWidth = 0.145f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "进入扫光亮度强度；推荐 0.7 到 1.25。"))
	float SweepIntensity = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "实时 Alpha 内侧硬边宽度，单位为卡面像素；推荐 1 到 2。"))
	float InnerEdgePixels = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "实时 Alpha 外侧硬边宽度，单位为卡面像素；推荐 3 到 6。"))
	float OuterEdgePixels = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "沿轮廓出现像素亮点的密度，0 到 1；推荐 0.10 到 0.25。"))
	float GlintDensity = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "轮廓像素亮点移动速度，单位为循环/秒；推荐 0.35 到 0.9。"))
	float GlintSpeed = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Selection", meta = (ToolTip = "像素亮点分布 Mask；推荐使用 Masks 压缩、关闭 sRGB、Nearest 过滤和 Mipmaps。为空时材质使用程序化方格噪声。"))
	TObjectPtr<UTexture2D> PixelClusterMask = nullptr;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSelectionConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Selection")
	bool bEnabled = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Selection")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Selection")
	FWacomFirstPersonCardSelectionStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardSelectionView
{
	bool bTargetActive = false;
	bool bReducedMotion = false;
	float Amount = 0.0f;
	float EnterProgress = 0.0f;
	float TimeSeconds = 0.0f;
	FWacomFirstPersonCardSelectionStyleData Style;
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardUseEffectKind : uint8
{
	DiamondWave UMETA(DisplayName = "Diamond Wave", ToolTip = "旧版中心向外菱形波放电消失；保留为可回退预设。"),
	EdgeFlip UMETA(DisplayName = "Pixel Edge Flip", ToolTip = "卡牌单面横向压缩成发光像素边后收起；打出后仍在手牌时会在目标槽位反向翻回。")
};

/** Playback semantics for a normal Played card-use surface effect. Visual tuning lives in the referenced material instance. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardUseEffectStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ToolTip = "普通使用卡牌采用的表现算法；Diamond Wave 保留旧菱形波，Pixel Edge Flip 使用单面 90 度像素翻面收牌。"))
	EWacomFirstPersonCardUseEffectKind EffectKind = EWacomFirstPersonCardUseEffectKind::EdgeFlip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ToolTip = "普通使用卡牌离场时临时绑定到唯一 Retainer 的材质实例；视觉颜色、菱形密度和波宽直接在材质实例中调整，必须保留 Texture 参数合同。"))
	TObjectPtr<UMaterialInstance> SurfaceEffectMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (Units = "s", ToolTip = "普通使用效果总时长，单位为秒；默认 0.36，推荐 0.28 到 0.46，不影响规则结算。"))
	float DurationSeconds = 0.36f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (Units = "s", ToolTip = "卡牌提交后保持完整卡面的中心充能时间，单位为秒；默认 0.04，推荐 0.02 到 0.07，包含在总时长内。"))
	float ConfirmHoldSeconds = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", Units = "s", ToolTip = "像素翻面开始时闪边脉冲的持续时间，单位为秒；默认 0.05，推荐 0.035 到 0.08，只影响一次性视觉冲击。"))
	float EdgeFlipImpactSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", ToolTip = "翻面确认阶段额外上提距离，单位为 UMG 逻辑像素；默认 12，推荐 8 到 18，不改变布局和命中区域。"))
	float EdgeFlipLiftPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", ToolTip = "翻面确认阶段额外缩放倍率；默认 1.04，推荐 1.02 到 1.07，不改变命中区域。"))
	float EdgeFlipScaleMultiplier = 1.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", ToolTip = "卡牌压缩到侧边时保留的最小横向比例；默认 0.06，推荐 0.035 到 0.10。必须大于零，避免 RenderTransform 奇异。"))
	float EdgeFlipMinimumHorizontalScale = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip|Return To Hand", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", Units = "s", ToolTip = "成功使用后仍留在手牌的卡牌，从提交姿态压缩到发光侧边的时间；默认 0.22 秒，推荐 0.16 到 0.28。"))
	float EdgeFlipReformOutSeconds = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip|Return To Hand", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", Units = "s", ToolTip = "卡牌成为侧边后、在目标槽位反向展开前的隐藏换位停顿；默认 0.06 秒，推荐 0.03 到 0.10。"))
	float EdgeFlipReformHiddenHoldSeconds = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip|Return To Hand", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", Units = "s", ToolTip = "卡牌在最新手牌槽位从侧边反向展开到完整卡面的时间；默认 0.18 秒，推荐 0.14 到 0.24。"))
	float EdgeFlipReformInSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Edge Flip|Return To Hand", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardUseEffectKind::EdgeFlip", Units = "s", ToolTip = "反向展开后轻微落定的时间；默认 0.04 秒，推荐 0.02 到 0.07。"))
	float EdgeFlipReformSettleSeconds = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Return To Hand", meta = (Units = "s", ToolTip = "成功使用后仍留在手牌的卡牌，从提交位置完全消失所需时间；默认 0.28 秒，推荐 0.22 到 0.34，不改变规则和手牌布局。"))
	float ReformDissolveOutSeconds = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Return To Hand", meta = (Units = "s", ToolTip = "卡牌完全透明后、在目标手牌槽位重新生成前的停顿；默认 0.08 秒，推荐 0.04 到 0.12，换位发生在此阶段且玩家不可见。"))
	float ReformHiddenHoldSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect|Return To Hand", meta = (Units = "s", ToolTip = "卡牌在最新手牌槽位由外圈和四角向中心重新生成的时间；默认 0.24 秒，推荐 0.18 到 0.30，不影响命中区域。"))
	float ReformBuildInSeconds = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ToolTip = "普通使用菱形波开始时立即播放的一次性 UI 2D 音效；留空表示静音。"))
	TObjectPtr<USoundBase> StartSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ToolTip = "普通使用音效音量倍率；1 为资产原始音量，推荐 0.5 到 1.2。"))
	float StartSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ToolTip = "普通使用音效基础音高倍率；1 为资产原始音高，推荐 0.85 到 1.15。"))
	float StartSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Use Effect", meta = (ToolTip = "每次播放在基础音高附近的随机比例；0.03 表示约正负 3%。"))
	float StartSoundPitchVariation = 0.03f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardUseEffectConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Card Use Effect")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Card Use Effect")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Card Use Effect")
	FWacomFirstPersonCardUseEffectStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardUseEffectView
{
	bool bActive = false;
	bool bReducedMotion = false;
	float Amount = 0.0f;
	float FlipProgress = 0.0f;
	float ImpactProgress = 0.0f;
	float TimeSeconds = 0.0f;
	FWacomFirstPersonCardUseEffectStyleData Style;
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardPlayedDissolveEffectKind : uint8
{
	PixelAsh UMETA(DisplayName = "Pixel Ash", ToolTip = "现有斜向上升像素灰烬消散。"),
	OrderedDither UMETA(DisplayName = "Ordered Dither", ToolTip = "Bayer 有序抖动透明棋盘消散，并让原卡面像素以主方向流和少量四周散射离场。")
};

/** Ordered-dither-only authoring values for a Played dissolve Style. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardOrderedDitherStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "Bayer 阈值矩阵边长；运行时只使用 4 或 8，小于等于 4 归一为 4，其余归一为 8。默认 4 可获得清晰的像素网点。"))
	int32 BayerMatrixSize = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "原色卡面与完全透明区域之间的 Bayer 棋盘过渡带归一化宽度；推荐 0.12 到 0.24，默认 0.18。"))
	float BandWidth = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "刚离开卡面的原色残留网点密度，0 到 1；推荐 0.22 到 0.34，默认 0.28。网点不参与接触阴影。"))
	float ResidueDensity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "残留网点在消散前沿后方存活的归一化带宽；默认 0.48，在 0.40 秒总时长和 0.05 秒停顿下约对应 0.14 秒。推荐 0.35 到 0.58。"))
	float ResidueTrailWidth = 0.48f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "主方向残留网点的最大移动距离，单位为 Retainer 像素；推荐 26 到 42，默认 34，不影响布局或命中。四散网点会再乘 ScatterStrength。"))
	float ResidueTravelPixels = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "沿卡牌消散方向移动的残片比例，0 到 1；推荐 0.65 到 0.85，默认 0.75。其余残片向四周散开。"))
	float ResidueMainDirectionRatio = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (Units = "deg", ToolTip = "主方向残片围绕卡牌消散方向的最大随机偏角，单位为度；推荐 10 到 28，默认 18。运行时只需 0 到 180。"))
	float ResidueDirectionSpreadDegrees = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve|Ordered Dither", meta = (ToolTip = "四散残片相对主方向移动距离的倍率；推荐 0.35 到 0.75，默认 0.55。数值越大越接近爆散。"))
	float ResidueScatterStrength = 0.55f;
};

/** Theme-neutral authoring values for a Played surface dissolve. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardPlayedDissolveStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "该 Style 使用的 Played 消散算法；Pixel Ash 保留旧灰烬，Ordered Dither 使用独立 Bayer 透明棋盘材质。"))
	EWacomFirstPersonCardPlayedDissolveEffectKind EffectKind =
		EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "Played 期间临时切换到的单 Retainer Surface Effect 材质；必须使用 Texture 作为 Retainer 采样参数。"))
	TObjectPtr<UMaterialInterface> SurfaceEffectMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "五级灰度硬边噪声 Mask；推荐 256×256、Masks 压缩、关闭 sRGB、Nearest、NoMipmaps、UI。"))
	TObjectPtr<UTexture2D> NoiseTexture = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (Units = "s", ToolTip = "完整 Played 消散时长，单位为秒；推荐 0.32 到 0.48。"))
	float DurationSeconds = 0.40f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (Units = "s", ToolTip = "提交成功后保持完整卡面的确认停顿，单位为秒；推荐 0.03 到 0.08，包含在总时长内。"))
	float ConfirmHoldSeconds = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "横向像素网格列数；推荐 72 到 120，默认 96 接近当前像素卡面密度。"))
	float GridColumns = 96.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (Units = "deg", ToolTip = "消散前沿在 UV 空间中的方向角，单位为度；默认 -78 表示以向上为主并略向右推进。"))
	float DirectionAngleDegrees = -78.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "噪声对消散前沿的扰动强度；推荐 0.16 到 0.42。"))
	float Jitter = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 消散前沿主色；默认暖象牙金。"))
	FLinearColor EdgeColor = FLinearColor(1.0f, 0.82f, 0.34f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 消散前沿少量折射高光色；默认低饱和蓝。"))
	FLinearColor EdgeAccentColor = FLinearColor(0.46f, 0.66f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 消散前沿宽度，使用归一化阈值单位；推荐 0.025 到 0.07。"))
	float EdgeWidth = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 消散前沿亮度倍率；推荐 0.8 到 1.8。"))
	float EdgeIntensity = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 前沿附近生成灰烬像素簇的密度，0 到 1；推荐 0.10 到 0.26。"))
	float AshDensity = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 消散前沿后方允许灰烬存活的阈值宽度；推荐 0.08 到 0.20。"))
	float AshTrailWidth = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 灰烬最大上升距离，单位为 Retainer 像素；推荐 14 到 30，避免超过 WBP bleed。"))
	float AshLiftPixels = 22.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::PixelAsh", EditConditionHides, ToolTip = "Pixel Ash 灰烬最大横向漂移，单位为 Retainer 像素；推荐 3 到 10。"))
	float AshDriftPixels = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (EditCondition = "EffectKind == EWacomFirstPersonCardPlayedDissolveEffectKind::OrderedDither", EditConditionHides, ShowOnlyInnerProperties, ToolTip = "Ordered Dither 独立的 Bayer 透明棋盘与短上浮残留参数。"))
	FWacomFirstPersonCardOrderedDitherStyleData OrderedDither;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "接触阴影在总时长前多少比例内完全消退，0 到 1；默认 0.25 约等于前 0.10 秒。"))
	float ShadowFadeFraction = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "Played 消散开始时立即播放的一次性 UI 2D 音效；留空表示静音。"))
	TObjectPtr<USoundBase> StartSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "Played 消散音效音量倍率；1 为资产原始音量，推荐 0.5 到 1.2。"))
	float StartSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "Played 消散音效基础音高倍率；1 为资产原始音高，推荐 0.85 到 1.15。"))
	float StartSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Played Dissolve", meta = (ToolTip = "每次播放在基础音高附近的随机比例；0.03 表示约正负 3%。"))
	float StartSoundPitchVariation = 0.03f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardPlayedDissolveConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Played Dissolve")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Played Dissolve")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Played Dissolve")
	FWacomFirstPersonCardPlayedDissolveStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardPlayedDissolveView
{
	bool bActive = false;
	bool bReducedMotion = false;
	float Amount = 0.0f;
	float TimeSeconds = 0.0f;
	float Seed = 0.0f;
	FWacomFirstPersonCardPlayedDissolveStyleData Style;
};

/** Playback and physical-response authoring for a successful hand-card target impact. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardHandTargetImpactStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact", meta = (ToolTip = "手牌目标预演和成功刻印期间临时绑定到唯一 Retainer 的 UI 材质实例；必须保留 Texture 参数合同。颜色、网格和刻线外观在材质实例中调整。"))
	TObjectPtr<UMaterialInstance> SurfaceEffectMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Preview", meta = (Units = "s", ToolTip = "有效手牌目标刻印从无到完整弱预演的淡入时长，单位为秒；默认 0.10，推荐 0.06 到 0.16。"))
	float PreviewFadeInSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Preview", meta = (Units = "s", ToolTip = "弱刻印亮度呼吸周期，单位为秒；默认 0.90，推荐 0.70 到 1.30。只改变材质亮度，不改变尺寸或位置。"))
	float PreviewPeriodSeconds = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Commit", meta = (Units = "s", ToolTip = "成功提交后等待目标刻印开始压下的时间，单位为秒；默认 0.07，推荐 0.04 到 0.10，让源卡表现先启动。"))
	float CommitDelaySeconds = 0.07f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Commit", meta = (Units = "s", ToolTip = "从成功提交到刻印峰值和离场 Gate 打开的时间，单位为秒；默认 0.11，推荐 0.09 到 0.14。"))
	float DepartureGateSeconds = 0.11f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Commit", meta = (Units = "s", ToolTip = "从成功提交到实体回弹峰值的时间，单位为秒；默认 0.16，推荐 0.13 到 0.20。"))
	float ReboundPeakSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Commit", meta = (Units = "s", ToolTip = "成功刻印完整播放时长，单位为秒；默认 0.29，推荐 0.24 到 0.38。目标留手时在此时归入最新手牌布局。"))
	float CommitDurationSeconds = 0.29f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Motion", meta = (ToolTip = "刻印峰值时目标卡的压缩倍率；默认 0.96，推荐 0.93 到 0.98，不改变命中区域。"))
	float CompressionScale = 0.96f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Motion", meta = (ToolTip = "刻印峰值前目标卡向下压入的距离，单位为 UMG 逻辑像素；默认 4，推荐 2 到 7，不影响布局。"))
	float CompressionTranslationPixels = 4.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Motion", meta = (ToolTip = "目标卡实体回弹峰值倍率；默认 1.05，推荐 1.03 到 1.08，不改变命中区域。"))
	float ReboundScale = 1.05f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Motion", meta = (ToolTip = "目标卡实体回弹时向上提起的距离，单位为 UMG 逻辑像素；默认 5，推荐 3 到 9，不影响布局。"))
	float ReboundLiftPixels = 5.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Motion", meta = (ToolTip = "刻印播放期间额外增加的绘制层级；默认 900，推荐 700 到 1200，只影响表现，不改变手牌顺序。"))
	int32 ZOrderBoost = 900;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Audio", meta = (ToolTip = "刻印峰值播放的一次性纸面压印或轻扣 UI 2D 音效；Preview 不播放，留空表示静音。"))
	TObjectPtr<USoundBase> ImpactSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Audio", meta = (ToolTip = "刻印音效音量倍率；1 为资产原始音量，推荐 0.5 到 1.2。"))
	float ImpactSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Audio", meta = (ToolTip = "刻印音效基础音高倍率；1 为资产原始音高，推荐 0.85 到 1.15。"))
	float ImpactSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Hand Target Impact|Audio", meta = (ToolTip = "每次刻印音高在基础值附近的随机比例；0.03 表示约正负 3%。"))
	float ImpactSoundPitchVariation = 0.03f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardHandTargetImpactConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Hand Target Impact")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Hand Target Impact")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Hand Target Impact")
	FWacomFirstPersonCardHandTargetImpactStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardHandTargetImpactView
{
	bool bActive = false;
	bool bPreview = false;
	bool bCommitted = false;
	bool bReducedMotion = false;
	float PreviewAmount = 0.0f;
	float CommitProgress = 0.0f;
	float TimeSeconds = 0.0f;
	float Seed = 0.0f;
	FWacomFirstPersonCardHandTargetImpactStyleData Style;
};

/** Playback and material authoring for explicit card-face data rewrites. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDataRewriteStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite", meta = (ToolTip = "费用变化期间临时绑定到 CostDigitImage 的 UI 材质实例；材质需要支持旧、新 PaperSprite 图集纹理与 UV Rect。它不替换整张卡牌的 Retainer 材质。"))
	TObjectPtr<UMaterialInstance> DigitRewriteMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Preview", meta = (Units = "s", ToolTip = "手牌目标费用预测的呼吸周期，单位为秒；默认 0.85，推荐 0.65 到 1.10。预览只作用于 CostDigitImage，不修改权威费用。"))
	float PreviewPulsePeriodSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Preview", meta = (ToolTip = "费用预测呼吸最低透明度；默认 0.38，推荐 0.25 到 0.55。只影响材质输出，不影响布局。"))
	float PreviewMinimumOpacity = 0.38f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Preview", meta = (ToolTip = "费用预测呼吸最高透明度；默认 0.90，推荐 0.75 到 1.0。"))
	float PreviewMaximumOpacity = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Preview", meta = (ToolTip = "费用预测呼吸峰值亮度倍率；默认 1.45，推荐 1.15 到 1.80。颜色由材质实例中的增益/减益色板控制。"))
	float PreviewPeakBrightness = 1.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Timing", meta = (Units = "s", ToolTip = "一次费用数字消散重组的完整时长，单位为秒；默认 0.34，推荐 0.28 到 0.42。只影响 CostDigitImage，不阻塞规则或输入。"))
	float DurationSeconds = 0.34f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Timing", meta = (Units = "s", ToolTip = "旧费用数字完成像素消散的时刻，单位为秒；默认 0.10，推荐 0.07 到 0.14。"))
	float OldDissolveEndSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Timing", meta = (Units = "s", ToolTip = "新费用数字开始从中心重组的时刻，单位为秒；默认 0.12，推荐 0.09 到 0.17，应不早于旧数字消散结束。"))
	float NewRevealStartSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Timing", meta = (Units = "s", ToolTip = "新费用数字完成像素重组的时刻，单位为秒；默认 0.25，推荐 0.20 到 0.31，应小于完整时长。"))
	float NewRevealEndSeconds = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Motion", meta = (ToolTip = "旧数字消散完成及新数字开始重组时的最小缩放倍率；默认 0.88，推荐 0.82 到 0.94。只写 CostDigitImage RenderTransform，不改变布局。"))
	float MinimumScale = 0.88f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Motion", meta = (ToolTip = "新数字重组后的回弹峰值缩放倍率；默认 1.10，推荐 1.04 到 1.16。随后在完整时长内缓出归位。"))
	float OvershootScale = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Motion", meta = (Units = "s", ToolTip = "新数字到达回弹峰值的时刻，单位为秒；默认 0.26，推荐 0.22 到 0.30，应不早于新数字重组结束。"))
	float OvershootPeakSeconds = 0.26f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Timing", meta = (Units = "s", ToolTip = "同一批多张卡费用变化时相邻卡开始重写的错峰间隔，单位为秒；默认 0.045，推荐 0.02 到 0.06。"))
	float SequenceStaggerSeconds = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Timing", meta = (Units = "s", ToolTip = "同一批多张卡重写允许的最大起播等待，单位为秒；默认 0.14，避免大手牌让反馈拖得过长。"))
	float MaxSequenceDelaySeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Audio", meta = (ToolTip = "费用新值开始显露时播放的一次性短促像素印刷 UI 2D 音效；同一批只由第一张卡请求，留空表示静音。"))
	TObjectPtr<USoundBase> RewriteSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Audio", meta = (ToolTip = "重写音效音量倍率；1 为资产原始音量，推荐 0.4 到 1.0。"))
	float RewriteSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Audio", meta = (ToolTip = "重写音效基础音高倍率；1 为资产原始音高，推荐 0.9 到 1.15。"))
	float RewriteSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Data Rewrite|Audio", meta = (ToolTip = "每批重写音高在基础值附近的随机比例；0.03 表示约正负 3%。"))
	float RewriteSoundPitchVariation = 0.03f;
};

/** Direct EffectBadge digit feedback; it never takes ownership of the card Retainer. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardEffectBadgeFeedbackStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge", meta = (ToolTip = "效果徽章数字变化时临时绑定到数字 UImage 的 UI 材质实例；不占用卡牌 Retainer。为空时仍保留局部透明度与缩放反馈。"))
	TObjectPtr<UMaterialInstance> DigitFeedbackMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Preview", meta = (Units = "s", ToolTip = "目标预览淡入时间，单位为秒；默认 0.10，推荐 0.06 到 0.16。"))
	float PreviewEnterSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Preview", meta = (Units = "s", ToolTip = "目标预览淡出时间，单位为秒；默认 0.08，推荐 0.05 到 0.14。"))
	float PreviewExitSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Preview", meta = (Units = "s", ToolTip = "预测数字低强度呼吸周期，单位为秒；默认 0.85，推荐 0.65 到 1.10。"))
	float PreviewPulsePeriodSeconds = 0.85f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Preview", meta = (ToolTip = "被规则跳过的 Badge 透明度；默认 0.28，推荐 0.18 到 0.40。"))
	float SkippedOpacity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Timing", meta = (Units = "s", ToolTip = "数值变化完整时长，单位为秒；默认 0.28，推荐 0.22 到 0.36。"))
	float ValueChangeDurationSeconds = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Timing", meta = (Units = "s", ToolTip = "新增 Badge 展开时长，单位为秒；默认 0.22，推荐 0.16 到 0.30。"))
	float AddedDurationSeconds = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Timing", meta = (Units = "s", ToolTip = "移除 Badge 收束时长，单位为秒；默认 0.18，推荐 0.12 到 0.26。"))
	float RemovedDurationSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Timing", meta = (Units = "s", ToolTip = "幸存 Badge 重排归位时长，单位为秒；默认 0.14，推荐 0.10 到 0.22。"))
	float ReflowDurationSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Timing", meta = (Units = "s", ToolTip = "同卡多项变化的错峰间隔，单位为秒；默认 0.035，推荐 0.02 到 0.06。"))
	float SequenceStaggerSeconds = 0.035f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Timing", meta = (Units = "s", ToolTip = "同卡多项变化最大附加等待，单位为秒；默认 0.12。"))
	float MaxSequenceDelaySeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Audio", meta = (ToolTip = "正式 Badge 变化在新值开始显现时的一次性 2D 音效；同批只由第一项请求，留空表示静音。"))
	TObjectPtr<USoundBase> ChangeSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Audio", meta = (ToolTip = "Badge 变化音效音量倍率；1 为原始音量。"))
	float ChangeSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Audio", meta = (ToolTip = "Badge 变化音效基础音高倍率；1 为原始音高。"))
	float ChangeSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Effect Badge|Audio", meta = (ToolTip = "Badge 变化音高随机范围；0.03 表示约正负 3%。"))
	float ChangeSoundPitchVariation = 0.03f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardEffectBadgeFeedbackConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	FWacomFirstPersonCardEffectBadgeFeedbackStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardEffectBadgeFeedbackItemView
{
	FName PresentationKey;
	EWacomFirstPersonCardEffectBadgeChangeKind ChangeKind =
		EWacomFirstPersonCardEffectBadgeChangeKind::ValueChanged;
	EWacomFirstPersonCardEffectBadgeValueDirection Direction =
		EWacomFirstPersonCardEffectBadgeValueDirection::Neutral;
	int32 OldValue = 0;
	int32 NewValue = 0;
	int32 Seed = 0;
	float OldDissolveAmount = 0.0f;
	float NewRevealAmount = 0.0f;
	float RootScale = 1.0f;
	float RootOpacity = 1.0f;
	/** Installs the zero-progress digit MID before a staggered item becomes active. */
	bool bPrepareMaterial = false;
	bool bActive = false;
};

struct WACOMAPP_API FWacomFirstPersonCardEffectBadgeFeedbackView
{
	bool bActive = false;
	bool bReducedMotion = false;
	bool bCompleted = false;
	float ReflowProgress = 0.0f;
	FWacomFirstPersonCardEffectBadgeFeedbackStyleData Style;
	TArray<FWacomFirstPersonCardEffectBadgeFeedbackItemView> Items;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDataRewriteConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	FWacomFirstPersonCardDataRewriteStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardDataRewriteView
{
	bool bActive = false;
	bool bReducedMotion = false;
	int32 FieldMask = 0;
	EWacomFirstPersonCardDataRewriteTone Tone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	float Progress = 0.0f;
	float OldDissolveAmount = 0.0f;
	float NewRevealAmount = 0.0f;
	float DigitScale = 1.0f;
	float Seed = 0.0f;
	FWacomFirstPersonCardDataRewriteStyleData Style;
};

/** Direct CostDigitImage preview; independent from the card Retainer surface. */
struct WACOMAPP_API FWacomFirstPersonCardCostPreviewView
{
	bool bActive = false;
	float PreviewAmount = 0.0f;
	float PulseAmount = 0.0f;
	EWacomFirstPersonCardDataRewriteTone Tone = EWacomFirstPersonCardDataRewriteTone::Neutral;
	int32 Seed = 0;
	FWacomFirstPersonCardDataRewriteStyleData Style;
};

/** Authoring data for the Battle Drawn card-back reveal. The Drawn enter transition owns time. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDrawRevealStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal", meta = (ToolTip = "抽牌翻面期间临时绑定到唯一 Retainer 的 UI 材质实例；材质负责牌背、正反面混合与边缘高光，必须保留 Texture Retainer 参数合同。"))
	TObjectPtr<UMaterialInstance> SurfaceEffectMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Timing", meta = (ToolTip = "Drawn Enter 归一化进度中保持完整牌背的结束比例；默认 0.45，推荐 0.35 到 0.55。它不会改变抽牌总时长。"))
	float BackHoldEndProgress = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Timing", meta = (ToolTip = "横向压缩到最窄并切换到正面的进度；默认 0.615，推荐 0.55 到 0.68，应晚于牌背保持结束。"))
	float FaceSwitchProgress = 0.615f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Timing", meta = (ToolTip = "正面从侧边展开完成的进度；默认 0.78，推荐 0.70 到 0.84，应晚于正面切换。"))
	float FaceExpandEndProgress = 0.78f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Motion", meta = (ToolTip = "翻面中点的横向最小缩放；默认 0.06，推荐 0.03 到 0.12。只修改 RenderTransform，不改变布局或命中区域。"))
	float MinimumHorizontalScale = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Timing", meta = (ToolTip = "接近手牌时开始落定压缩的进度；默认 0.82，推荐 0.78 到 0.88。"))
	float LandingStartProgress = 0.82f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Timing", meta = (ToolTip = "落定压缩达到峰值的进度；默认 0.90，推荐 0.86 到 0.94，应晚于落定开始。"))
	float LandingPeakProgress = 0.90f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Motion", meta = (ToolTip = "落定峰值缩放；默认 X=1.035、Y=0.96，推荐 X 1.01 到 1.06、Y 0.92 到 0.99。只影响局部反馈。"))
	FVector2D LandingScale = FVector2D(1.035f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Motion", meta = (ToolTip = "落定峰值向下位移，单位为 UMG 逻辑像素；默认 3，推荐 1 到 6，不改变手牌布局。"))
	float LandingTranslationYPixels = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Reduced Motion", meta = (ToolTip = "弱化动态时牌背开始交叉淡化成正面的 Drawn Enter 进度；默认 0.55，推荐 0.45 到 0.65。"))
	float ReducedCrossFadeStartProgress = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Draw Reveal|Reduced Motion", meta = (ToolTip = "弱化动态时牌背交叉淡化完成的 Drawn Enter 进度；默认 0.75，推荐 0.65 到 0.85。"))
	float ReducedCrossFadeEndProgress = 0.75f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDrawRevealConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Draw Reveal")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Draw Reveal")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Draw Reveal")
	FWacomFirstPersonCardDrawRevealStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardDrawRevealView
{
	bool bActive = false;
	bool bWaiting = false;
	bool bReducedMotion = false;
	float Progress = 0.0f;
	FWacomFirstPersonCardDrawRevealStyleData Style;
};

enum class EWacomFirstPersonCardGainRevealRarity : uint8
{
	Neutral,
	White,
	Blue,
	Yellow,
	Purple
};

/** Authoring data for the front-facing pixel crystallization used by explicit Gained enters. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardGainRevealStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal", meta = (ToolTip = "获得卡牌结晶入场期间临时绑定到唯一 Retainer 的 UI 材质实例；必须保留 Texture、Fake-3D 与实时 Alpha 接触阴影合同。"))
	TObjectPtr<UMaterialInstance> SurfaceEffectMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal|Timing", meta = (ToolTip = "Gained Enter 归一化进度中外缘结晶种子建立完成的比例；默认 0.12，推荐 0.08 到 0.20，不改变获得卡牌入场总时长。"))
	float SeedEstablishEndProgress = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal|Timing", meta = (ToolTip = "Gained Enter 归一化进度中正面卡牌完成结晶组装的比例；默认 0.62，推荐 0.52 到 0.72。"))
	float AssemblyEndProgress = 0.62f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal|Timing", meta = (ToolTip = "稀有度色硬像素外缘达到峰值的归一化进度；默认 0.70，推荐 0.64 到 0.78。"))
	float RarityEdgePeakProgress = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal|Timing", meta = (ToolTip = "外溢结晶和稀有度外缘完全收束的归一化进度；默认 0.84，推荐 0.78 到 0.92。"))
	float SettleEndProgress = 0.84f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal|Reduced Motion", meta = (ToolTip = "弱化动态时正面开始均匀交叉显现的 Gained Enter 进度；默认 0.25，推荐 0.15 到 0.35。"))
	float ReducedCrossFadeStartProgress = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Gain Reveal|Reduced Motion", meta = (ToolTip = "弱化动态时正面交叉显现完成的 Gained Enter 进度；默认 0.65，推荐 0.50 到 0.75。"))
	float ReducedCrossFadeEndProgress = 0.65f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardGainRevealConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Gain Reveal")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Gain Reveal")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Gain Reveal")
	FWacomFirstPersonCardGainRevealStyleData Style;
};

struct WACOMAPP_API FWacomFirstPersonCardGainRevealView
{
	bool bActive = false;
	bool bWaiting = false;
	bool bReducedMotion = false;
	float Progress = 0.0f;
	float Seed = 0.0f;
	EWacomFirstPersonCardGainRevealRarity Rarity =
		EWacomFirstPersonCardGainRevealRarity::Neutral;
	FWacomFirstPersonCardGainRevealStyleData Style;
};

/** Authoring data for the low-cost retained-card seal rendered by the shared Retainer. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardRetainSealStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Retain Seal", meta = (ToolTip = "保留牌封存期间临时绑定到唯一 Retainer 的 UI 材质实例；必须保留 Texture、Fake-3D 与实时 Alpha 接触阴影合同。"))
	TObjectPtr<UMaterialInstance> SurfaceEffectMaterialInstance = nullptr;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardRetainSealConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float SealingDurationSeconds = 0.32f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float SequenceStaggerSeconds = 0.045f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float PeakLiftPixels = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float PeakScale = 1.025f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float HeldLiftPixels = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float HeldScale = 1.01f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	float ReleaseDurationSeconds = 0.16f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	FWacomFirstPersonCardRetainSealStyleData Style;
};

enum class EWacomFirstPersonCardRetainSealPhase : uint8
{
	Inactive,
	Sealing,
	Held,
	Releasing
};

struct WACOMAPP_API FWacomFirstPersonCardRetainSealView
{
	bool bActive = false;
	bool bReducedMotion = false;
	EWacomFirstPersonCardRetainSealPhase Phase =
		EWacomFirstPersonCardRetainSealPhase::Inactive;
	float Progress = 0.0f;
	float Seed = 0.0f;
	FWacomFirstPersonCardRetainSealStyleData Style;
};

/** Card-surface material state shared by mutually exclusive Retainer effects. */
struct WACOMAPP_API FWacomFirstPersonCardSurfaceEffectView
{
	FWacomFirstPersonCardDrawRevealView DrawReveal;
	FWacomFirstPersonCardGainRevealView GainReveal;
	FWacomFirstPersonCardSelectionView Selection;
	FWacomFirstPersonCardHandTargetImpactView HandTargetImpact;
	FWacomFirstPersonCardUseEffectView CardUse;
	FWacomFirstPersonCardPlayedDissolveView PlayedDissolve;
	FWacomFirstPersonCardRetainSealView RetainSeal;
};

/** Authoring contract for a discard-to-draw pile transfer. The material instance owns glyph appearance. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardPileTransferStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "像素牌印使用的 UI 材质实例；材质负责卡背图案，位置、旋转、缩放和透明度由 Slate 批量顶点驱动。留空时不播放跨牌堆迁移。"))
	TObjectPtr<UMaterialInstance> GlyphMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "单枚牌印尺寸，单位为 UMG 逻辑像素；默认 42×66，推荐宽 30 到 60、高 48 到 90，不影响手牌布局或命中。"))
	FVector2D GlyphSize = FVector2D(42.0f, 66.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "牌印从弃牌堆蓄力后开始发射的等待时间，单位为秒；默认 0.08，推荐 0.04 到 0.12。"))
	float StartChargeSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "每枚牌印从弃牌堆飞到抽牌堆的时间，单位为秒；默认 0.36，推荐 0.26 到 0.48。"))
	float FlightSeconds = 0.36f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "确定性弧线路径数量；默认 3，推荐 2 到 5。运行时至少使用 1 条，不改变卡牌顺序。"))
	int32 LaneCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "相邻牌印的固定发射间隔，单位为秒；默认 0.045，推荐 0.03 到 0.07。数量较多时不会自动压缩，整体时长会随真实洗回数量自然增长。"))
	float BaseStaggerSeconds = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "最后一枚主体抵达后等待飘散像素自然排空的最短时间，单位为秒；默认 0.24，推荐 0.18 到 0.32。运行时还会确保不早于最长粒子寿命。"))
	float SettleSeconds = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "弧高占起止锚点距离的比例；默认 0.18，推荐 0.12 到 0.26，随后受最小和最大弧高限制。"))
	float ArcHeightRatio = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "最小弧高，单位为 UMG 逻辑像素；默认 48，推荐 28 到 72。"))
	float MinArcHeightPixels = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "最大弧高，单位为 UMG 逻辑像素；默认 128，推荐 96 到 180。"))
	float MaxArcHeightPixels = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Discard", meta = (Units = "s", ToolTip = "普通弃牌从完整卡面收束为牌印的总时长，单位为秒；默认 0.11，推荐 0.08 到 0.16。只影响离场表现，不改变规则结算。"))
	float DiscardCollapseSeconds = 0.11f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Discard", meta = (Units = "s", ToolTip = "普通弃牌收束开始后，牌印开始与卡面交叉显现的时间，单位为秒；默认 0.06，推荐为收束总时长的 45% 到 70%。"))
	float DiscardGlyphRevealStartSeconds = 0.06f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Discard", meta = (Units = "s", ToolTip = "普通弃牌牌印从卡牌原位飞入弃牌堆的单枚飞行时间，单位为秒；默认 0.28，推荐 0.20 到 0.38。"))
	float DiscardFlightSeconds = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Discard", meta = (Units = "s", ToolTip = "同批普通弃牌相邻牌印的发射间隔，单位为秒；默认 0.055，推荐 0.035 到 0.08。"))
	float DiscardStaggerSeconds = 0.055f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Discard", meta = (Units = "s", ToolTip = "每枚普通弃牌牌印抵达弃牌堆后，像素方印扩散的存活时间，单位为秒；默认 0.12，推荐 0.08 到 0.18。"))
	float DiscardImpactSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Discard", meta = (ToolTip = "弃牌堆接收方印相对牌印尺寸的峰值倍率；默认 1.55，推荐 1.2 到 2.2，不影响真实牌堆控件布局。"))
	float DiscardImpactScale = 1.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Reshuffle", meta = (Units = "s", ToolTip = "洗牌牌印逐枚抵达抽牌堆时，像素方印扩散的存活时间，单位为秒；默认 0.10，推荐 0.07 到 0.15。只影响接收反馈，不改变迁移计数或飞行时间。"))
	float ReshuffleImpactSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Reshuffle", meta = (ToolTip = "洗牌牌印抵达抽牌堆时，像素方印相对牌印尺寸的峰值倍率；默认 1.35，推荐 1.1 到 1.8，不影响真实牌堆控件布局。"))
	float ReshuffleImpactScale = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Reshuffle", meta = (ToolTip = "最后一枚洗牌牌印抵达时的方印强度倍率；默认 1.18，推荐 1.0 到 1.4。只增强最后一次收束，不增加额外牌印。"))
	float ReshuffleFinalImpactStrengthMultiplier = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "是否启用牌印后方的多色像素抖动拖尾；关闭后只保留牌印主体与飘散粒子，不改变卡牌数量、路径或抵达计数。"))
	bool bEnableTrail = true;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (Units = "s", ToolTip = "相邻拖尾段采样历史路径的时间间隔，单位为秒；默认 0.007，推荐 0.004 到 0.012。数值越大拖尾越长，不改变主体飞行时间。"))
	float TrailSampleIntervalSeconds = 0.007f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "高细节档每枚活动牌印的拖尾段数；默认 7，推荐 5 到 10。运行时会受总拖尾四边形预算限制。"))
	int32 HighDetailTrailSegmentsPerGlyph = 7;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "中细节档每枚活动牌印的拖尾段数；默认 5，推荐 3 到 7。"))
	int32 MediumDetailTrailSegmentsPerGlyph = 5;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "低细节档每枚活动牌印的拖尾段数；默认 3，推荐 1 到 5。"))
	int32 LowDetailTrailSegmentsPerGlyph = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "拖尾靠近牌印一端的宽度，单位为 UMG 逻辑像素；默认 10.5，推荐 6 到 18，不影响布局或命中。"))
	float TrailHeadWidthPixels = 10.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "拖尾末端宽度，单位为 UMG 逻辑像素；默认 3，推荐 1.5 到 6。"))
	float TrailTailWidthPixels = 3.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "拖尾靠近牌印一端的透明度倍率；默认 0.44，推荐 0.2 到 0.7。"))
	float TrailHeadOpacity = 0.44f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Trail", meta = (ToolTip = "拖尾末端的透明度倍率；默认 0.04，推荐 0 到 0.12。"))
	float TrailTailOpacity = 0.04f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "单帧最多提交的拖尾四边形数量；默认 120，推荐 72 到 180。达到上限只减少装饰拖尾，不减少真实牌印、粒子或改变抵达计数。"))
	int32 MaxTrailQuadCount = 120;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Particles", meta = (Units = "s", ToolTip = "单枚飘散像素的存活时间，单位为秒；默认 0.24，推荐 0.16 到 0.32。只影响辅助粒子与尾部排空时间，不改变牌印数量或抵达计数。"))
	float MoteLifetimeSeconds = 0.24f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Particles", meta = (ToolTip = "飘散像素的最小边长，单位为 UMG 逻辑像素；默认 6，推荐 4.5 到 9，不影响布局或命中。"))
	float MoteMinSizePixels = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Particles", meta = (ToolTip = "飘散像素的最大边长，单位为 UMG 逻辑像素；默认 13.5，推荐 9 到 18，不影响布局或命中。"))
	float MoteMaxSizePixels = 13.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Particles", meta = (ToolTip = "多数粒子沿牌印飞行反方向拉开的最大距离，单位为 UMG 逻辑像素；默认 28，推荐 18 到 42。粒子使用先快后慢的三次缓出运动，不改变牌印主体路径。"))
	float MoteBackwardDistancePixels = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Particles", meta = (ToolTip = "少量粒子向飞行路径两侧飘散的最大距离，单位为 UMG 逻辑像素；默认 14，推荐 8 到 24。粒子使用先快后慢的三次缓出运动。"))
	float MoteLateralDistancePixels = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "稳定估算的同时活动牌印不超过该数量时使用最高粒子密度；默认 6，推荐 4 到 8。"))
	int32 HighDetailMaxActiveGlyphs = 6;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "稳定估算的同时活动牌印不超过该数量时使用中等粒子密度；超过后使用低粒子密度。默认 14，推荐 10 到 20。"))
	int32 MediumDetailMaxActiveGlyphs = 14;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "高细节档每枚牌印在整段飞行内使用的确定性粒子槽数量；默认 14，推荐 8 到 20。槽位会错峰出现，并非全部同时存在。"))
	int32 HighDetailMoteSlotsPerGlyph = 14;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "中细节档每枚牌印在整段飞行内使用的确定性粒子槽数量；默认 8，推荐 5 到 12。"))
	int32 MediumDetailMoteSlotsPerGlyph = 8;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "低细节档每枚牌印在整段飞行内使用的确定性粒子槽数量；默认 4，推荐 2 到 8。"))
	int32 LowDetailMoteSlotsPerGlyph = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Performance", meta = (ToolTip = "单帧最多提交的飘散像素四边形数量；默认 240，推荐 160 到 320。达到上限只减少装饰粒子，不减少真实牌印或改变抵达计数。"))
	int32 MaxMoteQuadCount = 240;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Viewport", meta = (ToolTip = "牌印路径相对逻辑 Viewport 边缘保留的安全距离，单位为 UMG 逻辑像素；默认 36，推荐 20 到 64。起止锚点和弧线控制点都会被约束在安全区域内，避免在超宽屏或底部牌堆附近飞出画面。"))
	float SafeViewportPaddingPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "弱化动态时源牌堆静态牌印与目标闪光的总时长，单位为秒；默认 0.18，推荐 0.12 到 0.24。"))
	float ReducedMotionDurationSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Audio", meta = (ToolTip = "洗回迁移开始时播放的一次性 UI 2D 音效；留空表示静音，不会在触发时同步加载。"))
	TObjectPtr<USoundBase> StartSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Audio", meta = (ToolTip = "第一枚牌印发射时播放的一次性 UI 2D 音效；留空表示静音。"))
	TObjectPtr<USoundBase> TravelSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Audio", meta = (ToolTip = "全部牌印抵达抽牌堆时播放的一次性 UI 2D 音效；留空表示静音。"))
	TObjectPtr<USoundBase> CompleteSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Audio", meta = (ToolTip = "洗回牌印三段音效的统一音量倍率；1 为资产原始音量，推荐 0.5 到 1.2。"))
	float SoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer|Audio", meta = (ToolTip = "洗回牌印三段音效的统一基础音高倍率；1 为资产原始音高，推荐 0.85 到 1.15。"))
	float SoundPitchMultiplier = 1.0f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardPileTransferConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pile Transfer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pile Transfer")
	bool bDiscardToPileEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pile Transfer")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Pile Transfer")
	FWacomFirstPersonCardPileTransferStyleData Style;
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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite", meta = (Bitmask, BitmaskEnum = "/Script/WacomApp.EWacomFirstPersonCardDataRewriteField"))
	int32 DataRewriteFieldMask = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	EWacomFirstPersonCardDataRewriteTone DataRewriteTone =
		EWacomFirstPersonCardDataRewriteTone::Neutral;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	int32 DataRewriteSeed = 0;

	/**
	 * Explicit authoritative values for the cost rewrite. Target preview may already render the
	 * post-change value before this semantic hint arrives, so the digit animation must not infer
	 * its old value from the current brush.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	bool bHasDataRewriteCostValues = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	int32 DataRewriteCostBefore = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	int32 DataRewriteCostAfter = 0;

	/** Command presentation phases may opt into waiting for this otherwise decorative playback. */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Data Rewrite")
	bool bBlocksPresentationPhase = false;

	/** Formal EndTurn retains stay sealed until a later RetainedRelease hint. */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Retain Seal")
	bool bRetainUntilExplicitRelease = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Effect Badge")
	TArray<FWacomFirstPersonCardEffectBadgeChange> EffectBadgeChanges;
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
	EWacomFirstPersonCardDragOutDirection NoTargetCardDragOutDirection =
		EWacomFirstPersonCardDragOutDirection::Up;

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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer", meta = (ToolTip = "UI 层自己的交互意图。Battle / Run 适配层负责从规则语义映射到这里；SlotWidget 只消费本字段，不直接推断规则。"))
	EWacomFirstPersonCardInteractionIntent InteractionIntent =
		EWacomFirstPersonCardInteractionIntent::InspectOnly;
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

enum class EWacomFirstPersonCardLayerSourceClearMode : uint8
{
	None,
	RuntimeData,
	VisualState
};

enum class EWacomFirstPersonCardPresentationAnchorKind : uint8
{
	DrawPile,
	DiscardPile,
	PlayTarget
};

/** Logical UMG viewport-space point. Widget ownership stays outside the card layer. */
struct WACOMAPP_API FWacomFirstPersonCardPresentationAnchorPoint
{
	bool bValid = false;
	FVector2D WidgetPosition = FVector2D::ZeroVector;
};

struct WACOMAPP_API FWacomFirstPersonCardPresentationAnchorSet
{
	FWacomFirstPersonCardPresentationAnchorPoint DrawPile;
	FWacomFirstPersonCardPresentationAnchorPoint DiscardPile;
	FWacomFirstPersonCardPresentationAnchorPoint PlayTarget;

	const FWacomFirstPersonCardPresentationAnchorPoint& Get(
		EWacomFirstPersonCardPresentationAnchorKind Kind) const
	{
		switch (Kind)
		{
		case EWacomFirstPersonCardPresentationAnchorKind::DrawPile:
			return DrawPile;
		case EWacomFirstPersonCardPresentationAnchorKind::DiscardPile:
			return DiscardPile;
		case EWacomFirstPersonCardPresentationAnchorKind::PlayTarget:
		default:
			return PlayTarget;
		}
	}
};

/** One batch for the generic card-glyph renderer. Reflected Style assets retain their PileTransfer names. */
struct WACOMAPP_API FWacomFirstPersonCardGlyphTransferHint
{
	enum class ETransferKind : uint8
	{
		DiscardPileToDraw,
		DiscardToPile
	};

	int32 EventSequence = INDEX_NONE;
	TArray<FGuid> CardInstanceIds;
	ETransferKind TransferKind = ETransferKind::DiscardPileToDraw;
	EWacomFirstPersonCardPresentationAnchorKind SourceAnchorKind =
		EWacomFirstPersonCardPresentationAnchorKind::DiscardPile;
	EWacomFirstPersonCardPresentationAnchorKind TargetAnchorKind =
		EWacomFirstPersonCardPresentationAnchorKind::DrawPile;
	uint32 Seed = 0;
};

struct WACOMAPP_API FWacomFirstPersonCardGlyphTransferProgressView
{
	int32 EventSequence = INDEX_NONE;
	FWacomFirstPersonCardGlyphTransferHint::ETransferKind TransferKind =
		FWacomFirstPersonCardGlyphTransferHint::ETransferKind::DiscardPileToDraw;
	int32 LaunchedCount = 0;
	int32 ArrivedCount = 0;
	int32 TotalCount = 0;
	FVector2D LaunchDirection = FVector2D::ZeroVector;
	float ExpectedDurationSeconds = 0.0f;
	bool bCompleted = false;
	bool bReducedMotion = false;
	bool bWasForceCompleted = false;
};

// Compatibility aliases keep the existing reflected asset/lifecycle surface stable while the
// runtime renderer now serves both hand-to-discard and discard-to-draw transfers.
using FWacomFirstPersonCardPileTransferHint = FWacomFirstPersonCardGlyphTransferHint;
using FWacomFirstPersonCardPileTransferProgressView = FWacomFirstPersonCardGlyphTransferProgressView;

struct WACOMAPP_API FWacomFirstPersonCardLayerSourceLifecycleFrame
{
	FName SourceId = NAME_None;
	FWacomFirstPersonCardLayerPresentationFrame PresentationFrame;
	EWacomFirstPersonCardLayerSourceClearMode ClearMode =
		EWacomFirstPersonCardLayerSourceClearMode::None;

	bool bCommitPresentationFrame = false;
	bool bSetTransitionPresentationEnabled = false;
	bool bTransitionPresentationEnabled = true;
	bool bSetInteractionEnabled = false;
	bool bInteractionEnabled = false;
	bool bCancelActiveDrag = false;
	bool bBroadcastDragCancel = true;
	bool bSetPresentationAnchors = false;
	FWacomFirstPersonCardPresentationAnchorSet PresentationAnchors;
	bool bSetPileTransferHints = false;
	TArray<FWacomFirstPersonCardPileTransferHint> PileTransferHints;

	FName ResolveSourceId() const
	{
		return !SourceId.IsNone() ? SourceId : PresentationFrame.SourceId;
	}

	static FWacomFirstPersonCardLayerSourceLifecycleFrame FromPresentationFrame(
		const FWacomFirstPersonCardLayerPresentationFrame& Frame)
	{
		FWacomFirstPersonCardLayerSourceLifecycleFrame LifecycleFrame;
		LifecycleFrame.SourceId = Frame.SourceId;
		LifecycleFrame.PresentationFrame = Frame;
		LifecycleFrame.bCommitPresentationFrame = true;
		return LifecycleFrame;
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

	/** Runtime-only card presentation multiplier resolved from viewport pixels and global UI DPI. */
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PresentationScale = 1.0f;

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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TSoftObjectPtr<USoundBase> StartSound;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float StartSoundVolumeMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float StartSoundPitchMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	EWacomFirstPersonCardSlotTransitionKind TransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
};

/** One-shot native presentation fact emitted when a semantic card enter actually starts. */
struct WACOMAPP_API FWacomFirstPersonCardEnterTransitionStartedView
{
	FGuid CardInstanceId;
	EWacomFirstPersonCardSlotTransitionKind TransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
	FVector2D StartWidgetPosition = FVector2D::ZeroVector;
	FVector2D TargetWidgetPosition = FVector2D::ZeroVector;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDepthConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	bool bEnableFake3D = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float HoverMaxTiltDegrees = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float DragMaxTiltDegrees = 9.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float PressedTiltMultiplier = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float PerspectiveStrength = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	bool bEnableContactShadow = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float ResponseSpeed = 18.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float ReturnSpeed = 14.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float DragVelocityFilterSpeed = 16.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float DragVelocityForMaxTiltPixelsPerSecond = 1400.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float HoverContactShadowLift = 0.55f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float DragContactShadowLift = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float ContactShadowTiltOffsetPixels = 10.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float ContactShadowOpacityMultiplier = 1.5f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	bool bEnableSurfaceParallax = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float SurfaceParallaxStrength = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float DragSurfaceParallaxStrengthMultiplier = 0.75f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float SurfaceParallaxResponseSpeed = 20.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float SurfaceParallaxReturnSpeed = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float AttachmentParallaxDepthPixels = 5.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float AttachmentParallaxMaxOffsetPixels = 7.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	bool bEnableAttachmentCastShadow = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	FLinearColor AttachmentCastShadowColor = FLinearColor(0.006f, 0.009f, 0.018f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float AttachmentCastShadowOpacity = 0.17f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	FVector2D AttachmentCastShadowStaticOffsetPixels = FVector2D(2.0f, 2.5f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float AttachmentCastShadowCounterMotionRatio = 0.80f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	float AttachmentCastShadowMaxOffsetPixels = 6.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer|Depth")
	bool bReduceSurfaceParallaxMotion = false;
};

struct WACOMAPP_API FWacomFirstPersonCardDepthView
{
	bool bFake3DEnabled = false;
	FVector2D TiltDegrees = FVector2D::ZeroVector;
	float PerspectiveStrength = 0.0f;
	bool bContactShadowEnabled = false;
	float ContactShadowLift = 0.0f;
	FVector2D ContactShadowOffsetPixels = FVector2D::ZeroVector;
	float ContactShadowOpacityMultiplier = 1.0f;
	FWacomCardSurfacePerspectiveView SurfacePerspective;
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
	float DragCardTargetFocusLiftPixels = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragCardTargetFocusScale = 1.0f;
	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	int32 DragCardTargetFocusZOrderBoost = 0;

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
	FWacomFirstPersonCardDepthConfig CardDepth;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardSelectionConfig Selection;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardUseEffectConfig CardUseEffect;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardPlayedDissolveConfig PlayedDissolve;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardHandTargetImpactConfig HandTargetImpact;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardDataRewriteConfig DataRewrite;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardDrawRevealConfig DrawReveal;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardGainRevealConfig GainReveal;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardRetainSealConfig RetainSeal;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardEffectBadgeFeedbackConfig EffectBadgeFeedback;
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
	EWacomFirstPersonCardTransitionOriginMode GainedEnterOriginMode =
		EWacomFirstPersonCardTransitionOriginMode::HandAnchorOffset;

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
	bool bEnableEnterSounds = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TSoftObjectPtr<USoundBase> DrawnEnterSound;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TSoftObjectPtr<USoundBase> GainedEnterSound;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TSoftObjectPtr<USoundBase> RunHandEnterSound;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TSoftObjectPtr<USoundBase> HandAnchorEnterSound;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EnterSoundVolumeMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float EnterSoundPitchMultiplier = 1.0f;

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

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DiscardedExitStaggerSeconds = 0.06f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardInteractionFeedbackConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedScale = 0.985f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedTranslationYPixels = 2.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedInDurationSeconds = 0.045f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedOutDurationSeconds = 0.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PressedContactShadowLiftMultiplier = 0.35f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bReduceInteractionMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnableInvalidTargetPreview = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InvalidTargetPreviewEnterDuration = 0.08f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InvalidTargetPreviewExitDuration = 0.06f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor InvalidTargetPreviewColor = FLinearColor(0.62f, 0.12f, 0.32f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor InvalidTargetPreviewAccentColor = FLinearColor(0.18f, 0.32f, 0.48f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InvalidTargetPreviewOpacity = 0.20f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float InvalidTargetPreviewTightenPixels = 4.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyDuration = 0.20f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyShakePixels = 8.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyCompressScale = 0.97f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor DenyColor = FLinearColor(0.88f, 0.18f, 0.36f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FLinearColor DenyAccentColor = FLinearColor(0.16f, 0.28f, 0.46f, 1.0f);

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyOpacity = 0.42f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyCornerInsetPixels = 8.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyCornerLengthPixels = 14.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyCornerThicknessPixels = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyCrackLengthPixels = 30.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenyCrackThicknessPixels = 3.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TObjectPtr<USoundBase> DenySound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenySoundVolumeMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenySoundPitchMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DenySoundPitchVariation = 0.03f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnablePlayCommitFeedback = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitDuration = 0.12f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float PlayCommitScale = 1.015f;
};

USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardDragPickupConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bEnabled = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DurationSeconds = 0.14f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float RiseSeconds = 0.02f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float LiftPixels = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float ScaleMultiplier = 1.03f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bReducedMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TObjectPtr<USoundBase> Sound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float SoundVolumeMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float SoundPitchMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float SoundPitchVariation = 0.03f;
};

/** Atomic runtime contract propagated from the authored Anchor to every card slot. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardSlotRuntimeConfig
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardSlotMotionConfig Motion;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardSlotVisualConfig Visual;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardInteractionFeedbackConfig Interaction;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardDragPickupConfig DragPickup;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	FWacomFirstPersonCardDragConfig Drag;

};
