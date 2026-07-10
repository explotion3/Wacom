// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/Widget.h"
#include "Components/SlateWrapperTypes.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"

namespace
{
	constexpr float FallbackIntroDurationSeconds = 0.18f;
	constexpr float FallbackIntroYOffsetPixels = 8.0f;
	constexpr float FallbackIntroStartScale = 0.985f;
	constexpr float FallbackPulseDurationSeconds = 0.22f;
	constexpr float FallbackPulseScaleBoost = 0.018f;

	float EaseOutQuint(const float Alpha)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - Clamped, 5.0f);
	}

	float EaseOutQuart(const float Alpha)
	{
		const float Clamped = FMath::Clamp(Alpha, 0.0f, 1.0f);
		return 1.0f - FMath::Pow(1.0f - Clamped, 4.0f);
	}

	void StyleEntryText(UTextBlock* TextBlock, const int32 Size, const FLinearColor& Color, const bool bBold = false)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = Size;
		if (bBold)
		{
			Font.TypefaceFontName = TEXT("Bold");
		}
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(Color));
		TextBlock->SetShadowOffset(FVector2D(0.0f, 1.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.50f));
		TextBlock->SetAutoWrapText(false);
		TextBlock->SetClipping(EWidgetClipping::ClipToBoundsAlways);
	}

	UTextBlock* MakeEntryText(UWidgetTree& Tree, const FName WidgetName, const int32 Size, const FLinearColor& Color, const bool bBold = false)
	{
		UTextBlock* TextBlock = Tree.ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), WidgetName);
		StyleEntryText(TextBlock, Size, Color, bBold);
		return TextBlock;
	}

	UBorder* MakeEntryPill(UWidgetTree& Tree, const FName WidgetName, const FLinearColor& Color, const FMargin Padding = FMargin(7.0f, 3.0f))
	{
		UBorder* Border = Tree.ConstructWidget<UBorder>(UBorder::StaticClass(), WidgetName);
		if (Border)
		{
			Border->SetBrushColor(Color);
			Border->SetPadding(Padding);
			Border->SetContentColorAndOpacity(FLinearColor::White);
		}
		return Border;
	}

	void AddRowChild(UHorizontalBox* Row, UWidget* Child, const FMargin Padding, const FSlateChildSize Size = FSlateChildSize(ESlateSizeRule::Automatic))
	{
		if (!Row || !Child)
		{
			return;
		}

		if (UHorizontalBoxSlot* Slot = Row->AddChildToHorizontalBox(Child))
		{
			Slot->SetPadding(Padding);
			Slot->SetVerticalAlignment(VAlign_Center);
			Slot->SetSize(Size);
		}
	}

	void AddTextToBorder(UBorder* Border, UTextBlock* TextBlock)
	{
		if (!Border || !TextBlock)
		{
			return;
		}

		Border->SetContent(TextBlock);
		if (UBorderSlot* Slot = Cast<UBorderSlot>(TextBlock->Slot))
		{
			Slot->SetHorizontalAlignment(HAlign_Center);
			Slot->SetVerticalAlignment(VAlign_Center);
		}
	}

	bool AreStatusStacksEquivalent(
		const TMap<FGameplayTag, int32>& Left,
		const TMap<FGameplayTag, int32>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (const TPair<FGameplayTag, int32>& Pair : Left)
		{
			const int32* RightValue = Right.Find(Pair.Key);
			if (!RightValue || *RightValue != Pair.Value)
			{
				return false;
			}
		}
		return true;
	}

	bool ArePartEntryViewsEquivalent(
		const FWacomBattleEnemyPartEntryViewData& Left,
		const FWacomBattleEnemyPartEntryViewData& Right)
	{
		return Left.PartInstanceId == Right.PartInstanceId
			&& Left.Identity == Right.Identity
			&& Left.EnemySlotId == Right.EnemySlotId
			&& Left.PartSlotId == Right.PartSlotId
			&& Left.PartDisplayName.EqualTo(Right.PartDisplayName)
			&& Left.CurrentHp == Right.CurrentHp
			&& Left.MaxHp == Right.MaxHp
			&& Left.Shield == Right.Shield
			&& Left.CurrentInitiative == Right.CurrentInitiative
			&& Left.CurrentIntentDisplayName.EqualTo(Right.CurrentIntentDisplayName)
			&& Left.CurrentIntentInitiative == Right.CurrentIntentInitiative
			&& Left.CurrentIntentResistanceValue == Right.CurrentIntentResistanceValue
			&& Left.RuntimeStatuses.Num() == Right.RuntimeStatuses.Num()
			&& Left.RuntimeStatuses.HasAllExact(Right.RuntimeStatuses)
			&& Right.RuntimeStatuses.HasAllExact(Left.RuntimeStatuses)
			&& AreStatusStacksEquivalent(Left.RuntimeStatusStacks, Right.RuntimeStatusStacks)
			&& Left.bDestroyed == Right.bDestroyed
			&& Left.bActionPreviewWillAct == Right.bActionPreviewWillAct;
	}
}

void UWacomBattleEnemyPartEntryWidget::SetPartEntryViewData(const FWacomBattleEnemyPartEntryViewData& InView)
{
	const FWacomBattleEnemyPartEntryViewData PreviousView = CurrentView;
	const bool bHadViewData = bHasReceivedViewData;

	CurrentView = InView;
	bHasReceivedViewData = true;
	RefreshText();

	if (!bUsingGeneratedFallbackLayout)
	{
		return;
	}

	if (!bHadViewData)
	{
		StartFallbackIntroAnimation();
		return;
	}

	if (!PreviousView.bDestroyed && CurrentView.bDestroyed)
	{
		StartFallbackPulseAnimation(FLinearColor(0.96f, 0.34f, 0.24f, 1.0f), 1.0f);
	}
	else if (CurrentView.CurrentHp < PreviousView.CurrentHp)
	{
		StartFallbackPulseAnimation(FLinearColor(1.0f, 0.22f, 0.18f, 1.0f), 0.95f);
	}
	else if (CurrentView.Shield != PreviousView.Shield)
	{
		StartFallbackPulseAnimation(FLinearColor(0.30f, 0.70f, 1.0f, 1.0f), 0.70f);
	}
}

const FWacomBattleEnemyPartEntryViewData& UWacomBattleEnemyPartEntryWidget::GetEffectivePartEntryViewData() const
{
	return bHasActionPreview ? ActionPreviewView : CurrentView;
}

void UWacomBattleEnemyPartEntryWidget::SetActionPreview(const FWacomBattleEnemyPartEntryViewData& InPreviewView)
{
	if (bHasActionPreview && ArePartEntryViewsEquivalent(ActionPreviewView, InPreviewView))
	{
		return;
	}

	ActionPreviewView = InPreviewView;
	bHasActionPreview = true;
	RefreshText();
}

void UWacomBattleEnemyPartEntryWidget::ClearActionPreview()
{
	if (!bHasActionPreview)
	{
		return;
	}

	bHasActionPreview = false;
	RefreshText();
}

TSharedRef<SWidget> UWacomBattleEnemyPartEntryWidget::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		bUsingGeneratedFallbackLayout = true;
		EntryBackground = MakeEntryPill(
			*WidgetTree,
			TEXT("EntryBackground"),
			FLinearColor(0.095f, 0.11f, 0.13f, 0.92f),
			FMargin(8.0f, 5.0f));
		WidgetTree->RootWidget = EntryBackground;

		RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Root"));
		EntryBackground->SetContent(RootBox);

		RowBox = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowBox"));
		if (UVerticalBoxSlot* RowSlot = RootBox->AddChildToVerticalBox(RowBox))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		PartNameText = MakeEntryText(*WidgetTree, TEXT("PartNameText"), 15, FLinearColor(0.96f, 0.92f, 0.82f, 1.0f), true);
		PartNameText->SetMinDesiredWidth(74.0f);
		AddRowChild(RowBox, PartNameText, FMargin(0.0f, 0.0f, 8.0f, 0.0f), FSlateChildSize(ESlateSizeRule::Fill));

		UBorder* HpPill = MakeEntryPill(*WidgetTree, TEXT("HpPill"), FLinearColor(0.18f, 0.055f, 0.06f, 0.96f));
		UHorizontalBox* HpRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("HpRow"));
		HpLabelText = MakeEntryText(*WidgetTree, TEXT("HpLabelText"), 11, FLinearColor(1.0f, 0.58f, 0.56f, 1.0f), true);
		HpLabelText->SetText(FText::FromString(TEXT("HP")));
		HpText = MakeEntryText(*WidgetTree, TEXT("HpText"), 13, FLinearColor(1.0f, 0.90f, 0.82f, 1.0f), true);
		AddRowChild(HpRow, HpLabelText, FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		AddRowChild(HpRow, HpText, FMargin(0.0f));
		HpPill->SetContent(HpRow);
		AddRowChild(RowBox, HpPill, FMargin(0.0f, 0.0f, 5.0f, 0.0f));

		ShieldPill = MakeEntryPill(*WidgetTree, TEXT("ShieldPill"), FLinearColor(0.055f, 0.16f, 0.24f, 0.96f));
		UHorizontalBox* ShieldRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("ShieldRow"));
		ShieldLabelText = MakeEntryText(*WidgetTree, TEXT("ShieldLabelText"), 11, FLinearColor(0.58f, 0.82f, 1.0f, 1.0f), true);
		ShieldLabelText->SetText(FText::FromString(TEXT("SH")));
		ShieldText = MakeEntryText(*WidgetTree, TEXT("ShieldText"), 13, FLinearColor(0.88f, 0.96f, 1.0f, 1.0f), true);
		AddRowChild(ShieldRow, ShieldLabelText, FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		AddRowChild(ShieldRow, ShieldText, FMargin(0.0f));
		ShieldPill->SetContent(ShieldRow);
		AddRowChild(RowBox, ShieldPill, FMargin(0.0f, 0.0f, 5.0f, 0.0f));

		UBorder* InitiativePill = MakeEntryPill(*WidgetTree, TEXT("InitiativePill"), FLinearColor(0.20f, 0.14f, 0.045f, 0.96f));
		UHorizontalBox* InitiativeRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("InitiativeRow"));
		InitiativeLabelText = MakeEntryText(*WidgetTree, TEXT("InitiativeLabelText"), 11, FLinearColor(1.0f, 0.78f, 0.35f, 1.0f), true);
		InitiativeLabelText->SetText(FText::FromString(TEXT("INIT")));
		InitiativeText = MakeEntryText(*WidgetTree, TEXT("InitiativeText"), 13, FLinearColor(1.0f, 0.93f, 0.72f, 1.0f), true);
		AddRowChild(InitiativeRow, InitiativeLabelText, FMargin(0.0f, 0.0f, 4.0f, 0.0f));
		AddRowChild(InitiativeRow, InitiativeText, FMargin(0.0f));
		InitiativePill->SetContent(InitiativeRow);
		AddRowChild(RowBox, InitiativePill, FMargin(0.0f, 0.0f, 5.0f, 0.0f));

		IntentText = MakeEntryText(*WidgetTree, TEXT("IntentText"), 12, FLinearColor(0.78f, 0.84f, 0.92f, 1.0f));
		IntentText->SetMinDesiredWidth(60.0f);
		AddRowChild(RowBox, IntentText, FMargin(4.0f, 0.0f, 0.0f, 0.0f), FSlateChildSize(ESlateSizeRule::Fill));

		StatusText = MakeEntryText(*WidgetTree, TEXT("StatusText"), 11, FLinearColor(0.60f, 0.68f, 0.76f, 1.0f));
		if (UVerticalBoxSlot* StatusSlot = RootBox->AddChildToVerticalBox(StatusText))
		{
			StatusSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			StatusSlot->SetHorizontalAlignment(HAlign_Fill);
		}

		StatsText = MakeEntryText(*WidgetTree, TEXT("StatsText"), 10, FLinearColor(0.48f, 0.54f, 0.62f, 1.0f));
		if (UVerticalBoxSlot* StatsSlot = RootBox->AddChildToVerticalBox(StatsText))
		{
			StatsSlot->SetPadding(FMargin(0.0f, 3.0f, 0.0f, 0.0f));
		}

		DestroyedOverlay = MakeEntryPill(
			*WidgetTree,
			TEXT("DestroyedOverlay"),
			FLinearColor(0.04f, 0.04f, 0.045f, 0.90f),
			FMargin(8.0f, 3.0f));
		UTextBlock* DestroyedText = MakeEntryText(*WidgetTree, TEXT("DestroyedText"), 12, FLinearColor(0.96f, 0.66f, 0.52f, 1.0f), true);
		DestroyedText->SetText(FText::FromString(TEXT("DESTROYED")));
		AddTextToBorder(Cast<UBorder>(DestroyedOverlay), DestroyedText);
		if (UVerticalBoxSlot* DestroyedSlot = RootBox->AddChildToVerticalBox(DestroyedOverlay))
		{
			DestroyedSlot->SetPadding(FMargin(0.0f, 4.0f, 0.0f, 0.0f));
			DestroyedSlot->SetHorizontalAlignment(HAlign_Right);
		}
	}

	RefreshText();
	ApplyFallbackMotionVisual();
	return Super::RebuildWidget();
}

void UWacomBattleEnemyPartEntryWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!bUsingGeneratedFallbackLayout || (!bFallbackIntroActive && !bFallbackPulseActive))
	{
		return;
	}

	const float DeltaTime = FMath::Max(0.0f, InDeltaTime);
	if (bFallbackIntroActive)
	{
		FallbackIntroElapsedSeconds += DeltaTime;
		const float IntroProgress = (FallbackIntroElapsedSeconds - FallbackIntroDelaySeconds) / FallbackIntroDurationSeconds;
		if (IntroProgress >= 1.0f)
		{
			bFallbackIntroActive = false;
		}
	}

	if (bFallbackPulseActive)
	{
		FallbackPulseElapsedSeconds += DeltaTime;
		if (FallbackPulseElapsedSeconds >= FallbackPulseDurationSeconds)
		{
			bFallbackPulseActive = false;
		}
	}

	ApplyFallbackMotionVisual();
}

#if WITH_AUTOMATION_TESTS
void UWacomBattleEnemyPartEntryWidget::TickFallbackMotionForTest(float DeltaSeconds)
{
	NativeTick(FGeometry(), DeltaSeconds);
}
#endif

void UWacomBattleEnemyPartEntryWidget::NativeRefreshFromSnapshot(const FBattleSnapshot& /*Snap*/)
{
	RefreshText();
}

void UWacomBattleEnemyPartEntryWidget::RefreshText()
{
	const FWacomBattleEnemyPartEntryViewData& View = GetEffectivePartEntryViewData();
	const bool bDestroyed = View.bDestroyed;

	if (EntryBackground)
	{
		EntryBackground->SetBrushColor(GetFallbackBaseBackgroundColor());
	}

	if (PartNameText)
	{
		PartNameText->SetText(View.PartDisplayName.IsEmpty() ? FText::FromName(View.PartSlotId) : View.PartDisplayName);
		PartNameText->SetColorAndOpacity(FSlateColor(bDestroyed
			? FLinearColor(0.64f, 0.62f, 0.58f, 1.0f)
			: FLinearColor(0.96f, 0.92f, 0.82f, 1.0f)));
	}

	if (StatsText)
	{
		StatsText->SetText(FText::FromString(FString::Printf(TEXT("HP %d/%d  SH %d  INIT %d"), View.CurrentHp, View.MaxHp, View.Shield, View.CurrentInitiative)));
		const bool bHasStructuredStats = HpText || ShieldText || InitiativeText;
		StatsText->SetVisibility(bUsingGeneratedFallbackLayout || bHasStructuredStats
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (HpText)
	{
		HpText->SetText(FText::FromString(FString::Printf(TEXT("%d/%d"), View.CurrentHp, View.MaxHp)));
	}

	if (ShieldText)
	{
		ShieldText->SetText(View.Shield > 0
			? FText::AsNumber(View.Shield)
			: FText::GetEmpty());
		ShieldText->SetVisibility(View.Shield > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (ShieldPill)
	{
		ShieldPill->SetVisibility(View.Shield > 0 ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	if (InitiativeText)
	{
		InitiativeText->SetText(FText::AsNumber(View.CurrentInitiative));
	}

	if (IntentText)
	{
		if (View.CurrentIntentDisplayName.IsEmpty())
		{
			IntentText->SetText(FText::FromString(TEXT("No intent")));
		}
		else
		{
			IntentText->SetText(FText::FromString(FString::Printf(TEXT("%s  %d"), *View.CurrentIntentDisplayName.ToString(), View.CurrentIntentInitiative)));
		}
	}

	if (StatusList)
	{
		StatusList->SetStatuses(View.RuntimeStatuses, View.RuntimeStatusStacks);
		if (StatusText)
		{
			StatusText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}
	else if (StatusText)
	{
		const FText Status = BuildStatusText(View);
		StatusText->SetText(Status);
		StatusText->SetVisibility(Status.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
	}

	if (DestroyedOverlay)
	{
		DestroyedOverlay->SetVisibility(bDestroyed ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	ApplyFallbackMotionVisual();
}

FText UWacomBattleEnemyPartEntryWidget::BuildStatusText(const FWacomBattleEnemyPartEntryViewData& View) const
{
	TArray<FString> Parts;
	TArray<FGameplayTag> Tags;
	View.RuntimeStatuses.GetGameplayTagArray(Tags);
	Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		return A.GetTagName().LexicalLess(B.GetTagName());
	});

	for (const FGameplayTag& Tag : Tags)
	{
		const int32* Stack = View.RuntimeStatusStacks.Find(Tag);
		const int32 StackCount = Stack ? *Stack : 0;
		const FString StatusName = UWacomBattleEventPresentationBuilder::FormatStatusName(Tag);
		Parts.Add(StackCount > 1 ? FString::Printf(TEXT("%s x%d"), *StatusName, StackCount) : StatusName);
	}

	return Parts.IsEmpty()
		? FText::GetEmpty()
		: FText::FromString(FString::Printf(TEXT("Status  %s"), *FString::Join(Parts, TEXT(" / "))));
}

void UWacomBattleEnemyPartEntryWidget::SetFallbackIntroDelaySeconds(const float InDelaySeconds)
{
	FallbackIntroDelaySeconds = FMath::Max(0.0f, InDelaySeconds);
}

void UWacomBattleEnemyPartEntryWidget::StartFallbackIntroAnimation()
{
	bFallbackIntroActive = true;
	FallbackIntroElapsedSeconds = 0.0f;
	ApplyFallbackMotionVisual();
}

void UWacomBattleEnemyPartEntryWidget::StartFallbackPulseAnimation(
	const FLinearColor& InPulseTint,
	const float InIntensity)
{
	FallbackPulseTint = InPulseTint;
	FallbackPulseIntensity = FMath::Clamp(InIntensity, 0.0f, 1.0f);
	FallbackPulseElapsedSeconds = 0.0f;
	bFallbackPulseActive = true;
	ApplyFallbackMotionVisual();
}

float UWacomBattleEnemyPartEntryWidget::GetFallbackBaseOpacity() const
{
	const FWacomBattleEnemyPartEntryViewData& View = GetEffectivePartEntryViewData();
	const float PreviewOpacity = bHasActionPreview ? ActionPreviewRenderOpacity : 1.0f;
	return (View.bDestroyed ? 0.64f : 1.0f) * PreviewOpacity;
}

FLinearColor UWacomBattleEnemyPartEntryWidget::GetFallbackBaseBackgroundColor() const
{
	const FWacomBattleEnemyPartEntryViewData& View = GetEffectivePartEntryViewData();
	return View.bDestroyed
		? FLinearColor(0.045f, 0.048f, 0.055f, 0.82f)
		: FLinearColor(0.095f, 0.11f, 0.13f, 0.92f);
}

void UWacomBattleEnemyPartEntryWidget::ApplyFallbackMotionVisual()
{
	float IntroAlpha = 1.0f;
	if (bUsingGeneratedFallbackLayout && bFallbackIntroActive)
	{
		IntroAlpha = FMath::Clamp(
			(FallbackIntroElapsedSeconds - FallbackIntroDelaySeconds) / FallbackIntroDurationSeconds,
			0.0f,
			1.0f);
		IntroAlpha = EaseOutQuint(IntroAlpha);
	}

	float PulseAlpha = 0.0f;
	if (bUsingGeneratedFallbackLayout && bFallbackPulseActive)
	{
		const float NormalizedPulseTime = FMath::Clamp(FallbackPulseElapsedSeconds / FallbackPulseDurationSeconds, 0.0f, 1.0f);
		PulseAlpha = (1.0f - EaseOutQuart(NormalizedPulseTime)) * FallbackPulseIntensity;
	}

	SetRenderOpacity(GetFallbackBaseOpacity() * IntroAlpha);
	SetRenderTranslation(FVector2D(0.0f, FallbackIntroYOffsetPixels * (1.0f - IntroAlpha)));
	const float Scale = FMath::Lerp(FallbackIntroStartScale, 1.0f, IntroAlpha)
		+ (FallbackPulseScaleBoost * PulseAlpha);
	SetRenderScale(FVector2D(Scale, Scale));
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));

	if (EntryBackground)
	{
		const FLinearColor BaseColor = GetFallbackBaseBackgroundColor();
		const FLinearColor PulseColor = FMath::Lerp(BaseColor, FallbackPulseTint, 0.34f);
		EntryBackground->SetBrushColor(FMath::Lerp(BaseColor, PulseColor, PulseAlpha));
	}
}
