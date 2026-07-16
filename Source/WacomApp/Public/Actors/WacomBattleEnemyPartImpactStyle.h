// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "WacomBattleEnemyPartImpactStyle.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USoundBase;

/**
 * 场景敌人部位像素命中反馈的制作资产。
 *
 * Niagara/材质负责视觉，Cue Duration 仍由 Battle presentation cue 决定；
 * 本资产不保存或修改任何战斗规则数据。
 */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleEnemyPartImpactStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Assets",
		meta = (ToolTip = "TargetConfirmed、Damage 与 Destroyed 共用的 Niagara System。推荐使用 CPU Simulation、固定 Bounds，并暴露项目文档约定的 User 参数。"))
	TObjectPtr<UNiagaraSystem> ImpactSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Assets",
		meta = (ToolTip = "Niagara Sprite Renderer 使用的材质实例。必须来自支持 Niagara Sprite 的世界空间透明材质。"))
	TObjectPtr<UMaterialInterface> ImpactMaterialInstance = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Intensity",
		meta = (ToolTip = "TargetConfirmed 的基础视觉强度。无单位；推荐 0.5–1.0，只影响表现。"))
	float TargetConfirmedIntensity = 0.70f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Intensity",
		meta = (ToolTip = "Damage 强度公式的基础值。无单位；推荐 0.5–1.0，只影响表现。"))
	float DamageIntensityBase = 0.75f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Intensity",
		meta = (ToolTip = "Damage 强度公式中 sqrt(伤害量) 的倍率。无单位；推荐 0.05–0.2，只影响粒子数量和尺寸。"))
	float DamageIntensitySqrtScale = 0.10f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Intensity",
		meta = (ToolTip = "Damage 视觉强度下限。无单位；推荐 0.6–1.0。"))
	float DamageIntensityMin = 0.80f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Intensity",
		meta = (ToolTip = "Damage 视觉强度上限。无单位；推荐 1.2–2.2，避免高伤害产生过量粒子。"))
	float DamageIntensityMax = 1.80f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Intensity",
		meta = (ToolTip = "Destroyed 两段式崩裂的基础视觉强度。无单位；推荐 1.1–1.6，只影响表现。"))
	float DestroyedIntensity = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Sizing",
		meta = (ToolTip = "TargetConfirmed 相对当前命中部位屏幕投影包围尺寸的覆盖倍率。无单位；推荐 1.1–1.4，1.2 表示刻印略大于该部位，不会覆盖整个敌人。"))
	float TargetConfirmedCoverageMultiplier = 1.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Sizing",
		meta = (ToolTip = "Damage 冲击环相对当前命中部位屏幕投影包围尺寸的覆盖倍率。无单位；推荐 1.1–1.5，1.2 表示冲击环略越过该部位边缘。"))
	float DamageCoverageMultiplier = 1.20f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Sizing",
		meta = (ToolTip = "Destroyed 崩裂相对当前部位屏幕投影包围尺寸的覆盖倍率。无单位；推荐 1.2–1.5。"))
	float DestroyedCoverageMultiplier = 1.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Sizing",
		meta = (ToolTip = "无法取得有效 HitBounds 时使用的特效直径，单位：厘米。推荐 80–120cm；只影响表现，不影响命中范围。"))
	float FallbackImpactDiameterCentimeters = 96.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Sizing",
		meta = (ToolTip = "自适应特效直径下限，单位：厘米。推荐 60–90cm；避免很小的部位在第一人称视角中完全看不清。"))
	float MinimumImpactDiameterCentimeters = 72.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Sizing",
		meta = (ToolTip = "自适应特效直径上限，单位：厘米。推荐 220–320cm；避免异常大 HitBounds 让特效覆盖整屏。"))
	float MaximumImpactDiameterCentimeters = 260.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Placement",
		meta = (ToolTip = "特效从 ImpactAnchor 朝当前摄像机偏移的距离，单位：厘米。推荐 1–4cm，用于避免与 PaperSprite 共面闪烁；不影响命中。"))
	float CameraDepthOffsetCentimeters = 2.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Target Confirmed",
		meta = (ToolTip = "目标确认达到中心方印时播放的一次性声音。为空时静默跳过，不影响视觉。"))
	TObjectPtr<USoundBase> TargetConfirmedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Target Confirmed",
		meta = (ToolTip = "目标确认声音音量倍率。1 为原始音量；推荐 0.5–1.2。"))
	float TargetConfirmedSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Target Confirmed",
		meta = (ToolTip = "目标确认声音基础音高倍率。1 为原始音高；推荐 0.9–1.1。"))
	float TargetConfirmedSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Target Confirmed",
		meta = (ToolTip = "目标确认声音稳定随机音高变化比例。0.03 表示约 ±3%；推荐 0–0.08。"))
	float TargetConfirmedSoundPitchVariation = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Damage",
		meta = (ToolTip = "权威伤害命中时播放的一次性声音。为空时静默跳过，不影响视觉。"))
	TObjectPtr<USoundBase> DamageSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Damage",
		meta = (ToolTip = "伤害声音音量倍率。1 为原始音量；推荐 0.5–1.3。"))
	float DamageSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Damage",
		meta = (ToolTip = "伤害声音基础音高倍率。1 为原始音高；推荐 0.9–1.1。"))
	float DamageSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Damage",
		meta = (ToolTip = "伤害声音稳定随机音高变化比例。0.03 表示约 ±3%；推荐 0–0.08。"))
	float DamageSoundPitchVariation = 0.03f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Destroyed",
		meta = (ToolTip = "部位破坏时播放的一次性声音。为空时静默跳过，不影响粒子或破损换图。"))
	TObjectPtr<USoundBase> DestroyedSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Destroyed",
		meta = (ToolTip = "部位破坏声音音量倍率。1 为原始音量；推荐 0.6–1.3。"))
	float DestroyedSoundVolumeMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Destroyed",
		meta = (ToolTip = "部位破坏声音基础音高倍率。1 为原始音高；推荐 0.85–1.05。"))
	float DestroyedSoundPitchMultiplier = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Impact|Audio|Destroyed",
		meta = (ToolTip = "部位破坏声音稳定随机音高变化比例。0.03 表示约 ±3%；推荐 0–0.08。"))
	float DestroyedSoundPitchVariation = 0.03f;

	float ResolveDamageIntensity(int32 DamageAmount) const;
	bool HasValidVisualAssets() const;
};
