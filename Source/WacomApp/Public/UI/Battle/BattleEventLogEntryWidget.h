// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "BattleEventLogEntryWidget.generated.h"

class UTextBlock;

/**
 * Single battle event log row.
 *
 * WBP can bind MessageText and later use VisualTone/IconKey for custom color
 * or icon styling. It is display-only and does not submit battle commands.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UBattleEventLogEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|EventLog")
	void SetEventLogEntryData(const FBattleEventPresentationView& InEntry);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|EventLog")
	FBattleEventPresentationView GetCurrentEntry() const { return CurrentEntry; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> MessageText;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|EventLog", DisplayName = "On Event Log Entry Updated")
	void BP_OnEventLogEntryUpdated(const FBattleEventPresentationView& Entry);

private:
	UPROPERTY(Transient)
	FBattleEventPresentationView CurrentEntry;

	void ApplyCurrentEntryToWidgets();
};
