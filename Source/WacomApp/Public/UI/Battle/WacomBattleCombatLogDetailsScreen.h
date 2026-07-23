// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/BattleCombatLogTurnDividerWidget.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleCombatLogDetailsEntryWidget.h"
#include "UI/Battle/WacomBattleSecondaryPanelScreenBase.h"
#include "WacomBattleCombatLogDetailsScreen.generated.h"

class UCheckBox;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWacomBattleCombatActivityStyle;

/** Battle 战斗日志详情二级面板。只消费打开时复制的只读历史。 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomBattleCombatLogDetailsScreen : public UWacomBattleSecondaryPanelScreenBase
{
	GENERATED_BODY()

public:
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnDetailsModeChangedNative, bool);
	FOnDetailsModeChangedNative& OnDetailsModeChangedNative() { return DetailsModeChangedNative; }

	void SetCombatLogContext(
		const TArray<FWacomBattleCombatLogTurnSectionView>& InHistory,
		bool bInShowDetails);

	bool IsShowingDetails() const { return bShowDetails; }
	int32 GetRenderedEntryCount() const { return RenderedEntryCount; }
	void SetAuthoringDefaults(
		UWacomBattleCombatActivityStyle* InStyle,
		TSubclassOf<UWacomBattleCombatLogDetailsEntryWidget> InEntryClass,
		TSubclassOf<UBattleCombatLogTurnDividerWidget> InDividerClass);
	UWacomBattleCombatActivityStyle* GetActivityStyle() const { return ActivityStyle; }
	UClass* GetDetailsEntryWidgetClass() const { return DetailsEntryWidgetClass.Get(); }
	UClass* GetTurnDividerWidgetClass() const { return TurnDividerWidgetClass.Get(); }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCheckBox> DetailsToggle;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UScrollBox> HistoryScrollBox;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> HistoryList;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> EmptyText;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Combat Log Details",
		meta = (ToolTip = "战斗日志详情与常驻播报共同使用的图标和颜色样式。"))
	TObjectPtr<UWacomBattleCombatActivityStyle> ActivityStyle;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Combat Log Details",
		meta = (ToolTip = "根行动、结果与结果说明使用的正式自适应 Entry Widget 类。"))
	TSubclassOf<UWacomBattleCombatLogDetailsEntryWidget> DetailsEntryWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|Battle|Combat Log Details",
		meta = (ToolTip = "回合开始和结束分割线使用的 Widget 类。"))
	TSubclassOf<UBattleCombatLogTurnDividerWidget> TurnDividerWidgetClass;

private:
	UFUNCTION()
	void HandleDetailsToggleChanged(bool bIsChecked);

	void ResolveRuntimeBindings();
	void ClearRenderedEntries();
	void RebuildHistory();
	void AddTurnDivider(int32 TurnNumber, bool bIsStart);
	void AddDetailsEntry(const FWacomBattleCombatLogDetailsEntryView& Entry);
	void ScrollToLatestNextTick();

	UPROPERTY(Transient)
	TArray<FWacomBattleCombatLogTurnSectionView> HistorySnapshot;

	FOnDetailsModeChangedNative DetailsModeChangedNative;
	bool bShowDetails = false;
	bool bApplyingContext = false;
	int32 RenderedEntryCount = 0;
};
