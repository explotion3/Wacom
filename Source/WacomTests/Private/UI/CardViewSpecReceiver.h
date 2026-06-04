// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "PaperSprite.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardView.h"
#include "CardViewSpecReceiver.generated.h"

class UImage;
class UPanelWidget;

UCLASS()
class UWacomCardEffectBadgeSpecProbe : public UWacomCardEffectBadgeWidget
{
	GENERATED_BODY()

public:
	void SetDigitSpriteForTest(int32 Digit, UPaperSprite* Sprite)
	{
		DigitSprites.Add(Digit, TSoftObjectPtr<UPaperSprite>(Sprite));
	}

	void SetMinimumDigitCountForTest(int32 Count)
	{
		MinimumDigitCount = Count;
	}

	void SetInteriorDigitPaddingForTest(const FMargin& InPadding)
	{
		InteriorDigitPadding = InPadding;
	}

	UPanelWidget* GetDigitHostForTest() const { return DigitHost; }
	int32 GetApplyCountForTest() const { return UWacomCardEffectBadgeWidget::GetApplyCountForTest(); }
	int32 GetDigitImageUpdateCountForTest() const { return UWacomCardEffectBadgeWidget::GetDigitImageUpdateCountForTest(); }
};

UCLASS()
class UWacomCardViewSpecProbe : public UWacomCardView
{
	GENERATED_BODY()

public:
	void SetCostDigitIconForTest(int32 Digit, UPaperSprite* Sprite)
	{
		CostDigitIcons.Add(Digit, TSoftObjectPtr<UPaperSprite>(Sprite));
	}

	void SetCostDigitSizeForTest(const FVector2D& Size)
	{
		CostDigitSize = Size;
	}

	void ClearCostDigitIconsForTest()
	{
		CostDigitIcons.Reset();
	}

	void SetDurabilityDigitIconForTest(int32 Digit, UPaperSprite* Sprite)
	{
		DurabilityDigitIcons.Add(Digit, TSoftObjectPtr<UPaperSprite>(Sprite));
	}

	void SetRarityBorderSpriteForTest(const FGameplayTag& Rarity, UPaperSprite* Sprite)
	{
		RarityBorderSprites.Add(Rarity, TSoftObjectPtr<UPaperSprite>(Sprite));
	}

	UImage* GetCostDigitImageForTest() const { return CostDigitImage; }
	UWidget* GetDurabilityHostForTest() const { return DurabilityHost; }
	UPanelWidget* GetDurabilityDigitsHostForTest() const { return DurabilityDigitsHost; }
	UImage* GetRarityBorderForTest() const { return RarityBorder; }
	UPanelWidget* GetEffectStatsHostForTest() const { return EffectStatsHost; }
	int32 GetTextDisplayUpdateCountForTest() const { return UWacomCardView::GetTextDisplayUpdateCountForTest(); }
	int32 GetCostDisplayUpdateCountForTest() const { return UWacomCardView::GetCostDisplayUpdateCountForTest(); }
	int32 GetDurabilityDisplayUpdateCountForTest() const { return UWacomCardView::GetDurabilityDisplayUpdateCountForTest(); }
	int32 GetRarityDisplayUpdateCountForTest() const { return UWacomCardView::GetRarityDisplayUpdateCountForTest(); }
	int32 GetArtDisplayUpdateCountForTest() const { return UWacomCardView::GetArtDisplayUpdateCountForTest(); }
	int32 GetDisabledDisplayUpdateCountForTest() const { return UWacomCardView::GetDisabledDisplayUpdateCountForTest(); }
	int32 GetEffectBadgeDisplayUpdateCountForTest() const { return UWacomCardView::GetEffectBadgeDisplayUpdateCountForTest(); }
	UPanelWidget* GetEffectBadgeSlotForTest(int32 Index) const
	{
		switch (Index)
		{
		case 0: return EffectBadgeSlot1;
		case 1: return EffectBadgeSlot2;
		case 2: return EffectBadgeSlot3;
		case 3: return EffectBadgeSlot4;
		default: return nullptr;
		}
	}
};

UCLASS()
class UWacomCardViewEffectBadgeSlotProbe : public UWacomCardViewSpecProbe
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_EffectBadgeSlotProbe"));
		}

		if (!WidgetTree->RootWidget)
		{
			UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("CardViewRoot"));
			WidgetTree->RootWidget = Root;

			EffectStatsHost = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("EffectStatsHost"));
			Root->AddChild(EffectStatsHost);

			EffectBadgeSlot1 = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("EffectBadgeSlot1"));
			Root->AddChild(EffectBadgeSlot1);

			EffectBadgeSlot2 = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("EffectBadgeSlot2"));
			Root->AddChild(EffectBadgeSlot2);

			EffectBadgeSlot3 = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("EffectBadgeSlot3"));
			Root->AddChild(EffectBadgeSlot3);

			EffectBadgeSlot4 = WidgetTree->ConstructWidget<UOverlay>(
				UOverlay::StaticClass(),
				TEXT("EffectBadgeSlot4"));
			Root->AddChild(EffectBadgeSlot4);
		}

		return Super::RebuildWidget();
	}
};

UCLASS()
class UWacomCardViewSingleCostDigitProbe : public UWacomCardViewSpecProbe
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_SingleCostDigitProbe"));
		}

		if (!WidgetTree->RootWidget)
		{
			USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				TEXT("CardViewRoot"));
			WidgetTree->RootWidget = Root;

			CostDigitImage = WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(),
				TEXT("CostDigitImage"));
			Root->AddChild(CostDigitImage);
		}

		return Super::RebuildWidget();
	}
};

UCLASS()
class UWacomCardViewRetainerRefreshProbe : public UWacomCardView
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_RetainerRefreshProbe"));
		}

		if (!WidgetTree->RootWidget)
		{
			URetainerBox* Retainer = WidgetTree->ConstructWidget<URetainerBox>(
				URetainerBox::StaticClass(),
				TEXT("CardViewRetainer"));
			WidgetTree->RootWidget = Retainer;

			USizeBox* Body = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				TEXT("CardSizeBox"));
			Body->SetWidthOverride(296.0f);
			Body->SetHeightOverride(420.0f);
			CardSizeBox = Body;
			Retainer->AddChild(Body);
		}

		return Super::RebuildWidget();
	}
};
