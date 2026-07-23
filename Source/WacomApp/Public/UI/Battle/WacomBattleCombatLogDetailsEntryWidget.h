// Copyright Wacom. All Rights Reserved.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleStatusTooltipWidget.h"
#include "WacomBattleCombatLogDetailsEntryWidget.generated.h"

class UBorder;
class UImage;
class USizeBox;
class UTextBlock;
class UWidget;

/**
 * Passive, auto-height row used only by the Combat Log details screen.
 *
 * The short-lived BattleHUD feed intentionally keeps its own fixed-height row.
 */
UCLASS(Blueprintable, meta = (ToolTip = "战斗日志二级菜单的自适应层级条目。只显示 Details ViewData，不提交战斗命令。"))
class WACOMAPP_API UWacomBattleCombatLogDetailsEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UWacomBattleCombatLogDetailsEntryWidget(
		const FObjectInitializer& ObjectInitializer);

	void SetDetailsEntryData(
		const FWacomBattleCombatLogDetailsEntryView& InEntry,
		const FSlateBrush& InIconBrush);
	void ClearDetailsEntry();

	void SetStatusTooltipWidgetClass(
		TSubclassOf<UWacomBattleStatusTooltipWidget> InTooltipWidgetClass);
	TSubclassOf<UWacomBattleStatusTooltipWidget> GetStatusTooltipWidgetClass() const
	{
		return StatusTooltipWidgetClass;
	}

	const FWacomBattleCombatLogDetailsEntryView& GetCurrentEntry() const
	{
		return CurrentEntry;
	}
	float GetAppliedIndentWidth() const;
	bool HasHistoricalStatusTooltip() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> DetailsEntrySize = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> EntryRoot = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> IndentSpacer = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> EntryIconSize = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> EntryIcon = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TargetText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ValueText = nullptr;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Combat Log Details",
		meta = (AllowAbstract = "false", ToolTip = "状态结果图标悬停时使用的 Tooltip Widget。正式资产使用 WBP_BattleStatusTooltip；为空时回退到原生 Widget。"))
	TSubclassOf<UWacomBattleStatusTooltipWidget> StatusTooltipWidgetClass;

private:
	UPROPERTY(Transient)
	FWacomBattleCombatLogDetailsEntryView CurrentEntry;

	UPROPERTY(Transient)
	TObjectPtr<UWacomBattleStatusTooltipWidget> CachedStatusTooltip = nullptr;

	FSlateBrush CurrentIconBrush;
	bool bHasEntry = false;

	void ApplyCurrentEntry();
	void RefreshStatusTooltipBinding();
	void ClearStatusTooltipBinding();

	UFUNCTION()
	UWidget* HandleBuildStatusTooltipWidget();
};
