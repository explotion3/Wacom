// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WacomBackpackWorkspaceStyle.generated.h"

class UMaterialInterface;

/** 背包自由工作台的纯表现参数；不参与 Run 规则、容量、顺序或 SaveGame。 */
UCLASS(BlueprintType)
class WACOMAPP_API UWacomBackpackWorkspaceStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Layout",
		meta = (ToolTip = "工作台卡牌渲染尺寸，单位为像素。推荐从 220×320 附近调节；会影响默认布局、框选中心和边界约束，不改变规则。"))
	FVector2D CardRenderSize = FVector2D(220.0f, 320.0f);

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
		meta = (ToolTip = "自适应手风琴相邻卡牌最小露出宽度，单位为像素。默认 32；卡牌较多或可用宽度较小时使用。"))
	float AccordionMinimumExposurePixels = 32.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "自适应手风琴相邻卡牌最大露出宽度，单位为像素。默认 72；不会改变卡牌固定渲染尺寸。"))
	float AccordionMaximumExposurePixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "自适应手风琴从最左到最右的最大总角度，单位为度。推荐 8–18；仅影响展开表现。"))
	float AccordionMaximumAngleDegrees = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Pile Layout",
		meta = (ToolTip = "展开牌堆中悬停卡牌的上抬距离，单位为像素。推荐 32–64；不缩放卡面。"))
	float AccordionHoverLiftPixels = 48.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Carry",
		meta = (ToolTip = "携带扇形从最左到最右的最大总角度，单位为度。推荐 24–48；卡牌较少时实际角度会自动收紧。"))
	float FanMaximumAngleDegrees = 36.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Carry",
		meta = (ToolTip = "携带扇形相邻卡牌中心的水平距离，单位为像素。推荐 48–96；影响重叠密度和整体宽度。"))
	float FanCardSpacingPixels = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Backpack|Carry",
		meta = (ToolTip = "滚轮选中非默认当前牌时的上抬距离，单位为像素。推荐 40–80；默认最右牌不使用该抬升。"))
	float CurrentCardLiftPixels = 56.0f;

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
