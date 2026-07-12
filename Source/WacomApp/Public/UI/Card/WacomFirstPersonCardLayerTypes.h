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

UENUM(BlueprintType)
enum class EWacomFirstPersonCardInteractionFeedbackKind : uint8
{
	None UMETA(DisplayName = "None"),
	Pressed UMETA(DisplayName = "Pressed"),
	Confirm UMETA(DisplayName = "Confirm"),
	Commit UMETA(DisplayName = "Commit"),
	Deny UMETA(DisplayName = "Deny")
};

UENUM(BlueprintType)
enum class EWacomFirstPersonCardLayerFeedbackKind : uint8
{
	None UMETA(DisplayName = "None"),
	Retained UMETA(DisplayName = "Retained"),
	CardUseReform UMETA(DisplayName = "Card Use Reform")
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

/** Card-surface material state. Selection remains dormant; PlayedDissolve is the first production channel. */
struct WACOMAPP_API FWacomFirstPersonCardSurfaceEffectView
{
	FWacomFirstPersonCardSelectionView Selection;
	FWacomFirstPersonCardUseEffectView CardUse;
	FWacomFirstPersonCardPlayedDissolveView PlayedDissolve;
};

/** Authoring contract for a discard-to-draw pile transfer. The material instance owns glyph appearance. */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomFirstPersonCardPileTransferStyleData
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "像素牌印使用的 UI 材质实例；材质负责卡背图案，位置、旋转、缩放和透明度由 Slate 批量顶点驱动。留空时不播放跨牌堆迁移。"))
	TObjectPtr<UMaterialInstance> GlyphMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "单枚牌印尺寸，单位为 UMG 逻辑像素；默认 14×22，推荐宽 10 到 20、高 16 到 30，不影响手牌布局或命中。"))
	FVector2D GlyphSize = FVector2D(14.0f, 22.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "牌印从弃牌堆蓄力后开始发射的等待时间，单位为秒；默认 0.08，推荐 0.04 到 0.12。"))
	float StartChargeSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "每枚牌印从弃牌堆飞到抽牌堆的时间，单位为秒；默认 0.36，推荐 0.26 到 0.48。"))
	float FlightSeconds = 0.36f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "确定性弧线路径数量；默认 3，推荐 2 到 5。运行时至少使用 1 条，不改变卡牌顺序。"))
	int32 LaneCount = 3;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "相邻牌印的基础发射间隔，单位为秒；默认 0.045。数量较多时会自动压缩以满足总时长。"))
	float BaseStaggerSeconds = 0.045f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "全部牌印发射所允许的最长时间窗，单位为秒；默认 0.43，推荐 0.30 到 0.52。"))
	float MaxLaunchWindowSeconds = 0.43f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (Units = "s", ToolTip = "最后一枚抵达后抽牌堆方印收束的时间，单位为秒；默认 0.08，推荐 0.04 到 0.12。"))
	float SettleSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "弧高占起止锚点距离的比例；默认 0.18，推荐 0.12 到 0.26，随后受最小和最大弧高限制。"))
	float ArcHeightRatio = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "最小弧高，单位为 UMG 逻辑像素；默认 48，推荐 28 到 72。"))
	float MinArcHeightPixels = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "最大弧高，单位为 UMG 逻辑像素；默认 128，推荐 96 到 180。"))
	float MaxArcHeightPixels = 128.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|First Person Card Pile Transfer", meta = (ToolTip = "每枚牌印的残影层数；默认 2，推荐 0 到 3。残影与主体合并到同一 Slate 顶点批次。"))
	int32 TrailLayerCount = 2;

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

#if WITH_AUTOMATION_TESTS
	// Removed authoring options retained only as inert compile fixtures while the
	// legacy giant spec is split into focused contract tests.
	float DragTargetFeedbackOpacity = 0.0f;
	FLinearColor DragValidTargetColor = FLinearColor::Transparent;
	FLinearColor DragInvalidTargetColor = FLinearColor::Transparent;
	FLinearColor DragCardProbeTargetColor = FLinearColor::Transparent;
	float DragCommitReadyScale = 1.0f;
	float DragCardTargetProbeScale = 1.0f;
	bool bSnapAimArrowToValidWorldTarget = false;
	float DragAimArrowSnapBlend = 0.0f;
#endif
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

struct WACOMAPP_API FWacomFirstPersonCardPileTransferHint
{
	int32 EventSequence = INDEX_NONE;
	TArray<FGuid> CardInstanceIds;
	EWacomFirstPersonCardPresentationAnchorKind SourceAnchorKind =
		EWacomFirstPersonCardPresentationAnchorKind::DiscardPile;
	EWacomFirstPersonCardPresentationAnchorKind TargetAnchorKind =
		EWacomFirstPersonCardPresentationAnchorKind::DrawPile;
	uint32 Seed = 0;
};

struct WACOMAPP_API FWacomFirstPersonCardPileTransferProgressView
{
	int32 EventSequence = INDEX_NONE;
	int32 ArrivedCount = 0;
	int32 TotalCount = 0;
	bool bCompleted = false;
};

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
	EWacomFirstPersonCardSlotTransitionKind SoundTransitionKind =
		EWacomFirstPersonCardSlotTransitionKind::Default;
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
};

struct WACOMAPP_API FWacomFirstPersonCardDepthView
{
	bool bFake3DEnabled = false;
	FVector2D TiltDegrees = FVector2D::ZeroVector;
	float PerspectiveStrength = 0.0f;
	bool bContactShadowEnabled = false;
	float ContactShadowLift = 0.0f;
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
	bool bEnableDragPickupFeedback = true;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupDurationSeconds = 0.14f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupRiseSeconds = 0.02f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupLiftPixels = 12.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupScaleMultiplier = 1.03f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	bool bReduceDragPickupMotion = false;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	TObjectPtr<USoundBase> DragPickupSound = nullptr;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupSoundVolumeMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupSoundPitchMultiplier = 1.0f;

	UPROPERTY(BlueprintReadOnly, Category = "Wacom|First Person Card Layer")
	float DragPickupSoundPitchVariation = 0.03f;

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
	int32 RetainedFeedbackZOrderBoost = 180;

};
