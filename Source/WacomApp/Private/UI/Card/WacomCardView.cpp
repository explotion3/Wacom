// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardView.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/RetainerBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "PaperSprite.h"
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

	void SetDigitImageBrush(UImage& DigitImage, UPaperSprite& Sprite, const FVector2D& DesiredSize)
	{
		FSlateBrush DigitBrush = DigitImage.GetBrush();
		DigitBrush.SetResourceObject(&Sprite);
		DigitBrush.SetImageSize(FVector2f(
			FMath::Max(1.0f, DesiredSize.X),
			FMath::Max(1.0f, DesiredSize.Y)));
		DigitImage.SetBrush(DigitBrush);
	}

	bool AreTextViewsEquivalent(const FText& A, const FText& B)
	{
		return A.EqualTo(B);
	}

	bool AreEffectBadgesEquivalent(
		const TArray<FWacomCardViewEffectBadge>& A,
		const TArray<FWacomCardViewEffectBadge>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].Kind != B[Index].Kind
				|| A[Index].Value != B[Index].Value
				|| !AreTextViewsEquivalent(A[Index].DisplayText, B[Index].DisplayText))
			{
				return false;
			}
		}
		return true;
	}

	bool AreCardViewDataEquivalent(const FWacomCardViewData& A, const FWacomCardViewData& B)
	{
		return AreTextViewsEquivalent(A.Name, B.Name)
			&& AreTextViewsEquivalent(A.TypeText, B.TypeText)
			&& AreTextViewsEquivalent(A.Description, B.Description)
			&& A.Cost == B.Cost
			&& A.bShowCost == B.bShowCost
			&& A.Rarity == B.Rarity
			&& A.Value == B.Value
			&& A.bShowValue == B.bShowValue
			&& AreTextViewsEquivalent(A.PhysiqueText, B.PhysiqueText)
			&& A.bShowPhysique == B.bShowPhysique
			&& AreEffectBadgesEquivalent(A.EffectBadges, B.EffectBadges)
			&& A.bDisabled == B.bDisabled
			&& A.Durability == B.Durability
			&& A.bShowDurability == B.bShowDurability
			&& A.Art == B.Art;
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

		CostDigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("CostDigitImage"));
		CostDigitImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UVerticalBoxSlot* DigitSlot = Content->AddChildToVerticalBox(CostDigitImage))
		{
			DigitSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 2.f));
		}

		UHorizontalBox* HeaderStats = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HeaderStats"));
		if (UVerticalBoxSlot* HeaderStatsSlot = Content->AddChildToVerticalBox(HeaderStats))
		{
			HeaderStatsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		ValueText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ValueText"));
		ValueText->SetColorAndOpacity(FSlateColor(FLinearColor(0.75f, 0.9f, 1.f, 1.f)));
		HeaderStats->AddChild(ValueText);

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

		RarityBorder = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("RarityBorder"));
		RarityBorder->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* RaritySlot = Stack->AddChildToOverlay(RarityBorder))
		{
			RaritySlot->SetHorizontalAlignment(HAlign_Fill);
			RaritySlot->SetVerticalAlignment(VAlign_Fill);
		}

		DurabilityHost = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("DurabilityHost"));
		DurabilityHost->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* DurSlot = Stack->AddChildToOverlay(DurabilityHost))
		{
			DurSlot->SetHorizontalAlignment(HAlign_Right);
			DurSlot->SetVerticalAlignment(VAlign_Top);
		}
		{
			UImage* DurabilityBg = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DurabilityBg"));
			DurabilityBg->SetColorAndOpacity(FLinearColor(0.08f, 0.08f, 0.08f, 0.70f));
			if (UOverlaySlot* BgSlot = Cast<UOverlay>(DurabilityHost)->AddChildToOverlay(DurabilityBg))
			{
				BgSlot->SetHorizontalAlignment(HAlign_Fill);
				BgSlot->SetVerticalAlignment(VAlign_Fill);
			}
			DurabilityDigitsHost = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("DurabilityDigitsHost"));
			if (UOverlaySlot* DigitsSlot = Cast<UOverlay>(DurabilityHost)->AddChildToOverlay(DurabilityDigitsHost))
			{
				DigitsSlot->SetHorizontalAlignment(HAlign_Center);
				DigitsSlot->SetVerticalAlignment(VAlign_Center);
			}
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

FVector2D UWacomCardView::GetCardBodyHitSize() const
{
	if (bUseFixedCardBodyHitSize
		&& FixedCardBodyHitSize.X > 1.0f
		&& FixedCardBodyHitSize.Y > 1.0f)
	{
		return FixedCardBodyHitSize;
	}

	if (CardSizeBox)
	{
		const FVector2D CachedSize = CardSizeBox->GetCachedGeometry().GetLocalSize();
		if (CachedSize.X > 1.0f && CachedSize.Y > 1.0f)
		{
			return CachedSize;
		}
	}

	return FVector2D(DefaultCardBodyHitWidth, DefaultCardBodyHitHeight);
}

bool UWacomCardView::HasCardBodyHitGeometry() const
{
	if (!CardSizeBox)
	{
		return false;
	}

	const FVector2D CachedSize = CardSizeBox->GetCachedGeometry().GetLocalSize();
	return CachedSize.X > 1.0f && CachedSize.Y > 1.0f;
}

bool UWacomCardView::IsScreenPositionInsideCardBody(const FVector2D& ScreenPosition) const
{
	if (!CardSizeBox)
	{
		return false;
	}

	const FGeometry& BoundsGeometry = CardSizeBox->GetCachedGeometry();
	const FVector2D BoundsSize = BoundsGeometry.GetLocalSize();
	if (BoundsSize.X <= 1.0f || BoundsSize.Y <= 1.0f)
	{
		return false;
	}
	return IsLocalPositionInsideCardBodyBounds(
		BoundsGeometry.AbsoluteToLocal(ScreenPosition),
		BoundsSize);
}

bool UWacomCardView::IsLocalPositionInsideCardBodyBounds(
	const FVector2D& LocalPosition,
	const FVector2D& CardSizeBoxLocalSize) const
{
	if (CardSizeBoxLocalSize.X <= 1.0f || CardSizeBoxLocalSize.Y <= 1.0f)
	{
		return false;
	}

	const FVector2D BodySize = GetCardBodyHitSize();
	if (BodySize.X <= 1.0f || BodySize.Y <= 1.0f)
	{
		return false;
	}

	const FVector2D HitMin = (CardSizeBoxLocalSize - BodySize) * 0.5f;
	const FVector2D HitMax = HitMin + BodySize;
	return LocalPosition.X >= HitMin.X
		&& LocalPosition.Y >= HitMin.Y
		&& LocalPosition.X <= HitMax.X
		&& LocalPosition.Y <= HitMax.Y;
}

#if WITH_AUTOMATION_TESTS
bool UWacomCardView::IsLocalPositionInsideCardBodyWithBoundsForTest(
	const FVector2D& LocalPosition,
	const FVector2D& SimulatedCardSizeBoxLocalSize) const
{
	return IsLocalPositionInsideCardBodyBounds(LocalPosition, SimulatedCardSizeBoxLocalSize);
}
#endif

void UWacomCardView::NativeConstruct()
{
	Super::NativeConstruct();
	EnsureSurfaceFoilOverlay();
	ApplySurfaceFoilOverlay();
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::SetCardViewData(const FWacomCardViewData& InData)
{
	if (bCardViewDataAppliedToWidgets && AreCardViewDataEquivalent(CurrentData, InData))
	{
		return;
	}

	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::ApplyCurrentDataToWidgets()
{
	EnsureSpriteIconCachesBuilt();
	UpdateCostDisplay();
	UpdateDurabilityDisplay();

	SetOptionalText(ValueText, CurrentData.bShowValue ? FText::AsNumber(CurrentData.Value) : FText::GetEmpty());
	SetOptionalText(NameText, CurrentData.Name);
	SetOptionalText(TypeText, CurrentData.TypeText);
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
			if (UPaperSprite* Sprite = ResolvedRarityBorderSprites.FindRef(CurrentData.Rarity))
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

	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(CurrentData.bDisabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	bCardViewDataAppliedToWidgets = true;
	InvalidateCardViewRenderCache();
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
	EnsureSpriteIconCachesBuilt();

	if (CostDigitImage)
	{
		CostDigitImage->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (!CurrentData.bShowCost || !CostDigitImage || ResolvedCostDigitIcons.IsEmpty())
	{
		return;
	}

	const TArray<int32> Digits = SplitIntoDigits(CurrentData.Cost);
	if (Digits.Num() == 0)
	{
		return;
	}

	if (Digits.Num() == 1)
	{
		if (UPaperSprite* Sprite = ResolvedCostDigitIcons.FindRef(Digits[0]))
		{
			SetDigitImageBrush(*CostDigitImage, *Sprite, CostDigitSize);
			CostDigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
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

void UWacomCardView::UpdateDurabilityDisplay()
{
	EnsureSpriteIconCachesBuilt();

	if (!DurabilityHost)
	{
		return;
	}

	if (DurabilityDigitsHost)
	{
		DurabilityDigitsHost->ClearChildren();
		DurabilityDigitsHost->SetVisibility(ESlateVisibility::Collapsed);
	}

	const bool bCanBuildIconDigits = CurrentData.bShowDurability
		&& CurrentData.Durability > 0
		&& DurabilityDigitsHost
		&& !ResolvedDurabilityDigitIcons.IsEmpty()
		&& WidgetTree;

	if (bCanBuildIconDigits)
	{
		const TArray<int32> Digits = SplitIntoDigits(CurrentData.Durability);
		for (int32 Digit : Digits)
		{
			UPaperSprite* Sprite = ResolvedDurabilityDigitIcons.FindRef(Digit);
			if (!Sprite)
			{
				DurabilityDigitsHost->ClearChildren();
				break;
			}

			UImage* DigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
			SetDigitImageBrush(*DigitImage, *Sprite, DurabilityDigitSize);
			DurabilityDigitsHost->AddChild(DigitImage);
		}
	}

	const bool bHasDurabilityDigits = DurabilityDigitsHost && DurabilityDigitsHost->GetChildrenCount() > 0;
	if (DurabilityDigitsHost)
	{
		DurabilityDigitsHost->SetVisibility(
			bHasDurabilityDigits ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	DurabilityHost->SetVisibility(
		bHasDurabilityDigits ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

void UWacomCardView::EnsureSpriteIconCachesBuilt()
{
	if (bSpriteIconCachesBuilt)
	{
		return;
	}

	RebuildSpriteIconCaches();
}

void UWacomCardView::RebuildSpriteIconCaches()
{
	ResolvedCostDigitIcons.Reset();
	ResolvedDurabilityDigitIcons.Reset();
	ResolvedRarityBorderSprites.Reset();

	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : CostDigitIcons)
	{
		if (!Pair.Value.IsNull())
		{
			if (UPaperSprite* Sprite = Pair.Value.LoadSynchronous())
			{
				ResolvedCostDigitIcons.Add(Pair.Key, Sprite);
			}
		}
	}

	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : DurabilityDigitIcons)
	{
		if (!Pair.Value.IsNull())
		{
			if (UPaperSprite* Sprite = Pair.Value.LoadSynchronous())
			{
				ResolvedDurabilityDigitIcons.Add(Pair.Key, Sprite);
			}
		}
	}

	for (const TPair<FGameplayTag, TSoftObjectPtr<UPaperSprite>>& Pair : RarityBorderSprites)
	{
		if (Pair.Key.IsValid() && !Pair.Value.IsNull())
		{
			if (UPaperSprite* Sprite = Pair.Value.LoadSynchronous())
			{
				ResolvedRarityBorderSprites.Add(Pair.Key, Sprite);
			}
		}
	}

	bSpriteIconCachesBuilt = true;
}

void UWacomCardView::InvalidateCardViewRenderCache()
{
#if WITH_AUTOMATION_TESTS
	++RenderCacheInvalidationCountForTest;
	LastRetainerRenderRequestCountForTest = 0;
#endif

	InvalidateLayoutAndVolatility();

	if (!WidgetTree)
	{
		return;
	}

	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		if (URetainerBox* RetainerBox = Cast<URetainerBox>(Widget))
		{
			RetainerBox->RequestRender();
#if WITH_AUTOMATION_TESTS
			++LastRetainerRenderRequestCountForTest;
#endif
		}
	});
}

#undef LOCTEXT_NAMESPACE
