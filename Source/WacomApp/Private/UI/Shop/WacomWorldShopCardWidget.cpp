// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomWorldShopCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Styling/SlateBrush.h"
#include "UI/Card/WacomCardView.h"

namespace
{
	constexpr const TCHAR* RequiredCardViewClassPath =
		TEXT("/Game/Wacom/UI/Card/WBP_FirstPersonCardView.WBP_FirstPersonCardView_C");
	constexpr float WorldCardDesignWidth = 360.0f;
	constexpr float WorldCardDesignHeight = 488.0f;
	constexpr float PriceFooterWidth = 296.0f;
	constexpr float PriceFooterHeight = 52.0f;
}

UWacomWorldShopCardWidget::UWacomWorldShopCardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::Visible);
}

const TCHAR* UWacomWorldShopCardWidget::GetRequiredCardViewClassPath()
{
	return RequiredCardViewClassPath;
}

void UWacomWorldShopCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	EnsureFallbackWidgetTree();
	if (PrimaryActionButton)
	{
		PrimaryActionButton->OnPressed.AddUniqueDynamic(this, &ThisClass::HandlePrimaryPressed);
		PrimaryActionButton->OnReleased.AddUniqueDynamic(this, &ThisClass::HandlePrimaryReleased);
		PrimaryActionButton->OnClicked.AddUniqueDynamic(this, &ThisClass::HandlePrimaryClicked);
	}
	RefreshView();
}

void UWacomWorldShopCardWidget::NativeDestruct()
{
	if (PrimaryActionButton)
	{
		PrimaryActionButton->OnPressed.RemoveDynamic(this, &ThisClass::HandlePrimaryPressed);
		PrimaryActionButton->OnReleased.RemoveDynamic(this, &ThisClass::HandlePrimaryReleased);
		PrimaryActionButton->OnClicked.RemoveDynamic(this, &ThisClass::HandlePrimaryClicked);
	}
	CancelPendingPrimaryAction();
	PrimaryActionNative.Clear();
	Super::NativeDestruct();
}

void UWacomWorldShopCardWidget::SetOfferPresentation(
	const FWacomShopOfferPresentationView& InView,
	uint32 InGeneration)
{
	OfferView = InView;
	Generation = InGeneration;
	RefreshView();
}

void UWacomWorldShopCardWidget::CancelPendingPrimaryAction()
{
	bPrimaryPressed = false;
}

void UWacomWorldShopCardWidget::HandlePrimaryPressed()
{
	bPrimaryPressed = true;
}

void UWacomWorldShopCardWidget::HandlePrimaryReleased()
{
	// UButton::OnClicked is the authoritative inside-release signal. Keep the
	// pressed flag until it arrives; leaving the card does not submit.
}

void UWacomWorldShopCardWidget::HandlePrimaryClicked()
{
	if (!bPrimaryPressed
		|| !OfferView.OfferId.IsValid()
		|| OfferView.bPurchased
		|| !IsValid(OfferView.CardDefinition.Get()))
	{
		bPrimaryPressed = false;
		return;
	}
	bPrimaryPressed = false;
	PrimaryActionNative.Broadcast(OfferView.OfferId, Generation);
}

void UWacomWorldShopCardWidget::EnsureFallbackWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;
	}

	PrimaryActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PrimaryAction"));
	const FButtonStyle InvisibleButtonStyle = FButtonStyle()
		.SetNormal(FSlateNoResource())
		.SetHovered(FSlateNoResource())
		.SetPressed(FSlateNoResource())
		.SetDisabled(FSlateNoResource())
		.SetNormalPadding(FMargin(0.0f))
		.SetPressedPadding(FMargin(0.0f));
	PrimaryActionButton->SetStyle(InvisibleButtonStyle);
	WidgetTree->RootWidget = PrimaryActionButton;

	UScaleBox* ResolutionScale = WidgetTree->ConstructWidget<UScaleBox>(
		UScaleBox::StaticClass(),
		TEXT("WorldCardResolutionScale"));
	ResolutionScale->SetStretch(EStretch::ScaleToFit);
	ResolutionScale->SetStretchDirection(EStretchDirection::Both);
	PrimaryActionButton->SetContent(ResolutionScale);

	USizeBox* DesignSurface = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("WorldCardDesignSurface"));
	DesignSurface->SetWidthOverride(WorldCardDesignWidth);
	DesignSurface->SetHeightOverride(WorldCardDesignHeight);
	ResolutionScale->SetContent(DesignSurface);

	UVerticalBox* Layout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("CardAndFooter"));
	DesignSurface->SetContent(Layout);
	USpacer* TopPadding = WidgetTree->ConstructWidget<USpacer>(
		USpacer::StaticClass(),
		TEXT("TopPadding"));
	TopPadding->SetSize(FVector2D(1.0f, 8.0f));
	Layout->AddChildToVerticalBox(TopPadding);

	UClass* ResolvedCardClass = LoadClass<UWacomCardView>(nullptr, RequiredCardViewClassPath);
	if (!ResolvedCardClass || !ResolvedCardClass->IsChildOf(UWacomCardView::StaticClass()))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomWorldShopCardWidget] 无法加载正式卡面 %s，使用 C++ UWacomCardView fallback"),
			RequiredCardViewClassPath);
		ResolvedCardClass = UWacomCardView::StaticClass();
	}
	CardView = WidgetTree->ConstructWidget<UWacomCardView>(ResolvedCardClass, TEXT("CardView"));
	if (CardView)
	{
		// WBP_FPCardView 的 Retainer 再嵌入 World WidgetComponent 会产生第二层
		// render target 与色彩损失。World-safe adapter 直接复用精确内层卡面，
		// 由外部 ScaleBox 做 2x 超采样，不复制卡面内容或规则。
		CardView->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UVerticalBoxSlot* CardSlot = Layout->AddChildToVerticalBox(CardView))
		{
			CardSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
			CardSlot->SetHorizontalAlignment(HAlign_Center);
		}
	}

	USizeBox* FooterSize = WidgetTree->ConstructWidget<USizeBox>(
		USizeBox::StaticClass(),
		TEXT("PriceFooterSize"));
	FooterSize->SetWidthOverride(PriceFooterWidth);
	FooterSize->SetHeightOverride(PriceFooterHeight);
	UBorder* Footer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PriceFooter"));
	Footer->SetVisibility(ESlateVisibility::HitTestInvisible);
	Footer->SetBrushColor(FLinearColor(0.03f, 0.025f, 0.02f, 0.92f));
	Footer->SetPadding(FMargin(8.0f, 5.0f));
	FooterSize->SetContent(Footer);
	UVerticalBox* FooterTextLayout = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("FooterTextLayout"));
	Footer->SetContent(FooterTextLayout);
	PriceText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("PriceText"));
	PriceText->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo PriceFont = PriceText->GetFont();
		PriceFont.Size = 18;
		PriceText->SetFont(PriceFont);
	}
	StatusText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StatusText"));
	StatusText->SetJustification(ETextJustify::Center);
	{
		FSlateFontInfo StatusFont = StatusText->GetFont();
		StatusFont.Size = 13;
		StatusText->SetFont(StatusFont);
	}
	FooterTextLayout->AddChildToVerticalBox(PriceText);
	FooterTextLayout->AddChildToVerticalBox(StatusText);
	if (UVerticalBoxSlot* FooterSlot = Layout->AddChildToVerticalBox(FooterSize))
	{
		FooterSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		FooterSlot->SetHorizontalAlignment(HAlign_Center);
	}
	USpacer* BottomPadding = WidgetTree->ConstructWidget<USpacer>(
		USpacer::StaticClass(),
		TEXT("BottomPadding"));
	BottomPadding->SetSize(FVector2D(1.0f, 8.0f));
	Layout->AddChildToVerticalBox(BottomPadding);
}

void UWacomWorldShopCardWidget::RefreshView()
{
	if (CardView)
	{
		CardView->SetCardViewData(OfferView.CardViewData);
	}
	if (PriceText)
	{
		PriceText->SetText(OfferView.PriceText);
		PriceText->SetColorAndOpacity(OfferView.bCanPurchase
			? FSlateColor(FLinearColor::White)
			: FSlateColor(FLinearColor(0.65f, 0.65f, 0.65f, 1.0f)));
	}
	if (StatusText)
	{
		StatusText->SetText(OfferView.StatusText);
		StatusText->SetVisibility(OfferView.StatusText.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}
