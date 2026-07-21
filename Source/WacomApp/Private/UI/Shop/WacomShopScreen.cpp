// Copyright Wacom. All Rights Reserved.

#include "UI/Shop/WacomShopScreen.h"

#define LOCTEXT_NAMESPACE "WacomShopScreen"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"

#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "RunState.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Shop/WacomShopOfferRowListReconciler.h"
#include "UI/Shop/WacomShopOfferRowWidget.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopRefreshGate.h"
#include "UI/Shop/WacomShopScreenFlow.h"
#include "UI/Shop/WacomShopUpgradeRowListReconciler.h"
#include "UI/Shop/WacomShopUpgradeRowWidget.h"

namespace
{
	UTextBlock* MakeShopText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}

}

TSharedRef<SWidget> UWacomShopScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		ResetShopOfferRefreshDirtyGate();
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* PanelBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBg"));
		PanelBg->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.92f));
		PanelBg->SetPadding(FMargin(20.f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBg))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetOffsets(FMargin(-560.f, -360.f, 1120.f, 720.f));
			PanelSlot->SetAutoSize(false);
		}

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		PanelBg->AddChild(RootBox);

		TitleText = MakeShopText(WidgetTree, TEXT("TitleText"), LOCTEXT("Title", "商店"), 30);
		TitleText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
		}

		GoldText = MakeShopText(WidgetTree, TEXT("GoldText"), LOCTEXT("GoldInit", "金币：0"), 18);
		if (UVerticalBoxSlot* GoldSlot = RootBox->AddChildToVerticalBox(GoldText))
		{
			GoldSlot->SetHorizontalAlignment(HAlign_Center);
			GoldSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		}

		UHorizontalBox* Tabs = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Tabs"));
		if (UVerticalBoxSlot* TabsSlot = RootBox->AddChildToVerticalBox(Tabs))
		{
			TabsSlot->SetHorizontalAlignment(HAlign_Center);
			TabsSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
		}
		PurchaseTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("PurchaseTabButton"));
		UTextBlock* PurchaseTabText = MakeShopText(WidgetTree, TEXT("PurchaseTabText"), LOCTEXT("PurchaseTab", "购买"), 18);
		PurchaseTabButton->AddChild(PurchaseTabText);
		if (UHorizontalBoxSlot* TabSlot = Tabs->AddChildToHorizontalBox(PurchaseTabButton))
		{
			TabSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		}
		UpgradeTabButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpgradeTabButton"));
		UTextBlock* UpgradeTabText = MakeShopText(WidgetTree, TEXT("UpgradeTabText"), LOCTEXT("UpgradeTab", "强化"), 18);
		UpgradeTabButton->AddChild(UpgradeTabText);
		Tabs->AddChildToHorizontalBox(UpgradeTabButton);

		PageSwitcher = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass(), TEXT("PageSwitcher"));
		if (UVerticalBoxSlot* SwitcherSlot = RootBox->AddChildToVerticalBox(PageSwitcher))
		{
			SwitcherSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UVerticalBox* PurchasePage = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("PurchasePage"));
		PageSwitcher->AddChild(PurchasePage);
		EmptyText = MakeShopText(WidgetTree, TEXT("EmptyText"), LOCTEXT("Empty", "暂无商品"), 18);
		EmptyText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* EmptySlot = PurchasePage->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetPadding(FMargin(0.f, 12.f));
		}
		UScrollBox* OfferScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OfferScroll"));
		if (UVerticalBoxSlot* ScrollSlot = PurchasePage->AddChildToVerticalBox(OfferScroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		OfferList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("OfferList"));
		OfferScroll->AddChild(OfferList);

		UHorizontalBox* UpgradePage = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("UpgradePage"));
		PageSwitcher->AddChild(UpgradePage);
		UVerticalBox* UpgradeColumn = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeColumn"));
		if (UHorizontalBoxSlot* UpgradeColumnSlot = UpgradePage->AddChildToHorizontalBox(UpgradeColumn))
		{
			UpgradeColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			UpgradeColumnSlot->SetPadding(FMargin(0.f, 0.f, 18.f, 0.f));
		}
		UpgradeEmptyText = MakeShopText(WidgetTree, TEXT("UpgradeEmptyText"), LOCTEXT("UpgradeEmpty", "没有可强化的卡牌"), 18);
		UpgradeEmptyText->SetJustification(ETextJustify::Center);
		UpgradeColumn->AddChildToVerticalBox(UpgradeEmptyText);
		UScrollBox* UpgradeScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("UpgradeScroll"));
		if (UVerticalBoxSlot* UpgradeScrollSlot = UpgradeColumn->AddChildToVerticalBox(UpgradeScroll))
		{
			UpgradeScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UpgradeList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeList"));
		UpgradeScroll->AddChild(UpgradeList);

		UVerticalBox* Details = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("UpgradeDetails"));
		if (UHorizontalBoxSlot* DetailsSlot = UpgradePage->AddChildToHorizontalBox(Details))
		{
			DetailsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UHorizontalBox* CardCompare = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("CardCompare"));
		if (UVerticalBoxSlot* CardCompareSlot = Details->AddChildToVerticalBox(CardCompare))
		{
			CardCompareSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		auto AddCardView = [this, CardCompare](FName BoxName, FName ViewName) -> UWacomCardView*
		{
			USizeBox* Box = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), BoxName);
			Box->SetWidthOverride(210.f);
			Box->SetHeightOverride(300.f);
			UWacomCardView* View = WidgetTree->ConstructWidget<UWacomCardView>(UWacomCardView::StaticClass(), ViewName);
			Box->AddChild(View);
			if (UHorizontalBoxSlot* CardSlot = CardCompare->AddChildToHorizontalBox(Box))
			{
				CardSlot->SetPadding(FMargin(4.f));
			}
			return View;
		};
		CurrentCardView = AddCardView(TEXT("CurrentCardBox"), TEXT("CurrentCardView"));
		NextCardView = AddCardView(TEXT("NextCardBox"), TEXT("NextCardView"));
		UpgradeDetailsText = MakeShopText(WidgetTree, TEXT("UpgradeDetailsText"), LOCTEXT("SelectUpgrade", "选择一张卡牌查看强化差异"), 16);
		UpgradeDetailsText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* UpgradeDetailsSlot = Details->AddChildToVerticalBox(UpgradeDetailsText))
		{
			UpgradeDetailsSlot->SetPadding(FMargin(4.f, 8.f));
		}
		UpgradeActionButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("UpgradeActionButton"));
		UpgradeActionText = MakeShopText(WidgetTree, TEXT("UpgradeActionText"), LOCTEXT("UpgradeActionEmpty", "请选择卡牌"), 17);
		UpgradeActionText->SetJustification(ETextJustify::Center);
		UpgradeActionButton->AddChild(UpgradeActionText);
		if (UVerticalBoxSlot* UpgradeActionSlot = Details->AddChildToVerticalBox(UpgradeActionButton))
		{
			UpgradeActionSlot->SetHorizontalAlignment(HAlign_Center);
			UpgradeActionSlot->SetPadding(FMargin(4.f, 8.f));
		}
		PageSwitcher->SetActiveWidgetIndex(0);

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseText = MakeShopText(WidgetTree, TEXT("CloseText"), LOCTEXT("Close", "关闭"), 18);
		CloseText->SetJustification(ETextJustify::Center);
		CloseButton->AddChild(CloseText);
		if (UVerticalBoxSlot* CloseSlot = RootBox->AddChildToVerticalBox(CloseButton))
		{
			CloseSlot->SetHorizontalAlignment(HAlign_Center);
			CloseSlot->SetPadding(FMargin(0.f, 16.f, 0.f, 0.f));
		}
	}
	return Super::RebuildWidget();
}

void UWacomShopScreen::NativeConstruct()
{
	Super::NativeConstruct();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomShopScreen::HandleCloseClicked);
	}
	if (PurchaseTabButton)
	{
		PurchaseTabButton->OnClicked.AddUniqueDynamic(this, &UWacomShopScreen::HandlePurchaseTabClicked);
	}
	if (UpgradeTabButton)
	{
		UpgradeTabButton->OnClicked.AddUniqueDynamic(this, &UWacomShopScreen::HandleUpgradeTabClicked);
	}
	if (UpgradeActionButton)
	{
		UpgradeActionButton->OnClicked.AddUniqueDynamic(this, &UWacomShopScreen::HandleUpgradeActionClicked);
	}

	TrySubscribeRunSession();
	RefreshShop();
}

void UWacomShopScreen::NativeDestruct()
{
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UWacomShopScreen::HandleCloseClicked);
	}
	if (PurchaseTabButton)
	{
		PurchaseTabButton->OnClicked.RemoveDynamic(this, &UWacomShopScreen::HandlePurchaseTabClicked);
	}
	if (UpgradeTabButton)
	{
		UpgradeTabButton->OnClicked.RemoveDynamic(this, &UWacomShopScreen::HandleUpgradeTabClicked);
	}
	if (UpgradeActionButton)
	{
		UpgradeActionButton->OnClicked.RemoveDynamic(this, &UWacomShopScreen::HandleUpgradeActionClicked);
	}
	UnsubscribeRunSession();
	Super::NativeDestruct();
}

void UWacomShopScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	bDidEndShopVisit = false;
	OwnedShopVisitToken.Invalidate();
	TrySubscribeRunSession();
	RefreshShop();
}

void UWacomShopScreen::NativeOnDeactivated()
{
	UnsubscribeRunSession();
	FWacomShopScreenFlow::EndShopVisitOnDeactivate(
		Cast<AWacomPlayerController>(GetOwningPlayer()),
		ResolveRunSession(),
		OwnedShopVisitToken,
		bDidEndShopVisit);
	OwnedShopVisitToken.Invalidate();
	Super::NativeOnDeactivated();
}

void UWacomShopScreen::RefreshShop()
{
	TrySubscribeRunSession();
	URunSession* Run = ResolveRunSession();
	if (Run && !OwnedShopVisitToken.IsValid())
	{
		OwnedShopVisitToken = Run->GetActiveShopVisitToken();
	}
	FWacomShopRefreshGate& RefreshGate = GetShopRefreshGate();
	if (RefreshGate.SetRunSession(Run))
	{
		CachedShopSnapshot = FRunShopSnapshot();
	}

	FRunShopSnapshot Snapshot;
	const int32 CurrentGold = Run ? Run->GetGold() : 0;
	if (Run)
	{
		if (RefreshGate.BeginSnapshotRefresh(*Run) == EWacomShopSnapshotRefreshResult::ReuseCachedSnapshot)
		{
			Snapshot = CachedShopSnapshot;
		}
		else
		{
			Snapshot = BuildShopSnapshot();
			CachedShopSnapshot = Snapshot;
		}
	}
	else
	{
		Snapshot = FRunShopSnapshot();
		CachedShopSnapshot = Snapshot;
		ResetShopOfferRefreshDirtyGate();
	}

	if (TitleText)
	{
		TitleText->SetText(Snapshot.ShopId.IsNone()
			? LOCTEXT("Title", "商店")
			: FText::Format(LOCTEXT("TitleWithId", "商店：{0}"), FText::FromName(Snapshot.ShopId)));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::Format(
			LOCTEXT("GoldFmt", "金币：{0}"),
			FText::AsNumber(CurrentGold)));
	}

	RebuildOfferRows(Snapshot, CurrentGold);
	RebuildUpgradeRows(Snapshot, CurrentGold);
	if (!Snapshot.CardUpgradeService.bEnabled && ActivePage == EWacomShopPage::Upgrade)
	{
		SetActivePage(EWacomShopPage::Purchase);
	}
	else
	{
		SetActivePage(ActivePage);
	}
}

void UWacomShopScreen::SuppressEndShopVisitOnNextDeactivate()
{
	bDidEndShopVisit = true;
}

void UWacomShopScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

void UWacomShopScreen::HandlePurchaseTabClicked()
{
	SetActivePage(EWacomShopPage::Purchase);
}

void UWacomShopScreen::HandleUpgradeTabClicked()
{
	if (CachedShopSnapshot.CardUpgradeService.bEnabled)
	{
		SetActivePage(EWacomShopPage::Upgrade);
	}
}

void UWacomShopScreen::HandleUpgradeActionClicked()
{
	UpgradeSelectedCard();
}

#if WITH_AUTOMATION_TESTS
bool UWacomShopScreen::PurchaseOfferByIndex(int32 Index)
{
	if (!CachedOfferIds.IsValidIndex(Index))
	{
		return false;
	}
	return PurchaseOffer(CachedOfferIds[Index]);
}

bool UWacomShopScreen::SelectUpgradeByIndex(int32 Index)
{
	if (!CachedUpgradeViews.IsValidIndex(Index))
	{
		return false;
	}
	HandleUpgradeSelectionRequested(CachedUpgradeViews[Index].InstanceId);
	return true;
}

bool UWacomShopScreen::UpgradeSelectedCardForTest()
{
	return UpgradeSelectedCard();
}

FWacomShopCardUpgradePresentationView UWacomShopScreen::GetCachedUpgradeView(int32 Index) const
{
	return CachedUpgradeViews.IsValidIndex(Index)
		? CachedUpgradeViews[Index]
		: FWacomShopCardUpgradePresentationView();
}

FWacomShopOfferPresentationView UWacomShopScreen::GetCachedOfferView(int32 Index) const
{
	return CachedOfferViews.IsValidIndex(Index)
		? CachedOfferViews[Index]
		: FWacomShopOfferPresentationView();
}

FText UWacomShopScreen::BuildPurchaseFailureToastText(FName DisabledReason)
{
	return FWacomShopScreenFlow::BuildPurchaseFailureToastText(DisabledReason);
}

FText UWacomShopScreen::GetDisplayedGoldText() const
{
	return GoldText ? GoldText->GetText() : FText::GetEmpty();
}

UWacomShopOfferRowWidget* UWacomShopScreen::GetOfferRowWidgetForTest(int32 Index) const
{
	return OfferList && Index >= 0 && OfferList->GetChildrenCount() > Index
		? Cast<UWacomShopOfferRowWidget>(OfferList->GetChildAt(Index))
		: nullptr;
}

FWacomShopScreenAutomationTestView UWacomShopScreen::GetAutomationTestViewForTest() const
{
	FWacomShopScreenAutomationTestView View;
	View.DisplayedGoldText = GetDisplayedGoldText();
	View.CachedOfferCount = CachedOfferIds.Num();
	View.CachedUpgradeCount = CachedUpgradeViews.Num();
	View.ActivePage = ActivePage;
	View.SelectedUpgradeInstanceId = SelectedUpgradeInstanceId;
	View.bUpgradeServiceVisible = CachedShopSnapshot.CardUpgradeService.bEnabled;
	const FWacomShopCardUpgradePresentationView* Selected = CachedUpgradeViews.FindByPredicate(
		[this](const FWacomShopCardUpgradePresentationView& Candidate)
		{
			return Candidate.InstanceId == SelectedUpgradeInstanceId;
		});
	View.bUpgradeActionEnabled = Selected && Selected->bCanUpgrade;
	if (!ShopRefreshGate)
	{
		return View;
	}

	const FWacomShopRefreshGateCounters& Counters = ShopRefreshGate->GetCounters();
	View.OfferRefreshApplyCount = Counters.OfferRefreshApplyCount;
	View.OfferRefreshSkipCount = Counters.OfferRefreshSkipCount;
	View.UpgradeRefreshApplyCount = Counters.UpgradeRefreshApplyCount;
	View.UpgradeRefreshSkipCount = Counters.UpgradeRefreshSkipCount;
	View.SnapshotBuildCount = Counters.SnapshotBuildCount;
	View.SnapshotRevisionSkipCount = Counters.SnapshotRevisionSkipCount;
	return View;
}
#endif

void UWacomShopScreen::TrySubscribeRunSession()
{
	URunSession* Run = ResolveRunSession();
	if (SubscribedRunSession.Get() == Run)
	{
		return;
	}

	UnsubscribeRunSession();
	if (Run)
	{
		Run->OnRunStateChangedNative.AddUObject(this, &UWacomShopScreen::HandleRunStateChanged);
		SubscribedRunSession = Run;
	}
}

void UWacomShopScreen::UnsubscribeRunSession()
{
	if (URunSession* Run = SubscribedRunSession.Get())
	{
		Run->OnRunStateChangedNative.RemoveAll(this);
	}
	SubscribedRunSession.Reset();
}

void UWacomShopScreen::HandleRunStateChanged()
{
	RefreshShop();
}

URunSession* UWacomShopScreen::ResolveRunSession() const
{
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer());
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

FRunShopSnapshot UWacomShopScreen::BuildShopSnapshot() const
{
	if (URunSession* Run = ResolveRunSession())
	{
		return Run->BuildCurrentShopSnapshot();
	}
	return FRunShopSnapshot();
}

void UWacomShopScreen::RebuildOfferRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold)
{
	if (EmptyText)
	{
		EmptyText->SetVisibility(Snapshot.Offers.Num() == 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!OfferList)
	{
		CachedOfferIds.Reset();
		CachedOfferViews.Reset();
		return;
	}

	if (!GetShopRefreshGate().ShouldApplyOfferRows(Snapshot, CurrentGold))
	{
		return;
	}

	const TArray<FWacomShopOfferPresentationView> OfferViews =
		UWacomShopPresentationBuilder::BuildOfferPresentationViews(Snapshot, CurrentGold);
	CachedOfferIds.Reset();
	CachedOfferViews = OfferViews;
	CachedOfferIds.Reserve(OfferViews.Num());
	for (const FWacomShopOfferPresentationView& OfferView : OfferViews)
	{
		CachedOfferIds.Add(OfferView.OfferId);
	}

	FWacomShopOfferRowListReconciler::Reconcile(
		OfferList,
		OfferViews,
		[this](const FWacomShopOfferPresentationView& /*OfferView*/) -> UWacomShopOfferRowWidget*
		{
			if (!WidgetTree)
			{
				return nullptr;
			}

			UWacomShopOfferRowWidget* RowWidget = WidgetTree->ConstructWidget<UWacomShopOfferRowWidget>(
				UWacomShopOfferRowWidget::StaticClass());
			if (RowWidget)
			{
				RowWidget->OnPurchaseRequestedNative.AddUObject(this, &UWacomShopScreen::HandleOfferPurchaseRequested);
			}
			return RowWidget;
		},
		[](UWacomShopOfferRowWidget& RowWidget, const FWacomShopOfferPresentationView& OfferView)
		{
			RowWidget.SetOfferPresentationView(OfferView);
		});
}

void UWacomShopScreen::RebuildUpgradeRows(const FRunShopSnapshot& Snapshot, int32 CurrentGold)
{
	const bool bServiceEnabled = Snapshot.CardUpgradeService.bEnabled;
	if (UpgradeTabButton)
	{
		UpgradeTabButton->SetVisibility(bServiceEnabled
			? ESlateVisibility::Visible
			: ESlateVisibility::Collapsed);
		UpgradeTabButton->SetIsEnabled(bServiceEnabled);
	}
	if (!bServiceEnabled)
	{
		CachedUpgradeViews.Reset();
		SelectedUpgradeInstanceId.Invalidate();
		if (UpgradeList)
		{
			UpgradeList->ClearChildren();
		}
		ApplySelectedUpgradePresentation();
		return;
	}

	if (GetShopRefreshGate().ShouldApplyUpgradeRows(Snapshot, CurrentGold))
	{
		CachedUpgradeViews = UWacomShopUpgradePresentationBuilder::BuildUpgradePresentationViews(Snapshot);
		const bool bSelectionStillValid = CachedUpgradeViews.ContainsByPredicate(
			[this](const FWacomShopCardUpgradePresentationView& Candidate)
			{
				return Candidate.InstanceId == SelectedUpgradeInstanceId;
			});
		if (!bSelectionStillValid)
		{
			SelectedUpgradeInstanceId.Invalidate();
		}
		FWacomShopUpgradeRowListReconciler::Reconcile(
			UpgradeList,
			CachedUpgradeViews,
			SelectedUpgradeInstanceId,
			[this](const FWacomShopCardUpgradePresentationView&) -> UWacomShopUpgradeRowWidget*
			{
				if (!WidgetTree)
				{
					return nullptr;
				}
				UWacomShopUpgradeRowWidget* Row = WidgetTree->ConstructWidget<UWacomShopUpgradeRowWidget>(
					UWacomShopUpgradeRowWidget::StaticClass());
				if (Row)
				{
					Row->OnSelectionRequestedNative.AddUObject(
						this, &UWacomShopScreen::HandleUpgradeSelectionRequested);
				}
				return Row;
			});
	}
	if (UpgradeEmptyText)
	{
		UpgradeEmptyText->SetVisibility(CachedUpgradeViews.IsEmpty()
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	ApplySelectedUpgradePresentation();
}

void UWacomShopScreen::ApplySelectedUpgradePresentation()
{
	const FWacomShopCardUpgradePresentationView* Selected = CachedUpgradeViews.FindByPredicate(
		[this](const FWacomShopCardUpgradePresentationView& Candidate)
		{
			return Candidate.InstanceId == SelectedUpgradeInstanceId;
		});
	if (CurrentCardView)
	{
		CurrentCardView->SetVisibility(Selected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		if (Selected)
		{
			CurrentCardView->SetCardViewData(Selected->CurrentCardViewData);
		}
	}
	if (NextCardView)
	{
		NextCardView->SetVisibility(Selected ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
		if (Selected)
		{
			NextCardView->SetCardViewData(Selected->NextCardViewData);
		}
	}
	if (UpgradeDetailsText)
	{
		UpgradeDetailsText->SetText(Selected
			? Selected->ChangeSummaryText
			: LOCTEXT("SelectUpgrade", "选择一张卡牌查看强化差异"));
	}
	if (UpgradeActionText)
	{
		UpgradeActionText->SetText(Selected
			? Selected->ActionText
			: LOCTEXT("UpgradeActionEmpty", "请选择卡牌"));
	}
	if (UpgradeActionButton)
	{
		UpgradeActionButton->SetIsEnabled(Selected && Selected->bCanUpgrade);
	}
}

void UWacomShopScreen::SetActivePage(EWacomShopPage NewPage)
{
	if (NewPage == EWacomShopPage::Upgrade
		&& !CachedShopSnapshot.CardUpgradeService.bEnabled)
	{
		NewPage = EWacomShopPage::Purchase;
	}
	ActivePage = NewPage;
	if (PageSwitcher)
	{
		PageSwitcher->SetActiveWidgetIndex(ActivePage == EWacomShopPage::Purchase ? 0 : 1);
	}
}

void UWacomShopScreen::ResetShopOfferRefreshDirtyGate()
{
	GetShopRefreshGate().Reset();
	CachedShopSnapshot = FRunShopSnapshot();
	CachedUpgradeViews.Reset();
	SelectedUpgradeInstanceId.Invalidate();
	ActivePage = EWacomShopPage::Purchase;
}

FWacomShopRefreshGate& UWacomShopScreen::GetShopRefreshGate()
{
	if (!ShopRefreshGate)
	{
		ShopRefreshGate = MakeShared<FWacomShopRefreshGate>();
	}
	return *ShopRefreshGate;
}

void UWacomShopScreen::HandleOfferPurchaseRequested(FGuid OfferId)
{
	PurchaseOffer(OfferId);
}

void UWacomShopScreen::HandleUpgradeSelectionRequested(FGuid InstanceId)
{
	if (!CachedUpgradeViews.ContainsByPredicate(
		[InstanceId](const FWacomShopCardUpgradePresentationView& Candidate)
		{
			return Candidate.InstanceId == InstanceId;
		}))
	{
		return;
	}
	SelectedUpgradeInstanceId = InstanceId;
	SetActivePage(EWacomShopPage::Upgrade);
	FWacomShopUpgradeRowListReconciler::Reconcile(
		UpgradeList,
		CachedUpgradeViews,
		SelectedUpgradeInstanceId,
		[this](const FWacomShopCardUpgradePresentationView&) -> UWacomShopUpgradeRowWidget*
		{
			if (!WidgetTree)
			{
				return nullptr;
			}
			UWacomShopUpgradeRowWidget* Row = WidgetTree->ConstructWidget<UWacomShopUpgradeRowWidget>(
				UWacomShopUpgradeRowWidget::StaticClass());
			if (Row)
			{
				Row->OnSelectionRequestedNative.AddUObject(
					this, &UWacomShopScreen::HandleUpgradeSelectionRequested);
			}
			return Row;
		});
	ApplySelectedUpgradePresentation();
}

bool UWacomShopScreen::UpgradeSelectedCard()
{
	const FWacomShopCardUpgradePresentationView* Selected = CachedUpgradeViews.FindByPredicate(
		[this](const FWacomShopCardUpgradePresentationView& Candidate)
		{
			return Candidate.InstanceId == SelectedUpgradeInstanceId;
		});
	if (!Selected)
	{
		return false;
	}
	return FWacomShopScreenFlow::UpgradeCard(
		*this,
		Cast<AWacomPlayerController>(GetOwningPlayer()),
		ResolveRunSession(),
		ResolveToastSubsystem(),
		*Selected);
}

bool UWacomShopScreen::PurchaseOffer(FGuid OfferId)
{
	URunSession* Run = ResolveRunSession();
	if (!Run)
	{
		return false;
	}

	return FWacomShopScreenFlow::PurchaseOffer(
		*this,
		Cast<AWacomPlayerController>(GetOwningPlayer()),
		Run,
		ResolveToastSubsystem(),
		OfferId,
		CachedOfferViews);
}

UWacomAppToastSubsystem* UWacomShopScreen::ResolveToastSubsystem() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

#undef LOCTEXT_NAMESPACE
