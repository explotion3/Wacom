// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RunStateTypes.h"
#include "Styling/SlateBrush.h"
#include "WacomBackpackWorkspaceStyle.generated.h"

class UMaterialInterface;
class UWacomFirstPersonCardPlayedDissolveStyle;

/** 可换皮的区域视觉外观；所有字段只影响 UMG 表现。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBackpackZoneAppearance
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Visual",
		meta = (ToolTip = "区域标题、强调色带和图标的主色；应与其它区域保持可辨识差异。"))
	FLinearColor AccentColor = FLinearColor(0.36f, 0.70f, 0.86f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Visual",
		meta = (ToolTip = "区域牌框与标题底板的中性表面色；建议保持低对比，避免压过卡面。"))
	FLinearColor SurfaceColor = FLinearColor(0.035f, 0.055f, 0.078f, 0.96f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Visual",
		meta = (ToolTip = "区域单色图标 Brush；推荐使用透明蒙版纹理，运行时会乘以 AccentColor。"))
	FSlateBrush IconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Visual",
		meta = (ToolTip = "区域九宫格轮廓 Brush；为空时使用纯色 Border fallback。"))
	FSlateBrush FrameBrush;
};

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
	/** 当前制作资产版本。运行时与 Builder 禁止静默迁移已有资产。 */
	static constexpr int32 CurrentAssetVersion = 4;

	UWacomBackpackWorkspaceStyle();

	const FWacomBackpackZoneAppearance& ResolveZoneAppearance(EZoneKind Zone) const;
	float GetSafeCardDisplayScale() const
	{
		return FMath::Max(CardDisplayScale, 0.01f);
	}
	FVector2D GetCardDisplaySize() const
	{
		return CardRenderSize * GetSafeCardDisplayScale();
	}

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Version",
		meta = (ToolTip = "背包工作台 Style 资产版本。版本 4 引入焦点、选择、合法与拒绝四种非颜色像素图标；仅允许通过明确的白名单资产迁移修改已有资产。"))
	int32 AssetVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "工作台卡牌的原生渲染尺寸，单位为像素。默认 296×420，与 Battle 正式卡面制作尺寸一致；最终布局和命中尺寸还会乘以 CardDisplayScale。"))
	FVector2D CardRenderSize = FVector2D(296.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "背包卡牌统一显示缩放。默认 0.78，推荐 0.68–0.9；卡面、布局、命中、框选、Hand Lens 与 Carry 会共同使用该值，不影响 Battle 卡牌尺寸。必须大于 0。", ClampMin = "0.01"))
	float CardDisplayScale = 0.78f;

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
		meta = (ToolTip = "展开牌堆中悬停卡牌的上抬距离，单位为像素。版本 2 基线 36，推荐 24–56；不缩放卡面。"))
	float ExpandedCardHoverLiftPixels = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Hand Lens Strip",
		meta = (ToolTip = "展开牌堆 Hand Lens 完整卡之间的边缘间隔，单位为像素。版本 2 基线 24，推荐 0–32；不改变 CardDisplayScale 统一显示尺寸。"))
	float HandLensFullGapPixels = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Hand Lens Strip",
		meta = (ToolTip = "展开牌堆左右压缩段的期望可见露出，单位为像素。版本 2 基线 59，推荐 32–80；空间不足时自动减小。"))
	float HandLensCompressedExposurePixels = 59.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Hand Lens Strip",
		meta = (ToolTip = "展开牌堆压缩卡必须保留的最小可见露出，单位为像素。版本 2 基线 16，推荐 10–24；只作为严格布局下限。"))
	float HandLensMinimumExposurePixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Hand Lens Strip",
		meta = (ToolTip = "严格布局仍有剩余空间时，多提升一张完整卡所允许的边界堆顶重叠，单位为像素。版本 2 基线 178，推荐 120–220；只影响展开牌堆三段分配。"))
	float HandLensPromotionOverlapTolerancePixels = 178.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Focus Window Strip",
		meta = (ToolTip = "多卡携带时完整展示的最大卡牌数。默认 1，只完整展示当前滚轮卡；调高后会让相邻卡一并完整展开。", ClampMin = "1"))
	int32 FocusWindowMaximumCards = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Focus Window Strip",
		meta = (ToolTip = "中央窗口内完整卡面之间的边缘间隔，单位为像素。默认 24，推荐 8–32；不改变 CardDisplayScale 统一显示尺寸。"))
	float FocusWindowFullGapPixels = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Focus Window Strip",
		meta = (ToolTip = "中央窗口外压缩卡牌的期望可见露出，单位为像素。默认 56，推荐 32–72；空间不足时会自动减小。"))
	float FocusWindowCompressedExposurePixels = 56.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Focus Window Strip",
		meta = (ToolTip = "压缩卡牌仍需保留的最小可见露出，单位为像素。默认 16，推荐 10–24；求解器会先减少完整窗口卡数来避免低于此值。"))
	float FocusWindowMinimumExposurePixels = 16.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Focus Window Strip",
		meta = (ToolTip = "稳定命中条带切换焦点时的迟滞宽度，单位为像素。默认 8；用于避免边界来回振荡。"))
	float FocusHitHysteresisPixels = 8.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Hand Lens Strip",
		meta = (ToolTip = "展开牌堆 Hand Lens 横向重排到新三段布局的时间，单位为秒。版本 2 基线 0.32；使用 Ease-Out，只动画外层位置。"))
	float FocusReflowSeconds = 0.32f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Focus Window Strip",
		meta = (ToolTip = "鼠标离开实际卡身后取消活动焦点前的等待时间，单位为秒。默认 0.12；重新进入会取消计时，最后窗口位置保持冻结。"))
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
		meta = (ToolTip = "携带卡牌悬停在合法折叠牌堆后自动展开的延迟，单位为秒。推荐 0.25–0.5；离开或目标失效时取消。"))
	float PileHoverExpandDelaySeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Motion",
		meta = (ToolTip = "牌堆展开和收起的位移/角度动画时间，单位为秒。默认 0.18；简化动效模式会直接完成。"))
	float PileExpandSeconds = 0.18f;

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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Accessibility",
		meta = (ToolTip = "虚拟焦点像素图标。推荐透明白色蒙版；卡牌、牌堆和销毁目标共同使用。"))
	FSlateBrush FocusStateIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Accessibility",
		meta = (ToolTip = "已选择状态的像素图标。必须依靠轮廓形状即可与其它状态区分。"))
	FSlateBrush SelectedStateIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Accessibility",
		meta = (ToolTip = "合法投放状态的像素图标。必须依靠轮廓形状即可与拒绝状态区分。"))
	FSlateBrush ValidDropStateIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Accessibility",
		meta = (ToolTip = "拒绝投放状态的像素图标。必须依靠轮廓形状即可与合法状态区分。"))
	FSlateBrush RejectedDropStateIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Zone Visual",
		meta = (ToolTip = "备战区视觉：冷蓝色、卡组图标和双线轮廓。仅影响表现。"))
	FWacomBackpackZoneAppearance BattleDeckAppearance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Zone Visual",
		meta = (ToolTip = "特殊区视觉：紫色、菱形图标和切角轮廓。仅影响表现。"))
	FWacomBackpackZoneAppearance SpecialZoneAppearance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Zone Visual",
		meta = (ToolTip = "负重区视觉：琥珀色、负重图标和警示轮廓。仅影响表现。"))
	FWacomBackpackZoneAppearance BurdenZoneAppearance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Zone Visual",
		meta = (ToolTip = "销毁区视觉：红色、破损卡牌图标和危险轮廓。仅影响表现。"))
	FWacomBackpackZoneAppearance DestructiveAppearance;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Zone Visual",
		meta = (ToolTip = "未展开牌框的轮廓不透明度。推荐 0.55–0.8；不改变卡牌透明度。", ClampMin = "0.0", ClampMax = "1.0"))
	float InactivePileFrameOpacity = 0.68f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Zone Visual",
		meta = (ToolTip = "投放目标覆盖层的不透明度。推荐 0.12–0.28；覆盖层始终不参与命中。", ClampMin = "0.0", ClampMax = "1.0"))
	float DropFeedbackFillOpacity = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Detail",
		meta = (ToolTip = "详情层切换为右侧固定检查栏所需的最小逻辑宽度，单位为像素。默认 1600；低于该值使用避让浮层。"))
	float DetailDockBreakpointPixels = 1600.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Detail",
		meta = (ToolTip = "宽屏固定详情检查栏宽度，单位为像素。默认 360；会减少工作台可用宽度。"))
	float DetailDockWidthPixels = 360.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Detail",
		meta = (ToolTip = "窄屏避让浮层尺寸，单位为像素。默认 340×420；面板会夹紧在屏幕安全区域。"))
	FVector2D DetailFloatingSize = FVector2D(340.0f, 420.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Detail",
		meta = (ToolTip = "详情浮层与卡牌及屏幕边缘的安全间距，单位为像素。推荐 8–24。"))
	float DetailPanelPaddingPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Material",
		meta = (ToolTip = "可选的背包卡牌反馈材质。用于 fake-3D、选中或目标反馈；留空时工作台必须保持完整功能，材质不得改变命中几何。"))
	TObjectPtr<UMaterialInterface> CardFeedbackMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Material",
		meta = (ToolTip = "出售成功后原卡牌 Widget 使用的离场材质 Style。默认复用战斗 Exhausted 的 Ordered Dither；运行时在界面激活阶段缓存，不在投放热路径同步加载。"))
	TSoftObjectPtr<UWacomFirstPersonCardPlayedDissolveStyle> SaleDissolveStyle;
};
