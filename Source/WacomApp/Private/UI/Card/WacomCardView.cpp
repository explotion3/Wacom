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
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"
#include "PaperSprite.h"
#include "UObject/ConstructorHelpers.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardSemanticTextHitLayout.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Card/WacomPaperSpriteAtlasUtils.h"

#define LOCTEXT_NAMESPACE "WacomCardView"

namespace
{
	constexpr float DefaultCardWidth = 260.f;
	constexpr float DefaultCardHeight = 380.f;
	const FName ArtTextureParameterName(TEXT("ArtTexture"));
	const FName ArtDepthTextureParameterName(TEXT("ArtDepthTexture"));
	const FName ArtDepthEnabledParameterName(TEXT("ArtDepthEnabled"));
	const FName FrameTextureParameterName(TEXT("FrameTexture"));
	const FName RarityTextureParameterName(TEXT("RarityTexture"));
	const FName BackColorParameterName(TEXT("BackColor"));
	const FName RarityUVScaleBiasParameterName(TEXT("RarityUVScaleBias"));
	const FName RarityEnabledParameterName(TEXT("RarityEnabled"));
	const FName CardSourceInvSizeParameterName(TEXT("CardSourceInvSize"));
	const FName SurfaceTiltXParameterName(TEXT("TiltX"));
	const FName SurfaceTiltYParameterName(TEXT("TiltY"));
	const FName SurfaceParallaxStrengthParameterName(TEXT("ParallaxStrength"));
	const FName OldDigitTextureParameterName(TEXT("OldDigitTexture"));
	const FName NewDigitTextureParameterName(TEXT("NewDigitTexture"));
	const FName OldDigitUVRectParameterName(TEXT("OldDigitUVRect"));
	const FName NewDigitUVRectParameterName(TEXT("NewDigitUVRect"));
	const FName CostRewriteOldDissolveParameterName(TEXT("CostRewriteOldDissolve"));
	const FName CostRewriteNewRevealParameterName(TEXT("CostRewriteNewReveal"));
	const FName CostRewriteToneParameterName(TEXT("CostRewriteTone"));
	const FName CostRewriteSeedParameterName(TEXT("CostRewriteSeed"));
	const FName CostRewriteReducedMotionParameterName(TEXT("CostRewriteReducedMotion"));
	const FName DigitEffectModeParameterName(TEXT("DigitEffectMode"));
	const FName CostPreviewAmountParameterName(TEXT("CostPreviewAmount"));
	const FName CostPreviewPulseParameterName(TEXT("CostPreviewPulse"));
	const FName CostPreviewMinimumOpacityParameterName(TEXT("CostPreviewMinimumOpacity"));
	const FName CostPreviewMaximumOpacityParameterName(TEXT("CostPreviewMaximumOpacity"));
	const FName CostPreviewPeakBrightnessParameterName(TEXT("CostPreviewPeakBrightness"));

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

	bool AreCardViewTextViewsEquivalent(const FText& A, const FText& B)
	{
		return A.EqualTo(B);
	}

	bool AreCardViewEffectBadgesEquivalent(
		const TArray<FWacomCardViewEffectBadge>& A,
		const TArray<FWacomCardViewEffectBadge>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].PresentationKey != B[Index].PresentationKey
				|| A[Index].Kind != B[Index].Kind
				|| A[Index].Value != B[Index].Value
				|| A[Index].bHasPreviewValue != B[Index].bHasPreviewValue
				|| A[Index].PreviewValue != B[Index].PreviewValue
				|| A[Index].bPreviewSkipped != B[Index].bPreviewSkipped
				|| A[Index].ValueEmphasis != B[Index].ValueEmphasis
				|| !AreCardViewTextViewsEquivalent(A[Index].DisplayText, B[Index].DisplayText))
			{
				return false;
			}
		}
		return true;
	}

	bool AreCardFaceSemanticTokensEquivalent(
		const TArray<FWacomCardFaceSemanticTokenView>& A,
		const TArray<FWacomCardFaceSemanticTokenView>& B)
	{
		if (A.Num() != B.Num())
		{
			return false;
		}
		for (int32 Index = 0; Index < A.Num(); ++Index)
		{
			if (A[Index].SemanticId != B[Index].SemanticId
				|| A[Index].SourceTag != B[Index].SourceTag
				|| !AreCardViewTextViewsEquivalent(
					A[Index].DisplayText,
					B[Index].DisplayText)
				|| A[Index].StartIndex != B[Index].StartIndex
				|| A[Index].Length != B[Index].Length)
			{
				return false;
			}
		}
		return true;
	}

	bool AreCardViewDataFieldsEquivalent(const FWacomCardViewData& A, const FWacomCardViewData& B)
	{
		return AreCardViewTextViewsEquivalent(A.Name, B.Name)
			&& AreCardViewTextViewsEquivalent(A.TypeText, B.TypeText)
			&& AreCardFaceSemanticTokensEquivalent(
				A.TypeSemanticTokens,
				B.TypeSemanticTokens)
			&& AreCardViewTextViewsEquivalent(A.Description, B.Description)
			&& A.Cost == B.Cost
			&& A.bShowCost == B.bShowCost
			&& A.bHasCostPreview == B.bHasCostPreview
			&& A.PreviewCost == B.PreviewCost
			&& A.Rarity == B.Rarity
			&& A.Value == B.Value
			&& A.bShowValue == B.bShowValue
			&& AreCardViewTextViewsEquivalent(A.PhysiqueText, B.PhysiqueText)
			&& A.bShowPhysique == B.bShowPhysique
			&& AreCardViewEffectBadgesEquivalent(A.EffectBadges, B.EffectBadges)
			&& A.bDisabled == B.bDisabled
			&& A.Durability == B.Durability
			&& A.bShowDurability == B.bShowDurability
			&& A.Art == B.Art
			&& A.ArtDepthMap == B.ArtDepthMap;
	}

	bool AreTextDisplayFieldsEquivalent(const FWacomCardViewData& A, const FWacomCardViewData& B)
	{
		return AreCardViewTextViewsEquivalent(A.Name, B.Name)
			&& AreCardViewTextViewsEquivalent(A.TypeText, B.TypeText)
			&& AreCardFaceSemanticTokensEquivalent(
				A.TypeSemanticTokens,
				B.TypeSemanticTokens)
			&& A.Value == B.Value
			&& A.bShowValue == B.bShowValue;
	}

	bool AreCostDisplayFieldsEquivalent(const FWacomCardViewData& A, const FWacomCardViewData& B)
	{
		return A.Cost == B.Cost
			&& A.bShowCost == B.bShowCost
			&& A.bHasCostPreview == B.bHasCostPreview
			&& A.PreviewCost == B.PreviewCost;
	}

	bool AreDurabilityDisplayFieldsEquivalent(const FWacomCardViewData& A, const FWacomCardViewData& B)
	{
		return A.Durability == B.Durability
			&& A.bShowDurability == B.bShowDurability;
	}

	UWacomCardEffectBadgeWidget* FindReusableBadgeWidget(UPanelWidget& Host)
	{
		for (int32 ChildIndex = 0; ChildIndex < Host.GetChildrenCount(); ++ChildIndex)
		{
			if (UWacomCardEffectBadgeWidget* BadgeWidget =
				Cast<UWacomCardEffectBadgeWidget>(Host.GetChildAt(ChildIndex)))
			{
				return BadgeWidget;
			}
		}
		return nullptr;
	}

	UWacomCardEffectBadgeWidget* EnsureBadgeWidget(
		UWacomCardView& Owner,
		UPanelWidget& Host,
		UClass& BadgeClass)
	{
		if (UWacomCardEffectBadgeWidget* ExistingWidget = FindReusableBadgeWidget(Host))
		{
			return ExistingWidget;
		}

		UWacomCardEffectBadgeWidget* BadgeWidget =
			CreateWidget<UWacomCardEffectBadgeWidget>(&Owner, &BadgeClass);
		if (!BadgeWidget)
		{
			return nullptr;
		}

		Host.AddChild(BadgeWidget);
		return BadgeWidget;
	}

	bool ShouldRenderCardFaceEffectBadge(EWacomCardViewEffectBadgeKind Kind)
	{
		switch (Kind)
		{
		case EWacomCardViewEffectBadgeKind::Damage:
		case EWacomCardViewEffectBadgeKind::Poison:
		case EWacomCardViewEffectBadgeKind::Burn:
		case EWacomCardViewEffectBadgeKind::Heal:
		case EWacomCardViewEffectBadgeKind::Shield:
			return true;
		default:
			return false;
		}
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

void UWacomCardView::AppendPresentationSoftObjectPaths(
	TArray<FSoftObjectPath>& OutPaths) const
{
	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : CostDigitIcons)
	{
		if (!Pair.Value.IsNull())
		{
			OutPaths.Add(Pair.Value.ToSoftObjectPath());
		}
	}
	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : DurabilityDigitIcons)
	{
		if (!Pair.Value.IsNull())
		{
			OutPaths.Add(Pair.Value.ToSoftObjectPath());
		}
	}
	for (const TPair<FGameplayTag, TSoftObjectPtr<UPaperSprite>>& Pair : RarityBorderSprites)
	{
		if (!Pair.Value.IsNull())
		{
			OutPaths.Add(Pair.Value.ToSoftObjectPath());
		}
	}
	if (const UClass* BadgeClass = EffectBadgeWidgetClass.Get())
	{
		if (const UWacomCardEffectBadgeWidget* BadgeDefault =
			BadgeClass->GetDefaultObject<UWacomCardEffectBadgeWidget>())
		{
			BadgeDefault->AppendPresentationSoftObjectPaths(OutPaths);
		}
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
			DurabilityBackIcon = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("DurabilityBackIcon"));
			DurabilityBackIcon->SetColorAndOpacity(FLinearColor(0.08f, 0.08f, 0.08f, 0.70f));
			if (UOverlaySlot* BgSlot = Cast<UOverlay>(DurabilityHost)->AddChildToOverlay(DurabilityBackIcon))
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

bool UWacomCardView::TryResolveTypeSemanticTokenAtLocalPosition(
	const FVector2D& CardLocalPosition,
	FWacomCardFaceSemanticTokenView& OutToken) const
{
	if (!TypeText
		|| CurrentData.TypeText.IsEmpty()
		|| CurrentData.TypeSemanticTokens.IsEmpty()
		|| TypeText->GetVisibility() == ESlateVisibility::Collapsed)
	{
		return false;
	}

	const FGeometry& CardGeometry = GetCachedGeometry();
	const FGeometry& TypeGeometry = TypeText->GetCachedGeometry();
	const FVector2D TypeSize = TypeGeometry.GetLocalSize();
	if (CardGeometry.GetLocalSize().IsNearlyZero()
		|| TypeSize.IsNearlyZero())
	{
		return false;
	}

	const FVector2D AbsolutePosition =
		CardGeometry.LocalToAbsolute(CardLocalPosition);
	const FVector2D TypeLocalPosition =
		TypeGeometry.AbsoluteToLocal(AbsolutePosition);
	return WacomCardSemanticTextHitLayout::ResolveTokenAtLocalPosition(
		CurrentData.TypeText.ToString(),
		CurrentData.TypeSemanticTokens,
		TypeText->GetFont(),
		TypeSize,
		ETextJustify::Center,
		TypeLocalPosition,
		OutToken);
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

#if WITH_AUTOMATION_TESTS
FWacomCardViewAutomationTestView UWacomCardView::GetAutomationTestViewForTest() const
{
	FWacomCardViewAutomationTestView View;
	View.bSurfaceFoilEnabled = bSurfaceFoilEnabled;
	View.bHasSurfaceFoilOverlay = SurfaceFoilOverlay != nullptr;
	View.bSurfaceFoilVisible = SurfaceFoilOverlay
		&& SurfaceFoilOverlay->GetVisibility() != ESlateVisibility::Collapsed;
	View.bSurfaceFoilBrushConfigured = SurfaceFoilOverlay
		&& SurfaceFoilOverlay->GetBrush().GetResourceObject() != nullptr;
	View.RenderCacheInvalidationCount = RenderCacheInvalidationCountForTest;
	View.LastRetainerRenderRequestCount = LastRetainerRenderRequestCountForTest;
	View.TextDisplayUpdateCount = TextDisplayUpdateCountForTest;
	View.CostDisplayUpdateCount = CostDisplayUpdateCountForTest;
	View.DurabilityDisplayUpdateCount = DurabilityDisplayUpdateCountForTest;
	View.RarityDisplayUpdateCount = RarityDisplayUpdateCountForTest;
	View.ArtDisplayUpdateCount = ArtDisplayUpdateCountForTest;
	View.DisabledDisplayUpdateCount = DisabledDisplayUpdateCountForTest;
	View.EffectBadgeDisplayUpdateCount = EffectBadgeDisplayUpdateCountForTest;
	View.bSurfaceCompositeActive = bCardSurfaceCompositeActive;
	View.bHasCardOverlay = CardOverlay != nullptr;
	View.bHasCardSurfaceImage = CardSurfaceImage != nullptr;
	View.bHasCardSurfaceMaterial = CardSurfaceMaterial != nullptr;
	View.AppliedAttachmentOffsetPixels = AppliedAttachmentOffsetPixels;
	View.SurfacePerspectiveView = CardSurfacePerspectiveView;
	View.bHasDurabilityShadowImage = DurabilityShadowImage != nullptr;
	View.bDurabilityShadowVisible = DurabilityShadowImage
		&& DurabilityShadowImage->GetVisibility() != ESlateVisibility::Collapsed;
	View.DurabilityShadowOffsetPixels = DurabilityShadowImage
		? DurabilityShadowImage->GetRenderTransform().Translation
		: FVector2D::ZeroVector;
	View.DurabilityShadowColor = DurabilityShadowImage
		? DurabilityShadowImage->GetColorAndOpacity()
		: FLinearColor::Transparent;
	View.ResolvedSurfaceArt = ResolveCardSurfaceArtTexture();
	View.bCostDigitRewritePrepared = bCostDigitRewritePrepared;
	View.bCostDigitRewriteMaterialActive = bCostDigitRewriteMaterialActive;
	View.bCostDigitRewriteMaterialCached = CostDigitRewriteMaterialInstance != nullptr;
	View.CostDigitRewriteMaterialCreateCount = CostDigitRewriteMaterialCreateCountForTest;
	View.SpriteSynchronousFallbackCount = SpriteSynchronousFallbackCountForTest;
	View.bCostDigitPreviewMaterialActive = bCostDigitPreviewMaterialActive;
	View.CostDigitRewriteOldSprite = CostDigitRewriteOldSprite;
	View.CostDigitRewriteNewSprite = CostDigitRewriteNewSprite;
	View.CostDigitRewriteRenderTransform = CostDigitImage
		? CostDigitImage->GetRenderTransform()
		: FWidgetTransform();
	return View;
}
#endif

void UWacomCardView::NativeConstruct()
{
	Super::NativeConstruct();
	CacheAuthoredCardArtTexture();
	EnsureCardSurfaceImage();
	EnsureDurabilityShadowImage();
	CacheLegacySurfaceVisibility();
	CacheAttachmentAuthoredTransforms();
	CacheCostDigitAuthoredTransform();
	ApplySurfaceFoilOverlay();
	bCardViewDataAppliedToWidgets = false;
	ApplyCurrentDataToWidgets();
	ApplyCardSurfacePerspective();
}

void UWacomCardView::EnsureCardSurfaceImage()
{
	if (CardSurfaceImage || !WidgetTree || !CardOverlay)
	{
		return;
	}
	CardSurfaceImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("CardSurfaceImage_Runtime"));
	if (!CardSurfaceImage)
	{
		return;
	}
	CardSurfaceImage->SetVisibility(ESlateVisibility::Collapsed);
	if (UPanelSlot* SurfaceSlot = CardOverlay->InsertChildAt(0, CardSurfaceImage))
	{
		if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(SurfaceSlot))
		{
			OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
			OverlaySlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
}

void UWacomCardView::NativeDestruct()
{
	ResetEffectBadgeFeedback();
	ResetCostDigitRewrite();
	CostDigitRewriteMaterialInstance = nullptr;
	CostDigitRewriteMaterialSource = nullptr;
	ResetCardSurfacePerspectiveView();
	RestoreAttachmentAuthoredTransforms();
	ResetAttachmentCastShadowView();
	bCardSurfaceCompositeActive = false;
	SetLegacySurfaceVisibility(true);
	if (CardSurfaceImage)
	{
		CardSurfaceImage->SetVisibility(ESlateVisibility::Collapsed);
	}
	CardSurfaceMaterialInstance = nullptr;
	Super::NativeDestruct();
}

void UWacomCardView::SetCardSurfacePerspectiveView(
	const FWacomCardSurfacePerspectiveView& InView)
{
	CardSurfacePerspectiveView = InView;
	CardSurfacePerspectiveView.Strength = FMath::Max(0.0f, InView.Strength);
	if (!CardSurfacePerspectiveView.bEnabled || CardSurfacePerspectiveView.bReducedMotion)
	{
		CardSurfacePerspectiveView.TiltDegrees = FVector2D::ZeroVector;
		CardSurfacePerspectiveView.AttachmentOffsetPixels = FVector2D::ZeroVector;
		CardSurfacePerspectiveView.Strength = 0.0f;
	}
	ApplyCardSurfacePerspective();
}

void UWacomCardView::ResetCardSurfacePerspectiveView()
{
	CardSurfacePerspectiveView = FWacomCardSurfacePerspectiveView();
	ApplyCardSurfacePerspective();
}

void UWacomCardView::SetSurfaceFoilEnabled(bool bEnabled)
{
	bSurfaceFoilEnabled = bEnabled;
	ApplySurfaceFoilOverlay();
	InvalidateCardViewRenderCache();
}

bool UWacomCardView::PrepareCostDigitRewrite(const FWacomCardViewData& InNewData)
{
	return PrepareCostDigitRewrite(CurrentData, InNewData);
}

bool UWacomCardView::PrepareCostDigitRewrite(
	const FWacomCardViewData& InOldData,
	const FWacomCardViewData& InNewData)
{
	EnsureSpriteIconCachesBuilt();
	if (!CostDigitImage)
	{
		return false;
	}

	UPaperSprite* OldSprite = ResolveSingleCostDigitSprite(InOldData);
	UPaperSprite* NewSprite = ResolveSingleCostDigitSprite(InNewData);
	if (!OldSprite || !NewSprite || OldSprite == NewSprite || InOldData.Cost == InNewData.Cost)
	{
		return false;
	}

	CacheCostDigitAuthoredTransform();
	RestoreCostDigitAuthoredTransform();
	CostDigitRewriteOldSprite = OldSprite;
	CostDigitRewriteNewSprite = NewSprite;
	bCostDigitRewritePrepared = true;
	bCostDigitRewriteMaterialActive = false;
	SetDigitImageBrush(*CostDigitImage, *OldSprite, CostDigitSize);
	CostDigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void UWacomCardView::SetCostDigitRewriteView(
	const FWacomFirstPersonCardDataRewriteView& InView)
{
	if (!InView.bActive)
	{
		return;
	}
	if (!EnsureCostDigitRewriteMaterial(InView))
	{
		ResetCostDigitRewrite();
		return;
	}
	ApplyCostDigitRewriteMaterialParameters(InView);
}

void UWacomCardView::SetCostDigitPreviewView(
	const FWacomFirstPersonCardCostPreviewView& InView)
{
	if (!InView.bActive || InView.PreviewAmount <= KINDA_SMALL_NUMBER)
	{
		ResetCostDigitPreview();
		return;
	}
	if (!EnsureCostDigitPreviewMaterial(InView))
	{
		ResetCostDigitPreview();
		return;
	}

	CostDigitRewriteMaterialInstance->SetScalarParameterValue(DigitEffectModeParameterName, 1.0f);
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteToneParameterName,
		static_cast<float>(InView.Tone));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostPreviewAmountParameterName,
		FMath::Clamp(InView.PreviewAmount, 0.0f, 1.0f));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostPreviewPulseParameterName,
		FMath::Clamp(InView.PulseAmount, 0.0f, 1.0f));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteSeedParameterName,
		static_cast<float>(InView.Seed));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostPreviewMinimumOpacityParameterName,
		FMath::Clamp(InView.Style.PreviewMinimumOpacity, 0.0f, 1.0f));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostPreviewMaximumOpacityParameterName,
		FMath::Clamp(InView.Style.PreviewMaximumOpacity, 0.0f, 1.0f));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostPreviewPeakBrightnessParameterName,
		FMath::Max(0.0f, InView.Style.PreviewPeakBrightness));
	bCostDigitPreviewMaterialActive = true;
}

void UWacomCardView::ResetCostDigitPreview()
{
	if (!bCostDigitPreviewMaterialActive)
	{
		return;
	}
	bCostDigitPreviewMaterialActive = false;
	bCostDigitRewriteMaterialActive = false;
	bCostDigitRewritePrepared = false;
	CostDigitRewriteOldSprite = nullptr;
	CostDigitRewriteNewSprite = nullptr;
	RestoreCostDigitAuthoredTransform();
	if (CostDigitImage)
	{
		if (UPaperSprite* CurrentSprite = ResolveSingleCostDigitSprite(CurrentData))
		{
			SetDigitImageBrush(*CostDigitImage, *CurrentSprite, CostDigitSize);
			CostDigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CostDigitImage->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
}

void UWacomCardView::ResetCostDigitRewrite()
{
	RestoreCostDigitAuthoredTransform();
	bCostDigitRewriteMaterialActive = false;
	bCostDigitPreviewMaterialActive = false;
	bCostDigitRewritePrepared = false;
	CostDigitRewriteOldSprite = nullptr;
	CostDigitRewriteNewSprite = nullptr;
	if (!CostDigitImage)
	{
		return;
	}
	if (UPaperSprite* CurrentSprite = ResolveSingleCostDigitSprite(CurrentData))
	{
		SetDigitImageBrush(*CostDigitImage, *CurrentSprite, CostDigitSize);
		CostDigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		CostDigitImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWacomCardView::SetCardViewData(const FWacomCardViewData& InData)
{
	if (bCardViewDataAppliedToWidgets && AreCardViewDataFieldsEquivalent(CurrentData, InData))
	{
		return;
	}

	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

void UWacomCardView::ApplyCurrentDataToWidgets()
{
	EnsureSpriteIconCachesBuilt();

	const bool bForceFullApply = !bHasLastAppliedData || !bCardViewDataAppliedToWidgets;
	bool bUpdatedAnyDisplay = false;

	if (bForceFullApply || !AreCostDisplayFieldsEquivalent(LastAppliedData, CurrentData))
	{
		UpdateCostDisplay();
		bUpdatedAnyDisplay = true;
	}
	if (bForceFullApply || !AreDurabilityDisplayFieldsEquivalent(LastAppliedData, CurrentData))
	{
		UpdateDurabilityDisplay();
		bUpdatedAnyDisplay = true;
	}
	if (bForceFullApply || !AreTextDisplayFieldsEquivalent(LastAppliedData, CurrentData))
	{
		UpdateTextDisplays();
		bUpdatedAnyDisplay = true;
	}
	if (bForceFullApply || !AreCardViewEffectBadgesEquivalent(LastAppliedData.EffectBadges, CurrentData.EffectBadges))
	{
		UpdateEffectBadgeDisplays();
		bUpdatedAnyDisplay = true;
	}
	if (bForceFullApply
		|| LastAppliedData.Art != CurrentData.Art
		|| LastAppliedData.ArtDepthMap != CurrentData.ArtDepthMap)
	{
		UpdateArtDisplay();
		bUpdatedAnyDisplay = true;
	}
	if (bForceFullApply || LastAppliedData.Rarity != CurrentData.Rarity)
	{
		UpdateRarityBorderDisplay();
		bUpdatedAnyDisplay = true;
	}
	if (bForceFullApply || LastAppliedData.bDisabled != CurrentData.bDisabled)
	{
		UpdateDisabledDisplay();
		bUpdatedAnyDisplay = true;
	}

	bCardViewDataAppliedToWidgets = true;
	LastAppliedData = CurrentData;
	bHasLastAppliedData = true;

	if (bUpdatedAnyDisplay)
	{
		InvalidateCardViewRenderCache();
	}
}

void UWacomCardView::UpdateEffectBadgeDisplays()
{
#if WITH_AUTOMATION_TESTS
	++EffectBadgeDisplayUpdateCountForTest;
#endif

	TArray<UPanelWidget*> BadgeSlots;
	BadgeSlots.Reserve(4);
	BadgeSlots.Add(EffectBadgeSlot1);
	BadgeSlots.Add(EffectBadgeSlot2);
	BadgeSlots.Add(EffectBadgeSlot3);
	BadgeSlots.Add(EffectBadgeSlot4);

	const bool bUseFixedSlots = BadgeSlots.ContainsByPredicate([](const UPanelWidget* CandidateSlot)
	{
		return CandidateSlot != nullptr;
	});

	TArray<FWacomCardViewEffectBadge> RenderableBadges;
	RenderableBadges.Reserve(CurrentData.EffectBadges.Num());
	for (const FWacomCardViewEffectBadge& Badge : CurrentData.EffectBadges)
	{
		if (ShouldRenderCardFaceEffectBadge(Badge.Kind))
		{
			RenderableBadges.Add(Badge);
		}
	}

	UClass* BadgeClass = EffectBadgeWidgetClass
		? EffectBadgeWidgetClass.Get()
		: UWacomCardEffectBadgeWidget::StaticClass();

	if (bUseFixedSlots)
	{
		if (EffectStatsHost)
		{
			EffectStatsHost->ClearChildren();
			EffectStatsHost->SetVisibility(ESlateVisibility::Collapsed);
		}

		for (UPanelWidget* BadgeSlot : BadgeSlots)
		{
			if (!BadgeSlot)
			{
				continue;
			}

			BadgeSlot->SetVisibility(ESlateVisibility::Collapsed);
		}

		int32 BadgeIndex = 0;
		for (UPanelWidget* BadgeSlot : BadgeSlots)
		{
			if (!BadgeSlot)
			{
				continue;
			}

			if (!RenderableBadges.IsValidIndex(BadgeIndex))
			{
				break;
			}

			UWacomCardEffectBadgeWidget* BadgeWidget = EnsureBadgeWidget(*this, *BadgeSlot, *BadgeClass);
			if (!BadgeWidget)
			{
				continue;
			}

			BadgeWidget->SetEffectBadgeData(RenderableBadges[BadgeIndex]);
			if (EffectBadgeFeedbackConfig.IsSet())
			{
				BadgeWidget->SetEffectBadgeFeedbackConfig(EffectBadgeFeedbackConfig.GetValue());
			}
			BadgeSlot->SetVisibility(ESlateVisibility::HitTestInvisible);
			++BadgeIndex;
		}
		ApplyEffectBadgeFeedbackState();
		ApplyAttachmentCastShadowView();
		return;
	}

	if (EffectStatsHost)
	{
		while (EffectStatsHost->GetChildrenCount() > RenderableBadges.Num())
		{
			EffectStatsHost->RemoveChildAt(EffectStatsHost->GetChildrenCount() - 1);
		}

		for (int32 BadgeIndex = 0; BadgeIndex < RenderableBadges.Num(); ++BadgeIndex)
		{
			UWacomCardEffectBadgeWidget* BadgeWidget =
				Cast<UWacomCardEffectBadgeWidget>(EffectStatsHost->GetChildAt(BadgeIndex));
			if (!BadgeWidget)
			{
				BadgeWidget = CreateWidget<UWacomCardEffectBadgeWidget>(this, BadgeClass);
				if (BadgeWidget)
				{
					EffectStatsHost->AddChild(BadgeWidget);
				}
			}
			if (!BadgeWidget)
			{
				continue;
			}
			BadgeWidget->SetEffectBadgeData(RenderableBadges[BadgeIndex]);
			if (EffectBadgeFeedbackConfig.IsSet())
			{
				BadgeWidget->SetEffectBadgeFeedbackConfig(EffectBadgeFeedbackConfig.GetValue());
			}
		}
		EffectStatsHost->SetVisibility(RenderableBadges.Num() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	ApplyEffectBadgeFeedbackState();
	ApplyAttachmentCastShadowView();
}

void UWacomCardView::SetEffectBadgeFeedbackConfig(
	const FWacomFirstPersonCardEffectBadgeFeedbackConfig& InConfig)
{
	EffectBadgeFeedbackConfig = InConfig;
	for (UWacomCardEffectBadgeWidget* BadgeWidget : CollectEffectBadgeWidgets())
	{
		if (BadgeWidget)
		{
			BadgeWidget->SetEffectBadgeFeedbackConfig(InConfig);
		}
	}
}

void UWacomCardView::SetEffectBadgeFeedbackView(
	const FWacomFirstPersonCardEffectBadgeFeedbackView& InView)
{
	EffectBadgeFeedbackView = InView;
	ApplyEffectBadgeFeedbackState();
}

void UWacomCardView::ResetEffectBadgeFeedback()
{
	EffectBadgeFeedbackView.Reset();
	for (UWacomCardEffectBadgeWidget* BadgeWidget : CollectEffectBadgeWidgets())
	{
		if (BadgeWidget)
		{
			BadgeWidget->ResetEffectBadgeFeedback();
		}
	}
}

void UWacomCardView::PrimeCostDigitPresentationMaterial(UMaterialInterface* MaterialSource)
{
	if (!MaterialSource)
	{
		return;
	}
	if (!CostDigitRewriteMaterialInstance || CostDigitRewriteMaterialSource != MaterialSource)
	{
		CostDigitRewriteMaterialSource = MaterialSource;
		CostDigitRewriteMaterialInstance = UMaterialInstanceDynamic::Create(MaterialSource, this);
#if WITH_AUTOMATION_TESTS
		++CostDigitRewriteMaterialCreateCountForTest;
#endif
	}
}

bool UWacomCardView::IsCostDigitRewriteMaterialReady() const
{
	return bCostDigitRewriteMaterialActive
		&& CostDigitImage
		&& CostDigitRewriteMaterialInstance
		&& CostDigitImage->GetBrush().GetResourceObject() == CostDigitRewriteMaterialInstance;
}

bool UWacomCardView::IsEffectBadgeFeedbackMaterialReady() const
{
	if (!EffectBadgeFeedbackView.IsSet() || !EffectBadgeFeedbackView->bActive)
	{
		return false;
	}
	bool bFoundActiveItem = false;
	for (UWacomCardEffectBadgeWidget* BadgeWidget : CollectEffectBadgeWidgets())
	{
		if (!BadgeWidget)
		{
			continue;
		}
		const FName Key = BadgeWidget->GetEffectBadgeData().PresentationKey;
		const FWacomFirstPersonCardEffectBadgeFeedbackItemView* Item =
			EffectBadgeFeedbackView->Items.FindByPredicate([Key](
				const FWacomFirstPersonCardEffectBadgeFeedbackItemView& Candidate)
			{
				return Candidate.PresentationKey == Key
					&& (Candidate.bActive || Candidate.bPrepareMaterial);
			});
		if (!Item)
		{
			continue;
		}
		bFoundActiveItem = true;
		if (!BadgeWidget->IsEffectBadgeFeedbackMaterialReady())
		{
			return false;
		}
	}
	return bFoundActiveItem;
}

TArray<UWacomCardEffectBadgeWidget*> UWacomCardView::CollectEffectBadgeWidgets() const
{
	TArray<UWacomCardEffectBadgeWidget*> Result;
	const TArray<UPanelWidget*> Hosts = {
		EffectBadgeSlot1.Get(),
		EffectBadgeSlot2.Get(),
		EffectBadgeSlot3.Get(),
		EffectBadgeSlot4.Get()};
	for (UPanelWidget* Host : Hosts)
	{
		if (Host)
		{
			if (UWacomCardEffectBadgeWidget* Widget = FindReusableBadgeWidget(*Host))
			{
				Result.Add(Widget);
			}
		}
	}
	if (EffectStatsHost)
	{
		for (int32 Index = 0; Index < EffectStatsHost->GetChildrenCount(); ++Index)
		{
			if (UWacomCardEffectBadgeWidget* Widget =
				Cast<UWacomCardEffectBadgeWidget>(EffectStatsHost->GetChildAt(Index)))
			{
				Result.AddUnique(Widget);
			}
		}
	}
	return Result;
}

void UWacomCardView::ApplyEffectBadgeFeedbackState()
{
	const TArray<UWacomCardEffectBadgeWidget*> BadgeWidgets = CollectEffectBadgeWidgets();
	if (!EffectBadgeFeedbackView.IsSet() || !EffectBadgeFeedbackView->bActive)
	{
		for (UWacomCardEffectBadgeWidget* BadgeWidget : BadgeWidgets)
		{
			if (BadgeWidget)
			{
				BadgeWidget->ResetEffectBadgeFeedback();
			}
		}
		return;
	}

	for (UWacomCardEffectBadgeWidget* BadgeWidget : BadgeWidgets)
	{
		if (!BadgeWidget)
		{
			continue;
		}
		const FName Key = BadgeWidget->GetEffectBadgeData().PresentationKey;
		const FWacomFirstPersonCardEffectBadgeFeedbackItemView* Item =
			EffectBadgeFeedbackView->Items.FindByPredicate([Key](
				const FWacomFirstPersonCardEffectBadgeFeedbackItemView& Candidate)
			{
				return Candidate.PresentationKey == Key;
			});
		if (Item)
		{
			BadgeWidget->SetEffectBadgeFeedbackView(*Item);
		}
		else
		{
			// A new deterministic bundle replaces the previous one. A badge that is
			// absent from the active bundle must not retain an old digit MID or root
			// transform when another badge starts changing.
			BadgeWidget->ResetEffectBadgeFeedback();
		}
	}
}

void UWacomCardView::ApplySurfaceFoilOverlay()
{
	if (!SurfaceFoilOverlay)
	{
		return;
	}
	if (!bSurfaceFoilEnabled)
	{
		FSlateBrush Brush = SurfaceFoilOverlay->GetBrush();
		Brush.SetResourceObject(nullptr);
		SurfaceFoilOverlay->SetBrush(Brush);
		SurfaceFoilOverlay->SetVisibility(ESlateVisibility::Collapsed);
		return;
	}

	if (SurfaceFoilMaterial)
	{
		SurfaceFoilOverlay->SetBrushFromMaterial(SurfaceFoilMaterial);
		SurfaceFoilOverlay->SetColorAndOpacity(FLinearColor::White);
	}

	const bool bHasFoilBrush = SurfaceFoilOverlay->GetBrush().GetResourceObject() != nullptr;
	SurfaceFoilOverlay->SetVisibility(bHasFoilBrush ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	if (bCardSurfaceCompositeActive)
	{
		SurfaceFoilOverlay->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWacomCardView::UpdateCostDisplay()
{
#if WITH_AUTOMATION_TESTS
	++CostDisplayUpdateCountForTest;
#endif

	EnsureSpriteIconCachesBuilt();
	if (bCostDigitRewritePrepared && CostDigitRewriteOldSprite && CostDigitRewriteNewSprite)
	{
		UPaperSprite* RequestedSprite = ResolveSingleCostDigitSprite(CurrentData);
		if (RequestedSprite == CostDigitRewriteNewSprite && CostDigitImage)
		{
			SetDigitImageBrush(*CostDigitImage, *CostDigitRewriteOldSprite, CostDigitSize);
			CostDigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			return;
		}
		bCostDigitRewritePrepared = false;
		bCostDigitRewriteMaterialActive = false;
		CostDigitRewriteOldSprite = nullptr;
		CostDigitRewriteNewSprite = nullptr;
		RestoreCostDigitAuthoredTransform();
	}

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

UPaperSprite* UWacomCardView::ResolveSingleCostDigitSprite(
	const FWacomCardViewData& Data) const
{
	if (!Data.bShowCost)
	{
		return nullptr;
	}
	const TArray<int32> Digits = SplitIntoDigits(Data.Cost);
	return Digits.Num() == 1 ? ResolvedCostDigitIcons.FindRef(Digits[0]) : nullptr;
}

bool UWacomCardView::EnsureCostDigitRewriteMaterial(
	const FWacomFirstPersonCardDataRewriteView& View)
{
	if (!bCostDigitRewritePrepared
		|| !CostDigitImage
		|| !CostDigitRewriteOldSprite
		|| !CostDigitRewriteNewSprite
		|| !View.Style.DigitRewriteMaterialInstance)
	{
		return false;
	}
	FWacomPaperSpriteAtlasView OldAtlas;
	FWacomPaperSpriteAtlasView NewAtlas;
	if (!WacomPaperSpriteAtlas::Resolve(CostDigitRewriteOldSprite, OldAtlas)
		|| !WacomPaperSpriteAtlas::Resolve(CostDigitRewriteNewSprite, NewAtlas))
	{
		return false;
	}
	if (!CostDigitRewriteMaterialInstance
		|| CostDigitRewriteMaterialSource != View.Style.DigitRewriteMaterialInstance)
	{
		CostDigitRewriteMaterialSource = View.Style.DigitRewriteMaterialInstance;
		CostDigitRewriteMaterialInstance = UMaterialInstanceDynamic::Create(
			View.Style.DigitRewriteMaterialInstance,
			this);
#if WITH_AUTOMATION_TESTS
		++CostDigitRewriteMaterialCreateCountForTest;
#endif
	}
	if (!CostDigitRewriteMaterialInstance)
	{
		return false;
	}
	FSlateBrush DigitBrush = CostDigitImage->GetBrush();
	DigitBrush.SetResourceObject(CostDigitRewriteMaterialInstance);
	DigitBrush.SetImageSize(FVector2f(
		FMath::Max(1.0f, CostDigitSize.X),
		FMath::Max(1.0f, CostDigitSize.Y)));
	CostDigitImage->SetBrush(DigitBrush);
	CostDigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	CostDigitRewriteMaterialInstance->SetTextureParameterValue(
		OldDigitTextureParameterName,
		OldAtlas.Texture);
	CostDigitRewriteMaterialInstance->SetTextureParameterValue(
		NewDigitTextureParameterName,
		NewAtlas.Texture);
	CostDigitRewriteMaterialInstance->SetVectorParameterValue(
		OldDigitUVRectParameterName,
		FLinearColor(
			OldAtlas.StartUV.X,
			OldAtlas.StartUV.Y,
			OldAtlas.SizeUV.X,
			OldAtlas.SizeUV.Y));
	CostDigitRewriteMaterialInstance->SetVectorParameterValue(
		NewDigitUVRectParameterName,
		FLinearColor(
			NewAtlas.StartUV.X,
			NewAtlas.StartUV.Y,
			NewAtlas.SizeUV.X,
			NewAtlas.SizeUV.Y));
	bCostDigitRewriteMaterialActive = true;
	bCostDigitPreviewMaterialActive = false;
	return true;
}

bool UWacomCardView::EnsureCostDigitPreviewMaterial(
	const FWacomFirstPersonCardCostPreviewView& View)
{
	if (!CostDigitImage
		|| !CurrentData.bShowCost
		|| !CurrentData.bHasCostPreview
		|| CurrentData.Cost == CurrentData.PreviewCost
		|| !View.Style.DigitRewriteMaterialInstance)
	{
		return false;
	}
	EnsureSpriteIconCachesBuilt();
	FWacomCardViewData PreviewData = CurrentData;
	PreviewData.Cost = CurrentData.PreviewCost;
	PreviewData.bHasCostPreview = false;
	UPaperSprite* CurrentSprite = ResolveSingleCostDigitSprite(CurrentData);
	UPaperSprite* PreviewSprite = ResolveSingleCostDigitSprite(PreviewData);
	if (!CurrentSprite || !PreviewSprite)
	{
		return false;
	}

	CostDigitRewriteOldSprite = CurrentSprite;
	CostDigitRewriteNewSprite = PreviewSprite;
	bCostDigitRewritePrepared = true;
	FWacomFirstPersonCardDataRewriteView RewriteMaterialView;
	RewriteMaterialView.bActive = true;
	RewriteMaterialView.Style = View.Style;
	return EnsureCostDigitRewriteMaterial(RewriteMaterialView);
}

void UWacomCardView::ApplyCostDigitRewriteMaterialParameters(
	const FWacomFirstPersonCardDataRewriteView& View)
{
	if (!CostDigitRewriteMaterialInstance || !CostDigitImage)
	{
		return;
	}
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteOldDissolveParameterName,
		FMath::Clamp(View.OldDissolveAmount, 0.0f, 1.0f));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteNewRevealParameterName,
		FMath::Clamp(View.NewRevealAmount, 0.0f, 1.0f));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteToneParameterName,
		static_cast<float>(View.Tone));
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteSeedParameterName,
		View.Seed);
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(
		CostRewriteReducedMotionParameterName,
		View.bReducedMotion ? 1.0f : 0.0f);
	CostDigitRewriteMaterialInstance->SetScalarParameterValue(DigitEffectModeParameterName, 2.0f);
	CacheCostDigitAuthoredTransform();
	FWidgetTransform Transform = CostDigitAuthoredTransform;
	const float Scale = View.bReducedMotion
		? 1.0f
		: FMath::Max(0.01f, View.DigitScale);
	Transform.Scale.X *= Scale;
	Transform.Scale.Y *= Scale;
	CostDigitImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	CostDigitImage->SetRenderTransform(Transform);
}

void UWacomCardView::CacheCostDigitAuthoredTransform()
{
	if (!CostDigitImage || bCostDigitAuthoredTransformCached)
	{
		return;
	}
	CostDigitAuthoredTransform = CostDigitImage->GetRenderTransform();
	CostDigitAuthoredPivot = CostDigitImage->GetRenderTransformPivot();
	bCostDigitAuthoredTransformCached = true;
}

void UWacomCardView::RestoreCostDigitAuthoredTransform()
{
	if (!CostDigitImage || !bCostDigitAuthoredTransformCached)
	{
		return;
	}
	CostDigitImage->SetRenderTransformPivot(CostDigitAuthoredPivot);
	CostDigitImage->SetRenderTransform(CostDigitAuthoredTransform);
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
#if WITH_AUTOMATION_TESTS
	++DurabilityDisplayUpdateCountForTest;
#endif

	EnsureSpriteIconCachesBuilt();

	if (!DurabilityHost)
	{
		return;
	}

	if (DurabilityDigitsHost)
	{
		DurabilityDigitsHost->SetVisibility(ESlateVisibility::Collapsed);
	}

	bool bRenderedDurabilityDigits = false;
	const bool bCanBuildIconDigits = CurrentData.bShowDurability
		&& CurrentData.Durability > 0
		&& DurabilityDigitsHost
		&& !ResolvedDurabilityDigitIcons.IsEmpty()
		&& WidgetTree;

	if (bCanBuildIconDigits)
	{
		const TArray<int32> Digits = SplitIntoDigits(CurrentData.Durability);
		bool bDigitsComplete = true;
		for (int32 Index = 0; Index < Digits.Num(); ++Index)
		{
			const int32 Digit = Digits[Index];
			UPaperSprite* Sprite = ResolvedDurabilityDigitIcons.FindRef(Digit);
			if (!Sprite)
			{
				bDigitsComplete = false;
				break;
			}

			UImage* DigitImage = Cast<UImage>(DurabilityDigitsHost->GetChildAt(Index));
			if (!DigitImage)
			{
				DigitImage = EnsureDurabilityDigitImage(*DurabilityDigitsHost);
			}
			if (!DigitImage)
			{
				bDigitsComplete = false;
				break;
			}
			SetDigitImageBrush(*DigitImage, *Sprite, DurabilityDigitSize);
			DigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		}

		if (bDigitsComplete)
		{
			while (DurabilityDigitsHost->GetChildrenCount() > Digits.Num())
			{
				DurabilityDigitsHost->RemoveChildAt(DurabilityDigitsHost->GetChildrenCount() - 1);
			}
			bRenderedDurabilityDigits = Digits.Num() > 0;
		}
		else
		{
			while (DurabilityDigitsHost->GetChildrenCount() > 0)
			{
				DurabilityDigitsHost->RemoveChildAt(DurabilityDigitsHost->GetChildrenCount() - 1);
			}
		}
	}

	if (DurabilityDigitsHost)
	{
		DurabilityDigitsHost->SetVisibility(
			bRenderedDurabilityDigits ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	DurabilityHost->SetVisibility(
		bRenderedDurabilityDigits ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	RefreshDurabilityShadow();
}

UImage* UWacomCardView::EnsureDurabilityDigitImage(UPanelWidget& Host)
{
	if (!WidgetTree)
	{
		return nullptr;
	}

	UImage* DigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	Host.AddChild(DigitImage);
	return DigitImage;
}

void UWacomCardView::UpdateTextDisplays()
{
	SetOptionalText(ValueText, CurrentData.bShowValue ? FText::AsNumber(CurrentData.Value) : FText::GetEmpty());
	SetOptionalText(NameText, CurrentData.Name);
	SetOptionalText(TypeText, CurrentData.TypeText);

#if WITH_AUTOMATION_TESTS
	++TextDisplayUpdateCountForTest;
#endif
}

void UWacomCardView::UpdateArtDisplay()
{
	if (CardArt && CurrentData.Art)
	{
		CardArt->SetBrushFromTexture(CurrentData.Art);
		CardArt->SetColorAndOpacity(FLinearColor::White);
	}
	else if (CardArt && AuthoredCardArtTexture)
	{
		CardArt->SetBrushFromTexture(AuthoredCardArtTexture);
		CardArt->SetColorAndOpacity(FLinearColor::White);
	}
	RefreshCardSurfaceComposite();

#if WITH_AUTOMATION_TESTS
	++ArtDisplayUpdateCountForTest;
#endif
}

void UWacomCardView::UpdateRarityBorderDisplay()
{
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
	RefreshCardSurfaceComposite();

#if WITH_AUTOMATION_TESTS
	++RarityDisplayUpdateCountForTest;
#endif
}

void UWacomCardView::RefreshCardSurfaceComposite()
{
	EnsureCardSurfaceImage();
	CacheAuthoredCardArtTexture();
	UTexture2D* SurfaceArtTexture = ResolveCardSurfaceArtTexture();
	const bool bCanUseComposite = CardSurfaceImage
		&& CardSurfaceMaterial
		&& SurfaceArtTexture;
	if (bCanUseComposite && !CardSurfaceMaterialInstance)
	{
		CardSurfaceMaterialInstance = UMaterialInstanceDynamic::Create(CardSurfaceMaterial, this);
	}

	bCardSurfaceCompositeActive = bCanUseComposite && CardSurfaceMaterialInstance;
	if (!bCardSurfaceCompositeActive)
	{
		if (CardSurfaceImage)
		{
			CardSurfaceImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		SetLegacySurfaceVisibility(true);
		return;
	}

	CardSurfaceMaterialInstance->SetTextureParameterValue(ArtTextureParameterName, SurfaceArtTexture);
	CardSurfaceMaterialInstance->SetScalarParameterValue(
		ArtDepthEnabledParameterName,
		CurrentData.ArtDepthMap ? 1.0f : 0.0f);
	if (CurrentData.ArtDepthMap)
	{
		CardSurfaceMaterialInstance->SetTextureParameterValue(
			ArtDepthTextureParameterName,
			CurrentData.ArtDepthMap);
	}
	if (CardSurfaceFrameTexture)
	{
		CardSurfaceMaterialInstance->SetTextureParameterValue(
			FrameTextureParameterName,
			CardSurfaceFrameTexture);
	}
	CardSurfaceMaterialInstance->SetVectorParameterValue(BackColorParameterName, CardSurfaceBackColor);
	const float SourceWidth = FMath::Max(1.0f, static_cast<float>(SurfaceArtTexture->GetSizeX()));
	const float SourceHeight = FMath::Max(1.0f, static_cast<float>(SurfaceArtTexture->GetSizeY()));
	CardSurfaceMaterialInstance->SetVectorParameterValue(
		CardSourceInvSizeParameterName,
		FLinearColor(1.0f / SourceWidth, 1.0f / SourceHeight, SourceWidth, SourceHeight));
	ApplyRarityToCardSurfaceMaterial();
	ApplyCardSurfacePerspective();
	CardSurfaceImage->SetBrushFromMaterial(CardSurfaceMaterialInstance);
	CardSurfaceImage->SetColorAndOpacity(FLinearColor::White);
	CardSurfaceImage->SetVisibility(ESlateVisibility::HitTestInvisible);
	SetLegacySurfaceVisibility(false);
}

void UWacomCardView::CacheAuthoredCardArtTexture()
{
	if (bAuthoredCardArtTextureCached)
	{
		return;
	}
	if (!CardArt)
	{
		return;
	}

	AuthoredCardArtTexture = Cast<UTexture2D>(CardArt->GetBrush().GetResourceObject());
	bAuthoredCardArtTextureCached = true;
}

UTexture2D* UWacomCardView::ResolveCardSurfaceArtTexture() const
{
	return CurrentData.Art ? CurrentData.Art.Get() : AuthoredCardArtTexture.Get();
}

void UWacomCardView::ApplyCardSurfacePerspective()
{
	const FVector2D AppliedTilt = CardSurfacePerspectiveView.bEnabled
		&& !CardSurfacePerspectiveView.bReducedMotion
		? CardSurfacePerspectiveView.TiltDegrees
		: FVector2D::ZeroVector;
	const float AppliedStrength = CardSurfacePerspectiveView.bEnabled
		&& !CardSurfacePerspectiveView.bReducedMotion
		? FMath::Max(0.0f, CardSurfacePerspectiveView.Strength)
		: 0.0f;
	if (CardSurfaceMaterialInstance)
	{
		CardSurfaceMaterialInstance->SetScalarParameterValue(SurfaceTiltXParameterName, AppliedTilt.X);
		CardSurfaceMaterialInstance->SetScalarParameterValue(SurfaceTiltYParameterName, AppliedTilt.Y);
		CardSurfaceMaterialInstance->SetScalarParameterValue(
			SurfaceParallaxStrengthParameterName,
			AppliedStrength);
	}

	const FVector2D Offset = AppliedStrength > 0.0f
		? CardSurfacePerspectiveView.AttachmentOffsetPixels
		: FVector2D::ZeroVector;
	ApplyAttachmentParallaxOffset(Offset);
	ApplyAttachmentCastShadowView();
}

void UWacomCardView::EnsureDurabilityShadowImage()
{
	if (DurabilityShadowImage || !WidgetTree || !DurabilityBackIcon)
	{
		return;
	}
	UOverlay* ParentOverlay = Cast<UOverlay>(DurabilityBackIcon->GetParent());
	if (!ParentOverlay)
	{
		return;
	}
	const int32 SourceIndex = ParentOverlay->GetChildIndex(DurabilityBackIcon);
	if (SourceIndex == INDEX_NONE)
	{
		return;
	}

	DurabilityShadowImage = WidgetTree->ConstructWidget<UImage>(
		UImage::StaticClass(),
		TEXT("DurabilityShadowImage_Runtime"));
	if (!DurabilityShadowImage)
	{
		return;
	}
	DurabilityShadowImage->SetVisibility(ESlateVisibility::Collapsed);
	UOverlaySlot* ShadowSlot = Cast<UOverlaySlot>(
		ParentOverlay->InsertChildAt(SourceIndex, DurabilityShadowImage));
	const UOverlaySlot* SourceSlot = Cast<UOverlaySlot>(DurabilityBackIcon->Slot);
	if (ShadowSlot && SourceSlot)
	{
		ShadowSlot->SetPadding(SourceSlot->GetPadding());
		ShadowSlot->SetHorizontalAlignment(SourceSlot->GetHorizontalAlignment());
		ShadowSlot->SetVerticalAlignment(SourceSlot->GetVerticalAlignment());
	}
}

void UWacomCardView::ApplyAttachmentCastShadowView()
{
	for (UWacomCardEffectBadgeWidget* BadgeWidget : CollectEffectBadgeWidgets())
	{
		if (BadgeWidget)
		{
			BadgeWidget->SetAttachmentCastShadowView(CardSurfacePerspectiveView);
		}
	}
	RefreshDurabilityShadow();
}

void UWacomCardView::RefreshDurabilityShadow()
{
	EnsureDurabilityShadowImage();
	if (!DurabilityShadowImage || !DurabilityBackIcon || !DurabilityHost
		|| !CardSurfacePerspectiveView.bAttachmentCastShadowEnabled
		|| CardSurfacePerspectiveView.AttachmentCastShadowOpacity <= KINDA_SMALL_NUMBER
		|| !DurabilityHost->IsVisible()
		|| !DurabilityBackIcon->IsVisible()
		|| (DurabilityBackIcon->GetBrush().GetResourceObject() == nullptr
			&& DurabilityBackIcon->GetBrush().GetDrawType()
				== ESlateBrushDrawType::NoDrawType))
	{
		if (DurabilityShadowImage)
		{
			DurabilityShadowImage->SetVisibility(ESlateVisibility::Collapsed);
		}
		return;
	}

	DurabilityShadowImage->SetBrush(DurabilityBackIcon->GetBrush());
	FLinearColor ShadowColor = CardSurfacePerspectiveView.AttachmentCastShadowColor;
	ShadowColor.A = FMath::Clamp(
		CardSurfacePerspectiveView.AttachmentCastShadowOpacity,
		0.0f,
		1.0f);
	DurabilityShadowImage->SetColorAndOpacity(ShadowColor);
	DurabilityShadowImage->SetRenderTransformPivot(DurabilityBackIcon->GetRenderTransformPivot());
	FWidgetTransform ShadowTransform = DurabilityBackIcon->GetRenderTransform();
	ShadowTransform.Translation +=
		CardSurfacePerspectiveView.AttachmentCastShadowOffsetPixels;
	DurabilityShadowImage->SetRenderTransform(ShadowTransform);
	DurabilityShadowImage->SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UWacomCardView::ResetAttachmentCastShadowView()
{
	for (UWacomCardEffectBadgeWidget* BadgeWidget : CollectEffectBadgeWidgets())
	{
		if (BadgeWidget)
		{
			BadgeWidget->ResetAttachmentCastShadowView();
		}
	}
	if (DurabilityShadowImage)
	{
		DurabilityShadowImage->SetRenderTransform(FWidgetTransform());
		DurabilityShadowImage->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		DurabilityShadowImage->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UWacomCardView::CacheLegacySurfaceVisibility()
{
	AuthoredLegacySurfaceVisibilities.Reset();
	for (UWidget* Widget : { Cast<UWidget>(BackColor), Cast<UWidget>(CardArt), Cast<UWidget>(Frame),
		Cast<UWidget>(RarityBorder), Cast<UWidget>(SurfaceFoilOverlay) })
	{
		if (Widget)
		{
			AuthoredLegacySurfaceVisibilities.Add(Widget, Widget->GetVisibility());
		}
	}
	bLegacySurfaceVisibilityCached = true;
}

void UWacomCardView::SetLegacySurfaceVisibility(bool bVisible)
{
	if (!bLegacySurfaceVisibilityCached)
	{
		CacheLegacySurfaceVisibility();
	}
	for (const TPair<TWeakObjectPtr<UWidget>, ESlateVisibility>& Pair : AuthoredLegacySurfaceVisibilities)
	{
		if (UWidget* Widget = Pair.Key.Get())
		{
			Widget->SetVisibility(bVisible ? Pair.Value : ESlateVisibility::Collapsed);
		}
	}

	if (!bVisible)
	{
		return;
	}
	if (RarityBorder)
	{
		UPaperSprite* Sprite = CurrentData.Rarity.IsValid()
			? ResolvedRarityBorderSprites.FindRef(CurrentData.Rarity)
			: nullptr;
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
	ApplySurfaceFoilOverlay();
}

void UWacomCardView::CacheAttachmentAuthoredTransforms()
{
	AuthoredAttachmentTransforms.Reset();
	if (AttachmentParallaxHost)
	{
		AuthoredAttachmentTransforms.Add(
			AttachmentParallaxHost,
			AttachmentParallaxHost->GetRenderTransform());
		return;
	}
	const TArray<UWidget*> AttachmentWidgets = {
		EffectBadgeSlot1.Get(),
		EffectBadgeSlot2.Get(),
		EffectBadgeSlot3.Get(),
		EffectBadgeSlot4.Get(),
		DurabilityHost.Get()
	};
	for (UWidget* Widget : AttachmentWidgets)
	{
		if (Widget)
		{
			AuthoredAttachmentTransforms.Add(Widget, Widget->GetRenderTransform());
		}
	}
}

void UWacomCardView::RestoreAttachmentAuthoredTransforms()
{
	for (const TPair<TWeakObjectPtr<UWidget>, FWidgetTransform>& Pair : AuthoredAttachmentTransforms)
	{
		if (UWidget* Widget = Pair.Key.Get())
		{
			Widget->SetRenderTransform(Pair.Value);
		}
	}
	AppliedAttachmentOffsetPixels = FVector2D::ZeroVector;
}

void UWacomCardView::ApplyAttachmentParallaxOffset(const FVector2D& OffsetPixels)
{
	if (AuthoredAttachmentTransforms.IsEmpty())
	{
		CacheAttachmentAuthoredTransforms();
	}
	for (const TPair<TWeakObjectPtr<UWidget>, FWidgetTransform>& Pair : AuthoredAttachmentTransforms)
	{
		if (UWidget* Widget = Pair.Key.Get())
		{
			FWidgetTransform Transform = Pair.Value;
			Transform.Translation += OffsetPixels;
			Widget->SetRenderTransform(Transform);
		}
	}
	AppliedAttachmentOffsetPixels = OffsetPixels;
}

bool UWacomCardView::ApplyRarityToCardSurfaceMaterial()
{
	if (!CardSurfaceMaterialInstance)
	{
		return false;
	}
	CardSurfaceMaterialInstance->SetScalarParameterValue(RarityEnabledParameterName, 0.0f);
	UPaperSprite* Sprite = CurrentData.Rarity.IsValid()
		? ResolvedRarityBorderSprites.FindRef(CurrentData.Rarity)
		: nullptr;
	UTexture2D* Texture = Sprite ? Sprite->GetBakedTexture() : nullptr;
	if (!Sprite || !Texture || Texture->GetSizeX() <= 0 || Texture->GetSizeY() <= 0)
	{
		return false;
	}

	const FVector2D SourceUV = Sprite->GetSourceUV();
	const FVector2D SourceSize = Sprite->GetSourceSize();
	if (SourceSize.X <= 0.0f || SourceSize.Y <= 0.0f)
	{
		return false;
	}
	const FVector2D TextureSize(Texture->GetSizeX(), Texture->GetSizeY());
	const FVector2D Scale(SourceSize.X / TextureSize.X, SourceSize.Y / TextureSize.Y);
	const FVector2D Bias(SourceUV.X / TextureSize.X, SourceUV.Y / TextureSize.Y);
	if (!FMath::IsFinite(Scale.X) || !FMath::IsFinite(Scale.Y)
		|| !FMath::IsFinite(Bias.X) || !FMath::IsFinite(Bias.Y)
		|| Scale.X <= 0.0f || Scale.Y <= 0.0f
		|| Bias.X < 0.0f || Bias.Y < 0.0f
		|| Bias.X + Scale.X > 1.001f || Bias.Y + Scale.Y > 1.001f)
	{
		return false;
	}

	CardSurfaceMaterialInstance->SetTextureParameterValue(RarityTextureParameterName, Texture);
	CardSurfaceMaterialInstance->SetVectorParameterValue(
		RarityUVScaleBiasParameterName,
		FLinearColor(Scale.X, Scale.Y, Bias.X, Bias.Y));
	CardSurfaceMaterialInstance->SetScalarParameterValue(RarityEnabledParameterName, 1.0f);
	return true;
}

void UWacomCardView::UpdateDisabledDisplay()
{
	if (DisabledOverlay)
	{
		DisabledOverlay->SetVisibility(CurrentData.bDisabled ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

#if WITH_AUTOMATION_TESTS
	++DisabledDisplayUpdateCountForTest;
#endif
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
			UPaperSprite* Sprite = Pair.Value.Get();
			if (!Sprite)
			{
#if WITH_AUTOMATION_TESTS
				++SpriteSynchronousFallbackCountForTest;
#endif
				Sprite = Pair.Value.LoadSynchronous();
			}
			if (Sprite)
			{
				ResolvedCostDigitIcons.Add(Pair.Key, Sprite);
			}
		}
	}

	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : DurabilityDigitIcons)
	{
		if (!Pair.Value.IsNull())
		{
			UPaperSprite* Sprite = Pair.Value.Get();
			if (!Sprite)
			{
#if WITH_AUTOMATION_TESTS
				++SpriteSynchronousFallbackCountForTest;
#endif
				Sprite = Pair.Value.LoadSynchronous();
			}
			if (Sprite)
			{
				ResolvedDurabilityDigitIcons.Add(Pair.Key, Sprite);
			}
		}
	}

	for (const TPair<FGameplayTag, TSoftObjectPtr<UPaperSprite>>& Pair : RarityBorderSprites)
	{
		if (Pair.Key.IsValid() && !Pair.Value.IsNull())
		{
			UPaperSprite* Sprite = Pair.Value.Get();
			if (!Sprite)
			{
#if WITH_AUTOMATION_TESTS
				++SpriteSynchronousFallbackCountForTest;
#endif
				Sprite = Pair.Value.LoadSynchronous();
			}
			if (Sprite)
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
