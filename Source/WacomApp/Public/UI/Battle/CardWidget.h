// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "Snapshots/HandSnapshot.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "CardWidget.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UWacomCardView;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FWacomCardWidgetClicked, FGuid, CardInstanceId);

/**
 * 单张手牌 Widget。
 *
 * C++ 内置默认外观：
 *   SizeBox(120x160)
 *     └── Border(根据 Playable/Targeting 改背景色)
 *           └── VerticalBox
 *                 ├── NameText
 *                 ├── CostText
 *                 ├── ZoneText
 *                 └── RootButton(透明)
 *
 * WBP 子类可完全覆盖。
 *
 * WBP 约定（BindWidget）：
 * - RootButton : UButton
 * - NameText   : UTextBlock (Optional)
 * - CostText   : UTextBlock (Optional)
 * - ZoneText   : UTextBlock (Optional)
 * - FrameBorder: UBorder    (Optional，用于 Playable/Targeting 色变)
 */
UCLASS(Blueprintable)
class WACOMAPP_API UCardWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void ApplyCardSnapshot(const FHandCardSnapshot& InSnap);

	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|UI")
	void SetTargetingHighlight(bool bTargeting);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	FGuid GetCardInstanceId() const { return CachedSnap.InstanceId; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	const FHandCardSnapshot& GetCardSnapshot() const { return CachedSnap; }

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|UI")
	const FWacomCardViewData& GetCurrentCardViewData() const { return CurrentCardViewData; }

	/** 测试/诊断用：RootButton 当前是否允许点击。 */
	bool IsRootButtonEnabled() const;

	/** 测试/诊断用：模拟点击 RootButton。 */
	void RequestClickForTest();

	UPROPERTY(BlueprintAssignable, Category = "Wacom|Battle|UI")
	FWacomCardWidgetClicked OnCardClicked;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Data Applied")
	void BP_OnDataApplied(const FHandCardSnapshot& Snap);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Playable Changed")
	void BP_OnPlayableChanged(bool bPlayable);

	UFUNCTION(BlueprintImplementableEvent, Category = "Wacom|Battle|UI", DisplayName = "On Targeting Highlight Changed")
	void BP_OnTargetingHighlightChanged(bool bTargeting);

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> RootButton;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ZoneText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> FrameBorder;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWacomCardView> CardView;

private:
	UFUNCTION()
	void HandleRootButtonClicked();

	void UpdateFrameColor();
	void ApplyFallbackText(const FHandCardSnapshot& InSnap);

	FHandCardSnapshot CachedSnap;
	FWacomCardViewData CurrentCardViewData;
	bool bLastPlayable = false;
	bool bLastTargeting = false;
};
