// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/WacomBattleStatusIconWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/BorderSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/Widget.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattleStatusTooltipPresentation.h"
#include "UI/Battle/WacomBattleStatusTooltipWidget.h"

#define LOCTEXT_NAMESPACE "WacomBattleStatusIcons"

namespace
{
	constexpr float DefaultIconSize = 32.0f;
	constexpr float DefaultIconSpacing = 4.0f;

	void ConfigureDefaultBrush(FSlateBrush& Brush, const FLinearColor& Tint)
	{
		Brush.DrawAs = ESlateBrushDrawType::Box;
		Brush.TintColor = FSlateColor(Tint);
		Brush.SetImageSize(FVector2f(DefaultIconSize, DefaultIconSize));
	}

	bool IsBrushConfigured(const FSlateBrush& Brush)
	{
		return Brush.GetResourceObject() != nullptr || Brush.GetDrawType() != ESlateBrushDrawType::NoDrawType;
	}

	bool HasUsableBrushSize(const FSlateBrush& Brush)
	{
		const FVector2f ImageSize = Brush.GetImageSize();
		return ImageSize.X > KINDA_SMALL_NUMBER && ImageSize.Y > KINDA_SMALL_NUMBER;
	}

	void EnsureUsableBrushSize(FSlateBrush& Brush)
	{
		if (!HasUsableBrushSize(Brush))
		{
			Brush.SetImageSize(FVector2f(DefaultIconSize, DefaultIconSize));
		}
	}

	int32 GetStatusSortOrder(const FGameplayTag& Tag)
	{
		if (Tag == WacomTags::Status_Poison) { return 0; }
		if (Tag == WacomTags::Status_Slow) { return 1; }
		if (Tag == WacomTags::Status_Freeze) { return 2; }
		if (Tag == WacomTags::Status_Twilight) { return 3; }
		if (Tag == WacomTags::Status_Stunned) { return 4; }
		return 100;
	}

	FString MakeStatusIconWidgetName(const FWacomBattleStatusIconView& View, const int32 Index)
	{
		FString TagName = View.StatusTag.IsValid()
			? View.StatusTag.GetTagName().ToString()
			: FString(TEXT("Invalid"));
		TagName.ReplaceInline(TEXT("."), TEXT("_"));
		return FString::Printf(TEXT("StatusIcon_%02d_%s"), Index, *TagName);
	}

	void StyleStackText(UTextBlock* TextBlock)
	{
		if (!TextBlock)
		{
			return;
		}

		FSlateFontInfo Font = TextBlock->GetFont();
		Font.Size = 11;
		Font.TypefaceFontName = TEXT("Bold");
		TextBlock->SetFont(Font);
		TextBlock->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		TextBlock->SetJustification(ETextJustify::Center);
		TextBlock->SetShadowOffset(FVector2D(0.0f, 1.0f));
		TextBlock->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.65f));
	}

	bool AreStatusIconViewsEquivalent(
		const TArray<FWacomBattleStatusIconView>& Left,
		const TArray<FWacomBattleStatusIconView>& Right)
	{
		if (Left.Num() != Right.Num())
		{
			return false;
		}

		for (int32 Index = 0; Index < Left.Num(); ++Index)
		{
			if (Left[Index].StatusTag != Right[Index].StatusTag
				|| Left[Index].StackCount != Right[Index].StackCount
				|| Left[Index].InspectionHost != Right[Index].InspectionHost
				|| !Left[Index].DisplayName.EqualTo(Right[Index].DisplayName)
				|| !Left[Index].CoreEffectText.EqualTo(Right[Index].CoreEffectText)
				|| !Left[Index].TriggerTimingText.EqualTo(Right[Index].TriggerTimingText)
				|| !Left[Index].StackPolicyText.EqualTo(Right[Index].StackPolicyText))
			{
				return false;
			}
		}
		return true;
	}
}

UWacomBattleStatusIconWidget::UWacomBattleStatusIconWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StatusTooltipWidgetClass = UWacomBattleStatusTooltipWidget::StaticClass();
	EnsureStatusTooltipBinding();
}

void UWacomBattleStatusIconWidget::SetStatusIconView(const FWacomBattleStatusIconView& InView)
{
	CurrentView = InView;
	bHasAssignedStatusIconView = true;
	if (CachedTooltipWidget)
	{
		CachedTooltipWidget->SetStatusView(CurrentView);
	}
	RefreshDisplay();
}

void UWacomBattleStatusIconWidget::SetStatusInspectionEnabled(const bool bEnabled)
{
	if (bStatusInspectionEnabled == bEnabled)
	{
		return;
	}
	bStatusInspectionEnabled = bEnabled;
	RefreshDisplay();
}

void UWacomBattleStatusIconWidget::SetStatusActivationEnabled(const bool bEnabled)
{
	bStatusActivationEnabled = bEnabled;
}

void UWacomBattleStatusIconWidget::SetStatusTooltipWidgetClass(
	TSubclassOf<UWacomBattleStatusTooltipWidget> InTooltipWidgetClass)
{
	const TSubclassOf<UWacomBattleStatusTooltipWidget> ResolvedClass =
		InTooltipWidgetClass
			? InTooltipWidgetClass
			: TSubclassOf<UWacomBattleStatusTooltipWidget>(
				UWacomBattleStatusTooltipWidget::StaticClass());
	if (StatusTooltipWidgetClass == ResolvedClass)
	{
		return;
	}
	StatusTooltipWidgetClass = ResolvedClass;
	CachedTooltipWidget = nullptr;
}

TSharedRef<SWidget> UWacomBattleStatusIconWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(DefaultIconSize);
		Root->SetHeightOverride(DefaultIconSize);
		WidgetTree->RootWidget = Root;

		UOverlay* Overlay = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Content"));
		Root->AddChild(Overlay);

		IconImage = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), TEXT("IconImage"));
		IconImage->SetColorAndOpacity(FLinearColor::White);
		if (UOverlaySlot* IconSlot = Overlay->AddChildToOverlay(IconImage))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UBorder* GeneratedStackBadge = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("StackBadge"));
		GeneratedStackBadge->SetBrushColor(FLinearColor(0.018f, 0.024f, 0.040f, 0.96f));
		GeneratedStackBadge->SetPadding(FMargin(2.0f, 0.0f));
		StackBadge = GeneratedStackBadge;
		if (UOverlaySlot* BadgeSlot = Overlay->AddChildToOverlay(GeneratedStackBadge))
		{
			BadgeSlot->SetHorizontalAlignment(HAlign_Right);
			BadgeSlot->SetVerticalAlignment(VAlign_Bottom);
		}

		StackText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("StackText"));
		StyleStackText(StackText);
		GeneratedStackBadge->SetContent(StackText);
		if (UBorderSlot* TextSlot = Cast<UBorderSlot>(StackText->Slot))
		{
			TextSlot->SetHorizontalAlignment(HAlign_Center);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	return Super::RebuildWidget();
}

void UWacomBattleStatusIconWidget::SynchronizeProperties()
{
	// Blueprint defaults may deserialize after the native constructor. Bind before
	// UWidget installs tooltip metadata on the cached Slate widget.
	EnsureStatusTooltipBinding();
	Super::SynchronizeProperties();
}

void UWacomBattleStatusIconWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	EnsureStatusTooltipBinding();
	if (IsDesignTime() && bShowDesignTimePreview && !bHasAssignedStatusIconView)
	{
		CurrentView = BuildDesignTimePreviewView();
	}
	RefreshDisplay();
}

FWacomBattleStatusIconView UWacomBattleStatusIconWidget::BuildDesignTimePreviewView() const
{
	FWacomBattleStatusIconView View;
	View.StatusTag = PreviewStatusTag.IsValid()
		? PreviewStatusTag
		: WacomTags::Status_Poison;
	View.DisplayName = PreviewDisplayName.IsEmpty()
		? FText::FromString(UWacomBattleEventPresentationBuilder::FormatStatusName(View.StatusTag))
		: PreviewDisplayName;
	View.StackCount = FMath::Max(1, PreviewStackCount);
	View.InspectionHost = EWacomBattleStatusInspectionHost::Player;
	View.IconBrush = PreviewIconBrush;

	if (!IsBrushConfigured(View.IconBrush) && IconImage)
	{
		View.IconBrush = IconImage->GetBrush();
	}
	if (!IsBrushConfigured(View.IconBrush))
	{
		ConfigureDefaultBrush(View.IconBrush, FLinearColor(0.70f, 0.72f, 0.76f, 1.0f));
	}
	EnsureUsableBrushSize(View.IconBrush);
	FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(View);
	return View;
}

void UWacomBattleStatusIconWidget::EnsureStatusTooltipBinding()
{
	if (!ToolTipWidgetDelegate.IsBound())
	{
		ToolTipWidgetDelegate.BindDynamic(
			this,
			&ThisClass::HandleBuildStatusTooltipWidget);
	}
}

FReply UWacomBattleStatusIconWidget::NativeOnMouseButtonUp(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (bStatusInspectionEnabled
		&& bStatusActivationEnabled
		&& CurrentView.StatusTag.IsValid()
		&& InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		OnStatusIconActivatedNative.Broadcast(CurrentView);
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

UWidget* UWacomBattleStatusIconWidget::HandleBuildStatusTooltipWidget()
{
	if (!bStatusInspectionEnabled || !CurrentView.StatusTag.IsValid())
	{
		return nullptr;
	}

	if (!CachedTooltipWidget)
	{
		UClass* TooltipClass = StatusTooltipWidgetClass
			? StatusTooltipWidgetClass.Get()
			: UWacomBattleStatusTooltipWidget::StaticClass();
		CachedTooltipWidget = GetWorld()
			? CreateWidget<UWacomBattleStatusTooltipWidget>(GetWorld(), TooltipClass)
			: NewObject<UWacomBattleStatusTooltipWidget>(this, TooltipClass);
	}
	if (CachedTooltipWidget)
	{
		CachedTooltipWidget->SetStatusView(CurrentView);
	}
	return CachedTooltipWidget;
}

void UWacomBattleStatusIconWidget::RefreshDisplay()
{
	const bool bHasStatus = CurrentView.StatusTag.IsValid();
	SetVisibility(!bHasStatus
		? ESlateVisibility::Collapsed
		: (bStatusInspectionEnabled
			? ESlateVisibility::Visible
			: ESlateVisibility::HitTestInvisible));

	if (IconImage)
	{
		EnsureUsableBrushSize(CurrentView.IconBrush);
		IconImage->SetBrush(CurrentView.IconBrush);
		IconImage->SetVisibility(bHasStatus ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	const int32 DisplayStack = FMath::Max(1, CurrentView.StackCount);
	if (StackText)
	{
		StackText->SetText(FText::AsNumber(DisplayStack));
		StackText->SetVisibility(bHasStatus ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	if (StackBadge)
	{
		StackBadge->SetVisibility(bHasStatus ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

UWacomBattleStatusIconListWidget::UWacomBattleStatusIconListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	StatusIconWidgetClass = UWacomBattleStatusIconWidget::StaticClass();
	StatusTooltipWidgetClass = UWacomBattleStatusTooltipWidget::StaticClass();

	ConfigureDefaultBrush(PoisonIconBrush, FLinearColor(0.24f, 0.72f, 0.28f, 1.0f));
	ConfigureDefaultBrush(SlowIconBrush, FLinearColor(0.22f, 0.58f, 0.86f, 1.0f));
	ConfigureDefaultBrush(FreezeIconBrush, FLinearColor(0.55f, 0.86f, 1.0f, 1.0f));
	ConfigureDefaultBrush(TwilightIconBrush, FLinearColor(0.58f, 0.36f, 0.86f, 1.0f));
	ConfigureDefaultBrush(StunnedIconBrush, FLinearColor(1.0f, 0.72f, 0.22f, 1.0f));
	ConfigureDefaultBrush(FallbackStatusIconBrush, FLinearColor(0.70f, 0.72f, 0.76f, 1.0f));

	PreviewStatuses.AddTag(WacomTags::Status_Poison);
	PreviewStatuses.AddTag(WacomTags::Status_Slow);
	PreviewStatuses.AddTag(WacomTags::Status_Freeze);
	PreviewStatusStacks.Add(WacomTags::Status_Poison, 4);
	PreviewStatusStacks.Add(WacomTags::Status_Slow, 1);
	PreviewStatusStacks.Add(WacomTags::Status_Freeze, 2);
}

void UWacomBattleStatusIconListWidget::SetStatuses(
	const FGameplayTagContainer& InStatuses,
	const TMap<FGameplayTag, int32>& InStatusStacks)
{
	TArray<FWacomBattleStatusIconView> NextViews =
		BuildStatusIconViews(InStatuses, InStatusStacks);
	if (bHasAssignedStatusIconViews
		&& AreStatusIconViewsEquivalent(CurrentViews, NextViews))
	{
		return;
	}

	CurrentViews = MoveTemp(NextViews);
	bHasAssignedStatusIconViews = true;
	RefreshDisplay();
}

void UWacomBattleStatusIconListWidget::SetStatusIconViews(const TArray<FWacomBattleStatusIconView>& InViews)
{
	CurrentViews = InViews;
	for (FWacomBattleStatusIconView& View : CurrentViews)
	{
		EnsureUsableBrushSize(View.IconBrush);
		NormalizeViewForInspection(View);
	}
	bHasAssignedStatusIconViews = true;
	RefreshDisplay();
}

void UWacomBattleStatusIconListWidget::SetMaxVisibleStatuses(const int32 InMaxVisibleStatuses)
{
	const int32 NewLimit = FMath::Max(0, InMaxVisibleStatuses);
	if (MaxVisibleStatuses == NewLimit)
	{
		return;
	}

	MaxVisibleStatuses = NewLimit;
	RefreshDisplay();
}

void UWacomBattleStatusIconListWidget::SetInspectionHost(
	const EWacomBattleStatusInspectionHost InHost)
{
	if (InspectionHost == InHost)
	{
		return;
	}
	InspectionHost = InHost;
	for (FWacomBattleStatusIconView& View : CurrentViews)
	{
		NormalizeViewForInspection(View);
	}
	RefreshDisplay();
}

void UWacomBattleStatusIconListWidget::SetStatusInspectionEnabled(const bool bEnabled)
{
	if (bStatusInspectionEnabled == bEnabled)
	{
		return;
	}
	bStatusInspectionEnabled = bEnabled;
	RefreshDisplay();
}

void UWacomBattleStatusIconListWidget::SetStatusIconActivationEnabled(const bool bEnabled)
{
	if (bStatusIconActivationEnabled == bEnabled)
	{
		return;
	}
	bStatusIconActivationEnabled = bEnabled;
	for (UWacomBattleStatusIconWidget* IconWidget : IconWidgets)
	{
		if (IconWidget)
		{
			IconWidget->SetStatusActivationEnabled(bStatusIconActivationEnabled);
		}
	}
}

void UWacomBattleStatusIconListWidget::SetStatusTooltipWidgetClass(
	TSubclassOf<UWacomBattleStatusTooltipWidget> InTooltipWidgetClass)
{
	const TSubclassOf<UWacomBattleStatusTooltipWidget> ResolvedClass =
		InTooltipWidgetClass
			? InTooltipWidgetClass
			: TSubclassOf<UWacomBattleStatusTooltipWidget>(
				UWacomBattleStatusTooltipWidget::StaticClass());
	if (StatusTooltipWidgetClass == ResolvedClass)
	{
		return;
	}
	StatusTooltipWidgetClass = ResolvedClass;
	CachedOverflowTooltipWidget = nullptr;
	for (UWacomBattleStatusIconWidget* IconWidget : IconWidgets)
	{
		if (IconWidget)
		{
			IconWidget->SetStatusTooltipWidgetClass(StatusTooltipWidgetClass);
		}
	}
}

TArray<FWacomBattleStatusIconView>
UWacomBattleStatusIconListWidget::GetHiddenStatusIconViews() const
{
	const int32 VisibleStatusCount = MaxVisibleStatuses > 0
		? FMath::Min(MaxVisibleStatuses, CurrentViews.Num())
		: CurrentViews.Num();
	if (VisibleStatusCount >= CurrentViews.Num())
	{
		return {};
	}
	return TArray<FWacomBattleStatusIconView>(
		CurrentViews.GetData() + VisibleStatusCount,
		CurrentViews.Num() - VisibleStatusCount);
}

TSharedRef<SWidget> UWacomBattleStatusIconListWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		StatusContainer = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("StatusContainer"));
		if (UHorizontalBoxSlot* StatusSlot =
			Root->AddChildToHorizontalBox(StatusContainer))
		{
			StatusSlot->SetVerticalAlignment(VAlign_Center);
		}

		OverflowText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("OverflowText"));
		StyleStackText(OverflowText);
		if (UHorizontalBoxSlot* OverflowSlot =
			Root->AddChildToHorizontalBox(OverflowText))
		{
			OverflowSlot->SetPadding(FMargin(DefaultIconSpacing, 0.0f, 0.0f, 0.0f));
			OverflowSlot->SetVerticalAlignment(VAlign_Center);
		}
	}

	EnsureOverflowTooltipBinding();
	return Super::RebuildWidget();
}

void UWacomBattleStatusIconListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	if (IsDesignTime() && bShowDesignTimePreview && !bHasAssignedStatusIconViews)
	{
		CurrentViews = BuildStatusIconViews(PreviewStatuses, PreviewStatusStacks);
	}
	EnsureOverflowTooltipBinding();
	if (OverflowText)
	{
		// The child TextBlock may already own its Slate widget when a Blueprint
		// reaches PreConstruct. Re-sync so the newly bound delegate reaches Slate.
		OverflowText->SynchronizeProperties();
	}
	RefreshDisplay();
}

void UWacomBattleStatusIconListWidget::NativeDestruct()
{
	for (UWacomBattleStatusIconWidget* IconWidget : IconWidgets)
	{
		if (IconWidget)
		{
			IconWidget->OnStatusIconActivatedNative.RemoveAll(this);
		}
	}
	OnStatusIconActivatedNative.Clear();
	CachedOverflowTooltipWidget = nullptr;
	Super::NativeDestruct();
}

TArray<FWacomBattleStatusIconView> UWacomBattleStatusIconListWidget::BuildStatusIconViews(
	const FGameplayTagContainer& InStatuses,
	const TMap<FGameplayTag, int32>& InStatusStacks) const
{
	TArray<FGameplayTag> Tags;
	InStatuses.GetGameplayTagArray(Tags);
	Tags.RemoveAll([](const FGameplayTag& Tag)
	{
		return !Tag.IsValid() || Tag == WacomTags::Status_Shield;
	});
	Tags.Sort([](const FGameplayTag& A, const FGameplayTag& B)
	{
		const int32 AOrder = GetStatusSortOrder(A);
		const int32 BOrder = GetStatusSortOrder(B);
		if (AOrder != BOrder)
		{
			return AOrder < BOrder;
		}
		return A.GetTagName().LexicalLess(B.GetTagName());
	});

	TArray<FWacomBattleStatusIconView> Views;
	Views.Reserve(Tags.Num());
	for (const FGameplayTag& Tag : Tags)
	{
		const int32* Stack = InStatusStacks.Find(Tag);

		FWacomBattleStatusIconView View;
		View.StatusTag = Tag;
		View.DisplayName = FText::FromString(UWacomBattleEventPresentationBuilder::FormatStatusName(Tag));
		View.StackCount = FMath::Max(1, Stack ? *Stack : 1);
		View.IconBrush = ResolveIconBrush(Tag);
		EnsureUsableBrushSize(View.IconBrush);
		NormalizeViewForInspection(View);
		Views.Add(View);
	}

	return Views;
}

const FSlateBrush& UWacomBattleStatusIconListWidget::ResolveIconBrush(FGameplayTag StatusTag) const
{
	const FSlateBrush* Brush = nullptr;
	if (StatusTag == WacomTags::Status_Poison)
	{
		Brush = &PoisonIconBrush;
	}
	else if (StatusTag == WacomTags::Status_Slow)
	{
		Brush = &SlowIconBrush;
	}
	else if (StatusTag == WacomTags::Status_Freeze)
	{
		Brush = &FreezeIconBrush;
	}
	else if (StatusTag == WacomTags::Status_Twilight)
	{
		Brush = &TwilightIconBrush;
	}
	else if (StatusTag == WacomTags::Status_Stunned)
	{
		Brush = &StunnedIconBrush;
	}

	return (Brush && IsBrushConfigured(*Brush)) ? *Brush : FallbackStatusIconBrush;
}

void UWacomBattleStatusIconListWidget::NormalizeViewForInspection(
	FWacomBattleStatusIconView& InOutView) const
{
	InOutView.InspectionHost = InspectionHost;
	FWacomBattleStatusTooltipPresentationBuilder::PopulateRuleText(InOutView);
}

void UWacomBattleStatusIconListWidget::HandleStatusIconActivated(
	const FWacomBattleStatusIconView& View)
{
	if (bStatusInspectionEnabled && bStatusIconActivationEnabled)
	{
		OnStatusIconActivatedNative.Broadcast(View);
	}
}

void UWacomBattleStatusIconListWidget::EnsureOverflowTooltipBinding()
{
	if (OverflowText && !OverflowText->ToolTipWidgetDelegate.IsBound())
	{
		OverflowText->ToolTipWidgetDelegate.BindDynamic(
			this,
			&ThisClass::HandleBuildOverflowTooltipWidget);
	}
}

UWidget* UWacomBattleStatusIconListWidget::HandleBuildOverflowTooltipWidget()
{
	const TArray<FWacomBattleStatusIconView> HiddenViews = GetHiddenStatusIconViews();
	if (!bStatusInspectionEnabled || HiddenViews.IsEmpty())
	{
		return nullptr;
	}

	if (!CachedOverflowTooltipWidget)
	{
		UClass* TooltipClass = StatusTooltipWidgetClass
			? StatusTooltipWidgetClass.Get()
			: UWacomBattleStatusTooltipWidget::StaticClass();
		CachedOverflowTooltipWidget = GetWorld()
			? CreateWidget<UWacomBattleStatusTooltipWidget>(GetWorld(), TooltipClass)
			: NewObject<UWacomBattleStatusTooltipWidget>(this, TooltipClass);
	}
	if (CachedOverflowTooltipWidget)
	{
		CachedOverflowTooltipWidget->SetOverflowViews(HiddenViews);
	}
	return CachedOverflowTooltipWidget;
}

void UWacomBattleStatusIconListWidget::RefreshDisplay()
{
	SetVisibility(CurrentViews.IsEmpty()
		? ESlateVisibility::Collapsed
		: (bStatusInspectionEnabled
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::HitTestInvisible));
	const int32 VisibleStatusCount = MaxVisibleStatuses > 0
		? FMath::Min(MaxVisibleStatuses, CurrentViews.Num())
		: CurrentViews.Num();
	OverflowStatusCount = FMath::Max(0, CurrentViews.Num() - VisibleStatusCount);
	if (OverflowText)
	{
		OverflowText->SetText(OverflowStatusCount > 0
			? FText::FromString(FString::Printf(TEXT("+%d"), OverflowStatusCount))
			: FText::GetEmpty());
		OverflowText->SetVisibility(OverflowStatusCount > 0
			? (bStatusInspectionEnabled
				? ESlateVisibility::Visible
				: ESlateVisibility::HitTestInvisible)
			: ESlateVisibility::Collapsed);
	}

	if (!StatusContainer || !WidgetTree)
	{
		return;
	}

	if (WidgetTree->RootWidget && WidgetTree->RootWidget != StatusContainer)
	{
		WidgetTree->RootWidget->SetVisibility(bStatusInspectionEnabled
			? ESlateVisibility::SelfHitTestInvisible
			: ESlateVisibility::HitTestInvisible);
	}
	StatusContainer->ClearChildren();
	StatusContainer->SetVisibility(bStatusInspectionEnabled
		? ESlateVisibility::SelfHitTestInvisible
		: ESlateVisibility::HitTestInvisible);
	IconWidgets.Reset();

	UClass* EntryClass = StatusIconWidgetClass
		? StatusIconWidgetClass.Get()
		: UWacomBattleStatusIconWidget::StaticClass();

	for (int32 Index = 0; Index < VisibleStatusCount; ++Index)
	{
		const FWacomBattleStatusIconView& View = CurrentViews[Index];
		UWacomBattleStatusIconWidget* IconWidget =
			WidgetTree->ConstructWidget<UWacomBattleStatusIconWidget>(
				EntryClass,
				*MakeStatusIconWidgetName(View, Index));
		if (!IconWidget)
		{
			continue;
		}

		IconWidget->SetStatusTooltipWidgetClass(StatusTooltipWidgetClass);
		IconWidget->SetStatusInspectionEnabled(bStatusInspectionEnabled);
		IconWidget->SetStatusActivationEnabled(bStatusIconActivationEnabled);
		IconWidget->OnStatusIconActivatedNative.RemoveAll(this);
		IconWidget->OnStatusIconActivatedNative.AddUObject(
			this,
			&ThisClass::HandleStatusIconActivated);
		IconWidget->SetStatusIconView(View);
		IconWidgets.Add(IconWidget);

		if (UHorizontalBox* HorizontalContainer = Cast<UHorizontalBox>(StatusContainer))
		{
			if (UHorizontalBoxSlot* IconSlot = HorizontalContainer->AddChildToHorizontalBox(IconWidget))
			{
				IconSlot->SetPadding(FMargin(Index == 0 ? 0.0f : DefaultIconSpacing, 0.0f, 0.0f, 0.0f));
				IconSlot->SetVerticalAlignment(VAlign_Center);
			}
		}
		else
		{
			StatusContainer->AddChild(IconWidget);
		}
	}
}

#undef LOCTEXT_NAMESPACE
