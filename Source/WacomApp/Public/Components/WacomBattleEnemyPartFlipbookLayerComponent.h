// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PaperFlipbookComponent.h"
#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif
#include "WacomBattleEnemyPartFlipbookLayerComponent.generated.h"

class UPaperFlipbook;

/**
 * 敌人部位可直接在 Host Blueprint 视口中制作的 Flipbook 层。
 *
 * 组件自身的 Transform、Flipbook、Tint、Material、SortOrder、PlayRate、Looping
 * 就是 authored truth；运行时只在同一个组件上临时换片并恢复。
 */
UCLASS(ClassGroup = (Wacom), meta = (BlueprintSpawnableComponent,
	ToolTip = "敌人部位的真实 Flipbook 视觉层。必须直接挂在 Enemy Part 组件下，可在 Host Blueprint 视口直接调整。"))
class WACOMAPP_API UWacomBattleEnemyPartFlipbookLayerComponent : public UPaperFlipbookComponent
{
	GENERATED_BODY()

public:
	UWacomBattleEnemyPartFlipbookLayerComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layer",
		meta = (ToolTip = "部位内稳定视觉层 ID，例如 SlimeTrio.Left.Main。动画 Style 只按此 ID 精确寻址。"))
	FName LayerId = TEXT("Main");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layer|Playback",
		meta = (ToolTip = "authored Idle 的初始播放位置，单位秒。推荐用 0–0.12 秒制造多个部位的稳定错帧；运行时恢复时会回到此值。"))
	float InitialPlaybackPositionSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layer|Destroyed",
		meta = (ToolTip = "本层可选破损终态 Flipbook。为空时部位 Destroyed 后保持原资源。"))
	TObjectPtr<UPaperFlipbook> DestroyedFlipbook = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wacom|Battle|Scene Enemy|Visual Layer|Destroyed",
		meta = (ToolTip = "破损终态 Flipbook 播放倍率。必须为有限正数；非循环播放并停在最后一帧。"))
	float DestroyedPlayRate = 1.0f;

#if WITH_EDITOR
	virtual EDataValidationResult IsDataValid(FDataValidationContext& Context) const override;
#endif

protected:
	virtual void OnRegister() override;
	virtual void OnUnregister() override;
};
