// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyIntentPresentationStyle.generated.h"

/** 一个稳定 IntentId 对应的 UI-only 图标。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyIntentIconEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent",
		meta = (ToolTip = "要匹配的完整稳定 IntentId。必须非空且不可重复；不会根据显示名或效果猜测。"))
	FName IntentId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent",
		meta = (ToolTip = "该 Intent 在紧凑敌人面板中显示的图标 Brush。推荐使用 16×16 至 32×32 的白色像素图标，由 WBP 负责外层底色。"))
	FSlateBrush IconBrush;
};

/**
 * 敌人 Intent 的项目级 UI 表现映射。
 *
 * 该资产只解释稳定 IntentId；不读取规则效果、不修改 Battle state。
 */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleEnemyIntentPresentationStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent",
		meta = (ToolTip = "没有精确 IntentId 映射时显示的白色四角星 fallback。必须配置有效资源，保证未知内容仍可读。"))
	FSlateBrush FallbackIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent",
		meta = (ToolTip = "稳定 IntentId 到图标的精确映射。新增敌人内容时在此补充条目；空或重复 IntentId 会由 Validator 拒绝。"))
	TArray<FWacomBattleEnemyIntentIconEntry> IntentIcons;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "Intent 详情中 Effect.Damage 使用的通用图标。必须配置有效资源。"))
	FSlateBrush DamageEffectIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "Intent 详情中 Status.Shield 使用的通用图标。必须配置有效资源。"))
	FSlateBrush ShieldEffectIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "未知 Intent 效果的安全回退图标。未知 GameplayTag 仍会完整显示。"))
	FSlateBrush FallbackEffectIconBrush;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "伤害效果行的 UI 色调，只影响表现。"))
	FLinearColor DamageEffectTint = FLinearColor(1.0f, 0.36f, 0.28f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "护盾效果行的 UI 色调，只影响表现。"))
	FLinearColor ShieldEffectTint = FLinearColor(0.32f, 0.72f, 1.0f, 1.0f);

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "状态效果行的 UI 色调；图标资源仍由状态 Catalog 提供。"))
	FLinearColor StatusEffectTint = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent|Effects",
		meta = (ToolTip = "未知效果行的 UI 色调，只影响表现。"))
	FLinearColor UnknownEffectTint = FLinearColor(0.76f, 0.78f, 0.82f, 1.0f);

	const FSlateBrush* ResolveIntentIcon(FName IntentId) const;
	static bool IsIconBrushUsable(const FSlateBrush& Brush);

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
