// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CommonUserWidget.h"
#include "CoreMinimal.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "WacomBattleStatusTooltipWidget.generated.h"

class UImage;
class UTextBlock;
class UWidget;

/**
 * Passive status inspection tooltip.
 *
 * Placement and viewport clamping are owned by Slate's native tooltip path.
 * This widget only renders immutable status presentation data.
 */
UCLASS(Blueprintable, meta = (ToolTip = "战斗状态的鼠标悬浮说明。只显示 UI ViewData，由 Slate 原生 Tooltip 负责跟随鼠标和视口避让。"))
class WACOMAPP_API UWacomBattleStatusTooltipWidget : public UCommonUserWidget
{
	GENERATED_BODY()

public:
	void SetStatusView(const FWacomBattleStatusIconView& InView);
	void SetOverflowViews(const TArray<FWacomBattleStatusIconView>& InHiddenViews);

	const FWacomBattleStatusIconView& GetStatusView() const { return CurrentStatusView; }
	const TArray<FWacomBattleStatusIconView>& GetOverflowViews() const { return CurrentOverflowViews; }
	bool IsShowingOverflow() const { return bShowingOverflow; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativePreConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> TooltipIcon = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TitleText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StackText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> CoreEffectText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TriggerTimingText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> StackPolicyText = nullptr;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> OverflowBodyText = nullptr;

private:
	void RefreshDisplay();

	UPROPERTY(Transient)
	FWacomBattleStatusIconView CurrentStatusView;

	UPROPERTY(Transient)
	TArray<FWacomBattleStatusIconView> CurrentOverflowViews;

	bool bShowingOverflow = false;
};
