// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "PaperSprite.h"
#include "Components/WrapBox.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"

#define LOCTEXT_NAMESPACE "WacomCardView"

namespace
{
	constexpr float DefaultCardWidth = 260.f;
	constexpr float DefaultCardHeight = 380.f;

	void SetOptionalText(UTextBlock* TextBlock, const FText& Text)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(Text);
		TextBlock->SetVisibility(Text.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	void SetOptionalNumberText(UTextBlock* TextBlock, int32 Value, bool bShow)
	{
		if (!TextBlock)
		{
			return;
		}

		TextBlock->SetText(FText::AsNumber(Value));
		TextBlock->SetVisibility(bShow ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

UWacomCardView::UWacomCardView(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (UClass* Loaded = LoadObject<UClass>(
		nullptr,
		TEXT("/Game/Wacom/UI/Card/WBP_CardEffectBadge.WBP_CardEffectBadge_C")))
	{
		EffectBadgeWidgetClass = Loaded;
	}
	else
	{
		EffectBadgeWidgetClass = UWacomCardEffectBadgeWidget::StaticClass();
	}

	static ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> SurfaceFoilMaterialFinder(
		TEXT("/Game/DreamMaterials/Card/M_CardSurface_CosmicFoil.M_CardSurface_CosmicFoil"));
	if (SurfaceFoilMaterialFinder.Succeeded())
	{
		SurfaceFoilMaterial = SurfaceFoilMaterialFinder.Get();
	}
}

TSharedRef<SWidget> UWacomCardView::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("CardViewRoot"));
		Root->SetWidthOverride(DefaultCardWidth);
		Root->SetHeightOverride(DefaultCardHeight);
		WidgetTree->RootWidget = Root;

		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("CardViewOverlay"));
		Root->AddChild(Stack);

		UBorder* Body = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CardBody"));
		Body->SetBrushColor(FLinearColor(0.06f, 0.055f, 0.045f, 0.95f));
		Body->SetPadding(FMargin(8.f));
		if (UOverlaySlot* BodySlot = Stack->AddChildToOverlay(Body))
		{
			BodySlot->SetHorizontalAlignment(HAlign_Fill);
			BodySlot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardContent"));
		Body->AddChild(Content);

		CostText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("CostText"));
		CostText->SetJustification(ETextJustify::Left);
		CostText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.85f, 0.65f, 1.f)));
		if (UVerticalBoxSlot* CostSlot = Content->AddChildToVerticalBox(CostText))
		{
			CostSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		CostDigitsHost = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("CostDigitsHost"));
		CostDigitsHost->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* DigitsSlot = Content->AddChildToVerticalBox(CostDigitsHost))
		{
			DigitsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		UHorizontalBox* HeaderStats = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderStats"));
		if (UVerticalBoxSlot* HeaderStatsSlot = Content->AddChildToVerticalBox(HeaderStats))
		{
			HeaderStatsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValueText"));
		ValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.9f, 1.f, 1.f)));
		HeaderStats->AddChild(ValueText);

		PhysiqueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PhysiqueText"));
		PhysiqueText->SetAutoWrapText(true);
		PhysiqueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 1.f, 0.75f, 1.f)));
		HeaderStats->AddChild(PhysiqueText);

		NameText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("NameText"));
		NameText->SetJustification(ETextJustify::Center);
		NameText->SetAutoWrapText(true);
		NameText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		{
			FSlateFontInfo Font = NameText->GetFont();
			Font.Size = 14;
			NameText->SetFont(Font);
		}
		if (UVerticalBoxSlot* NameSlot = Content->AddChildToVerticalBox(NameText))
		{
			NameSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		CardArt = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CardArt"));
		CardArt->SetColorAndOpacity(FLinearColor(0.18f, 0.18f, 0.18f, 1.f));
		if (UVerticalBoxSlot* ArtSlot = Content->AddChildToVerticalBox(CardArt))
		{
			ArtSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ArtSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		SurfaceFoilOverlay = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("SurfaceFoilOverlay"));
		SurfaceFoilOverlay->SetVisibility(ESlateVisibility::Collapsed);
		SurfaceFoilOverlay->SetColorAndOpacity(FLinearColor::White);
		if (UOverlaySlot* FoilSlot = Stack->AddChildToOverlay(SurfaceFoilOverlay))
		{
			FoilSlot->SetHorizontalAlignment(HAlign_Fill);
			FoilSlot->SetVerticalAlignment(VAlign_Fill);
		}

		TypeText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TypeText"));
		TypeText->SetJustification(ETextJustify::Center);
		TypeText->SetAutoWrapText(true);
		TypeText->SetColorAndOpacity(FSlateColor(FLinearColor(0.8f, 0.75f, 0.65f, 1.f)));
		if (UVerticalBoxSlot* TypeSlot = Content->AddChildToVerticalBox(TypeText))
		{
			TypeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		EffectStatsHost = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("EffectStatsHost"));
		if (UVerticalBoxSlot* BadgeSlot = Content->AddChildToVerticalBox(EffectStatsHost))
		{
			BadgeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		DescriptionText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("DescriptionText"));
		DescriptionText->SetJustification(ETextJustify::Center);
		DescriptionText->SetAutoWrapText(true);
		DescriptionText->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.92f, 0.86f, 1.f)));
		{
			FSlateFontInfo Font = DescriptionText->GetFont();
			Font.Size = 11;
			DescriptionText->SetFont(Font);
		}
		if (UVerticalBoxSlot* DescSlot = Content->AddChildToVerticalBox(DescriptionText))
		{
			DescSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		RarityBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RarityBorder"));
		RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* RaritySlot = Stack->AddChildToOverlay(RarityBorder))
		{
			RaritySlot->SetHorizontalAlignment(HAlign_Fill);
			RaritySlot->SetVerticalAlignment(VAlign_Fill);
		}

		DisabledOverlay = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DisabledOverlay"));
		DisabledOverlay->SetBrushColor(FLinearColor(0.f, 0.f, 0.f, 0.45f));
		DisabledOverlay->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* DisabledSlot = Stack->AddChildToOverlay(DisabledOverlay))
		{
			DisabledSlot->SetHorizontalAlignment(HAlign_Fill);
			DisabledSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}

	return Super::RebuildWidget();
}

void UWacomCardView::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureSurfaceFoilOverlay();
	ApplySurfaceFoilOverlay();
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::SetCardViewData(const FWacomCardViewData& InData)
{
	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::ApplyCurrentDataToWidgets()
{
	UpdateCostDisplay();

	SetOptionalNumberText(ValueText, CurrentData.Value, CurrentData.bShowValue);
	SetOptionalText(PhysiqueText, CurrentData.bShowPhysique ? CurrentData.PhysiqueText : FText::GetEmpty());
	SetOptionalText(NameText, CurrentData.Name);
	SetOptionalText(TypeText, CurrentData.TypeText);
	SetOptionalText(DescriptionText, CurrentData.Description);
	if (EffectStatsHost)
	{
		EffectStatsHost->ClearChildren();
		UClass* BadgeClass = EffectBadgeWidgetClass
			? EffectBadgeWidgetClass.Get()
			: UWacomCardEffectBadgeWidget::StaticClass();
		for (const FWacomCardViewEffectBadge& Badge : CurrentData.EffectBadges)
		{
			UWacomCardEffectBadgeWidget* BadgeWidget = CreateWidget<UWacomCardEffectBadgeWidget>(this, BadgeClass);
			if (!BadgeWidget)
			{
				continue;
			}
			BadgeWidget->SetEffectBadgeData(Badge);
			EffectStatsHost->AddChild(BadgeWidget);
		}
		EffectStatsHost->SetVisibility(CurrentData.EffectBadges.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (CardArt)
	{
		if (CurrentData.Art)
		{
			CardArt->SetBrushFromTexture(CurrentData.Art);
			CardArt->SetColorAndOpacity(FLinearColor::White);
		}
	}

	if (RarityBorder)
	{
		if (CurrentData.Rarity.IsValid())
		{
			TSoftObjectPtr<UPaperSprite>* SpritePtr = RarityBorderSprites.Find(CurrentData.Rarity);
			if (SpritePtr && !SpritePtr->IsNull())
			{
				UPaperSprite* Sprite = SpritePtr->LoadSynchronous();
				if (Sprite)
				{
					RarityBorder->SetBrushResourceObject(Sprite);
					RarityBorder->SetVisibility(ESlateVisibility::HitTestInvisible);
				}
				else
				{
					RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
				}
			}
			else
			{
				RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
			}
		}
		else
		{
			RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(CurrentData.bDisabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UWacomCardView::EnsureSurfaceFoilOverlay()
{
	if (SurfaceFoilOverlay || !WidgetTree)
	{
		return;
	}

	UOverlay* HostOverlay = Cast<UOverlay>(WidgetTree->RootWidget);
	if (!HostOverlay)
	{
		WidgetTree->ForEachWidget([&HostOverlay](UWidget* Widget)
		{
			if (!HostOverlay)
			{
				HostOverlay = Cast<UOverlay>(Widget);
			}
		});
	}

	if (!HostOverlay)
	{
		return;
	}

	SurfaceFoilOverlay = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("SurfaceFoilOverlay_Runtime"));
	SurfaceFoilOverlay->SetVisibility(ESlateVisibility::Collapsed);
	SurfaceFoilOverlay->SetColorAndOpacity(FLinearColor::White);

	if (UOverlaySlot* FoilSlot = HostOverlay->AddChildToOverlay(SurfaceFoilOverlay))
	{
		FoilSlot->SetHorizontalAlignment(HAlign_Fill);
		FoilSlot->SetVerticalAlignment(VAlign_Fill);
	}
}

void UWacomCardView::ApplySurfaceFoilOverlay()
{
	if (!SurfaceFoilOverlay)
	{
		return;
	}

	if (SurfaceFoilMaterial)
	{
		SurfaceFoilOverlay->SetBrushFromMaterial(SurfaceFoilMaterial);
		SurfaceFoilOverlay->SetColorAndOpacity(FLinearColor::White);
	}

	const bool bHasFoilBrush = SurfaceFoilOverlay->GetBrush().GetResourceObject() != nullptr;
	SurfaceFoilOverlay->SetVisibility(bHasFoilBrush ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UWacomCardView::UpdateCostDisplay()
{
	const bool bTextOnly = !CostDigitsHost || CostDigitIcons.IsEmpty() || !CurrentData.bShowCost;

	if (CostText)
	{
		CostText->SetText(FText::AsNumber(CurrentData.Cost));
		CostText->SetVisibility(
			(bTextOnly && CurrentData.bShowCost)
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Collapsed);
	}

	if (CostDigitsHost)
	{
		CostDigitsHost->ClearChildren();
		CostDigitsHost->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (bTextOnly)
	{
		return;
	}

	const TArray<int32> Digits = SplitIntoDigits(CurrentData.Cost);
	if (Digits.Num() == 0)
	{
		return;
	}

	for (int32 Digit : Digits)
	{
		TSoftObjectPtr<UPaperSprite>* IconPtr = CostDigitIcons.Find(Digit);
		if (!IconPtr || IconPtr->IsNull())
		{
			continue;
		}

		UPaperSprite* Sprite = IconPtr->LoadSynchronous();
		if (!Sprite)
		{
			continue;
		}

		UImage* DigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		DigitImage->SetBrushResourceObject(Sprite);
		CostDigitsHost->AddChild(DigitImage);
	}

	if (CostDigitsHost->GetChildrenCount() > 0)
	{
		CostDigitsHost->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
}

TArray<int32> UWacomCardView::SplitIntoDigits(int32 Value)
{
	TArray<int32> Result;
	if (Value <= 0)
	{
		Result.Add(0);
		return Result;
	}

	TArray<int32> Reversed;
	while (Value > 0)
	{
		Reversed.Add(Value % 10);
		Value /= 10;
	}

	for (int32 i = Reversed.Num() - 1; i >= 0; --i)
	{
		Result.Add(Reversed[i]);
	}
	return Result;
}

#undef LOCTEXT_NAMESPACE
