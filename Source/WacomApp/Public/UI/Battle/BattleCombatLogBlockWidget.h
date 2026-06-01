// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "BattleCombatLogBlockWidget.generated.h"

class UTextBlock;
class UVerticalBox;

/**
 * Single player-facing combat log command block.
 *
 * WBP can bind HeaderText and DetailsBox for custom styling. The C++ fallback
 * renders a readable header plus compact detail lines.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UBattleCombatLogBlockWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Combat Log")
	void SetCombatLogBlockData(const FWacomBattleCombatLogBlockView& InBlock);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Combat Log")
	FWacomBattleCombatLogBlockView GetCurrentBlock() const { return CurrentBlock; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HeaderText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> DetailsBox;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|Combat Log", DisplayName = "On Combat Log Block Updated")
	void BP_OnCombatLogBlockUpdated(const FWacomBattleCombatLogBlockView& Block);

private:
	UPROPERTY(Transient)
	FWacomBattleCombatLogBlockView CurrentBlock;

	void ApplyCurrentBlockToWidgets();
};
