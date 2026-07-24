// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationTypes.h"
#include "WacomBattleIntentTooltipWidget.generated.h"

class UImage;
class UPanelWidget;
class UTextBlock;
class UWacomBattleIntentEffectRowWidget;

/** Passive mouse-following Intent tooltip content. Slate owns placement and clamping. */
UCLASS(Blueprintable, meta = (ToolTip = "敌人 Intent 的鼠标 Tooltip 内容。无 Tick、不可命中，由标准 ToolTipWidgetDelegate 创建。"))
class WACOMAPP_API UWacomBattleIntentTooltipWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	UWacomBattleIntentTooltipWidget(const FObjectInitializer& ObjectInitializer);

	void SetIntentViewData(
		const FWacomBattleIntentPresentationViewData& InView);
	void SetEffectRowWidgetClass(
		TSubclassOf<UWacomBattleIntentEffectRowWidget> InClass);
	TSubclassOf<UWacomBattleIntentEffectRowWidget> GetEffectRowWidgetClass() const
	{
		return EffectRowWidgetClass;
	}

	const FWacomBattleIntentPresentationViewData& GetIntentViewData() const
	{
		return CurrentView;
	}

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Wacom|Battle|Enemy Intent",
		meta = (ToolTip = "Tooltip 中每条 Intent 效果使用的被动 Row 类。为空时使用 C++ 默认 Row。"))
	TSubclassOf<UWacomBattleIntentEffectRowWidget> EffectRowWidgetClass;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IntentIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentMetaText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> EffectsList = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> OverflowText = nullptr;

private:
	FWacomBattleIntentPresentationViewData CurrentView;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UWacomBattleIntentEffectRowWidget>> EffectRows;

	void RefreshDisplay();
	void RebuildEffectRows();
};
