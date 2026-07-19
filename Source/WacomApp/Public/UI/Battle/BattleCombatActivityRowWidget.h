// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "BattleCombatActivityRowWidget.generated.h"

class UBorder;
class UImage;
class USizeBox;
class UTextBlock;

/** BattleHUD 常驻活动播报器中的一行，只消费 ViewData。 */
UCLASS(Blueprintable, meta = (ToolTip = "BattleHUD 常驻活动播报器中的单行。只显示 ViewData，不提交 Battle 命令。"))
class WACOMAPP_API UBattleCombatActivityRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Activity", meta = (ToolTip = "设置活动行 ViewData 与已解析图标。只刷新 UI。"))
	void SetActivityRowData(const FWacomBattleCombatActivityRowView& InRow, const FSlateBrush& InIconBrush);

	void SetPlaybackPresentation(float Opacity, float TranslationY);
	void ClearActivityRow();

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Activity")
	FWacomBattleCombatActivityRowView GetCurrentRow() const { return CurrentRow; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> RowRoot;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> ActivityIcon;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> IndentSpacer;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActivityText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Combat Activity", DisplayName = "On Activity Row Updated",
		meta = (ToolTip = "活动行数据刷新后的纯表现钩子。不得提交 Battle 命令。"))
	void BP_OnActivityRowUpdated(const FWacomBattleCombatActivityRowView& Row);

private:
	UPROPERTY(Transient)
	FWacomBattleCombatActivityRowView CurrentRow;

	FSlateBrush CurrentIconBrush;
	bool bHasRow = false;

	void ApplyCurrentRow();
};
