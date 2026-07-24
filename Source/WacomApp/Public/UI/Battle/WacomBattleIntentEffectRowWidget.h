// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationTypes.h"
#include "WacomBattleIntentEffectRowWidget.generated.h"

class UImage;
class UTextBlock;

/** Passive, no-Tick row for one localized enemy Intent effect. */
UCLASS(Blueprintable, meta = (ToolTip = "敌人 Intent 的单条只读效果行。只消费表现 ViewData，不读取 Behavior DataAsset 或战斗状态。"))
class WACOMAPP_API UWacomBattleIntentEffectRowWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetEffectRowViewData(
		const FWacomBattleIntentEffectRowViewData& InView);

	const FWacomBattleIntentEffectRowViewData& GetEffectRowViewData() const
	{
		return CurrentView;
	}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EffectIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TargetText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EffectText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CoreRuleText = nullptr;

private:
	FWacomBattleIntentEffectRowViewData CurrentView;
	void RefreshDisplay();
};
