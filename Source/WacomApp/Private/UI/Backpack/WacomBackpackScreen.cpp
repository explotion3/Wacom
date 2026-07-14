// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreen.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Engine/GameInstance.h"
#include "Misc/PackageName.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"
#include "UI/Backpack/BackpackRuntimeZoneBuilder.h"
#include "UI/Backpack/WacomBackpackCardDetailController.h"
#include "UI/Backpack/WacomBackpackCommandFlow.h"
#include "UI/Backpack/WacomBackpackDeckCardListReconciler.h"
#include "UI/Backpack/WacomBackpackHeaderPresenter.h"
#include "UI/Backpack/WacomBackpackSpecialZoneListReconciler.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/Backpack/WacomBackpackStorageRefreshGate.h"
#include "UI/Backpack/WacomBackpackWorkspaceReconciler.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZoneRackWidget.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

namespace
{
template <typename TWidget>
TSubclassOf<TWidget> LoadOptionalWidgetClass(const TCHAR* ClassPath);

void AttachChildToHostAndFill(UPanelWidget& Host, UWidget& Child)
{
	if (Child.GetParent() != &Host)
	{
		Child.RemoveFromParent();
		Host.AddChild(&Child);
	}

	if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(Child.Slot))
	{
		OverlaySlot->SetHorizontalAlignment(HAlign_Fill);
		OverlaySlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UVerticalBoxSlot* VerticalSlot = Cast<UVerticalBoxSlot>(Child.Slot))
	{
		VerticalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		VerticalSlot->SetHorizontalAlignment(HAlign_Fill);
		VerticalSlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(Child.Slot))
	{
		HorizontalSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		HorizontalSlot->SetHorizontalAlignment(HAlign_Fill);
		HorizontalSlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UBorderSlot* BorderSlot = Cast<UBorderSlot>(Child.Slot))
	{
		BorderSlot->SetHorizontalAlignment(HAlign_Fill);
		BorderSlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (USizeBoxSlot* SizeSlot = Cast<USizeBoxSlot>(Child.Slot))
	{
		SizeSlot->SetHorizontalAlignment(HAlign_Fill);
		SizeSlot->SetVerticalAlignment(VAlign_Fill);
	}
	else if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(Child.Slot))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
		CanvasSlot->SetAlignment(FVector2D::ZeroVector);
		CanvasSlot->SetAutoSize(false);
	}
}
}

UWacomBackpackScreen::UWacomBackpackScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	if (!CardWidgetClass)
	{
		CardWidgetClass = LoadOptionalWidgetClass<UWacomDeckCardWidget>(
			TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	}

	if (!SpecialZoneWidgetClass)
	{
		SpecialZoneWidgetClass = LoadOptionalWidgetClass<UWacomSpecialZoneWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_WacomSpecialZoneWidget.WBP_WacomSpecialZoneWidget_C"));
	}

	if (!CardDetailPanelClass)
	{
		CardDetailPanelClass = LoadOptionalWidgetClass<UWacomCardDetailPanel>(
			TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C"));
	}
	if (!WorkspaceWidgetClass)
	{
		WorkspaceWidgetClass = UWacomBackpackWorkspaceWidget::StaticClass();
	}
	if (!ZoneRackWidgetClass)
	{
		ZoneRackWidgetClass = UWacomBackpackZoneRackWidget::StaticClass();
	}
	if (!DeleteConfirmWidgetClass)
	{
		DeleteConfirmWidgetClass = UWacomBackpackDeleteConfirmWidget::StaticClass();
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
			&WorkspaceHost,
			&ZoneRackHost,
			&DeleteTargetHost,
			&DeleteConfirmHost,
			&ArrangeAllButton,
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
	EnsureWorkspaceWidgets();
	return Super::RebuildWidget();
}

void UWacomBackpackScreen::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureRuntimeZoneWidgets();
	EnsureWorkspaceWidgets();
	BindOwningLayerTransition();

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackScreen::HandleCloseClicked);
	}
	if (ArrangeAllButton)
	{
		ArrangeAllButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackScreen::HandleArrangeAllClicked);
	}

	TrySubscribeAndRefresh();
}

void UWacomBackpackScreen::NativeDestruct()
{
	CancelWorkspaceInteraction();
	UnbindOwningLayerTransition();
	bOwningLayerTransitioning = false;
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
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UWacomBackpackScreen::HandleCloseClicked);
	}
	if (ArrangeAllButton)
	{
		ArrangeAllButton->OnClicked.RemoveDynamic(this, &UWacomBackpackScreen::HandleArrangeAllClicked);
	}
	if (WorkspaceWidget)
	{
		WorkspaceWidget->OnLayoutGeometryReadyNative.RemoveAll(this);
	}

	if (UWacomRunViewModelProvider* Provider = SubscribedProvider.Get())
	{
		Provider->OnRunViewModelRefreshedNative.RemoveAll(this);
	}
	SubscribedProvider = nullptr;
	if (ZoneRackWidget)
	{
		ZoneRackWidget->OnZoneActivatedNative.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UWacomBackpackScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	BindOwningLayerTransition();
	// CommonUI Stack 重新激活时（背包从 GameMenu 顶层重新显示），事件订阅可能错过期间的广播；
	// 无条件刷新一次保底。
	TrySubscribeAndRefresh();
	RebuildWorkspaceFromCachedSnapshot();
	if (WorkspaceWidget)
	{
		WorkspaceWidget->RequestLayoutGeometryRefresh();
		WorkspaceWidget->SetKeyboardFocus();
	}
}

void UWacomBackpackScreen::TrySubscribeAndRefresh()
{
	EnsureRuntimeZoneWidgets();
	EnsureWorkspaceWidgets();

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
	if (WorkspaceMutationRefreshDeferralDepth > 0)
	{
		bWorkspaceMutationRefreshDeferred = true;
		return;
	}
	RebuildAll();
}

void UWacomBackpackScreen::BeginWorkspaceMutationRefreshDeferral()
{
	++WorkspaceMutationRefreshDeferralDepth;
}

void UWacomBackpackScreen::EndWorkspaceMutationRefreshDeferral(bool bForceRefresh)
{
	if (!ensureMsgf(
		WorkspaceMutationRefreshDeferralDepth > 0,
		TEXT("Workspace mutation refresh deferral ended without a matching begin.")))
	{
		WorkspaceMutationRefreshDeferralDepth = 0;
		bWorkspaceMutationRefreshDeferred = false;
		return;
	}

	--WorkspaceMutationRefreshDeferralDepth;
	if (WorkspaceMutationRefreshDeferralDepth == 0
		&& (bForceRefresh || bWorkspaceMutationRefreshDeferred))
	{
		bWorkspaceMutationRefreshDeferred = false;
		RebuildAll();
	}
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

void UWacomBackpackScreen::NativeOnDeactivated()
{
	CancelWorkspaceInteraction();
	HideCardDetailPanel();
	Super::NativeOnDeactivated();
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
		&BurdenCardsBox
	});
}

void UWacomBackpackScreen::EnsureWorkspaceWidgets()
{
	if (!WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel = MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	}
	if (WorkspaceHost && !WorkspaceWidget)
	{
		UClass* ClassToUse = WorkspaceWidgetClass
			? WorkspaceWidgetClass.Get()
			: UWacomBackpackWorkspaceWidget::StaticClass();
		WorkspaceWidget = CreateWidget<UWacomBackpackWorkspaceWidget>(this, ClassToUse);
	}
	if (WorkspaceHost && WorkspaceWidget)
	{
		AttachChildToHostAndFill(*WorkspaceHost, *WorkspaceWidget);
	}
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetVisibility(ESlateVisibility::Visible);
		WorkspaceWidget->SetInteractionModel(WorkspaceInteractionModel, WorkspaceStyle);
		WorkspaceWidget->SetCardFaceRetainedRenderingEnabled(!bOwningLayerTransitioning);
		WorkspaceWidget->OnReleaseIntentNative.RemoveAll(this);
		WorkspaceWidget->OnReleaseIntentNative.AddUObject(this, &UWacomBackpackScreen::HandleWorkspaceReleaseIntent);
		WorkspaceWidget->OnInteractionChangedNative.RemoveAll(this);
		WorkspaceWidget->OnInteractionChangedNative.AddUObject(this, &UWacomBackpackScreen::HandleWorkspaceInteractionChanged);
		WorkspaceWidget->OnLayoutGeometryReadyNative.RemoveAll(this);
		WorkspaceWidget->OnLayoutGeometryReadyNative.AddUObject(
			this,
			&UWacomBackpackScreen::HandleWorkspaceLayoutGeometryReady);
	}

	if (ZoneRackHost && !ZoneRackWidget)
	{
		UClass* ClassToUse = ZoneRackWidgetClass
			? ZoneRackWidgetClass.Get()
			: UWacomBackpackZoneRackWidget::StaticClass();
		ZoneRackWidget = CreateWidget<UWacomBackpackZoneRackWidget>(this, ClassToUse);
	}
	if (ZoneRackWidget)
	{
		ZoneRackWidget->OnZoneActivatedNative.RemoveAll(this);
		ZoneRackWidget->OnZoneActivatedNative.AddUObject(this, &UWacomBackpackScreen::HandleZoneActivated);
	}
	if (ZoneRackHost && ZoneRackWidget)
	{
		AttachChildToHostAndFill(*ZoneRackHost, *ZoneRackWidget);
	}
	if (DeleteConfirmHost && !DeleteConfirmWidget)
	{
		UClass* ClassToUse = DeleteConfirmWidgetClass
			? DeleteConfirmWidgetClass.Get()
			: UWacomBackpackDeleteConfirmWidget::StaticClass();
		DeleteConfirmWidget = CreateWidget<UWacomBackpackDeleteConfirmWidget>(this, ClassToUse);
		if (DeleteConfirmWidget)
		{
			DeleteConfirmWidget->OnConfirmNative.AddUObject(this, &UWacomBackpackScreen::HandleWorkspaceDeleteConfirmed);
			DeleteConfirmWidget->OnCancelNative.AddUObject(this, &UWacomBackpackScreen::HandleWorkspaceDeleteCancelled);
			AttachChildToHostAndFill(*DeleteConfirmHost, *DeleteConfirmWidget);
		}
	}
}

FWacomBackpackWorkspaceStateStore& UWacomBackpackScreen::GetWorkspaceStateStore(URunSession* Run)
{
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UWacomBackpackWorkspaceStateSubsystem* Subsystem = GI->GetSubsystem<UWacomBackpackWorkspaceStateSubsystem>())
		{
			return Subsystem->GetStoreForRun(Run);
		}
	}
	if (!WorkspaceStateFallback)
	{
		WorkspaceStateFallback = MakeShared<FWacomBackpackWorkspaceStateStore>();
	}
	WorkspaceStateFallback->BindToRun(Run);
	return *WorkspaceStateFallback;
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

FText UWacomBackpackScreen::BuildMoveZoneNameTextForTest(EZoneKind Zone)
{
	return FWacomBackpackCommandFlow::BuildMoveZoneNameText(Zone);
}

FText UWacomBackpackScreen::BuildMoveFailureToastTextForTest(FName DisabledReason)
{
	return FWacomBackpackCommandFlow::BuildMoveFailureToastText(DisabledReason);
}

FText UWacomBackpackScreen::BuildDeleteFailureToastTextForTest(FName DisabledReason)
{
	return FWacomBackpackCommandFlow::BuildDeleteFailureToastText(DisabledReason);
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
	View.ZoneRackEntryCount = ZoneRackWidget ? ZoneRackWidget->GetZoneEntryCount() : 0;
	View.WorkspaceCardCount = ActiveWorkspaceCardWidgets.Num();
	if (WorkspaceWidget)
	{
		const FWacomBackpackWorkspaceAutomationTestView WorkspaceView = WorkspaceWidget->GetAutomationTestView();
		View.ActiveWorkspaceZone = WorkspaceView.ActiveZone;
		View.ActiveWorkspaceOwnerInstanceId = WorkspaceView.ActiveZoneOwnerInstanceId;
	}
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
		GetWorkspaceStateStore(nullptr);
		bHasLastAppliedStorageSnapshot = false;
		ActiveWorkspaceCardWidgets.Reset();
		if (WorkspaceWidget && WorkspaceWidget->GetCardCanvas())
		{
			WorkspaceWidget->GetCardCanvas()->ClearChildren();
			WorkspaceWidget->SetEmptyStateVisible(true);
		}
		if (ZoneRackWidget)
		{
			ZoneRackWidget->SetZoneEntries(TConstArrayView<FWacomBackpackZoneRackEntryView>());
		}
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
	bHasLastAppliedStorageSnapshot = true;
	LastAppliedStorageSnapshot = Snapshot;

	RebuildWorkspaceChrome(Snapshot);
	RebuildBattleDeckZone(Snapshot);
	RebuildBackpackZone(Snapshot);
	RebuildSpecialZones(Snapshot);
	RebuildBurdenZone(Snapshot);
}

void UWacomBackpackScreen::RebuildWorkspaceChrome(const FRunBackpackStorageSnapshot& Snapshot)
{
	EnsureWorkspaceWidgets();
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceWidget || !ZoneRackWidget)
	{
		return;
	}

	FWacomBackpackWorkspaceStateStore& StateStore = GetWorkspaceStateStore(Run);
	FWacomBackpackZoneKey ActiveZone = StateStore.GetActiveZone();
	TArray<FWacomBackpackZoneRackEntryView> RackEntries = UWacomBackpackScreenPresenter::BuildZoneRackEntries(
		Snapshot,
		ActiveZone.Zone,
		ActiveZone.OwnerInstanceId);
	const bool bActiveStillExists = RackEntries.ContainsByPredicate(
		[&ActiveZone](const FWacomBackpackZoneRackEntryView& Entry)
		{
			return Entry.HasSameIdentity(ActiveZone.Zone, ActiveZone.OwnerInstanceId);
		});
	if (!bActiveStillExists)
	{
		ActiveZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
		StateStore.SetActiveZone(ActiveZone);
		RackEntries = UWacomBackpackScreenPresenter::BuildZoneRackEntries(
			Snapshot,
			ActiveZone.Zone,
			ActiveZone.OwnerInstanceId);
	}
	else if (!StateStore.HasActiveZone())
	{
		StateStore.SetActiveZone(ActiveZone);
	}

	ZoneRackWidget->SetZoneEntries(RackEntries);
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*WorkspaceWidget,
		Snapshot,
		ActiveZone,
		StateStore,
		WorkspaceStyle,
		[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
		[this](UWacomDeckCardWidget* RemovedWidget) { HideCardDetailPanelIfSourceRemoved(RemovedWidget); },
		&ActiveWorkspaceCardWidgets);
	WorkspaceWidget->BindWorkspaceCards(
		ActiveWorkspaceCardWidgets,
		Run->GetBackpackStorageSnapshotRevision());
}

void UWacomBackpackScreen::RebuildWorkspaceFromCachedSnapshot()
{
	if (bHasLastAppliedStorageSnapshot)
	{
		RebuildWorkspaceChrome(LastAppliedStorageSnapshot);
	}
}

void UWacomBackpackScreen::HandleZoneActivated(EZoneKind Zone, FGuid OwnerInstanceId)
{
	if (!bHasLastAppliedStorageSnapshot)
	{
		return;
	}
	const FWacomBackpackZoneKey Requested = FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId);
	const TArray<FWacomBackpackZoneRackEntryView> Entries = UWacomBackpackScreenPresenter::BuildZoneRackEntries(
		LastAppliedStorageSnapshot,
		Requested.Zone,
		Requested.OwnerInstanceId);
	if (!Requested.IsValid() || !Entries.ContainsByPredicate(
		[&Requested](const FWacomBackpackZoneRackEntryView& Entry)
		{
			return Entry.HasSameIdentity(Requested.Zone, Requested.OwnerInstanceId);
		}))
	{
		return;
	}
	CancelWorkspaceInteraction();
	GetWorkspaceStateStore(GetRunSession()).SetActiveZone(Requested);
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandleWorkspaceInteractionChanged()
{
	if (WorkspaceInteractionModel && WorkspaceInteractionModel->IsCarrying())
	{
		HideCardDetailPanel();
		FWacomBackpackZoneKey Target;
		if (ResolveWorkspaceRackTarget(Target) && ZoneRackWidget)
		{
			const FWacomBackpackWorkspaceCarryState& Carry = WorkspaceInteractionModel->GetCarry();
			bool bRejected = false;
			if (!(Carry.SourceZone == Target))
			{
				const FRunDeckBatchMoveRequest PreviewRequest = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
					Carry,
					Target,
					Carry.RemainingInstanceIds);
				URunSession* Run = GetRunSession();
				bRejected = !Run || !Run->ValidateMoveInstancesAtomic(PreviewRequest).bCanExecute;
			}
			ZoneRackWidget->SetDropPreviewForZone(Target.Zone, Target.OwnerInstanceId, true, bRejected);
			return;
		}
	}
	if (ZoneRackWidget)
	{
		ZoneRackWidget->SetDropPreviewForZone(EZoneKind::Backpack, FGuid(), false, false);
	}
}

void UWacomBackpackScreen::HandleWorkspaceLayoutGeometryReady(FVector2D LayoutSize)
{
	if (LayoutSize.X > 1.0f && LayoutSize.Y > 1.0f)
	{
		RebuildWorkspaceFromCachedSnapshot();
	}
}

void UWacomBackpackScreen::ApplyOwningLayerTransitionState(bool bTransitioning)
{
	bOwningLayerTransitioning = bTransitioning;
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetCardFaceRetainedRenderingEnabled(!bOwningLayerTransitioning);
	}
}

void UWacomBackpackScreen::BindOwningLayerTransition()
{
	UWacomPrimaryGameLayout* Layout = nullptr;
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWacomGameUIManagerSubsystem* UIManager =
			GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>())
		{
			Layout = UIManager->GetPrimaryLayout();
		}
	}

	if (BoundPrimaryLayout.Get() != Layout)
	{
		UnbindOwningLayerTransition();
		BoundPrimaryLayout = Layout;
		if (Layout)
		{
			Layout->OnLayerTransitioningChangedNative.AddUObject(
				this,
				&UWacomBackpackScreen::HandleOwningLayerTransitioningChanged);
		}
	}

	ApplyOwningLayerTransitionState(
		Layout && Layout->IsLayerTransitioning(WacomUITags::UI_Layer_GameMenu.GetTag()));
}

void UWacomBackpackScreen::UnbindOwningLayerTransition()
{
	if (UWacomPrimaryGameLayout* Layout = BoundPrimaryLayout.Get())
	{
		Layout->OnLayerTransitioningChangedNative.RemoveAll(this);
	}
	BoundPrimaryLayout.Reset();
}

void UWacomBackpackScreen::HandleOwningLayerTransitioningChanged(
	FGameplayTag LayerTag,
	bool bTransitioning)
{
	if (LayerTag.MatchesTagExact(WacomUITags::UI_Layer_GameMenu.GetTag()))
	{
		ApplyOwningLayerTransitionState(bTransitioning);
	}
}

bool UWacomBackpackScreen::ResolveWorkspaceRackTarget(FWacomBackpackZoneKey& OutTarget) const
{
	if (!WorkspaceWidget || !ZoneRackWidget || !WorkspaceInteractionModel || !WorkspaceInteractionModel->IsCarrying())
	{
		return false;
	}
	const FVector2D AbsolutePointer = WorkspaceWidget->GetCachedGeometry().LocalToAbsolute(
		WorkspaceInteractionModel->GetCarry().PointerPosition);
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid OwnerInstanceId;
	if (!ZoneRackWidget->FindZoneAtAbsolutePosition(AbsolutePointer, Zone, OwnerInstanceId))
	{
		return false;
	}
	OutTarget = FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId);
	return OutTarget.IsValid();
}

bool UWacomBackpackScreen::IsWorkspaceDeleteTarget() const
{
	if (!WorkspaceWidget || !DeleteTargetHost || !WorkspaceInteractionModel || !WorkspaceInteractionModel->IsCarrying())
	{
		return false;
	}
	const FVector2D AbsolutePointer = WorkspaceWidget->GetCachedGeometry().LocalToAbsolute(
		WorkspaceInteractionModel->GetCarry().PointerPosition);
	return DeleteTargetHost->GetCachedGeometry().IsUnderLocation(AbsolutePointer);
}

void UWacomBackpackScreen::BeginWorkspaceDeleteConfirmation(TConstArrayView<FGuid> InstanceIds)
{
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceInteractionModel || InstanceIds.IsEmpty())
	{
		return;
	}
	const FWacomBackpackWorkspaceCarryState CarrySnapshot = WorkspaceInteractionModel->GetCarry();
	const FRunDeckBatchDeleteRequest Request = FWacomBackpackCommandFlow::BuildBatchDeleteRequest(
		CarrySnapshot,
		InstanceIds);
	const FRunDeckBatchDeletePreview Preview = FWacomBackpackCommandFlow::PreviewBatchDelete(Run, Request);
	if (!Preview.Validation.bCanExecute)
	{
		return;
	}
	PendingDeleteConfirmation = MakeShared<FWacomBackpackPendingDeleteConfirmation>();
	PendingDeleteConfirmation->bPending = true;
	PendingDeleteConfirmation->SuspendedCarry = CarrySnapshot;
	PendingDeleteConfirmation->RequestedInstanceIds = TArray<FGuid>(InstanceIds);
	PendingDeleteConfirmation->PreviewCardCount = InstanceIds.Num();
	PendingDeleteConfirmation->PreviewGoldReward = Preview.TotalGoldReward;
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetCarryInputSuspended(true);
	}
	else
	{
		WorkspaceInteractionModel->SetCarryInputSuspended(true);
	}
	if (DeleteConfirmWidget)
	{
		DeleteConfirmWidget->SetPreview(InstanceIds.Num(), Preview.TotalGoldReward);
	}
	if (DeleteConfirmHost)
	{
		DeleteConfirmHost->SetVisibility(ESlateVisibility::Visible);
	}
	if (DeleteConfirmWidget)
	{
		DeleteConfirmWidget->FocusDefaultAction();
	}
}

void UWacomBackpackScreen::HandleWorkspaceDeleteCancelled()
{
	if (PendingDeleteConfirmation && PendingDeleteConfirmation->bPending && WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel->RestoreCarry(PendingDeleteConfirmation->SuspendedCarry);
	}
	PendingDeleteConfirmation.Reset();
	if (DeleteConfirmHost) DeleteConfirmHost->SetVisibility(ESlateVisibility::Collapsed);
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetCarryInputSuspended(false);
		WorkspaceWidget->RefreshInteractionPresentation();
		WorkspaceWidget->SetKeyboardFocus();
	}
	else if (WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel->SetCarryInputSuspended(false);
	}
}

void UWacomBackpackScreen::HandleWorkspaceDeleteConfirmed()
{
	if (!PendingDeleteConfirmation || !PendingDeleteConfirmation->bPending)
	{
		return;
	}
	const FWacomBackpackWorkspaceCarryState SuspendedCarry =
		PendingDeleteConfirmation->SuspendedCarry;
	const TArray<FGuid> RequestedInstanceIds =
		PendingDeleteConfirmation->RequestedInstanceIds;
	const FRunDeckBatchDeleteRequest Request = FWacomBackpackCommandFlow::BuildBatchDeleteRequest(
		SuspendedCarry,
		RequestedInstanceIds);
	BeginWorkspaceMutationRefreshDeferral();
	const FRunDeckBatchOperationResult Result = FWacomBackpackCommandFlow::SubmitBatchDelete(
		*this,
		GetRunSession(),
		Request);
	if (!Result.bSucceeded)
	{
		HandleWorkspaceDeleteCancelled();
		EndWorkspaceMutationRefreshDeferral(false);
		return;
	}
	if (WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel->RestoreCarry(SuspendedCarry);
		WorkspaceInteractionModel->CommitReleasedCards(RequestedInstanceIds);
		WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(Result.StorageRevision);
	}
	PendingDeleteConfirmation.Reset();
	if (DeleteConfirmHost) DeleteConfirmHost->SetVisibility(ESlateVisibility::Collapsed);
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetCarryInputSuspended(false);
	}
	else if (WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel->SetCarryInputSuspended(false);
	}
	ResetBackpackRefreshDirtyGate();
	EndWorkspaceMutationRefreshDeferral(true);
}

void UWacomBackpackScreen::HandleWorkspaceReleaseIntent(
	const FWacomBackpackWorkspaceReleaseIntent& Intent)
{
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceWidget || !WorkspaceInteractionModel || Intent.InstanceIds.IsEmpty())
	{
		return;
	}
	const UWacomBackpackWorkspaceStyle* Style = WorkspaceStyle
		? WorkspaceStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackWorkspaceCarryState& Carry = WorkspaceInteractionModel->GetCarry();
	if (IsWorkspaceDeleteTarget())
	{
		BeginWorkspaceDeleteConfirmation(Intent.InstanceIds);
		return;
	}
	FWacomBackpackZoneKey RackTarget;
	if (ResolveWorkspaceRackTarget(RackTarget))
	{
		HandleWorkspaceRackReleaseIntent(Intent, RackTarget);
		return;
	}
	const TArray<FWacomBackpackCarriedFanLayout> Fan =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFanLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			Carry.PointerPosition,
			Style->FanMaximumAngleDegrees,
			Style->FanCardSpacingPixels,
			Style->CurrentCardLiftPixels);
	const FVector2D WorkspaceSize = WorkspaceWidget->GetLayoutSpaceSize();
	FWacomBackpackWorkspaceStateStore& StateStore = GetWorkspaceStateStore(Run);
	const FWacomBackpackZoneKey ActiveZone = StateStore.GetActiveZone();
	for (const FGuid InstanceId : Intent.InstanceIds)
	{
		const int32 CarryIndex = Carry.RemainingInstanceIds.IndexOfByKey(InstanceId);
		if (!Fan.IsValidIndex(CarryIndex))
		{
			continue;
		}
		const FVector2D ClampedCenter = FWacomBackpackWorkspaceLayoutSolver::ClampCardCenterToVisibleBounds(
			Fan[CarryIndex].Transform.CardCenter,
			WorkspaceSize,
			Style->CardRenderSize,
			Style->MinimumVisibleFraction);
		FWacomBackpackWorkspaceLayoutEntry Entry;
		Entry.NormalizedPosition = ClampedCenter / WorkspaceSize;
		Entry.AngleDegrees = Fan[CarryIndex].Transform.AngleDegrees;
		Entry.LayerRank = Fan[CarryIndex].Transform.LayerRank;
		Entry.bHasManualPlacement = true;
		StateStore.SetLayout(ActiveZone, InstanceId, Entry);
	}
	WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandleWorkspaceRackReleaseIntent(
	const FWacomBackpackWorkspaceReleaseIntent& Intent,
	const FWacomBackpackZoneKey& RackTarget)
{
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceWidget || !WorkspaceInteractionModel
		|| !WorkspaceInteractionModel->IsCarrying() || Intent.InstanceIds.IsEmpty())
	{
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry = WorkspaceInteractionModel->GetCarry();
	if (Carry.SourceZone == RackTarget)
	{
		FWacomBackpackWorkspaceStateStore& Store = GetWorkspaceStateStore(Run);
		FWacomBackpackCommandFlow::CollectSameZone(
			Store,
			Carry.SourceZone,
			RackTarget,
			Intent.InstanceIds);
		WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
		RebuildWorkspaceFromCachedSnapshot();
		return;
	}

	const FRunDeckBatchMoveRequest Request = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
		Carry,
		RackTarget,
		Intent.InstanceIds);
	BeginWorkspaceMutationRefreshDeferral();
	const FRunDeckBatchOperationResult Result = FWacomBackpackCommandFlow::SubmitBatchMove(*this, Run, Request);
	if (Result.bSucceeded)
	{
		WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
		WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(Result.StorageRevision);
		ResetBackpackRefreshDirtyGate();
	}
	EndWorkspaceMutationRefreshDeferral(Result.bSucceeded);
	if (!Result.bSucceeded)
	{
		WorkspaceWidget->RefreshInteractionPresentation();
	}
}

void UWacomBackpackScreen::CancelWorkspaceInteraction()
{
	if (WorkspaceWidget)
	{
		WorkspaceWidget->CancelInteraction();
	}
	else if (WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel->CancelTransientState();
	}
}

void UWacomBackpackScreen::HandleArrangeAllClicked()
{
	URunSession* Run = GetRunSession();
	if (!Run)
	{
		return;
	}
	FWacomBackpackWorkspaceStateStore& StateStore = GetWorkspaceStateStore(Run);
	FWacomBackpackCommandFlow::ArrangeAll(StateStore, StateStore.GetActiveZone());
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::ResetBackpackRefreshDirtyGate()
{
	GetStorageRefreshGate().Reset();
}

void UWacomBackpackScreen::RebuildTopStats(UWacomRunViewModel* VM)
{
	FWacomBackpackHeaderPresenter::Apply(
		FWacomBackpackHeaderPresenterContext{
			BattleDeckTitleText,
			BackpackTitleText,
			GoldText,
			BattleDeckZoneSection
		},
		VM);
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
	FWacomBackpackSpecialZoneListReconciler::Reconcile(
		SpecialZonesPanel,
		Snapshot.SpecialZones,
		[this](const FRunSpecialStorageView& /*SpecialView*/)
		{
			UClass* ZoneWidgetClass = SpecialZoneWidgetClass ? SpecialZoneWidgetClass.Get() : UWacomSpecialZoneWidget::StaticClass();
			UWacomSpecialZoneWidget* ZoneWidget = CreateWidget<UWacomSpecialZoneWidget>(this, ZoneWidgetClass);
			if (ZoneWidget)
			{
				ZoneWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);
				ZoneWidget->OnCardHoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardHovered);
				ZoneWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardUnhovered);
			}
			return ZoneWidget;
		},
		[this](UWacomSpecialZoneWidget& ZoneWidget, const FRunSpecialStorageView& SpecialView)
		{
			ZoneWidget.SetSpecialZoneView(SpecialView, this, CardWidgetClass);
		},
		[this](UWacomSpecialZoneWidget* RemovedWidget)
		{
			if (RemovedWidget && RemovedWidget->ContainsCardWidget(CardDetailSourceWidget.Get()))
			{
				HideCardDetailPanel();
			}
		});
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
	FWacomBackpackCommandFlow::HandleBattleEnabledToggle(*this, GetRunSession(), InstanceId);
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
	if (WorkspaceInteractionModel && WorkspaceInteractionModel->IsCarrying())
	{
		HideCardDetailPanel();
		return false;
	}
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
