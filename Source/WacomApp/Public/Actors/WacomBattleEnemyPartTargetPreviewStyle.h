// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WacomBattleEnemyPartTargetPreviewStyle.generated.h"

class UMaterialInterface;
class UNiagaraSystem;

/**
 * 拖拽卡牌悬停世界敌人部位时的像素目标预演制作资产。
 *
 * 只描述表现资产、节奏和空间覆盖；目标是否合法仍由 Battle/App 的既有
 * drop resolver 决定，本资产不会提交命令或改变命中范围。
 */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleEnemyPartTargetPreviewStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Assets",
		meta = (ToolTip = "目标预演使用的 Niagara System。推荐使用 CPU Simulation、固定 Bounds，并暴露项目约定的 Preview User 参数。"))
	TObjectPtr<UNiagaraSystem> PreviewSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Assets",
		meta = (ToolTip = "目标预演 Sprite Renderer 使用的材质实例。材质负责有效/无效像素框配色、线宽和辉光。"))
	TObjectPtr<UMaterialInterface> PreviewMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ToolTip = "敌人交互描边 padded-quad 代理使用的 ring-only Alpha 材质。为空时只关闭描边，不影响精灵轮廓命中、Niagara 预演或命令提交。"))
	TObjectPtr<UMaterialInterface> OutlineMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ToolTip = "目标选择中合法但未悬停部位的暖金色描边颜色。该值作为 Unlit Emissive 线性颜色使用，推荐各通道不超过 1，避免曝光后洗白。"))
	FLinearColor SelectableOutlineColor = FLinearColor(0.45f, 0.16f, 0.015f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ClampMin = "0.0", ClampMax = "2.0", ToolTip = "合法但未悬停描边向外扩张的源纹理像素数。合法范围 0–2，默认 1 source pixel；双环 shader 最多支持 2，只影响表现。"))
	float SelectableOutlineThicknessSourcePixels = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "合法但未悬停描边 Alpha。合法范围 0–1，默认 0.55。"))
	float SelectableOutlineAlpha = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ToolTip = "Idle 存活悬停或目标选择中合法悬停部位的亮金色描边颜色。该值作为 Unlit Emissive 线性颜色使用，推荐各通道不超过 1，避免曝光后洗白。"))
	FLinearColor HoveredOutlineColor = FLinearColor(0.80f, 0.34f, 0.025f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ClampMin = "0.0", ClampMax = "2.0", ToolTip = "悬停描边向外扩张的源纹理像素数。合法范围 0–2，默认 2 source pixels；双环 shader 最多支持 2，只影响表现。"))
	float HoveredOutlineThicknessSourcePixels = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "悬停描边 Alpha。合法范围 0–1，默认 0.95。"))
	float HoveredOutlineAlpha = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ToolTip = "第二个源像素描边环相对内环的颜色强度倍率。无单位，推荐 0.35–0.60；只在厚度达到 2 source pixels 时生效。"))
	float OutlineOuterColorMultiplier = 0.45f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Outline",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "第二个源像素描边环相对内环的 Alpha 倍率。合法范围 0–1，推荐 0.55–0.80；只在厚度达到 2 source pixels 时生效。"))
	float OutlineOuterAlphaMultiplier = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Timing",
		meta = (ToolTip = "目标框从部位外侧收束到最终位置的时间，单位：秒。推荐 0.14–0.22；只影响表现，不延迟规则。"))
	float EnterSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Timing",
		meta = (ToolTip = "鼠标离开部位后目标框淡出的时间，单位：秒。推荐 0.08–0.14；只影响表现。"))
	float ExitSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Timing",
		meta = (ToolTip = "有效目标停留时的弱亮度呼吸周期，单位：秒。推荐 0.8–1.4；Reduced Motion 下禁用，不改变目标框尺寸。"))
	float PulsePeriodSeconds = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Availability",
		meta = (ToolTip = "可用部位中心图标淡入到完整强度的时间，单位：秒。推荐 0.08–0.16；Simplified Motion 下直接显示最终状态。"))
	float AvailabilityEnterSeconds = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Availability",
		meta = (ToolTip = "可用部位中心图标在目标选择结束后的淡出时间，单位：秒。推荐 0.08–0.14；只影响表现。"))
	float AvailabilityExitSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Availability",
		meta = (ToolTip = "中心图标尺寸相对 interaction visual（配置异常时为 transient fallback）摄像机平面投影较短边的倍率。无单位；推荐 0.18–0.28。"))
	float AvailabilityIconSizeMultiplier = 0.22f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Availability",
		meta = (ToolTip = "中心图标最小尺寸，单位：厘米。用于保证小部位仍可读；推荐 10–16cm。"))
	float MinimumAvailabilityIconSizeCentimeters = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Availability",
		meta = (ToolTip = "中心图标最大尺寸，单位：厘米。用于避免 Boss 大部位图标过大；推荐 24–36cm。"))
	float MaximumAvailabilityIconSizeCentimeters = 28.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Availability",
		meta = (ToolTip = "可用中心图标的基础语义强度。无单位；推荐 0.20–0.40。Flash 设置只降低额外辉光，不隐藏该语义标记。"))
	float AvailabilityBaseIntensity = 0.28f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "有效目标框相对 interaction visual（配置异常时为 transient fallback）屏幕平面投影宽高的覆盖倍率。无单位；推荐 1.05–1.20，1.10 表示略越过命中边界。"))
	float ValidCoverageMultiplier = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "无效断裂目标框相对 interaction visual（配置异常时为 transient fallback）屏幕平面投影宽高的覆盖倍率。无单位；推荐 1.02–1.15。"))
	float InvalidCoverageMultiplier = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "无法取得有效 interaction visual 或 transient fallback bounds 时使用的目标框宽高，单位：厘米。推荐 80–120cm；只影响表现，不影响命中。"))
	FVector2D FallbackSizeCentimeters = FVector2D(96.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "目标框单轴最小尺寸，单位：厘米。推荐 48–80cm；避免小部位提示不可读。"))
	float MinimumAxisSizeCentimeters = 56.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "目标框单轴最大尺寸，单位：厘米。推荐 220–360cm；避免异常视觉 bounds 覆盖整屏。"))
	float MaximumAxisSizeCentimeters = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Placement",
		meta = (ToolTip = "目标框从 ImpactAnchor 朝当前摄像机偏移的距离，单位：厘米。推荐 1–4cm，用于避免与 PaperSprite 共面闪烁；不影响命中。"))
	float CameraDepthOffsetCentimeters = 2.0f;

	bool HasValidVisualAssets() const;
	bool HasValidOutlineAsset() const;
};
