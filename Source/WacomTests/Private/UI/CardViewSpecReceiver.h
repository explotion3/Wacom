// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "PaperSprite.h"
#include "UI/Card/WacomCardView.h"
#include "CardViewSpecReceiver.generated.h"

class UImage;
class UPanelWidget;

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
