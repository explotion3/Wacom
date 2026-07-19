// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartAnimationStyle.generated.h"

class UPaperFlipbook;

/** MultiPartVisualLayers 部位行动使用的一段非循环 Flipbook。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyPartAnimationClip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation",
		meta = (ToolTip = "部位语义行动使用的 PaperFlipbook。为空时该 Clip 不可播放；不会根据资源名自动推断动画。"))
	TObjectPtr<UPaperFlipbook> Flipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation",
		meta = (ToolTip = "该段动画的播放倍率。1 表示原速；推荐 0.5–2.0。必须为有限正数，否则运行时立即跳过并由资产校验报错。"))
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "部位行动动画的语义命中点，按归一化播放进度计算。0 表示起播即命中，1 表示结束时命中；推荐 0.45–0.65。"))
	float ImpactNormalizedTime = 0.55f;

	bool IsRuntimeUsable() const
	{
		return Flipbook != nullptr
			&& FMath::IsFinite(PlayRate)
			&& PlayRate > 0.0f
			&& FMath::IsFinite(ImpactNormalizedTime)
			&& ImpactNormalizedTime >= 0.0f
			&& ImpactNormalizedTime <= 1.0f;
	}
};

/**
 * MultiPartVisualLayers 单个 PartActor 的语义行动动画制作资产。
 *
 * authored Idle 继续读取 PartActor.VisualLayers；本资产只声明一个明确主层上的
 * Default Action 与显式 Intent Action，不拥有 Destroyed 终态。
 */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleEnemyPartAnimationStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation",
		meta = (ToolTip = "行动动画要原地切换的唯一 VisualLayer LayerId。必须精确指向一个有效 Flipbook 层；不会按数组顺序自动选择。"))
	FName TargetVisualLayerId = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation|Action",
		meta = (ToolTip = "未配置对应 IntentId 时使用的默认行动动画。Flipbook 为空表示不提供默认动画，请求会立即完成且不阻塞战斗。"))
	FWacomBattleEnemyPartAnimationClip DefaultActionClip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation|Action",
		meta = (ToolTip = "IntentId 到部位行动动画的显式映射。精确映射优先于 DefaultActionClip；IntentId 必须非空，不会根据名称或效果猜测。"))
	TMap<FName, FWacomBattleEnemyPartAnimationClip> ActionClipsByIntentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Part Animation|Destroyed",
		meta = (ToolTip = "可选的整只敌人终态动画。Host 内最多一个 Part Style 可以配置；使用同一个 TargetVisualLayerId，播放完成后停在最后一帧。"))
	FWacomBattleEnemyPartAnimationClip EnemyDestroyedClip;

	const FWacomBattleEnemyPartAnimationClip* ResolveActionClip(FName IntentId) const;
	const FWacomBattleEnemyPartAnimationClip* ResolveEnemyDestroyedClip() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
