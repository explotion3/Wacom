// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomCardEffectBadgeWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "PaperSprite.h"

namespace
{
	bool AreTextViewsEquivalent(const FText& A, const FText& B)
	{
		return A.EqualTo(B);
	}

	bool AreEffectBadgeDataEquivalent(
		const FWacomCardViewEffectBadge& A,
		const FWacomCardViewEffectBadge& B)
	{
		return A.Kind == B.Kind
			&& A.Value == B.Value
			&& AreTextViewsEquivalent(A.DisplayText, B.DisplayText);
	}
}

TSharedRef<SWidget> UWacomCardEffectBadgeWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_EffectBadge"));
		}

		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("BadgeRoot"));
		WidgetTree->RootWidget = Root;

		BadgeFrameImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("BadgeFrameImage"));
		BadgeFrameImage->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* FrameSlot = Root->AddChildToOverlay(BadgeFrameImage))
		{
			FrameSlot->SetHorizontalAlignment(HAlign_Fill);
			FrameSlot->SetVerticalAlignment(VAlign_Fill);
		}

		DigitHost = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("DigitHost"));
		DigitHost->SetVisibility(ESlateVisibility::Collapsed);
		if (UOverlaySlot* DigitSlot = Root->AddChildToOverlay(DigitHost))
		{
			DigitSlot->SetHorizontalAlignment(HAlign_Center);
			DigitSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void UWacomCardEffectBadgeWidget::NativeConstruct()
{
	Super::NativeConstruct();
	bHasAppliedData = false;
	ApplyCurrentDataToWidgets();
}

void UWacomCardEffectBadgeWidget::SetEffectBadgeData(const FWacomCardViewEffectBadge& InData)
{
	if (bHasAppliedData && AreEffectBadgeDataEquivalent(CurrentData, InData))
	{
		return;
	}

	CurrentData = InData;
	ApplyCurrentDataToWidgets();
}

FText UWacomCardEffectBadgeWidget::GetValueText() const
{
	return FText::AsNumber(CurrentData.Value);
}

void UWacomCardEffectBadgeWidget::ApplyCurrentDataToWidgets()
{
#if WITH_AUTOMATION_TESTS
	++ApplyCountForTest;
#endif

	EnsureSpriteCachesBuilt();
	UpdateFrameImage();
	UpdateDigitImages();
	bHasAppliedData = true;
}

void UWacomCardEffectBadgeWidget::EnsureSpriteCachesBuilt()
{
	if (bSpriteCachesBuilt)
	{
		return;
	}

	RebuildSpriteCaches();
}

void UWacomCardEffectBadgeWidget::RebuildSpriteCaches()
{
	ResolvedBadgeFrameSprites.Reset();
	ResolvedDigitSprites.Reset();

	for (const TPair<EWacomCardViewEffectBadgeKind, TSoftObjectPtr<UPaperSprite>>& Pair : BadgeFrameSprites)
	{
		if (!Pair.Value.IsNull())
		{
			if (UPaperSprite* Sprite = Pair.Value.LoadSynchronous())
			{
				ResolvedBadgeFrameSprites.Add(Pair.Key, Sprite);
			}
		}
	}

	for (const TPair<int32, TSoftObjectPtr<UPaperSprite>>& Pair : DigitSprites)
	{
		if (!Pair.Value.IsNull())
		{
			if (UPaperSprite* Sprite = Pair.Value.LoadSynchronous())
			{
				ResolvedDigitSprites.Add(Pair.Key, Sprite);
			}
		}
	}

	bSpriteCachesBuilt = true;
}

void UWacomCardEffectBadgeWidget::UpdateFrameImage()
{
	if (!BadgeFrameImage)
	{
		return;
	}

	if (UPaperSprite* Sprite = ResolvedBadgeFrameSprites.FindRef(CurrentData.Kind))
	{
		SetSpriteBrush(*BadgeFrameImage, *Sprite, BadgeFrameDrawSize);
		BadgeFrameImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		return;
	}

	BadgeFrameImage->SetVisibility(ESlateVisibility::Collapsed);
}

void UWacomCardEffectBadgeWidget::UpdateDigitImages()
{
#if WITH_AUTOMATION_TESTS
	++DigitImageUpdateCountForTest;
#endif

	if (!DigitHost)
	{
		return;
	}

	DigitHost->SetVisibility(ESlateVisibility::Collapsed);

	if (!WidgetTree || ResolvedDigitSprites.IsEmpty())
	{
		while (DigitHost->GetChildrenCount() > 0)
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
		return;
	}

	const TArray<int32> Digits = SplitIntoDigits(CurrentData.Value);
	if (Digits.IsEmpty())
	{
		while (DigitHost->GetChildrenCount() > 0)
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
		return;
	}

	bool bDigitsComplete = true;
	for (int32 Index = 0; Index < Digits.Num(); ++Index)
	{
		const int32 Digit = Digits[Index];
		UPaperSprite* Sprite = ResolvedDigitSprites.FindRef(Digit);
		if (!Sprite)
		{
			bDigitsComplete = false;
			break;
		}

		UImage* DigitImage = Cast<UImage>(DigitHost->GetChildAt(Index));
		if (!DigitImage)
		{
			DigitImage = EnsureDigitImage(Index);
		}
		if (!DigitImage)
		{
			bDigitsComplete = false;
			break;
		}

		SetSpriteBrush(*DigitImage, *Sprite, DigitDrawSize);
		DigitImage->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UPanelSlot* AddedDigitSlot = DigitImage->Slot)
		{
			if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(AddedDigitSlot))
			{
				const bool bInteriorDigit = Index > 0 && Index < Digits.Num() - 1;
				HorizontalSlot->SetPadding(bInteriorDigit ? InteriorDigitPadding : FMargin());
			}
		}
	}

	if (bDigitsComplete)
	{
		while (DigitHost->GetChildrenCount() > Digits.Num())
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
	}
	else
	{
		while (DigitHost->GetChildrenCount() > 0)
		{
			DigitHost->RemoveChildAt(DigitHost->GetChildrenCount() - 1);
		}
		return;
	}

	DigitHost->SetVisibility(
		DigitHost->GetChildrenCount() > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
}

UImage* UWacomCardEffectBadgeWidget::EnsureDigitImage(int32 Index)
{
	if (!WidgetTree || !DigitHost)
	{
		return nullptr;
	}

	UImage* DigitImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
	DigitHost->AddChild(DigitImage);
	return DigitImage;
}

TArray<int32> UWacomCardEffectBadgeWidget::SplitIntoDigits(int32 Value) const
{
	TArray<int32> Result;
	if (Value <= 0)
	{
		Result.Add(0);
	}
	else
	{
		TArray<int32> Reversed;
		while (Value > 0)
		{
			Reversed.Add(Value % 10);
			Value /= 10;
		}

		for (int32 Index = Reversed.Num() - 1; Index >= 0; --Index)
		{
			Result.Add(Reversed[Index]);
		}
	}

	const int32 DesiredDigitCount = FMath::Max(1, MinimumDigitCount);
	while (Result.Num() < DesiredDigitCount)
	{
		Result.Insert(0, 0);
	}

	return Result;
}

void UWacomCardEffectBadgeWidget::SetSpriteBrush(UImage& Image, UPaperSprite& Sprite, const FVector2D& DesiredSize)
{
	FSlateBrush Brush = Image.GetBrush();
	Brush.SetResourceObject(&Sprite);
	Brush.SetImageSize(FVector2f(
		FMath::Max(1.0f, DesiredSize.X),
		FMath::Max(1.0f, DesiredSize.Y)));
	Image.SetBrush(Brush);
}
