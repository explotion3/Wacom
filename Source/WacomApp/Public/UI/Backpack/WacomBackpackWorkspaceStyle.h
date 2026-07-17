// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WacomBackpackWorkspaceStyle.generated.h"

class UMaterialInterface;

/** 背包活动卡的 Fake3D、速度倾斜和接触阴影制作参数。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBackpackActiveCardDepthStyle
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "悬停时卡面指针倾斜的最大角度，单位为度。推荐 4–8；不改变卡牌布局角度。"))
	float HoverMaximumTiltDegrees = 6.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "携带时由鼠标速度产生的最大倾斜角度，单位为度。推荐 5–10；简化动效模式会关闭。"))
	float CarryMaximumTiltDegrees = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "达到最大携带倾斜所需的鼠标速度，单位为像素/秒。推荐 1000–1800；越小越灵敏。"))
	float CarryVelocityForMaximumTiltPixelsPerSecond = 1400.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "Fake3D 与接触阴影进入目标状态的指数响应速度，单位为每秒。推荐 14–24。"))
	float ResponseSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "Fake3D 与接触阴影回到静止状态的指数响应速度，单位为每秒。推荐 12–22。"))
	float ReturnSpeed = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "携带速度倾斜的速度滤波响应，单位为每秒。推荐 14–24；越大越直接。"))
	float VelocityFilterSpeed = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "卡面 Fake3D 透视强度。推荐 0.08–0.18；只影响活动卡材质表现。"))
	float PerspectiveStrength = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "活动卡接触阴影不透明度倍率。推荐 1–1.8；不改变卡面透明度。"))
	float ContactShadowOpacityMultiplier = 1.5f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "卡面倾斜时接触阴影的最大偏移，单位为像素。推荐 6–14。"))
	float ContactShadowTiltOffsetPixels = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Depth",
		meta = (ToolTip = "活动卡表面附件视差深度，单位为像素。推荐 3–7；不影响命中和布局。"))
	float SurfaceParallaxDepthPixels = 5.0f;
};

/** 背包自由工作台的纯表现参数；不参与 Run 规则、容量、顺序或 SaveGame。 */
UCLASS(BlueprintType)
class WACOMAPP_API UWacomBackpackWorkspaceStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "工作台卡牌渲染尺寸，单位为像素。默认 296×420，与 Battle 的正式卡牌主体制作尺寸一致；会影响默认布局、框选中心和边界约束，不改变规则。"))
	FVector2D CardRenderSize = FVector2D(296.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "卡牌放下后必须留在工作台内的最小可见比例。合法范围 0–1，推荐 0.3；会影响边界夹紧，不影响卡牌命中尺寸。", ClampMin = "0.0", ClampMax = "1.0"))
	float MinimumVisibleFraction = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "默认整理布局的卡牌水平与垂直间距，单位为像素。推荐 24–64；会影响布局密度，不改变手动摆放记录。"))
	FVector2D DefaultCardSpacing = FVector2D(36.0f, 44.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "默认整理布局距工作台边缘的安全边距，单位为像素。推荐 32–96；会影响可用布局面积。"))
	FVector2D WorkspacePadding = FVector2D(56.0f, 56.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "折叠区域牌堆的逻辑尺寸，单位为像素。默认 260×220；影响牌堆命中、吸附与通量整理避让，不缩放完整卡面。"))
	FVector2D PileCollapsedSize = FVector2D(260.0f, 220.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "折叠牌堆中相邻真实卡牌的默认水平露出，单位像素。推荐 10–24；只改变牌堆宽度，不缩放卡面。"))
	float PileCollapsedExposurePixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "整堆移动释放后的吸附网格尺寸，单位为像素。推荐 8–24；只影响牌堆布局，不改变规则。"))
	float PileSnapGridPixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "牌堆标题拖柄距工作台边缘的最小安全距离，单位为像素。推荐 16–40；保证牌堆始终可找回。"))
	float PileEdgeMarginPixels = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "展开牌堆中悬停卡牌的上抬距离，单位为像素。推荐 32–64；不缩放卡面。"))
	float ExpandedCardHoverLiftPixels = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Adaptive Strip",
		meta = (ToolTip = "展开牌堆与多卡携带的默认相邻露出，单位为像素。默认 48，推荐 32–72；空间不足时可自动压缩，不改变 296×420 卡面尺寸。"))
	float AdaptiveStripExposurePixels = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Adaptive Strip",
		meta = (ToolTip = "焦点卡左右卡组额外让位距离，单位为像素。默认 32，推荐 16–48；只影响外层姿态和牌框预留，不改变命中身份或规则。"))
	float AdaptiveStripFocusSeparationPixels = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Adaptive Strip",
		meta = (ToolTip = "稳定命中条带切换焦点时的迟滞宽度，单位为像素。默认 8；用于避免边界来回振荡。"))
	float FocusHitHysteresisPixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Adaptive Strip",
		meta = (ToolTip = "展开牌堆邻居让位到新焦点布局的时间，单位为秒。默认 0.18；只动画局部位置和角度。"))
	float FocusReflowSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Adaptive Strip",
		meta = (ToolTip = "鼠标离开后焦点牌列返回中性紧凑牌列的时间，单位为秒。默认 0.14。"))
	float FocusReturnSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Adaptive Strip",
		meta = (ToolTip = "鼠标离开展开牌框后开始恢复前的等待时间，单位为秒。默认 0.12；重新进入会取消恢复。"))
	float FocusExitDelaySeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Carry",
		meta = (ToolTip = "滚轮选中非默认当前牌时的上抬距离，单位为像素。推荐 40–80；默认最右牌不使用该抬升。"))
	float CurrentCardLiftPixels = 56.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Carry",
		meta = (ToolTip = "携带当前卡在活动运动层内高于其他交接卡的额外层级。默认 1000；只影响绘制遮挡，不改变卡牌顺序或释放语义。"))
	int32 CurrentCardZOrderBoost = 1000;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "携带视觉锚点追赶鼠标的指数响应速度，单位为每秒。默认 34；规则命中始终使用未延迟的真实鼠标位置。"))
	float CarryFollowResponseSpeed = 34.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "携带卡牌视觉允许落后真实鼠标的最大距离，单位为像素。默认 14；不影响投放目标判定。"))
	float CarryMaximumVisualLagPixels = 14.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "悬停卡从基础布局进入上抬回正姿态的时间，单位为秒。推荐 0.12–0.2。"))
	float HoverEnterSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "悬停卡离开后返回基础姿态的时间，单位为秒。推荐 0.08–0.16。"))
	float HoverExitSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "开始携带时拾牌反馈的持续时间，单位为秒。默认 0.14；不缩放卡面。"))
	float CarryPickupSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "开始携带时整组卡牌的额外上提距离，单位为像素。推荐 6–16；只作用于局部运动根。"))
	float CarryPickupLiftPixels = 10.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "滚轮切换当前卡时，新旧卡交接到目标姿态的时间，单位为秒。默认 0.14。"))
	float CarryCurrentTransitionSeconds = 0.14f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "ESC 取消携带后从当前视觉姿态返回来源布局的时间，单位为秒。推荐 0.12–0.22。"))
	float CancelReturnSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion")
	FWacomBackpackActiveCardDepthStyle ActiveCardDepthMotion;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "卡牌放下后收敛到目标布局的时间，单位为秒。推荐 0.12–0.24；只影响表现。"))
	float SettleSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "同区牌匣收拢到默认布局的时间，单位为秒。推荐 0.14–0.28；只影响表现。"))
	float CollectSeconds = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "无效释放反馈持续时间，单位为秒。推荐 0.12–0.25；不改变失败事务的携带恢复语义。"))
	float RejectedFeedbackSeconds = 0.16f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "携带卡牌悬停在合法折叠牌堆后自动展开的延迟，单位为秒。推荐 0.25–0.5；离开或目标失效时取消。"))
	float PileHoverExpandDelaySeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "牌堆展开和收起的位移/角度动画时间，单位为秒。默认 0.18；简化动效模式会直接完成。"))
	float PileExpandSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "整堆释放后收敛到吸附位置的时间，单位为秒。默认 0.12；直接拖动阶段保持一比一跟随。"))
	float PileSnapSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Color",
		meta = (ToolTip = "框选矩形和已选卡牌的主色。建议保持与背景有明显对比；只影响表现。"))
	FLinearColor SelectionColor = FLinearColor(0.2f, 0.72f, 1.0f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Color",
		meta = (ToolTip = "卡牌选中、合法目标或拒绝状态覆盖层的不透明度。合法范围 0–1，推荐 0.18–0.28；不改变卡面固定缩放、命中范围或布局。", ClampMin = "0.0", ClampMax = "1.0"))
	float CardStateOverlayOpacity = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Color",
		meta = (ToolTip = "合法牌匣目标预览颜色。建议使用明确的正向颜色；只影响表现。"))
	FLinearColor ValidTargetColor = FLinearColor(0.25f, 0.9f, 0.45f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Color",
		meta = (ToolTip = "被拒绝牌匣目标预览颜色。建议与合法目标区分明显；只影响表现。"))
	FLinearColor RejectedTargetColor = FLinearColor(1.0f, 0.22f, 0.18f, 0.9f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Material",
		meta = (ToolTip = "可选的背包卡牌反馈材质。用于 fake-3D、选中或目标反馈；留空时工作台必须保持完整功能，材质不得改变命中几何。"))
	TObjectPtr<UMaterialInterface> CardFeedbackMaterial = nullptr;
};
