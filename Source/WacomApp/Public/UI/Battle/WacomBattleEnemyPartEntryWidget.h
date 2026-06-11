// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleWidgetBase.h"
#include "WacomBattleEnemyPanelViewData.h"
#include "WacomBattleEnemyPartEntryWidget.generated.h"

class UTextBlock;
class UBorder;
class UHorizontalBox;
class UVerticalBox;
class UWidget;

UCLASS(Blueprintable, meta = (ToolTip = "敌人面板中的单个部位条目。只渲染 FWacomBattleEnemyPartEntryViewData。"))
class WACOMAPP_API UWacomBattleEnemyPartEntryWidget : public UWacomBattleWidgetBase
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|Battle|Enemy Panel")
	virtual void SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView);

	UFUNCTION(BlueprintPure, Category = "Wacom|Battle|Enemy Panel", meta = (ToolTip = "返回当前部位条目的只读展示数据。用于 WBP 表现读取，不应反向修改战斗状态。"))
	const FWacomBattleEnemyPartEntryViewData& GetPartEntryViewData() const { return CurrentView; }

	void SetFallbackIntroDelaySeconds(float InDelaySeconds);

#if WITH_AUTOMATION_TESTS
	void TickFallbackMotionForTest(float DeltaSeconds);
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeRefreshFromSnapshot(const FBattleSnapshot& Snap) override;

private:
	void RefreshText();
	FText BuildStatusText() const;
	void StartFallbackIntroAnimation();
	void StartFallbackPulseAnimation(const FLinearColor& InPulseTint, float InIntensity);
	void ApplyFallbackMotionVisual();
	float GetFallbackBaseOpacity() const;
	FLinearColor GetFallbackBaseBackgroundColor() const;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UVerticalBox> RootBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> RowBox = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> EntryBackground = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PartNameText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HpLabelText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HpText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ShieldPill = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldLabelText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShieldText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InitiativeLabelText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> InitiativeText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatsText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> IntentText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText = nullptr;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> DestroyedOverlay = nullptr;

	UPROPERTY(Transient)
	FWacomBattleEnemyPartEntryViewData CurrentView;

	FLinearColor FallbackPulseTint = FLinearColor::White;
	float FallbackIntroDelaySeconds = 0.0f;
	float FallbackIntroElapsedSeconds = 1.0f;
	float FallbackPulseElapsedSeconds = 999.0f;
	float FallbackPulseIntensity = 0.0f;
	bool bUsingGeneratedFallbackLayout = false;
	bool bHasReceivedViewData = false;
	bool bFallbackIntroActive = false;
	bool bFallbackPulseActive = false;
};
