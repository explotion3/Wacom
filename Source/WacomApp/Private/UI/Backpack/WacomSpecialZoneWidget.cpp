// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomSpecialZoneWidget.h"

#define LOCTEXT_NAMESPACE "WacomSpecialZone"

#include "Blueprint/DragDropOperation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"

#include "Cards/CardDefinition.h"
#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"

namespace
{
UTextBlock* CreateSpecialZoneText(UWidgetTree* WidgetTree, FName Name, const FText& Text, int32 FontSize)
{
	UTextBlock* TextBlock = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
	TextBlock->SetText(Text);
	FSlateFontInfo Font = TextBlock->GetFont();
	Font.Size = FontSize;
	TextBlock->SetFont(Font);
	return TextBlock;
}

void LogSpecialZoneBindingWarningOnce(FName Key, const TCHAR* Message)
{
	static TSet<FName> LoggedKeys;
	if (!LoggedKeys.Contains(Key))
	{
		LoggedKeys.Add(Key);
		UE_LOG(LogTemp, Warning, TEXT("%s"), Message);
	}
}

UPanelWidget* GetRootPanel(UWidgetTree* WidgetTree)
{
	return WidgetTree ? Cast<UPanelWidget>(WidgetTree->RootWidget) : nullptr;
}
}

void UWacomSpecialZoneWidget::SetSpecialZoneView(
	const FRunSpecialStorageView& InView,
	UWacomBackpackScreen* InOwnerScreen,
	TSubclassOf<UWacomDeckCardWidget> InCardWidgetClass)
{
	CurrentView = InView;
	OwnerScreen = InOwnerScreen;
	CardWidgetClass = InCardWidgetClass;
	if (!CardWidgetClass)
	{
		CardWidgetClass = UWacomDeckCardWidget::StaticClass();
	}

	EnsureRuntimeWidgets();
	RebuildFromCurrentView();
}

#if WITH_AUTOMATION_TESTS
FText UWacomSpecialZoneWidget::GetZoneTitleTextForTest() const
{
	return TitleText ? TitleText->GetText() : FText::GetEmpty();
}

bool UWacomSpecialZoneWidget::IsBattleReadyBadgeVisibleForTest() const
{
	return BattleReadyBadge && BattleReadyBadge->GetVisibility() != ESlateVisibility::Collapsed;
}

UDragDropOperation* UWacomSpecialZoneWidget::BuildOwnerCardDragOperationForTest() const
{
	return OwnerCardWidget ? OwnerCardWidget->BuildDragOperation() : nullptr;
}

UDragDropOperation* UWacomSpecialZoneWidget::BuildContentCardDragOperationForTest(int32 Index) const
{
	return ContentCardWidgets.IsValidIndex(Index) && ContentCardWidgets[Index]
		? ContentCardWidgets[Index]->BuildDragOperation()
		: nullptr;
}

bool UWacomSpecialZoneWidget::RequestContentCardBattleEnabledToggleForTest(int32 Index) const
{
	return ContentCardWidgets.IsValidIndex(Index) && ContentCardWidgets[Index]
		? ContentCardWidgets[Index]->RequestBattleEnabledToggle()
		: false;
}

UWacomDeckCardWidget* UWacomSpecialZoneWidget::GetContentCardWidgetForTest(int32 Index) const
{
	return ContentCardWidgets.IsValidIndex(Index) ? ContentCardWidgets[Index] : nullptr;
}
#endif

FGuid UWacomSpecialZoneWidget::GetOwnerCardInstanceId() const
{
	return CurrentView.OwnerCard.Instance.InstanceId;
}

bool UWacomSpecialZoneWidget::ContainsCardWidget(const UWacomDeckCardWidget* Widget) const
{
	if (!Widget)
	{
		return false;
	}

	if (OwnerCardWidget == Widget)
	{
		return true;
	}

	return ContentCardWidgets.ContainsByPredicate(
		[Widget](const TObjectPtr<UWacomDeckCardWidget>& Candidate)
		{
			return Candidate.Get() == Widget;
		});
}

TSharedRef<SWidget> UWacomSpecialZoneWidget::RebuildWidget()
{
	EnsureRuntimeWidgets();
	return Super::RebuildWidget();
}

void UWacomSpecialZoneWidget::EnsureRuntimeWidgets()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_SpecialZone"));
	}

	if (!WidgetTree->RootWidget)
	{
		UBorder* RootBorder = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SpecialZoneBorder"));
		RootBorder->SetBrushColor(FLinearColor(0.11f, 0.09f, 0.16f, 0.82f));
		RootBorder->SetPadding(FMargin(12.f, 10.f));
		WidgetTree->RootWidget = RootBorder;

		UVerticalBox* ZoneVBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("SpecialZoneVBox"));
		RootBorder->AddChild(ZoneVBox);

		UHorizontalBox* TitleRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SpecialZoneTitleRow"));
		if (UVerticalBoxSlot* TitleRowSlot = ZoneVBox->AddChildToVerticalBox(TitleRow))
		{
			TitleRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 6.f));
		}

		TitleText = CreateSpecialZoneText(
			WidgetTree,
			TEXT("TitleText"),
			LOCTEXT("SpecialZoneTitlePlaceholder", "[ 特殊存放区 ]"),
			15);
		if (UHorizontalBoxSlot* TitleSlot = TitleRow->AddChildToHorizontalBox(TitleText))
		{
			TitleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			TitleSlot->SetVerticalAlignment(VAlign_Center);
		}

		BattleReadyBadge = CreateSpecialZoneText(
			WidgetTree,
			TEXT("BattleReadyBadge"),
			LOCTEXT("BattleReadyBadge", "已入战"),
			13);
		BattleReadyBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.75f, 1.f, 1.f)));
		BattleReadyBadge->SetVisibility(ESlateVisibility::Collapsed);
		if (UHorizontalBoxSlot* BadgeSlot = TitleRow->AddChildToHorizontalBox(BattleReadyBadge))
		{
			BadgeSlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
			BadgeSlot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBox* MainCardRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("SpecialZoneMainCardRow"));
		if (UVerticalBoxSlot* MainCardRowSlot = ZoneVBox->AddChildToVerticalBox(MainCardRow))
		{
			MainCardRowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		UTextBlock* MainCardLabel = CreateSpecialZoneText(
			WidgetTree,
			TEXT("SpecialZoneMainCardLabel"),
			LOCTEXT("SpecialZoneMainCardLabel", "主卡"),
			13);
		if (UHorizontalBoxSlot* LabelSlot = MainCardRow->AddChildToHorizontalBox(MainCardLabel))
		{
			LabelSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		OwnerCardHost = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("OwnerCardHost"));
		if (UWrapBox* OwnerWrapBox = Cast<UWrapBox>(OwnerCardHost))
		{
			OwnerWrapBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		}
		if (UHorizontalBoxSlot* OwnerSlot = MainCardRow->AddChildToHorizontalBox(OwnerCardHost))
		{
			OwnerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ContentDropTargetHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentDropTargetHost"));
		if (UVerticalBoxSlot* ContentHostSlot = ZoneVBox->AddChildToVerticalBox(ContentDropTargetHost))
		{
			ContentHostSlot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}
	}

	if (!TitleText)
	{
		if (UPanelWidget* RootPanel = GetRootPanel(WidgetTree))
		{
			TitleText = CreateSpecialZoneText(
				WidgetTree,
				TEXT("TitleText_Runtime"),
				LOCTEXT("SpecialZoneTitlePlaceholder", "[ 特殊存放区 ]"),
				15);
			RootPanel->AddChild(TitleText);
			LogSpecialZoneBindingWarningOnce(TEXT("SpecialZoneTitleText"), TEXT("[SpecialZone] TitleText 未绑定，已追加 C++ fallback 标题"));
		}
		else
		{
			LogSpecialZoneBindingWarningOnce(TEXT("SpecialZoneTitleTextMissing"), TEXT("[SpecialZone] TitleText 未绑定，且 RootWidget 不是 PanelWidget，标题无法显示"));
		}
	}

	if (!BattleReadyBadge)
	{
		if (UPanelWidget* RootPanel = GetRootPanel(WidgetTree))
		{
			BattleReadyBadge = CreateSpecialZoneText(
				WidgetTree,
				TEXT("BattleReadyBadge_Runtime"),
				LOCTEXT("BattleReadyBadge", "已入战"),
				13);
			BattleReadyBadge->SetColorAndOpacity(FSlateColor(FLinearColor(0.2f, 0.75f, 1.f, 1.f)));
			BattleReadyBadge->SetVisibility(ESlateVisibility::Collapsed);
			RootPanel->AddChild(BattleReadyBadge);
			LogSpecialZoneBindingWarningOnce(TEXT("BattleReadyBadge"), TEXT("[SpecialZone] BattleReadyBadge 未绑定，已追加 C++ fallback 标记"));
		}
	}

	if (!OwnerCardHost)
	{
		if (UPanelWidget* RootPanel = GetRootPanel(WidgetTree))
		{
			OwnerCardHost = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("OwnerCardHost_Runtime"));
			if (UWrapBox* OwnerWrapBox = Cast<UWrapBox>(OwnerCardHost))
			{
				OwnerWrapBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
			}
			RootPanel->AddChild(OwnerCardHost);
			LogSpecialZoneBindingWarningOnce(TEXT("OwnerCardHost"), TEXT("[SpecialZone] OwnerCardHost 未绑定，已追加 C++ fallback 主卡槽"));
		}
		else
		{
			LogSpecialZoneBindingWarningOnce(TEXT("OwnerCardHostMissing"), TEXT("[SpecialZone] OwnerCardHost 未绑定，且 RootWidget 不是 PanelWidget，主卡无法显示"));
		}
	}

	if (!ContentDropTargetHost)
	{
		if (UPanelWidget* RootPanel = GetRootPanel(WidgetTree))
		{
			ContentDropTargetHost = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ContentDropTargetHost_Runtime"));
			RootPanel->AddChild(ContentDropTargetHost);
			LogSpecialZoneBindingWarningOnce(TEXT("ContentDropTargetHost"), TEXT("[SpecialZone] ContentDropTargetHost 未绑定，已追加 C++ fallback 内容槽"));
		}
		else
		{
			LogSpecialZoneBindingWarningOnce(TEXT("ContentDropTargetHostMissing"), TEXT("[SpecialZone] ContentDropTargetHost 未绑定，且 RootWidget 不是 PanelWidget，内容区无法显示"));
		}
	}

	if (!ContentDropTarget && ContentDropTargetHost)
	{
		ContentDropTargetHost->ClearChildren();
		ContentDropTarget = WidgetTree->ConstructWidget<UWacomZoneDropTarget>(UWacomZoneDropTarget::StaticClass(), TEXT("SpecialZoneDropTarget"));
		ContentDropTarget->SetOwnerScreen(OwnerScreen.Get());

		if (!ContentCardsBox)
		{
			ContentCardsBox = WidgetTree->ConstructWidget<UWrapBox>(UWrapBox::StaticClass(), TEXT("ContentCardsBox"));
			ContentCardsBox->SetInnerSlotPadding(FVector2D(8.f, 8.f));
		}

		USizeBox* SpecialSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("SpecialZoneDropContent"));
		SpecialSize->SetMinDesiredHeight(220.f);
		SpecialSize->AddChild(ContentCardsBox);
		ContentDropTarget->SetDropContent(SpecialSize);
		ContentDropTargetHost->AddChild(ContentDropTarget);
	}
	else if (ContentDropTarget)
	{
		ContentDropTarget->SetOwnerScreen(OwnerScreen.Get());
	}
}

void UWacomSpecialZoneWidget::RebuildFromCurrentView()
{
	EnsureRuntimeWidgets();

	const UCardDefinition* OwnerCard = CurrentView.OwnerCard.Instance.Definition;
	const FText OwnerName = OwnerCard ? OwnerCard->DisplayName : LOCTEXT("UnknownSpecialZoneOwner", "未知主卡");
	if (TitleText)
	{
		TitleText->SetText(UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(
			OwnerName,
			CurrentView.ContentCards.Num(),
			CurrentView.Capacity));
	}

	if (BattleReadyBadge)
	{
		BattleReadyBadge->SetVisibility(UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(CurrentView.OwnerCard.PhysicalZone));
	}

	if (ContentDropTarget)
	{
		ContentDropTarget->Configure(EZoneKind::SpecialZone, CurrentView.OwnerCard.Instance.InstanceId);
		ContentDropTarget->SetOwnerScreen(OwnerScreen.Get());
	}

	TArray<FWacomBackpackDeckCardListItem> OwnerItems;
	FWacomBackpackDeckCardListItem OwnerItem;
	OwnerItem.CardView = CurrentView.OwnerCard;
	OwnerItem.Role = EWacomBackpackDeckCardListReuseRole::SpecialOwner;
	OwnerItem.ProjectedBadgeText = CurrentView.bOwnerInBattleDeck
		? LOCTEXT("SpecialZoneOwnerInBattleDeck", "已出战")
		: FText::GetEmpty();
	OwnerItems.Add(MoveTemp(OwnerItem));

	TArray<TObjectPtr<UWacomDeckCardWidget>> OwnerWidgets;
	FWacomBackpackDeckCardListReconciler::Reconcile(
		OwnerCardHost,
		OwnerItems,
		[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
		[this](UWacomDeckCardWidget* RemovedWidget) { OnCardUnhoveredNative.Broadcast(RemovedWidget); },
		&OwnerWidgets);
	OwnerCardWidget = OwnerWidgets.IsValidIndex(0) ? OwnerWidgets[0].Get() : nullptr;

	TArray<FWacomBackpackDeckCardListItem> DesiredContentCards;
	DesiredContentCards.Reserve(CurrentView.ContentCards.Num());
	for (const FRunStorageCardView& CardView : CurrentView.ContentCards)
	{
		FWacomBackpackDeckCardListItem Desired;
		Desired.CardView = CardView;
		Desired.Role = EWacomBackpackDeckCardListReuseRole::SpecialContent;
		Desired.bRightClickToggleEnabled = true;
		DesiredContentCards.Add(MoveTemp(Desired));
	}
	FWacomBackpackDeckCardListReconciler::Reconcile(
		ContentCardsBox,
		DesiredContentCards,
		[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
		[this](UWacomDeckCardWidget* RemovedWidget) { OnCardUnhoveredNative.Broadcast(RemovedWidget); },
		&ContentCardWidgets);
}

UWacomDeckCardWidget* UWacomSpecialZoneWidget::CreateCardWidget(const FRunStorageCardView& CardView)
{
	UClass* ClassToUse = CardWidgetClass ? CardWidgetClass.Get() : UWacomDeckCardWidget::StaticClass();
	UWacomDeckCardWidget* CardWidget = GetWorld()
		? CreateWidget<UWacomDeckCardWidget>(this, ClassToUse)
		: NewObject<UWacomDeckCardWidget>(this, ClassToUse);
	if (!CardWidget)
	{
		return nullptr;
	}

	CardWidget->SetCard(CardView.Instance, CardView.PhysicalZone, CardView.ZoneOwnerInstanceId);
	CardWidget->SetMoveEnabled(true);
	CardWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomSpecialZoneWidget::HandleBattleEnabledToggleRequested);
	CardWidget->OnCardHoveredNative.AddUObject(this, &UWacomSpecialZoneWidget::HandleCardHovered);
	CardWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomSpecialZoneWidget::HandleCardUnhovered);
	return CardWidget;
}

void UWacomSpecialZoneWidget::HandleBattleEnabledToggleRequested(FGuid InstanceId)
{
	OnBattleEnabledToggleRequestedNative.Broadcast(InstanceId);
}

void UWacomSpecialZoneWidget::HandleCardHovered(UWacomDeckCardWidget* SourceWidget)
{
	OnCardHoveredNative.Broadcast(SourceWidget);
}

void UWacomSpecialZoneWidget::HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget)
{
	OnCardUnhoveredNative.Broadcast(SourceWidget);
}

#undef LOCTEXT_NAMESPACE
