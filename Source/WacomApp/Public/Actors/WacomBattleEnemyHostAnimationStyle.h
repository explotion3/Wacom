// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyHostAnimationStyle.generated.h"

class UPaperFlipbook;

/** Host 语义动画使用的一段非循环 Flipbook。 */
USTRUCT(BlueprintType)
struct WACOMAPP_API FWacomBattleEnemyHostAnimationClip
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Host Animation",
		meta = (ToolTip = "语义动画使用的 PaperFlipbook。为空时该 Clip 不可播放；不会根据资源名自动推断动画。"))
	TObjectPtr<UPaperFlipbook> Flipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Host Animation",
		meta = (ToolTip = "该段动画的播放倍率。1 表示原速；推荐 0.5–2.0。必须为有限正数，否则运行时立即跳过并由资产校验报错。"))
	float PlayRate = 1.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Host Animation",
		meta = (ClampMin = "0.0", ClampMax = "1.0", ToolTip = "行动动画的语义命中点，按归一化播放进度计算。0 表示起播即命中，1 表示结束时命中；推荐 0.45–0.65。DestroyedClip 忽略此字段。"))
	float ImpactNormalizedTime = 0.55f;

	bool IsPlaybackUsable() const
	{
		return Flipbook != nullptr
			&& FMath::IsFinite(PlayRate)
			&& PlayRate > 0.0f;
	}

	bool IsRuntimeUsable() const
	{
		return IsPlaybackUsable()
			&& FMath::IsFinite(ImpactNormalizedTime)
			&& ImpactNormalizedTime >= 0.0f
			&& ImpactNormalizedTime <= 1.0f;
	}
};

/**
 * SimpleHostVisual 敌人的语义动画制作资产。
 *
 * Idle 继续读取 Host.HostFlipbook；本资产只声明行动与整体死亡的一次性动画。
 */
UCLASS(BlueprintType, Const)
class WACOMAPP_API UWacomBattleEnemyHostAnimationStyle : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Host Animation|Action",
		meta = (ToolTip = "未配置对应 IntentId 时使用的默认行动动画。Flipbook 为空表示不提供默认行动动画，请求会立即完成且不阻塞战斗。"))
	FWacomBattleEnemyHostAnimationClip DefaultActionClip;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Host Animation|Action",
		meta = (ToolTip = "IntentId 到行动动画的显式映射。精确映射优先于 DefaultActionClip；IntentId 必须非空，不会根据 Intent、Actor 或资源名称猜测。"))
	TMap<FName, FWacomBattleEnemyHostAnimationClip> ActionClipsByIntentId;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Host Animation|Destroyed",
		meta = (ToolTip = "敌人全部部位被破坏后播放的整体终态动画。Flipbook 为空表示不播放整体终态；播放完成后停在最后一帧。"))
	FWacomBattleEnemyHostAnimationClip DestroyedClip;

	const FWacomBattleEnemyHostAnimationClip* ResolveActionClip(FName IntentId) const;
	const FWacomBattleEnemyHostAnimationClip* ResolveDestroyedClip() const;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif
};
