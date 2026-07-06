// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreen.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Misc/PackageName.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"
#include "UI/Backpack/BackpackRuntimeZoneBuilder.h"
#include "UI/Backpack/WacomBackpackCardDetailController.h"
#include "UI/Backpack/WacomBackpackCommandFlow.h"
#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/Backpack/WacomBackpackStorageRefreshGate.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

namespace
{
template <typename TWidget>
TSubclassOf<TWidget> LoadOptionalWidgetClass(const TCHAR* ClassPath);
}

UWacomBackpackScreen::UWacomBackpackScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!CardWidgetClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C")))
		{
			CardWidgetClass = Loaded;
		}
		else
		{
			CardWidgetClass = UWacomDeckCardWidget::StaticClass();
		}
	}

	if (!SpecialZoneWidgetClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Backpack/WBP_WacomSpecialZoneWidget.WBP_WacomSpecialZoneWidget_C")))
		{
			SpecialZoneWidgetClass = Loaded;
		}
		else
		{
			SpecialZoneWidgetClass = UWacomSpecialZoneWidget::StaticClass();
		}
	}

	if (!CardDetailPanelClass)
	{
		if (UClass* Loaded = LoadObject<UClass>(
			nullptr,
			TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C")))
		{
			CardDetailPanelClass = Loaded;
		}
		else
		{
			CardDetailPanelClass = UWacomCardDetailPanel::StaticClass();
		}
	}

	if (!DeleteZoneSectionWidgetClass)
	{
		DeleteZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackDeleteZone.WBP_BackpackDeleteZone_C"));
	}
	if (!BattleDeckZoneSectionWidgetClass)
	{
		BattleDeckZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackBattleDeckZone.WBP_BackpackBattleDeckZone_C"));
	}
	if (!FluxContentZoneSectionWidgetClass)
	{
		FluxContentZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackFluxContentZone.WBP_BackpackFluxContentZone_C"));
	}
	if (!SpecialZonesSectionWidgetClass)
	{
		SpecialZonesSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackSpecialZones.WBP_BackpackSpecialZones_C"));
	}
	if (!BurdenZoneSectionWidgetClass)
	{
		BurdenZoneSectionWidgetClass = LoadOptionalWidgetClass<UWacomBackpackZoneSectionWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackBurdenZone.WBP_BackpackBurdenZone_C"));
	}
}

namespace
{
template <typename TWidget>
TSubclassOf<TWidget> LoadOptionalWidgetClass(const TCHAR* ClassPath)
{
	const FString ObjectPath(ClassPath);
	const FString PackagePath = FPackageName::ObjectPathToPackageName(ObjectPath);
	if (!FPackageName::DoesPackageExist(PackagePath))
	{
		return TWidget::StaticClass();
	}

	if (UClass* Loaded = LoadObject<UClass>(nullptr, ClassPath))
	{
		return Loaded;
	}
	return TWidget::StaticClass();
}

}

TSharedRef<SWidget> UWacomBackpackScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		ResetBackpackRefreshDirtyGate();
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}
		FBackpackFallbackLayoutBuilder::Build(FBackpackFallbackLayoutBuilderContext{
			this,
			WidgetTree,
			DeleteZoneSectionWidgetClass,
			BattleDeckZoneSectionWidgetClass,
			FluxContentZoneSectionWidgetClass,
			SpecialZonesSectionWidgetClass,
			BurdenZoneSectionWidgetClass,
			&TitleText,
			&GoldText,
			&BackpackTitleText,
			&DeleteZoneHost,
			&BattleDeckZoneHost,
			&FluxContentDropTargetHost,
			&SpecialZonesHost,
			&BurdenZoneHost,
			&CardDetailLayer,
			&CloseButton,
			&BattleDeckZoneSection,
			&FluxContentZoneSection,
			&BurdenZoneSection
		});
	}
	return Super::RebuildWidget();
}

void UWacomBackpackScreen::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureRuntimeZoneWidgets();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackScreen::HandleCloseClicked);
	}

	TrySubscribeAndRefresh();
}

void UWacomBackpackScreen::NativeDestruct()
{
	if (CardDetailController)
	{
		CardDetailController->Hide();
		CardDetailController.Reset();
	}
	else
	{
		if (CardDetailPanel)
		{
			CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
		}
		CardDetailSourceWidget = nullptr;
	}

	if (UWacomRunViewModelProvider* Provider = SubscribedProvider.Get())
	{
		Provider->OnRunViewModelRefreshedNative.RemoveAll(this);
	}
	SubscribedProvider = nullptr;

	Super::NativeDestruct();
}

void UWacomBackpackScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	// CommonUI Stack 重新激活时（背包从 GameMenu 顶层重新显示），事件订阅可能错过期间的广播；
	// 无条件刷新一次保底。
	TrySubscribeAndRefresh();
}

void UWacomBackpackScreen::TrySubscribeAndRefresh()
{
	EnsureRuntimeZoneWidgets();

	if (!SubscribedProvider.Get())
	{
		if (UWacomRunViewModelProvider* Provider = GetProvider())
		{
			Provider->OnRunViewModelRefreshedNative.AddUObject(
				this, &UWacomBackpackScreen::HandleViewModelRefreshed);
			SubscribedProvider = Provider;
		}
	}
	RebuildAll();
}

void UWacomBackpackScreen::HandleViewModelRefreshed()
{
	RebuildAll();
}

UWacomRunViewModelProvider* UWacomBackpackScreen::GetProvider() const
{
	UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomRunViewModelProvider>() : nullptr;
}

UWacomRunViewModel* UWacomBackpackScreen::GetViewModel() const
{
	UWacomRunViewModelProvider* Provider = GetProvider();
	return Provider ? Provider->GetRunViewModel() : nullptr;
}

URunSession* UWacomBackpackScreen::GetRunSession() const
{
	return ResolveRunSession();
}

URunSession* UWacomBackpackScreen::ResolveRunSession() const
{
#if WITH_AUTOMATION_TESTS
	if (RunSessionOverrideForTest)
	{
		return RunSessionOverrideForTest;
	}
#endif
	APlayerController* PC = GetOwningPlayer();
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(PC);
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

FText UWacomBackpackScreen::BuildSpecialZoneTitleText(const FText& OwnerName, int32 CardCount, int32 Capacity)
{
	return UWacomBackpackScreenPresenter::BuildSpecialZoneTitleText(OwnerName, CardCount, Capacity);
}

ESlateVisibility UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind OwnerZone)
{
	return UWacomBackpackScreenPresenter::GetSpecialZoneBattleReadyBadgeVisibility(OwnerZone);
}

FText UWacomBackpackScreen::BuildBurdenZoneTitleText(int32 CardCount)
{
	return UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(CardCount);
}

FVector2D UWacomBackpackScreen::ComputeCardDetailPanelPosition(
	FVector2D AnchorPosition,
	FVector2D AnchorSize,
	FVector2D LayerSize,
	FVector2D PanelSize,
	float Padding)
{
	return UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		PanelSize,
		Padding);
}

bool UWacomBackpackScreen::IsCardDetailPanelVisible() const
{
	return GetCardDetailController().IsVisible();
}

FText UWacomBackpackScreen::GetCardDetailPanelNameText() const
{
	return GetCardDetailController().GetNameText();
}

bool UWacomBackpackScreen::HandleZoneDropRequested(const UWacomCardDragOperation& CardOp, EZoneKind TargetZone, FGuid TargetZoneOwnerInstanceId)
{
	return FWacomBackpackCommandFlow::HandleZoneDropRequested(
		*this,
		GetRunSession(),
		CardOp,
		TargetZone,
		TargetZoneOwnerInstanceId);
}

bool UWacomBackpackScreen::HandleDeleteDropRequested(const UWacomCardDragOperation& CardOp)
{
	return FWacomBackpackCommandFlow::HandleDeleteDropRequested(*this, GetRunSession(), CardOp);
}

bool UWacomBackpackScreen::CanPreviewZoneDrop(
	const UWacomCardDragOperation& CardOp,
	EZoneKind TargetZone,
	FGuid TargetZoneOwnerInstanceId) const
{
	return FWacomBackpackCommandFlow::ValidateZoneDropPreview(
		GetRunSession(),
		CardOp,
		TargetZone,
		TargetZoneOwnerInstanceId).bCanExecute;
}

bool UWacomBackpackScreen::CanPreviewDeleteDrop(const UWacomCardDragOperation& CardOp) const
{
	return FWacomBackpackCommandFlow::ValidateDeleteDropPreview(GetRunSession(), CardOp).bCanExecute;
}

void UWacomBackpackScreen::EnsureRuntimeZoneWidgets()
{
	FBackpackRuntimeZoneBuilder::Ensure(FBackpackRuntimeZoneBuilderContext{
		this,
		WidgetTree,
		&CardDetailLayer,
		&DeleteZoneHost,
		&BattleDeckZoneHost,
		&FluxContentDropTargetHost,
		&SpecialZonesHost,
		&BurdenZoneHost,
		&DeleteZoneTitleText,
		&BurdenZoneTitleText,
		&BattleDeckCardsBox,
		&FluxContentCardsBox,
		&SpecialZonesPanel,
		&BurdenCardsBox,
		&DeleteDropTarget,
		&BattleDeckDropTarget,
		&BackpackDropTarget,
		&BurdenDropTarget
	});
}

void UWacomBackpackScreen::ClearCardBoxes()
{
	HideCardDetailPanel();
	ResetBackpackRefreshDirtyGate();
	if (BattleDeckCardsBox) { BattleDeckCardsBox->ClearChildren(); }
	if (FluxContentCardsBox) { FluxContentCardsBox->ClearChildren(); }
	if (SpecialZonesPanel)  { SpecialZonesPanel->ClearChildren(); }
	if (BurdenCardsBox)     { BurdenCardsBox->ClearChildren(); }
}

#if WITH_AUTOMATION_TESTS
UWacomDeckCardWidget* UWacomBackpackScreen::GetBattleDeckCardWidgetForTest(int32 Index) const
{
	return BattleDeckCardsBox && Index >= 0 && BattleDeckCardsBox->GetChildrenCount() > Index
		? Cast<UWacomDeckCardWidget>(BattleDeckCardsBox->GetChildAt(Index))
		: nullptr;
}

UWacomDeckCardWidget* UWacomBackpackScreen::GetFluxContentCardWidgetForTest(int32 Index) const
{
	return FluxContentCardsBox && Index >= 0 && FluxContentCardsBox->GetChildrenCount() > Index
		? Cast<UWacomDeckCardWidget>(FluxContentCardsBox->GetChildAt(Index))
		: nullptr;
}

UWacomDeckCardWidget* UWacomBackpackScreen::GetBurdenCardWidgetForTest(int32 Index) const
{
	return BurdenCardsBox && Index >= 0 && BurdenCardsBox->GetChildrenCount() > Index
		? Cast<UWacomDeckCardWidget>(BurdenCardsBox->GetChildAt(Index))
		: nullptr;
}

UWacomSpecialZoneWidget* UWacomBackpackScreen::GetSpecialZoneWidgetForTest(int32 Index) const
{
	return SpecialZonesPanel && Index >= 0 && SpecialZonesPanel->GetChildrenCount() > Index
		? Cast<UWacomSpecialZoneWidget>(SpecialZonesPanel->GetChildAt(Index))
		: nullptr;
}

FWacomBackpackScreenAutomationTestView UWacomBackpackScreen::GetAutomationTestViewForTest() const
{
	FWacomBackpackScreenAutomationTestView View;
	if (!StorageRefreshGate)
	{
		return View;
	}

	const FWacomBackpackStorageRefreshGateCounters& Counters = StorageRefreshGate->GetCounters();
	View.ListRefreshApplyCount = Counters.ListRefreshApplyCount;
	View.ListRefreshSkipCount = Counters.ListRefreshSkipCount;
	View.SnapshotBuildCount = Counters.SnapshotBuildCount;
	View.SnapshotRevisionSkipCount = Counters.SnapshotRevisionSkipCount;
	return View;
}
#endif

UWacomDeckCardWidget* UWacomBackpackScreen::CreateCardWidget(const FCardInstance& Inst, EZoneKind FromZone, FGuid FromZoneOwnerInstanceId)
{
	if (!CardWidgetClass)
	{
		return nullptr;
	}

	UWacomDeckCardWidget* CardWidget = CreateWidget<UWacomDeckCardWidget>(this, CardWidgetClass);
	if (!CardWidget)
	{
		return nullptr;
	}
	CardWidget->SetCard(Inst, FromZone, FromZoneOwnerInstanceId);

	URunSession* Run = GetRunSession();
	if (Run)
	{
		// 主体按钮只作为展示和拖拽热区，不绑定点击移动语义。
		CardWidget->SetMoveEnabled(true);
	}

	CardWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);
	CardWidget->OnCardHoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardHovered);
	CardWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardUnhovered);

	return CardWidget;
}

UWacomDeckCardWidget* UWacomBackpackScreen::CreateCardWidget(const FRunStorageCardView& CardView)
{
	return CreateCardWidget(CardView.Instance, CardView.PhysicalZone, CardView.ZoneOwnerInstanceId);
}

void UWacomBackpackScreen::RebuildAll()
{
	UWacomRunViewModel* VM = GetViewModel();
	URunSession* Run = GetRunSession();

	RebuildTopStats(VM);

	if (!Run)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Backpack] RebuildAll: RunSession 未就位，列表跳过重建"));
		GetStorageRefreshGate().ForgetRunSession();
		ClearCardBoxes();
		return;
	}

	FWacomBackpackStorageRefreshGate& RefreshGate = GetStorageRefreshGate();
	if (RefreshGate.BeginRefresh(*Run) == EWacomBackpackStorageRefreshGateResult::SkipSnapshotRevision)
	{
		return;
	}

	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	if (!RefreshGate.ShouldApplySnapshot(Snapshot))
	{
		return;
	}

	RebuildBattleDeckZone(Snapshot);
	RebuildBackpackZone(Snapshot);
	RebuildSpecialZones(Snapshot);
	RebuildBurdenZone(Snapshot);
}

void UWacomBackpackScreen::ResetBackpackRefreshDirtyGate()
{
	GetStorageRefreshGate().Reset();
}

void UWacomBackpackScreen::RebuildTopStats(UWacomRunViewModel* VM)
{
	if (VM)
	{
		const FText BattleDeckTitle = UWacomBackpackScreenPresenter::BuildBattleDeckTitleText(
			VM->GetBattleDeckCount(),
			VM->GetBattleDeckCapacity());
		if (BattleDeckTitleText)
		{
			BattleDeckTitleText->SetText(BattleDeckTitle);
		}
		if (BattleDeckZoneSection)
		{
			BattleDeckZoneSection->SetZoneTitleText(BattleDeckTitle);
		}

		if (BackpackTitleText)
		{
			BackpackTitleText->SetText(UWacomBackpackScreenPresenter::BuildBackpackTitleText());
		}
		if (GoldText)
		{
			GoldText->SetText(UWacomBackpackScreenPresenter::BuildGoldText(VM->GetGold()));
		}
	}
}

void UWacomBackpackScreen::RebuildBattleDeckZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (BattleDeckCardsBox)
	{
		TArray<FWacomBackpackDeckCardListItem> DesiredCards;
		DesiredCards.Reserve(Snapshot.BattleDeckPhysicalCards.Num() + Snapshot.BattleDeckProjectedCards.Num());
		for (const FRunStorageCardView& CardView : Snapshot.BattleDeckPhysicalCards)
		{
			FWacomBackpackDeckCardListItem Desired;
			Desired.CardView = CardView;
			Desired.Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
			DesiredCards.Add(MoveTemp(Desired));
		}

		for (const FRunStorageCardView& ProjectedView : Snapshot.BattleDeckProjectedCards)
		{
			FWacomBackpackDeckCardListItem Desired;
			Desired.CardView = ProjectedView;
			Desired.Role = EWacomBackpackDeckCardListReuseRole::BattleDeckProjected;
			Desired.ProjectedBadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedView, Snapshot);
			Desired.bRightClickToggleEnabled = true;
			DesiredCards.Add(MoveTemp(Desired));
		}

		FWacomBackpackDeckCardListReconciler::Reconcile(
			BattleDeckCardsBox,
			DesiredCards,
			[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
			[this](UWacomDeckCardWidget* RemovedWidget) { HideCardDetailPanelIfSourceRemoved(RemovedWidget); });
	}
}

void UWacomBackpackScreen::RebuildBackpackZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	RebuildFluxContentCards(Snapshot);
}

void UWacomBackpackScreen::RebuildFluxContentCards(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (FluxContentZoneSection)
	{
		FluxContentZoneSection->SetZoneTitleText(UWacomBackpackScreenPresenter::BuildFluxContentTitleText(
			Snapshot.FluxContentCount,
			Snapshot.FluxCapacity));
	}

	if (FluxContentCardsBox)
	{
		TArray<FWacomBackpackDeckCardListItem> DesiredCards;
		DesiredCards.Reserve(Snapshot.Flux.ContentCards.Num());
		for (const FRunStorageCardView& CardView : Snapshot.Flux.ContentCards)
		{
			FWacomBackpackDeckCardListItem Desired;
			Desired.CardView = CardView;
			Desired.Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
			DesiredCards.Add(MoveTemp(Desired));
		}
		FWacomBackpackDeckCardListReconciler::Reconcile(
			FluxContentCardsBox,
			DesiredCards,
			[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
			[this](UWacomDeckCardWidget* RemovedWidget) { HideCardDetailPanelIfSourceRemoved(RemovedWidget); });
	}
}

void UWacomBackpackScreen::RebuildSpecialZones(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (SpecialZonesPanel)
	{
		TMap<FGuid, UWacomSpecialZoneWidget*> ExistingByOwnerId;
		for (int32 ChildIndex = 0; ChildIndex < SpecialZonesPanel->GetChildrenCount(); ++ChildIndex)
		{
			UWacomSpecialZoneWidget* ExistingWidget = Cast<UWacomSpecialZoneWidget>(SpecialZonesPanel->GetChildAt(ChildIndex));
			if (!ExistingWidget)
			{
				continue;
			}

			const FGuid OwnerInstanceId = ExistingWidget->GetOwnerCardInstanceId();
			if (OwnerInstanceId.IsValid())
			{
				ExistingByOwnerId.Add(OwnerInstanceId, ExistingWidget);
			}
		}

		TSet<UWacomSpecialZoneWidget*> UsedZoneWidgets;
		for (int32 DesiredIndex = 0; DesiredIndex < Snapshot.SpecialZones.Num(); ++DesiredIndex)
		{
			const FRunSpecialStorageView& SpecialView = Snapshot.SpecialZones[DesiredIndex];
			UWacomSpecialZoneWidget* ZoneWidget = ExistingByOwnerId.FindRef(SpecialView.OwnerCard.Instance.InstanceId);
			if (!ZoneWidget)
			{
				UClass* ZoneWidgetClass = SpecialZoneWidgetClass ? SpecialZoneWidgetClass.Get() : UWacomSpecialZoneWidget::StaticClass();
				ZoneWidget = CreateWidget<UWacomSpecialZoneWidget>(this, ZoneWidgetClass);
				if (ZoneWidget)
				{
					ZoneWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);
					ZoneWidget->OnCardHoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardHovered);
					ZoneWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardUnhovered);
				}
			}
			if (!ZoneWidget)
			{
				continue;
			}
			ZoneWidget->SetSpecialZoneView(SpecialView, this, CardWidgetClass);
			UsedZoneWidgets.Add(ZoneWidget);

			if (ZoneWidget->GetParent() == SpecialZonesPanel)
			{
				SpecialZonesPanel->ShiftChild(DesiredIndex, ZoneWidget);
				if (UVerticalBoxSlot* ZoneSlot = Cast<UVerticalBoxSlot>(ZoneWidget->Slot))
				{
					ZoneSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				}
			}
			else if (UVerticalBoxSlot* ZoneSlot = SpecialZonesPanel->AddChildToVerticalBox(ZoneWidget))
			{
				ZoneSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
				SpecialZonesPanel->ShiftChild(DesiredIndex, ZoneWidget);
			}
		}
		for (const TPair<FGuid, UWacomSpecialZoneWidget*>& ExistingPair : ExistingByOwnerId)
		{
			if (ExistingPair.Value && !UsedZoneWidgets.Contains(ExistingPair.Value))
			{
				if (ExistingPair.Value->ContainsCardWidget(CardDetailSourceWidget.Get()))
				{
					HideCardDetailPanel();
				}
				ExistingPair.Value->RemoveFromParent();
			}
		}
	}
}

void UWacomBackpackScreen::RebuildBurdenZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	const ESlateVisibility BurdenVisibility = UWacomBackpackScreenPresenter::GetBurdenZoneVisibility(Snapshot.BurdenCount);
	if (BurdenZoneHost)
	{
		BurdenZoneHost->SetVisibility(BurdenVisibility);
	}
	if (BurdenZoneSection)
	{
		BurdenZoneSection->SetVisibility(BurdenVisibility);
	}

	const FText BurdenTitle = UWacomBackpackScreenPresenter::BuildBurdenZoneTitleText(Snapshot.BurdenCount);
	if (BurdenZoneTitleText)
	{
		BurdenZoneTitleText->SetText(BurdenTitle);
	}
	if (BurdenZoneSection)
	{
		BurdenZoneSection->SetZoneTitleText(BurdenTitle);
	}

	if (BurdenCardsBox)
	{
		TArray<FWacomBackpackDeckCardListItem> DesiredCards;
		DesiredCards.Reserve(Snapshot.BurdenCards.Num());
		for (const FRunStorageCardView& CardView : Snapshot.BurdenCards)
		{
			FWacomBackpackDeckCardListItem Desired;
			Desired.CardView = CardView;
			Desired.Role = EWacomBackpackDeckCardListReuseRole::PhysicalList;
			DesiredCards.Add(MoveTemp(Desired));
		}
		FWacomBackpackDeckCardListReconciler::Reconcile(
			BurdenCardsBox,
			DesiredCards,
			[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
			[this](UWacomDeckCardWidget* RemovedWidget) { HideCardDetailPanelIfSourceRemoved(RemovedWidget); });
	}
}

void UWacomBackpackScreen::HandleBattleEnabledToggle(FGuid InstanceId)
{
	FWacomBackpackCommandFlow::HandleBattleEnabledToggle(GetRunSession(), InstanceId);
}

void UWacomBackpackScreen::HandleCardHovered(UWacomDeckCardWidget* SourceWidget)
{
	ShowCardDetailForCardWidget(SourceWidget);
}

void UWacomBackpackScreen::HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget)
{
	HideCardDetailPanelIfSourceRemoved(SourceWidget);
}

bool UWacomBackpackScreen::ShowCardDetailForCardWidget(UWacomDeckCardWidget* SourceWidget)
{
	return GetCardDetailController().ShowForCardWidget(SourceWidget);
}

void UWacomBackpackScreen::HideCardDetailPanel()
{
	GetCardDetailController().Hide();
}

void UWacomBackpackScreen::HideCardDetailPanelIfSourceRemoved(UWacomDeckCardWidget* RemovedWidget)
{
	GetCardDetailController().HideIfSourceRemoved(RemovedWidget);
}

UWacomCardDetailPanel* UWacomBackpackScreen::EnsureCardDetailPanel()
{
	return GetCardDetailController().EnsurePanel();
}

void UWacomBackpackScreen::PositionCardDetailPanelNear(UWacomDeckCardWidget* SourceWidget)
{
	GetCardDetailController().PositionNear(SourceWidget);
}

FWacomBackpackCardDetailController& UWacomBackpackScreen::GetCardDetailController()
{
	if (!CardDetailController)
	{
		CardDetailController = MakeShared<FWacomBackpackCardDetailController>(*this);
	}
	return *CardDetailController;
}

const FWacomBackpackCardDetailController& UWacomBackpackScreen::GetCardDetailController() const
{
	return const_cast<UWacomBackpackScreen*>(this)->GetCardDetailController();
}

FWacomBackpackStorageRefreshGate& UWacomBackpackScreen::GetStorageRefreshGate()
{
	if (!StorageRefreshGate)
	{
		StorageRefreshGate = MakeShared<FWacomBackpackStorageRefreshGate>();
	}
	return *StorageRefreshGate;
}

void UWacomBackpackScreen::HandleCloseClicked()
{
	HideCardDetailPanel();
	DeactivateWidget();
}

FReply UWacomBackpackScreen::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// B 键关闭：和打开是同一个键，CommonUI Menu 模式下 EnhancedInput IA 被屏蔽，
	// 必须在 widget 层自己拦。父类已经处理 ESC。
	if (InKeyEvent.GetKey() == EKeys::B)
	{
		DeactivateWidget();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
