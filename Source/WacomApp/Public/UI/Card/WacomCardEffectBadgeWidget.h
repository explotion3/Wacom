// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "WacomCardEffectBadgeWidget.generated.h"

class UImage;
class UPanelWidget;
class UPaperSprite;

#if WITH_AUTOMATION_TESTS
struct FWacomCardEffectBadgeAutomationTestView
{
	int32 ApplyCount = 0;
	int32 DigitImageUpdateCount = 0;
};
#endif

/**
 * Visual-only numeric effect badge for card faces.
 *
 * Data source: UWacomCardView creates one widget per FWacomCardViewEffectBadge.
 * This widget does not submit battle, backpack, or run commands.
 */
UCLASS(Blueprintable)
class WACOMAPP_API UWacomCardEffectBadgeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Wacom|CardView")
	void SetEffectBadgeData(const FWacomCardViewEffectBadge& InData);

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	const FWacomCardViewEffectBadge& GetEffectBadgeData() const { return CurrentData; }

	UFUNCTION(BlueprintPure, Category = "Wacom|CardView")
	FText GetValueText() const;

#if WITH_AUTOMATION_TESTS
	FWacomCardEffectBadgeAutomationTestView GetAutomationTestViewForTest() const
	{
		FWacomCardEffectBadgeAutomationTestView View;
		View.ApplyCount = ApplyCountForTest;
		View.DigitImageUpdateCount = DigitImageUpdateCountForTest;
		return View;
	}
#endif

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> BadgeFrameImage;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> DigitHost;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ToolTip = "效果徽章类型对应的底图 PaperSprite。Damage / Poison / Burn / Heal / Shield 等类型可分别配置美术底板；缺失时只显示数字。"))
	TMap<EWacomCardViewEffectBadgeKind, TSoftObjectPtr<UPaperSprite>> BadgeFrameSprites;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ToolTip = "效果徽章数字 0-9 对应的 PaperSprite。DigitHost 绑定且数字资源齐全时使用图片数字；缺失时该徽章只显示底图。"))
	TMap<int32, TSoftObjectPtr<UPaperSprite>> DigitSprites;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ClampMin = "1", UIMin = "8", UIMax = "256", ToolTip = "效果徽章底图显示尺寸，单位为 UMG 布局像素。用于 C++ fallback 和运行时设置 BadgeFrameImage Brush 尺寸。"))
	FVector2D BadgeFrameDrawSize = FVector2D(92.0f, 44.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ClampMin = "1", UIMin = "6", UIMax = "96", ToolTip = "效果徽章单个数字图标尺寸，单位为 UMG 布局像素。"))
	FVector2D DigitDrawSize = FVector2D(22.0f, 30.0f);

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ClampMin = "1", UIMin = "1", UIMax = "6", ToolTip = "效果徽章图片数字的最小显示位数。默认 3，数值 1 会显示为 001；超过位数时显示真实位数。"))
	int32 MinimumDigitCount = 3;

	UPROPERTY(EditDefaultsOnly, Category = "Wacom|CardView|Effect Badge", meta = (ToolTip = "效果徽章中间数字图标的布局 Padding，单位为 UMG 布局像素。默认左右各 1，用于拉开三位图片数字间距。"))
	FMargin InteriorDigitPadding = FMargin(1.0f, 0.0f, 1.0f, 0.0f);

private:
	UPROPERTY(Transient)
	FWacomCardViewEffectBadge CurrentData;

	UPROPERTY(Transient)
	TMap<EWacomCardViewEffectBadgeKind, TObjectPtr<UPaperSprite>> ResolvedBadgeFrameSprites;

	UPROPERTY(Transient)
	TMap<int32, TObjectPtr<UPaperSprite>> ResolvedDigitSprites;

	bool bSpriteCachesBuilt = false;
	bool bHasAppliedData = false;

#if WITH_AUTOMATION_TESTS
	int32 ApplyCountForTest = 0;
	int32 DigitImageUpdateCountForTest = 0;
#endif

	void ApplyCurrentDataToWidgets();
	void EnsureSpriteCachesBuilt();
	void RebuildSpriteCaches();
	void UpdateFrameImage();
	void UpdateDigitImages();
	UImage* EnsureDigitImage(int32 Index);
	TArray<int32> SplitIntoDigits(int32 Value) const;
	static void SetSpriteBrush(UImage& Image, UPaperSprite& Sprite, const FVector2D& DesiredSize);
};
