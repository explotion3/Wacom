// Copyright Wacom. All Rights Reserved.

#include "UI/Battle/CardWidget.h"

#define LOCTEXT_NAMESPACE "WacomCard"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Cards/CardDefinition.h"
#include "Types/WacomEnums.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"

namespace
{
	FText ZoneToText(EHandZone Zone)
	{
		switch (Zone)
		{
		case EHandZone::Left:  return NSLOCTEXT("Wacom.UI", "Zone.L", "L");
		case EHandZone::Both:  return NSLOCTEXT("Wacom.UI", "Zone.B", "双");
		case EHandZone::Right: return NSLOCTEXT("Wacom.UI", "Zone.R", "R");
		default:               return NSLOCTEXT("Wacom.UI", "Zone.None", "-");
		}
	}
}

TSharedRef<SWidget> UCardWidget::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("Root"));
		Root->SetWidthOverride(130.0f);
		Root->SetHeightOverride(170.0f);
		WidgetTree->RootWidget = Root;

		UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Stack"));
		Root->AddChild(Stack);

		HoverVisualRoot = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("HoverVisualRoot"));
		if (UOverlaySlot* VisualSlot = Stack->AddChildToOverlay(HoverVisualRoot))
		{
			VisualSlot->SetHorizontalAlignment(HAlign_Fill);
			VisualSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Layer 0: Frame Border
		FrameBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("FrameBorder"));
		FrameBorder->SetBrushColor(FLinearColor(0.1f, 0.1f, 0.1f, 0.9f));
		FrameBorder->SetPadding(FMargin(6));
		if (UOverlay* VisualRootOverlay = Cast<UOverlay>(HoverVisualRoot))
		{
			if (UOverlaySlot* BSlot = VisualRootOverlay->AddChildToOverlay(FrameBorder))
			{
				BSlot->SetHorizontalAlignment(HAlign_Fill);
				BSlot->SetVerticalAlignment(VAlign_Fill);
			}
		}
		else if (UOverlaySlot* BSlot = Stack->AddChildToOverlay(FrameBorder))
		{
			BSlot->SetHorizontalAlignment(HAlign_Fill);
			BSlot->SetVerticalAlignment(VAlign_Fill);
		}

		// Content inside border
		UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Content"));
		FrameBorder->SetContent(Content);

		CardView = WidgetTree->ConstructWidget<UWacomCardView>(UWacomCardView::StaticClass(), TEXT("CardView"));
		if (UVerticalBoxSlot* CardViewSlot = Content->AddChildToVerticalBox(CardView))
		{
			CardViewSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ZoneText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("ZoneText"));
		ZoneText->SetText(FText::FromString(TEXT("-")));
		ZoneText->SetJustification(ETextJustify::Center);
		ZoneText->SetColorAndOpacity(FSlateColor(FLinearColor(0.7f, 0.7f, 0.9f)));
		Content->AddChildToVerticalBox(ZoneText);
		ZoneText->SetVisibility(ESlateVisibility::Collapsed);

		// Layer 1: 透明按钮铺满，拦截点击
		RootButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RootButton"));
		// 把 button 背景调成透明
		FButtonStyle ButtonStyle = RootButton->GetStyle();
		ButtonStyle.Normal.TintColor   = FSlateColor(FLinearColor(1, 1, 1, 0));
		ButtonStyle.Hovered.TintColor  = FSlateColor(FLinearColor(1, 1, 1, 0.15f));
		ButtonStyle.Pressed.TintColor  = FSlateColor(FLinearColor(1, 1, 1, 0.3f));
		ButtonStyle.Disabled.TintColor = FSlateColor(FLinearColor(1, 1, 1, 0));
		RootButton->SetStyle(ButtonStyle);
		if (UOverlaySlot* BtnSlot = Stack->AddChildToOverlay(RootButton))
		{
			BtnSlot->SetHorizontalAlignment(HAlign_Fill);
			BtnSlot->SetVerticalAlignment(VAlign_Fill);
		}
	}
	return Super::RebuildWidget();
}

void UCardWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	// Button 绑定移到 NativeConstruct。
}

void UCardWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (RootButton)
	{
		if (!RootButton->OnClicked.IsAlreadyBound(this, &UCardWidget::HandleRootButtonClicked))
		{
			RootButton->OnClicked.AddDynamic(this, &UCardWidget::HandleRootButtonClicked);
		}
		if (!RootButton->OnHovered.IsAlreadyBound(this, &UCardWidget::HandleRootButtonHovered))
		{
			RootButton->OnHovered.AddDynamic(this, &UCardWidget::HandleRootButtonHovered);
		}
		if (!RootButton->OnUnhovered.IsAlreadyBound(this, &UCardWidget::HandleRootButtonUnhovered))
		{
			RootButton->OnUnhovered.AddDynamic(this, &UCardWidget::HandleRootButtonUnhovered);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CardWidget] RootButton 未绑定，战斗手牌无法点击：%s"), *GetName());
	}
}

void UCardWidget::NativeDestruct()
{
	RequestUnhover();
	Super::NativeDestruct();
}

void UCardWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
	RequestHover();
}

void UCardWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	RequestUnhover();
	Super::NativeOnMouseLeave(InMouseEvent);
}

void UCardWidget::ApplyCardSnapshot(const FHandCardSnapshot& InSnap)
{
	CachedSnap = InSnap;

	CurrentCardViewData = BuildCardViewDataFromSnapshot(InSnap);

	if (CardView)
	{
		CardView->SetCardViewData(CurrentCardViewData);
	}
	ApplyZoneText(InSnap);

	if (RootButton)
	{
		RootButton->SetIsEnabled(InSnap.bIsPlayable);
	}

	if (bLastPlayable != InSnap.bIsPlayable)
	{
		bLastPlayable = InSnap.bIsPlayable;
		BP_OnPlayableChanged(InSnap.bIsPlayable);
	}

	UpdateFrameColor();
	BP_OnDataApplied(InSnap);
}

bool UCardWidget::IsRootButtonEnabled() const
{
	return RootButton ? RootButton->GetIsEnabled() : false;
}

void UCardWidget::RequestClickForTest()
{
	if (!RootButton || !RootButton->GetIsEnabled())
	{
		return;
	}
	HandleRootButtonClicked();
}

void UCardWidget::RequestHoverForTest()
{
	RequestHover();
}

void UCardWidget::RequestUnhoverForTest()
{
	RequestUnhover();
}

FWidgetTransform UCardWidget::GetHoverVisualRenderTransformForTest() const
{
	const UWidget* Target = GetHoverTransformTarget();
	return Target ? Target->GetRenderTransform() : FWidgetTransform();
}

FVector2D UCardWidget::GetHoverVisualRenderTransformPivotForTest() const
{
	const UWidget* Target = GetHoverTransformTarget();
	return Target ? Target->GetRenderTransformPivot() : FVector2D(0.5f, 0.5f);
}

void UCardWidget::ApplyZoneText(const FHandCardSnapshot& InSnap)
{
	if (ZoneText)
	{
		ZoneText->SetText(ZoneToText(InSnap.Zone));
	}
}

FWacomCardViewData UCardWidget::BuildCardViewDataFromSnapshot(const FHandCardSnapshot& InSnap) const
{
	FWacomCardViewData Data = UWacomCardPresentationBuilder::BuildCardViewData(InSnap.Definition);
	Data.Cost = InSnap.RuntimeCost;
	Data.bShowCost = InSnap.Definition != nullptr;
	Data.bDisabled = !InSnap.bIsPlayable;
	return Data;
}

void UCardWidget::SetTargetingHighlight(bool bTargeting)
{
	if (bLastTargeting == bTargeting) { return; }
	bLastTargeting = bTargeting;
	UpdateFrameColor();
	BP_OnTargetingHighlightChanged(bTargeting);
}

void UCardWidget::UpdateFrameColor()
{
	if (!FrameBorder) { return; }
	if (bLastTargeting)
	{
		FrameBorder->SetBrushColor(FLinearColor(0.9f, 0.7f, 0.1f, 0.95f)); // 黄色高亮
	}
	else if (bLastPlayable)
	{
		FrameBorder->SetBrushColor(FLinearColor(0.1f, 0.3f, 0.15f, 0.95f)); // 深绿
	}
	else
	{
		FrameBorder->SetBrushColor(FLinearColor(0.15f, 0.15f, 0.15f, 0.9f)); // 灰
	}
}

void UCardWidget::HandleRootButtonClicked()
{
	OnCardClicked.Broadcast(CachedSnap.InstanceId);
}

void UCardWidget::HandleRootButtonHovered()
{
	RequestHover();
}

void UCardWidget::HandleRootButtonUnhovered()
{
	RequestUnhover();
}

void UCardWidget::RequestHover()
{
	if (bIsHovered)
	{
		return;
	}

	bIsHovered = true;
	ApplyHoverFeedback();
	BP_OnHoverChanged(true);
	OnCardHoveredNative.Broadcast(this);
}

void UCardWidget::RequestUnhover()
{
	if (!bIsHovered)
	{
		return;
	}

	bIsHovered = false;
	RestoreHoverFeedback();
	BP_OnHoverChanged(false);
	OnCardUnhoveredNative.Broadcast(this);
}

void UCardWidget::ApplyHoverFeedback()
{
	if (!bEnableHoverFeedback)
	{
		return;
	}

	CaptureBaseHoverTransformIfNeeded();

	FWidgetTransform HoverTransform = BaseHoverRenderTransform;
	HoverTransform.Translation += FVector2D(0.0f, -HoverLift);
	HoverTransform.Scale = FVector2D(
		BaseHoverRenderTransform.Scale.X * HoverScale,
		BaseHoverRenderTransform.Scale.Y * HoverScale);

	if (UWidget* Target = GetHoverTransformTarget())
	{
		Target->SetRenderTransformPivot(FVector2D(0.5f, 1.0f));
		Target->SetRenderTransform(HoverTransform);
	}
}

void UCardWidget::RestoreHoverFeedback()
{
	if (!bHasBaseHoverRenderTransform)
	{
		return;
	}

	if (UWidget* Target = CachedHoverTransformTarget.Get())
	{
		Target->SetRenderTransform(BaseHoverRenderTransform);
		Target->SetRenderTransformPivot(BaseHoverRenderTransformPivot);
	}
}

void UCardWidget::CaptureBaseHoverTransformIfNeeded()
{
	UWidget* Target = GetHoverTransformTarget();
	if (!Target)
	{
		return;
	}

	if (bHasBaseHoverRenderTransform && CachedHoverTransformTarget.Get() == Target)
	{
		return;
	}

	CachedHoverTransformTarget = Target;
	BaseHoverRenderTransform = Target->GetRenderTransform();
	BaseHoverRenderTransformPivot = Target->GetRenderTransformPivot();
	bHasBaseHoverRenderTransform = true;
}

UWidget* UCardWidget::GetHoverTransformTarget() const
{
	return HoverVisualRoot ? HoverVisualRoot.Get() : const_cast<UCardWidget*>(this);
}

#undef LOCTEXT_NAMESPACE

