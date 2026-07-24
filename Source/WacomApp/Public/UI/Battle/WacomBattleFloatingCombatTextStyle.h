// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "WacomBattleFloatingCombatTextStyle.generated.h"

class UNiagaraSystem;

/**
 * HUD 级战斗飘字的纯表现配置。
 *
 * 精确数值和目标身份来自 WacomBattle 事件；本资产只控制颜色、节奏、布局和可选 Niagara 装饰。
 */
UCLASS(BlueprintType, Const, meta = (ToolTip = "玩家与敌人共用的 HUD 战斗飘字样式。只控制表现，不定义伤害、护盾、DOT 或暴击规则。"))
class WACOMAPP_API UWacomBattleFloatingCombatTextStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Color",
		meta = (ToolTip = "实际 HP 损失的文字颜色。推荐使用清晰的珊瑚红。"))
	FLinearColor HpDamageColor = FLinearColor(1.0f, 0.30f, 0.24f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Color",
		meta = (ToolTip = "护盾吸收和护盾获得的文字颜色。推荐使用青蓝色。"))
	FLinearColor ShieldColor = FLinearColor(0.20f, 0.88f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Color",
		meta = (ToolTip = "周期伤害（例如中毒）的文字颜色。状态图标仍由 Battle Status Catalog 提供。"))
	FLinearColor PeriodicDamageColor = FLinearColor(0.72f, 0.34f, 0.88f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Color",
		meta = (ToolTip = "未来 Battle 明确提供暴击事实时使用的文字颜色。当前正式规则不会触发。"))
	FLinearColor CriticalDamageColor = FLinearColor(1.0f, 0.82f, 0.22f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Icon",
		meta = (ToolTip = "护盾吸收或获得时使用的图标。推荐 24×24 至 32×32 的硬像素 Brush。"))
	FSlateBrush ShieldIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Icon",
		meta = (ToolTip = "未来暴击飘字可选图标。为空时仍显示“暴击 -N”文字。"))
	FSlateBrush CriticalIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Timing",
		meta = (ToolTip = "单条飘字淡入时间，单位秒。推荐 0.05–0.12。"))
	float FadeInSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Timing",
		meta = (ToolTip = "飘字保持完整可读的时间，单位秒。推荐 0.30–0.50。"))
	float ReadableHoldSeconds = 0.35f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Timing",
		meta = (ToolTip = "飘字向上漂移并淡出的时间，单位秒。推荐 0.20–0.40。"))
	float FadeOutSeconds = 0.26f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Timing",
		meta = (ToolTip = "同一目标连续数值之间的最小错峰，单位秒。默认 0.08；不同目标不互相阻塞。"))
	float SameTargetStaggerSeconds = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Motion",
		meta = (ToolTip = "普通模式淡出阶段向上漂移距离，单位 UI 像素。简化动态模式不产生位移。"))
	float DriftDistancePixels = 44.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Motion",
		meta = (ToolTip = "暴击飘字的初始放大倍率。只用于未来合成或正式暴击事实；简化动态模式固定为 1。"))
	float CriticalStartScale = 1.18f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "单条飘字的布局尺寸，单位 UI 像素。它决定文本和图标的稳定命中外框，但飘字本身始终不可命中。"))
	FVector2D EntrySize = FVector2D(176.0f, 42.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "同一目标并发 lane 的水平间距，单位 UI 像素。推荐 18–34。"))
	float LaneSpacingPixels = 26.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "同一目标并发飘字的垂直错位，单位 UI 像素。推荐 8–18。"))
	float StackSpacingPixels = 12.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "玩家状态条几何中心之外追加的屏幕偏移，单位 UI 像素。"))
	FVector2D PlayerAnchorOffset = FVector2D(120.0f, 52.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "敌人部位 ImpactAnchor 投影后追加的屏幕偏移，单位 UI 像素。"))
	FVector2D EnemyAnchorOffset = FVector2D(0.0f, -34.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "飘字与 Viewport 四边的安全留白，单位 UI 像素。"))
	float ViewportSafePaddingPixels = 24.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Layout",
		meta = (ToolTip = "同一目标允许同时播放的飘字数量。达到容量后继续排队，不丢失数值。推荐 3–5。"))
	int32 MaxConcurrentPerTarget = 4;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Niagara",
		meta = (ToolTip = "敌人护盾吸收、获得或击破时的可选像素 Niagara。资源缺失不会影响 UMG 精确数值。"))
	TObjectPtr<UNiagaraSystem> ShieldNiagara = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Niagara",
		meta = (ToolTip = "敌人周期伤害时的可选像素 Niagara。资源缺失不会影响 UMG 精确数值。"))
	TObjectPtr<UNiagaraSystem> PeriodicNiagara = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Floating Text|Niagara",
		meta = (ToolTip = "未来敌人暴击事实使用的可选像素 Niagara。当前正式规则不会触发。"))
	TObjectPtr<UNiagaraSystem> CriticalNiagara = nullptr;

	bool ValidateStyle(TArray<FText>& OutErrors) const;
};
