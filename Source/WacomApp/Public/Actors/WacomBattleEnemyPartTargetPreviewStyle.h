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

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Timing",
		meta = (ToolTip = "目标框从部位外侧收束到最终位置的时间，单位：秒。推荐 0.14–0.22；只影响表现，不延迟规则。"))
	float EnterSeconds = 0.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Timing",
		meta = (ToolTip = "鼠标离开部位后目标框淡出的时间，单位：秒。推荐 0.08–0.14；只影响表现。"))
	float ExitSeconds = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Timing",
		meta = (ToolTip = "有效目标停留时的弱亮度呼吸周期，单位：秒。推荐 0.8–1.4；Reduced Motion 下禁用，不改变目标框尺寸。"))
	float PulsePeriodSeconds = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "有效目标框相对 HitBounds 屏幕平面投影宽高的覆盖倍率。无单位；推荐 1.05–1.20，1.10 表示略越过命中边界。"))
	float ValidCoverageMultiplier = 1.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "无效断裂目标框相对 HitBounds 屏幕平面投影宽高的覆盖倍率。无单位；推荐 1.02–1.15。"))
	float InvalidCoverageMultiplier = 1.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "无法取得有效 HitBounds 时使用的目标框宽高，单位：厘米。推荐 80–120cm；只影响表现，不影响命中。"))
	FVector2D FallbackSizeCentimeters = FVector2D(96.0f, 96.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "目标框单轴最小尺寸，单位：厘米。推荐 48–80cm；避免小部位提示不可读。"))
	float MinimumAxisSizeCentimeters = 56.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Sizing",
		meta = (ToolTip = "目标框单轴最大尺寸，单位：厘米。推荐 220–360cm；避免异常 HitBounds 覆盖整屏。"))
	float MaximumAxisSizeCentimeters = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Target Preview|Placement",
		meta = (ToolTip = "目标框从 ImpactAnchor 朝当前摄像机偏移的距离，单位：厘米。推荐 1–4cm，用于避免与 PaperSprite 共面闪烁；不影响命中。"))
	float CameraDepthOffsetCentimeters = 2.0f;

	bool HasValidVisualAssets() const;
};
