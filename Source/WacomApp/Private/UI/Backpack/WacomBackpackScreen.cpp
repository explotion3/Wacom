// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreen.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/BorderSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/LocalPlayer.h"
#include "CommonInputSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/PackageName.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"
#include "UI/Backpack/WacomBackpackCardDetailController.h"
#include "UI/Backpack/WacomBackpackControlsHelpWidget.h"
#include "UI/Backpack/WacomBackpackCommandFlow.h"
#include "UI/Backpack/WacomBackpackInteractionHintPresenter.h"
#include "UI/Backpack/WacomBackpackStorageRefreshGate.h"
#include "UI/Backpack/WacomBackpackWorkspaceReconciler.h"
#include "UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "UI/Backpack/WacomBackpackWorkspaceLayoutSolver.h"
#include "UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackDeleteConfirmWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomPrimaryGameLayout.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/ViewModels/WacomRunViewModel.h"
#include "UI/ViewModels/WacomRunViewModelProvider.h"

#if WITH_EDITOR
#include "UI/Backpack/WacomBackpackPIEValidationState.h"
#endif

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
#if WITH_EDITOR
	const EWacomBackpackPIEValidationMode PIEValidationMode =
		GetWacomBackpackPIEValidationMode();
	bPIEValidationEmptySnapshot =
		PIEValidationMode == EWacomBackpackPIEValidationMode::EmptySnapshot;
	if (PIEValidationMode == EWacomBackpackPIEValidationMode::NativeFallback)
	{
		CardWidgetClass = UWacomDeckCardWidget::StaticClass();
		CardDetailPanelClass = UWacomCardDetailPanel::StaticClass();
		WorkspaceWidgetClass = UWacomBackpackWorkspaceWidget::StaticClass();
		DeleteConfirmWidgetClass = UWacomBackpackDeleteConfirmWidget::StaticClass();
		return;
	}
#endif

	if (!CardWidgetClass)
	{
		CardWidgetClass = LoadOptionalWidgetClass<UWacomDeckCardWidget>(
			TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	}

	if (!CardDetailPanelClass)
	{
		CardDetailPanelClass = LoadOptionalWidgetClass<UWacomCardDetailPanel>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackCardDetailPanel.WBP_BackpackCardDetailPanel_C"));
	}
	if (!WorkspaceWidgetClass)
	{
		WorkspaceWidgetClass = UWacomBackpackWorkspaceWidget::StaticClass();
	}
	if (!DeleteConfirmWidgetClass)
	{
		DeleteConfirmWidgetClass = UWacomBackpackDeleteConfirmWidget::StaticClass();
	}
	if (!ControlsHelpWidgetClass)
	{
		ControlsHelpWidgetClass = LoadOptionalWidgetClass<UWacomBackpackControlsHelpWidget>(
			TEXT("/Game/Wacom/UI/Backpack/WBP_BackpackControlsHelp.WBP_BackpackControlsHelp_C"));
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
			&TitleText,
			&GoldText,
			&InteractionHintText,
			&ControlsHelpButton,
			&ControlsHelpHost,
			&WorkspaceHost,
			&DeleteTargetHost,
			&DeleteTargetBackground,
			&DeleteTargetOutline,
			&DeleteTargetIcon,
			&DeleteTargetFocusIcon,
			&DeleteTargetLabel,
			&DeleteTargetCountText,
			&DeleteConfirmHost,
			&ArrangeAllButton,
			&ResetPilePositionsButton,
			&CardDetailLayer,
			&CardDetailDockHost,
			&CardDetailDockSize,
			&CardDetailEmptyText,
			&CloseButton
		});
	}
	EnsureWorkspaceWidgets();
	return Super::RebuildWidget();
}

void UWacomBackpackScreen::NativeConstruct()
{
	Super::NativeConstruct();

	EnsureWorkspaceWidgets();
	UpdateCardDetailPlacementMode();
	UpdateDeleteTargetPresentation(false, false, 0);

	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackScreen::HandleCloseClicked);
	}
	if (ArrangeAllButton)
	{
		ArrangeAllButton->OnClicked.AddUniqueDynamic(this, &UWacomBackpackScreen::HandleArrangeAllClicked);
	}
	if (ResetPilePositionsButton)
	{
		ResetPilePositionsButton->OnClicked.AddUniqueDynamic(
			this, &UWacomBackpackScreen::HandleResetPilePositionsClicked);
	}
	if (ControlsHelpButton)
	{
		ControlsHelpButton->OnClicked.AddUniqueDynamic(
			this, &ThisClass::HandleControlsHelpClicked);
	}

	RefreshInteractionHints();
}

void UWacomBackpackScreen::NativeDestruct()
{
	CancelWorkspaceInteraction();
	// Workspace children deliberately release transient bindings in NativeDestruct.
	// The next construction must therefore perform an authoritative reconcile even
	// when the Run storage revision itself has not changed.
	ResetBackpackRefreshDirtyGate();
	UnbindActiveSubscriptions();
	HideControlsHelp(false);
	bOwningLayerTransitioning = false;
	bCardDetailDocked = false;
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
	if (ResetPilePositionsButton)
	{
		ResetPilePositionsButton->OnClicked.RemoveDynamic(
			this, &UWacomBackpackScreen::HandleResetPilePositionsClicked);
	}
	if (ControlsHelpButton)
	{
		ControlsHelpButton->OnClicked.RemoveDynamic(
			this, &ThisClass::HandleControlsHelpClicked);
	}
	if (WorkspaceWidget)
	{
		WorkspaceWidget->OnLayoutGeometryReadyNative.RemoveAll(this);
	}

	Super::NativeDestruct();
}

void UWacomBackpackScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	BindActiveSubscriptions();
	// CommonUI Stack 重新激活时可能错过事件；RefreshGate 会在 revision 变化时
	// 重建权威 Scene，未变化时则保留现有 Widget 身份与缓存。
	RebuildAll();
	if (WorkspaceWidget)
	{
		WorkspaceWidget->RequestLayoutGeometryRefresh();
		WorkspaceWidget->SetKeyboardFocus();
#if WITH_AUTOMATION_TESTS
		++WorkspaceFocusRequestCountForTest;
#endif
	}
	RefreshInteractionHints();
}

void UWacomBackpackScreen::BindActiveSubscriptions()
{
	BindOwningLayerTransition();
	BindRuntimeSettings();
	BindCommonInput();
	BindRunViewModelProvider();
}

void UWacomBackpackScreen::UnbindActiveSubscriptions()
{
	UnbindRunViewModelProvider();
	UnbindCommonInput();
	UnbindRuntimeSettings();
	UnbindOwningLayerTransition();
}

void UWacomBackpackScreen::BindRunViewModelProvider()
{
	UWacomRunViewModelProvider* Provider = GetProvider();
	if (SubscribedProvider.Get() == Provider)
	{
		return;
	}

	UnbindRunViewModelProvider();
	if (Provider)
	{
		Provider->OnRunViewModelRefreshedNative.AddUObject(
			this, &UWacomBackpackScreen::HandleViewModelRefreshed);
		SubscribedProvider = Provider;
	}
}

void UWacomBackpackScreen::UnbindRunViewModelProvider()
{
	if (UWacomRunViewModelProvider* Provider = SubscribedProvider.Get())
	{
		Provider->OnRunViewModelRefreshedNative.RemoveAll(this);
	}
	SubscribedProvider = nullptr;
}

void UWacomBackpackScreen::HandleViewModelRefreshed()
{
	if (!IsActivated())
	{
		return;
	}
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
#if WITH_AUTOMATION_TESTS
	if (UWacomRunViewModelProvider* Override = RunViewModelProviderOverrideForTest.Get())
	{
		return Override;
	}
#endif
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
	HideControlsHelp(false);
	if (WorkspaceWidget)
	{
		WorkspaceWidget->ResetSaleDepartures();
	}
	CancelWorkspaceInteraction();
	HideCardDetailPanel();
	UnbindActiveSubscriptions();
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

bool UWacomBackpackScreen::IsCardDetailPanelVisible() const
{
	return GetCardDetailController().IsVisible();
}

FText UWacomBackpackScreen::GetCardDetailPanelNameText() const
{
	return GetCardDetailController().GetNameText();
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
		WorkspaceWidget->OnBrowseFocusChangedNative.RemoveAll(this);
		WorkspaceWidget->OnBrowseFocusChangedNative.AddUObject(
			this, &UWacomBackpackScreen::HandleWorkspaceBrowseFocusChanged);
		WorkspaceWidget->OnLayoutGeometryReadyNative.RemoveAll(this);
		WorkspaceWidget->OnLayoutGeometryReadyNative.AddUObject(
			this,
			&UWacomBackpackScreen::HandleWorkspaceLayoutGeometryReady);
		WorkspaceWidget->OnPileExpansionRequestedNative.RemoveAll(this);
		WorkspaceWidget->OnPileExpansionRequestedNative.AddUObject(
			this,
			&UWacomBackpackScreen::HandlePileExpansionRequested);
		WorkspaceWidget->OnPileMoveCommittedNative.RemoveAll(this);
		WorkspaceWidget->OnPileMoveCommittedNative.AddUObject(
			this,
			&UWacomBackpackScreen::HandlePileMoveCommitted);
		WorkspaceWidget->OnCollapseExpandedPileRequestedNative.RemoveAll(this);
		WorkspaceWidget->OnCollapseExpandedPileRequestedNative.AddUObject(
			this,
			&UWacomBackpackScreen::HandleCollapseExpandedPileRequested);
		WorkspaceWidget->OnPileCollapseAnimationFinishedNative.RemoveAll(this);
		WorkspaceWidget->OnPileCollapseAnimationFinishedNative.AddUObject(
			this,
			&UWacomBackpackScreen::HandlePileCollapseAnimationFinished);
		WorkspaceWidget->OnControlsHelpRequestedNative.RemoveAll(this);
		WorkspaceWidget->OnControlsHelpRequestedNative.AddUObject(
			this, &ThisClass::ShowControlsHelp);
	}
	// The legacy confirmation host remains bound for serialized WBP compatibility,
	// but selling is now committed atomically as soon as a valid drop is released.
	if (DeleteConfirmHost)
	{
		DeleteConfirmHost->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ControlsHelpHost && !ControlsHelpWidget)
	{
		UClass* ClassToUse = ControlsHelpWidgetClass
			? ControlsHelpWidgetClass.Get()
			: UWacomBackpackControlsHelpWidget::StaticClass();
		ControlsHelpWidget = CreateWidget<UWacomBackpackControlsHelpWidget>(this, ClassToUse);
		if (ControlsHelpWidget)
		{
			ControlsHelpWidget->OnCloseRequestedNative.RemoveAll(this);
			ControlsHelpWidget->OnCloseRequestedNative.AddUObject(
				this, &ThisClass::HideControlsHelp, true);
			AttachChildToHostAndFill(*ControlsHelpHost, *ControlsHelpWidget);
			ControlsHelpHost->SetVisibility(ESlateVisibility::Collapsed);
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

#if WITH_AUTOMATION_TESTS
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
	View.WorkspaceFocusRequestCount = WorkspaceFocusRequestCountForTest;
	View.bHasRunViewModelProviderSubscription = SubscribedProvider != nullptr;
	View.bHasRuntimeSettingsSubscription = BoundSettingsSubsystem.IsValid()
		&& RuntimeSettingsChangedHandle.IsValid();
	View.bHasCommonInputSubscription = BoundCommonInputSubsystem.IsValid();
	View.bHasOwningLayerTransitionSubscription = BoundPrimaryLayout.IsValid();
	View.CurrentInputType = CurrentInputType;
	if (WorkspaceWidget)
	{
		const FWacomBackpackWorkspaceAutomationTestView WorkspaceView = WorkspaceWidget->GetAutomationTestView();
		View.WorkspaceCardCount = WorkspaceView.WorkspaceCardCount;
		View.WorkspacePileCount = WorkspaceView.PileCount;
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
		if (WorkspaceWidget)
		{
			WorkspaceWidget->ResetWorkspaceScene();
		}
		HideCardDetailPanel();
		ResetBackpackRefreshDirtyGate();
		return;
	}

#if WITH_EDITOR
	if (bPIEValidationEmptySnapshot)
	{
		FRunBackpackStorageSnapshot EmptySnapshot = Run->BuildBackpackStorageSnapshot();
		EmptySnapshot.Flux.MainCards.Reset();
		EmptySnapshot.Flux.ContentCards.Reset();
		EmptySnapshot.Flux.FluxCapacity = 0;
		EmptySnapshot.SpecialZones.Reset();
		EmptySnapshot.BurdenCards.Reset();
		EmptySnapshot.BattleDeckPhysicalCards.Reset();
		EmptySnapshot.BattleDeckProjectedCards.Reset();
		EmptySnapshot.FluxCapacity = 0;
		EmptySnapshot.BattleDeckCapacity = 0;
		EmptySnapshot.BackpackPhysicalCount = 0;
		EmptySnapshot.FluxContentCount = 0;
		EmptySnapshot.BattleDeckPhysicalCount = 0;
		EmptySnapshot.BurdenCount = 0;
		EmptySnapshot.bDeleteFunctionAvailable = false;

		bHasLastAppliedStorageSnapshot = true;
		LastAppliedStorageSnapshot = EmptySnapshot;
		RebuildWorkspaceChrome(EmptySnapshot);
		HandleWorkspaceInteractionChanged();
		return;
	}
#endif

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
	if (WorkspaceInteractionModel && WorkspaceInteractionModel->IsCarrying())
	{
		// An authoritative refresh may preserve a still-valid Carry after an
		// optimistic transaction was rejected. Advance its expected revision
		// only after the accepted Snapshot has reconciled the carried identities,
		// so a retry does not remain permanently stale.
		WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(
			Run->GetBackpackStorageSnapshotRevision());
	}
	HandleWorkspaceInteractionChanged();
}

void UWacomBackpackScreen::RebuildWorkspaceChrome(const FRunBackpackStorageSnapshot& Snapshot)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(Wacom_Backpack_WorkspaceChromeRebuild);
	EnsureWorkspaceWidgets();
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceWidget)
	{
		return;
	}

	FWacomBackpackWorkspaceStateStore& StateStore = GetWorkspaceStateStore(Run);
	FWacomBackpackWorkspaceReconciler::Reconcile(
		*WorkspaceWidget,
		Snapshot,
		StateStore,
		WorkspaceInteractionModel.Get(),
		WorkspaceStyle,
		[this](const FRunStorageCardView& CardView) { return CreateCardWidget(CardView); },
		[this](UWacomDeckCardWidget* RemovedWidget) { HideCardDetailPanelIfSourceRemoved(RemovedWidget); },
		nullptr,
		Run->GetBackpackStorageSnapshotRevision());
	GetCardDetailController().RepositionVisibleSource();
}

void UWacomBackpackScreen::RebuildWorkspaceFromCachedSnapshot()
{
	if (bHasLastAppliedStorageSnapshot)
	{
		RebuildWorkspaceChrome(LastAppliedStorageSnapshot);
	}
}

void UWacomBackpackScreen::HandlePileExpansionRequested(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	bool bExpandOnly)
{
	URunSession* Run = GetRunSession();
	if (!Run || !bHasLastAppliedStorageSnapshot)
	{
		return;
	}
	const FWacomBackpackZoneKey Requested = FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId);
	if (!Requested.IsValid() || Requested.Zone == EZoneKind::Backpack)
	{
		return;
	}
	FWacomBackpackWorkspaceStateStore& Store = GetWorkspaceStateStore(Run);
	if (Store.IsPileExpanded(Requested))
	{
		Store.BringPileToFront(Requested);
		if (bExpandOnly)
		{
			RebuildWorkspaceFromCachedSnapshot();
			return;
		}
		bHasPendingPileExpansionAfterCollapse = false;
		bPendingPileExpansionRequiresCarryHover = false;
		if (WorkspaceWidget
			&& WorkspaceWidget->BeginPileCollapseAnimation(Zone, OwnerInstanceId))
		{
			return;
		}
		Store.SetExpandedPile(TOptional<FWacomBackpackZoneKey>());
		RebuildWorkspaceFromCachedSnapshot();
		return;
	}

	if (Store.GetExpandedPile().IsSet())
	{
		const FWacomBackpackZoneKey Previous = Store.GetExpandedPile().GetValue();
		if (WorkspaceWidget
			&& WorkspaceWidget->BeginPileCollapseAnimation(
				Previous.Zone, Previous.OwnerInstanceId))
		{
			bHasPendingPileExpansionAfterCollapse = true;
			bPendingPileExpansionRequiresCarryHover = bExpandOnly;
			PendingPileExpansionZone = Requested.Zone;
			PendingPileExpansionOwnerInstanceId = Requested.OwnerInstanceId;
			return;
		}
	}
	bHasPendingPileExpansionAfterCollapse = false;
	bPendingPileExpansionRequiresCarryHover = false;
	Store.SetExpandedPile(Requested);
	Store.BringPileToFront(Requested);
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandlePileMoveCommitted(
	EZoneKind Zone,
	FGuid OwnerInstanceId,
	FVector2D NormalizedPosition)
{
	URunSession* Run = GetRunSession();
	const FWacomBackpackZoneKey Key = FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId);
	if (!Run || !Key.IsValid() || Zone == EZoneKind::BurdenZone)
	{
		return;
	}
	FWacomBackpackWorkspaceStateStore& Store = GetWorkspaceStateStore(Run);
	FWacomBackpackWorkspacePileLayoutEntry Entry;
	if (const FWacomBackpackWorkspacePileLayoutEntry* Existing = Store.FindPileLayout(Key))
	{
		Entry = *Existing;
	}
	Entry.NormalizedPosition = FVector2D(
		FMath::Clamp(NormalizedPosition.X, 0.0f, 1.0f),
		FMath::Clamp(NormalizedPosition.Y, 0.0f, 1.0f));
	Entry.LayerRank = Store.BringPileToFront(Key);
	Entry.bHasManualPlacement = true;
	Store.SetPileLayout(Key, Entry);
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandleCollapseExpandedPileRequested()
{
	bHasPendingPileExpansionAfterCollapse = false;
	bPendingPileExpansionRequiresCarryHover = false;
	URunSession* Run = GetRunSession();
	if (!Run)
	{
		return;
	}
	FWacomBackpackWorkspaceStateStore& Store = GetWorkspaceStateStore(Run);
	if (Store.GetExpandedPile().IsSet() && WorkspaceWidget)
	{
		const FWacomBackpackZoneKey Expanded = Store.GetExpandedPile().GetValue();
		if (WorkspaceWidget->BeginPileCollapseAnimation(
			Expanded.Zone, Expanded.OwnerInstanceId))
		{
			return;
		}
	}
	Store.SetExpandedPile(TOptional<FWacomBackpackZoneKey>());
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandlePileCollapseAnimationFinished(
	EZoneKind Zone,
	FGuid OwnerInstanceId)
{
	URunSession* Run = GetRunSession();
	if (!Run)
	{
		return;
	}
	FWacomBackpackWorkspaceStateStore& Store = GetWorkspaceStateStore(Run);
	const FWacomBackpackZoneKey Completed = FWacomBackpackZoneKey::Make(Zone, OwnerInstanceId);
	if (Store.IsPileExpanded(Completed))
	{
		Store.SetExpandedPile(TOptional<FWacomBackpackZoneKey>());
		if (bHasPendingPileExpansionAfterCollapse)
		{
			const FWacomBackpackZoneKey Pending = FWacomBackpackZoneKey::Make(
				PendingPileExpansionZone,
				PendingPileExpansionOwnerInstanceId);
			bool bStillRequested = true;
			if (bPendingPileExpansionRequiresCarryHover)
			{
				FWacomBackpackZoneKey CurrentTarget;
				bStillRequested = WorkspaceInteractionModel
					&& WorkspaceInteractionModel->IsCarrying()
					&& ResolveWorkspacePileTarget(CurrentTarget)
					&& CurrentTarget == Pending;
			}
			bHasPendingPileExpansionAfterCollapse = false;
			bPendingPileExpansionRequiresCarryHover = false;
			if (bStillRequested && Pending.IsValid())
			{
				Store.SetExpandedPile(Pending);
				Store.BringPileToFront(Pending);
			}
		}
		RebuildWorkspaceFromCachedSnapshot();
	}
}

void UWacomBackpackScreen::HandleWorkspaceInteractionChanged()
{
	RefreshInteractionHints();
	if (WorkspaceInteractionModel && WorkspaceInteractionModel->IsCarrying())
	{
		HideCardDetailPanel();
		const FWacomBackpackWorkspaceCarryState& Carry = WorkspaceInteractionModel->GetCarry();
		const bool bDeleteTarget = IsWorkspaceDeleteTarget();
		if (bDeleteTarget)
		{
			TArray<FGuid, TInlineAllocator<1>> PrimaryReleaseIds;
			if (Carry.RemainingInstanceIds.IsValidIndex(Carry.CurrentIndex))
			{
				PrimaryReleaseIds.Add(
					Carry.RemainingInstanceIds[Carry.CurrentIndex]);
			}
			const FRunDeckBatchDeleteRequest DeleteRequest =
				FWacomBackpackCommandFlow::BuildBatchDeleteRequest(
					Carry,
					PrimaryReleaseIds);
			const FRunDeckBatchDeletePreview DeletePreview =
				FWacomBackpackCommandFlow::PreviewBatchDelete(
					GetRunSession(),
					DeleteRequest);
			const bool bDeleteAllowed = DeletePreview.Validation.bCanExecute;
			UpdateDeleteTargetPresentation(
				true,
				true,
				Carry.RemainingInstanceIds.Num(),
				bDeleteAllowed
					? NAME_None
					: DeletePreview.Validation.DisabledReason);
			if (WorkspaceWidget)
			{
				WorkspaceWidget->SetPileDropFeedback(
					EZoneKind::Backpack,
					FGuid(),
					FWacomBackpackDropFeedbackView());
				WorkspaceWidget->SetCarryDropFeedbackState(
					bDeleteAllowed,
					!bDeleteAllowed);
			}
			return;
		}
		UpdateDeleteTargetPresentation(
			true,
			false,
			Carry.RemainingInstanceIds.Num());
		FWacomBackpackZoneKey Target;
		if (ResolveWorkspacePileTarget(Target) && WorkspaceWidget)
		{
			FWacomBackpackDropFeedbackView Feedback;
			Feedback.State = EWacomBackpackDropFeedbackState::Valid;
			FWacomBackpackZonePileView TargetView;
			if (WorkspaceWidget->FindPileView(Target.Zone, Target.OwnerInstanceId, TargetView))
			{
				Feedback.CurrentCount = TargetView.CardCount;
				Feedback.Capacity = TargetView.Capacity;
				Feedback.bHasCapacity = TargetView.bHasCapacity;
			}
			const bool bReturningToSource = Carry.SourceZone == Target;
			Feedback.IncomingCount = bReturningToSource
				? 0
				: Carry.RemainingInstanceIds.Num();
			Feedback.Message = FText::Format(
				bReturningToSource
					? LOCTEXT("ReturnToPileFeedback", "放回 {0}")
					: LOCTEXT("MoveToPileFeedback", "放入 {0}"),
				TargetView.Title.IsEmpty()
					? LOCTEXT("UnknownPileFeedback", "目标区域")
					: TargetView.Title);
			if (!(Carry.SourceZone == Target))
			{
				const FRunDeckBatchMoveRequest PreviewRequest = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
					Carry,
					Target,
					Carry.RemainingInstanceIds);
				URunSession* Run = GetRunSession();
				FRunDeckBatchOperationValidation Validation;
				if (Run)
				{
					Validation = Run->ValidateMoveInstancesAtomic(PreviewRequest);
				}
				if (!Run || !Validation.bCanExecute)
				{
					Feedback.State = EWacomBackpackDropFeedbackState::Rejected;
					Feedback.Message = FWacomBackpackCommandFlow::BuildMoveFailureToastText(
						Run ? Validation.DisabledReason : NAME_None);
				}
			}
			WorkspaceWidget->SetPileDropFeedback(
				Target.Zone, Target.OwnerInstanceId, Feedback);
			WorkspaceWidget->SetCarryDropFeedbackState(
				Feedback.State == EWacomBackpackDropFeedbackState::Valid,
				Feedback.State == EWacomBackpackDropFeedbackState::Rejected);
			return;
		}
		if (WorkspaceWidget)
		{
			WorkspaceWidget->SetPileDropFeedback(
				EZoneKind::Backpack,
				FGuid(),
				FWacomBackpackDropFeedbackView());
			EWacomBackpackWorkspaceReleaseTargetKind FocusedKind =
				EWacomBackpackWorkspaceReleaseTargetKind::Pointer;
			FWacomBackpackZoneKey FocusedZone;
			const bool bSemanticFlux = WorkspaceWidget->GetFocusedReleaseTarget(
				FocusedKind, FocusedZone)
				&& FocusedKind == EWacomBackpackWorkspaceReleaseTargetKind::Flux;
			const bool bPointerFlux = FSlateApplication::IsInitialized()
				&& WorkspaceWidget->GetCachedGeometry().IsUnderLocation(
					FSlateApplication::Get().GetCursorPos());
			WorkspaceWidget->SetCarryDropFeedbackState(
				bSemanticFlux || bPointerFlux,
				false);
		}
		return;
	}
	UpdateDeleteTargetPresentation(false, false, 0);
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetCarryDropFeedbackState(false, false);
		WorkspaceWidget->SetPileDropFeedback(
			EZoneKind::Backpack,
			FGuid(),
			FWacomBackpackDropFeedbackView());
	}
}

void UWacomBackpackScreen::HandleWorkspaceLayoutGeometryReady(FVector2D LayoutSize)
{
	UpdateCardDetailPlacementMode();
	if (LayoutSize.X > 1.0f && LayoutSize.Y > 1.0f)
	{
		RebuildWorkspaceFromCachedSnapshot();
	}
}

void UWacomBackpackScreen::UpdateCardDetailPlacementMode()
{
	const UWacomBackpackWorkspaceStyle* Style = WorkspaceStyle
		? WorkspaceStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	float LogicalWidth = GetCachedGeometry().GetLocalSize().X;
	if (LogicalWidth <= 1.0f)
	{
		const float ViewportScale = FMath::Max(0.01f, UWidgetLayoutLibrary::GetViewportScale(this));
		LogicalWidth = UWidgetLayoutLibrary::GetViewportSize(this).X / ViewportScale;
	}
	const bool bShouldDock = CardDetailDockHost
		&& CardDetailDockSize
		&& FWacomBackpackCardDetailController::ShouldUseDockedMode(
			LogicalWidth,
			Style->DetailDockBreakpointPixels);
	if (CardDetailDockSize)
	{
		CardDetailDockSize->SetWidthOverride(FMath::Max(1.0f, Style->DetailDockWidthPixels));
		CardDetailDockSize->SetVisibility(bShouldDock
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	const bool bModeChanged = bCardDetailDocked != bShouldDock;
	bCardDetailDocked = bShouldDock;
	if (bModeChanged && CardDetailController)
	{
		CardDetailController->RepositionVisibleSource();
	}
	SetCardDetailOccupied(IsCardDetailPanelVisible());
}

void UWacomBackpackScreen::UpdateDeleteTargetPresentation(
	bool bCarrying,
	bool bPointerInside,
	int32 CardCount,
	FName DisabledReason)
{
	const UWacomBackpackWorkspaceStyle* Style = WorkspaceStyle
		? WorkspaceStyle.Get()
		: GetDefault<UWacomBackpackWorkspaceStyle>();
	const FWacomBackpackZoneAppearance& Appearance = Style->DestructiveAppearance;
	const bool bDeleteFunctionAvailable = bHasLastAppliedStorageSnapshot
		&& LastAppliedStorageSnapshot.bDeleteFunctionAvailable;
	bDeleteTargetRejectedPresentation =
		!bDeleteFunctionAvailable || !DisabledReason.IsNone();
	if (DeleteTargetBackground)
	{
		FLinearColor Surface = Appearance.SurfaceColor;
		if (bPointerInside && !bDeleteTargetRejectedPresentation)
		{
			Surface = FMath::Lerp(Surface, Appearance.AccentColor, 0.42f);
		}
		else if (bCarrying)
		{
			Surface = FMath::Lerp(Surface, Appearance.AccentColor, 0.16f);
		}
		if (bDeleteTargetRejectedPresentation)
		{
			Surface = FMath::Lerp(
				Surface,
				FLinearColor(0.08f, 0.08f, 0.08f, Surface.A),
				0.52f);
		}
		Surface.A = bCarrying ? 0.96f : 0.72f;
		DeleteTargetBackground->SetBrushColor(Surface);
	}
	if (DeleteTargetOutline)
	{
		if (Appearance.FrameBrush.GetResourceObject())
		{
			DeleteTargetOutline->SetBrush(Appearance.FrameBrush);
		}
		FLinearColor Outline = Appearance.AccentColor;
		Outline.A = bPointerInside && !bDeleteTargetRejectedPresentation
			? 1.0f
			: (bCarrying ? 0.78f : 0.42f);
		DeleteTargetOutline->SetBrushColor(Outline);
		DeleteTargetOutline->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	if (DeleteTargetIcon)
	{
		const FSlateBrush& IconBrush = bDeleteTargetRejectedPresentation
			? Style->RejectedDropStateIconBrush
			: Appearance.IconBrush;
		const bool bHasIcon = IconBrush.GetResourceObject() != nullptr;
		if (bHasIcon)
		{
			DeleteTargetIcon->SetBrush(IconBrush);
			DeleteTargetIcon->SetColorAndOpacity(
				bDeleteTargetRejectedPresentation
					? FLinearColor::White
					: Appearance.AccentColor);
		}
		DeleteTargetIcon->SetVisibility(bHasIcon
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DeleteTargetFocusIcon)
	{
		EWacomBackpackWorkspaceReleaseTargetKind FocusedKind =
			EWacomBackpackWorkspaceReleaseTargetKind::Pointer;
		FWacomBackpackZoneKey FocusedZone;
		const bool bDeleteFocused = bCarrying
			&& WorkspaceWidget
			&& WorkspaceWidget->GetFocusedReleaseTarget(FocusedKind, FocusedZone)
			&& FocusedKind == EWacomBackpackWorkspaceReleaseTargetKind::Delete;
		DeleteTargetFocusIcon->SetBrush(Style->FocusStateIconBrush);
		DeleteTargetFocusIcon->SetVisibility(bDeleteFocused
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
	if (DeleteTargetLabel)
	{
		DeleteTargetLabel->SetText(!DisabledReason.IsNone()
			? FWacomBackpackCommandFlow::BuildDeleteFailureToastText(
				DisabledReason)
			: (bPointerInside && bDeleteFunctionAvailable
				? LOCTEXT("DeleteTargetRelease", "释放以销毁")
				: (bCarrying && bDeleteFunctionAvailable
					? LOCTEXT("DeleteTargetCarry", "拖到这里销毁")
					: LOCTEXT("DeleteTargetIdle", "销毁区"))));
	}
	if (DeleteTargetCountText)
	{
		DeleteTargetCountText->SetText(!bDeleteFunctionAvailable
			? LOCTEXT(
				"DeleteTargetProviderRequired",
				"需要删牌能力卡")
			: (CardCount > 0
				? FText::Format(
					LOCTEXT("DeleteTargetCardCount", "{0} 张卡牌"),
					FText::AsNumber(CardCount))
				: FText::GetEmpty()));
		DeleteTargetCountText->SetVisibility(
			!bDeleteFunctionAvailable || CardCount > 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}
}

void UWacomBackpackScreen::SetCardDetailOccupied(bool bOccupied)
{
	if (CardDetailEmptyText)
	{
		CardDetailEmptyText->SetVisibility(!bOccupied && bCardDetailDocked
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
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
#if WITH_AUTOMATION_TESTS
	Layout = PrimaryLayoutOverrideForTest.Get();
#endif
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (!Layout)
		{
			if (UWacomGameUIManagerSubsystem* UIManager =
				GameInstance->GetSubsystem<UWacomGameUIManagerSubsystem>())
			{
				Layout = UIManager->GetPrimaryLayout();
			}
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

void UWacomBackpackScreen::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = nullptr;
#if WITH_AUTOMATION_TESTS
	Settings = SettingsSubsystemOverrideForTest.Get();
#endif
	if (!Settings && GetGameInstance())
	{
		Settings = GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>();
	}
	if (BoundSettingsSubsystem.Get() == Settings && RuntimeSettingsChangedHandle.IsValid())
	{
		HandleRuntimeSettingsChanged(
			Settings->GetCurrentSnapshot(),
			EWacomRuntimeSettingsChangeReason::Startup);
		return;
	}
	UnbindRuntimeSettings();
	if (!Settings)
	{
		return;
	}
	BoundSettingsSubsystem = Settings;
	RuntimeSettingsChangedHandle = Settings->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomBackpackScreen::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		Settings->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomBackpackScreen::UnbindRuntimeSettings()
{
	if (UWacomSettingsSubsystem* Settings = BoundSettingsSubsystem.Get())
	{
		if (RuntimeSettingsChangedHandle.IsValid())
		{
			Settings->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
		}
	}
	BoundSettingsSubsystem.Reset();
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomBackpackScreen::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetSimplifiedMotion(
			Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified);
	}
}

void UWacomBackpackScreen::BindCommonInput()
{
	UCommonInputSubsystem* InputSubsystem = nullptr;
#if WITH_AUTOMATION_TESTS
	InputSubsystem = CommonInputSubsystemOverrideForTest.Get();
#endif
	if (!InputSubsystem)
	{
		ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
		InputSubsystem = LocalPlayer
			? LocalPlayer->GetSubsystem<UCommonInputSubsystem>()
			: nullptr;
	}
	if (BoundCommonInputSubsystem.Get() == InputSubsystem)
	{
		if (InputSubsystem)
		{
			HandleInputMethodChanged(InputSubsystem->GetCurrentInputType());
		}
		return;
	}

	UnbindCommonInput();
	BoundCommonInputSubsystem = InputSubsystem;
	if (InputSubsystem)
	{
		InputSubsystem->OnInputMethodChangedNative.AddUObject(
			this, &ThisClass::HandleInputMethodChanged);
		HandleInputMethodChanged(InputSubsystem->GetCurrentInputType());
	}
}

void UWacomBackpackScreen::UnbindCommonInput()
{
	if (UCommonInputSubsystem* InputSubsystem = BoundCommonInputSubsystem.Get())
	{
		InputSubsystem->OnInputMethodChangedNative.RemoveAll(this);
	}
	BoundCommonInputSubsystem.Reset();
}

void UWacomBackpackScreen::HandleInputMethodChanged(ECommonInputType InputType)
{
	CurrentInputType = InputType;
	RefreshInteractionHints();
}

void UWacomBackpackScreen::RefreshInteractionHints()
{
	const EWacomBackpackWorkspaceInteractionMode InteractionMode = WorkspaceInteractionModel
		? WorkspaceInteractionModel->GetMode()
		: EWacomBackpackWorkspaceInteractionMode::Idle;
	bool bExpandedPile = false;
	if (URunSession* Run = GetRunSession())
	{
		bExpandedPile = GetWorkspaceStateStore(Run).GetExpandedPile().IsSet();
	}
	const FWacomBackpackInteractionHintView View =
		FWacomBackpackInteractionHintPresenter::Build(
			CurrentInputType,
			InteractionMode,
			bExpandedPile);
	if (InteractionHintText)
	{
		InteractionHintText->SetText(View.ContextHint);
	}
	if (ControlsHelpWidget)
	{
		ControlsHelpWidget->SetHelpText(View.HelpText);
	}
}

void UWacomBackpackScreen::HandleControlsHelpClicked()
{
	if (IsControlsHelpVisible())
	{
		HideControlsHelp(true);
	}
	else
	{
		ShowControlsHelp();
	}
}

void UWacomBackpackScreen::ShowControlsHelp()
{
	EnsureWorkspaceWidgets();
	if (!ControlsHelpHost || !ControlsHelpWidget || IsControlsHelpVisible())
	{
		return;
	}

	RefreshInteractionHints();
	if (FSlateApplication::IsInitialized())
	{
		FocusBeforeControlsHelp = FSlateApplication::Get().GetUserFocusedWidget(0);
	}
	ControlsHelpHost->SetVisibility(ESlateVisibility::Visible);
	ControlsHelpWidget->SetKeyboardFocus();
}

void UWacomBackpackScreen::HideControlsHelp(bool bRestoreFocus)
{
	if (ControlsHelpHost)
	{
		ControlsHelpHost->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (!bRestoreFocus)
	{
		FocusBeforeControlsHelp.Reset();
		return;
	}

	bool bRestored = false;
	if (FSlateApplication::IsInitialized())
	{
		if (const TSharedPtr<SWidget> PreviousFocus = FocusBeforeControlsHelp.Pin())
		{
			bRestored = FSlateApplication::Get().SetUserFocus(
				0, PreviousFocus, EFocusCause::SetDirectly);
		}
	}
	FocusBeforeControlsHelp.Reset();
	if (!bRestored && WorkspaceWidget)
	{
		WorkspaceWidget->SetKeyboardFocus();
	}
}

bool UWacomBackpackScreen::IsControlsHelpVisible() const
{
	return ControlsHelpHost
		&& ControlsHelpHost->GetVisibility() != ESlateVisibility::Collapsed;
}

bool UWacomBackpackScreen::ResolveWorkspacePileTarget(FWacomBackpackZoneKey& OutTarget) const
{
	if (!WorkspaceWidget || !WorkspaceInteractionModel || !WorkspaceInteractionModel->IsCarrying())
	{
		return false;
	}
	EWacomBackpackWorkspaceReleaseTargetKind SemanticKind;
	FWacomBackpackZoneKey SemanticZone;
	if (WorkspaceWidget->GetFocusedReleaseTarget(SemanticKind, SemanticZone))
	{
		if (SemanticKind == EWacomBackpackWorkspaceReleaseTargetKind::Pile)
		{
			OutTarget = SemanticZone;
			return OutTarget.IsValid();
		}
		return false;
	}
	const FVector2D AbsolutePointer = WorkspaceWidget->GetCachedGeometry().LocalToAbsolute(
		WorkspaceInteractionModel->GetCarry().PointerPosition);
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid OwnerInstanceId;
	if (!WorkspaceWidget->FindPileAtAbsolutePosition(AbsolutePointer, Zone, OwnerInstanceId))
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
	EWacomBackpackWorkspaceReleaseTargetKind SemanticKind;
	FWacomBackpackZoneKey SemanticZone;
	if (WorkspaceWidget->GetFocusedReleaseTarget(SemanticKind, SemanticZone))
	{
		return SemanticKind == EWacomBackpackWorkspaceReleaseTargetKind::Delete;
	}
	const FVector2D AbsolutePointer = WorkspaceWidget->GetCachedGeometry().LocalToAbsolute(
		WorkspaceInteractionModel->GetCarry().PointerPosition);
	return DeleteTargetHost->GetCachedGeometry().IsUnderLocation(AbsolutePointer);
}

void UWacomBackpackScreen::SubmitWorkspaceDelete(TConstArrayView<FGuid> InstanceIds)
{
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceInteractionModel || !WorkspaceInteractionModel->IsCarrying()
		|| InstanceIds.IsEmpty())
	{
		return;
	}
	const FWacomBackpackWorkspaceCarryState CarrySnapshot =
		WorkspaceInteractionModel->GetCarry();
	const FRunDeckBatchDeleteRequest Request = FWacomBackpackCommandFlow::BuildBatchDeleteRequest(
		CarrySnapshot,
		InstanceIds);
	const FRunDeckBatchDeletePreview Preview = FWacomBackpackCommandFlow::PreviewBatchDelete(Run, Request);
	if (!Preview.Validation.bCanExecute)
	{
		UpdateDeleteTargetPresentation(
			true,
			IsWorkspaceDeleteTarget(),
			CarrySnapshot.RemainingInstanceIds.Num(),
			Preview.Validation.DisabledReason);
		if (WorkspaceWidget)
		{
			WorkspaceWidget->SetCarryDropFeedbackState(false, true);
		}
		// Submit once to reuse the command flow's canonical failure toast. The Run
		// operation is atomic and performs the same validation before any mutation.
		FWacomBackpackCommandFlow::SubmitBatchDelete(*this, Run, Request);
		return;
	}
	BeginWorkspaceMutationRefreshDeferral();
	const FRunDeckBatchOperationResult Result = FWacomBackpackCommandFlow::SubmitBatchDelete(
		*this,
		Run,
		Request);
	if (!Result.bSucceeded)
	{
		EndWorkspaceMutationRefreshDeferral(false);
		return;
	}
	if (WorkspaceWidget)
	{
		// Capture and detach the original visual before the deferred authoritative
		// Snapshot reconcile removes the sold identities from the Registry.
		WorkspaceWidget->BeginSaleDeparture(InstanceIds);
	}
	WorkspaceInteractionModel->CommitReleasedCards(InstanceIds);
	WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(Result.StorageRevision);
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
	if (Intent.TargetKind == EWacomBackpackWorkspaceReleaseTargetKind::Delete)
	{
		SubmitWorkspaceDelete(Intent.InstanceIds);
		return;
	}
	if (Intent.TargetKind == EWacomBackpackWorkspaceReleaseTargetKind::Pile)
	{
		if (Intent.TargetZone.IsValid())
		{
			HandleWorkspacePileReleaseIntent(Intent, Intent.TargetZone);
		}
		return;
	}
	if (IsWorkspaceDeleteTarget())
	{
		SubmitWorkspaceDelete(Intent.InstanceIds);
		return;
	}
	FWacomBackpackZoneKey PileTarget;
	if (ResolveWorkspacePileTarget(PileTarget))
	{
		HandleWorkspacePileReleaseIntent(Intent, PileTarget);
		return;
	}
	const bool bSemanticFlux =
		Intent.TargetKind == EWacomBackpackWorkspaceReleaseTargetKind::Flux;
	const FVector2D AbsolutePointer = WorkspaceWidget->GetCachedGeometry().LocalToAbsolute(
		Carry.PointerPosition);
	if (!bSemanticFlux && !WorkspaceWidget->GetCachedGeometry().IsUnderLocation(AbsolutePointer))
	{
		// 工作台外既不是通量空白，也不是合法目标；保持携带并显示拒绝状态。
		return;
	}
	const FVector2D WorkspaceSize = WorkspaceWidget->GetLayoutSpaceSize();
	const FVector2D CardDisplaySize = Style->GetCardDisplaySize();
	const float AvailableStripWidth = FMath::Max(
		CardDisplaySize.X,
		WorkspaceSize.X - FMath::Max(0.0f, Style->PileEdgeMarginPixels) * 2.0f);
	const TArray<FWacomBackpackCarriedStripLayout> Strip =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedFocusWindowLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			Carry.PointerPosition,
			AvailableStripWidth,
			CardDisplaySize.X,
			Style->FocusWindowMaximumCards,
			Style->FocusWindowFullGapPixels,
			Style->FocusWindowCompressedExposurePixels,
			Style->FocusWindowMinimumExposurePixels,
			Style->CurrentCardLiftPixels);
	FWacomBackpackWorkspaceStateStore& StateStore = GetWorkspaceStateStore(Run);
	const FWacomBackpackZoneKey FluxZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	auto SaveFluxLayouts = [&]()
	{
		for (const FGuid InstanceId : Intent.InstanceIds)
		{
			const int32 CarryIndex = Carry.RemainingInstanceIds.IndexOfByKey(InstanceId);
			if (!Strip.IsValidIndex(CarryIndex))
			{
				continue;
			}
			const FVector2D ClampedCenter = FWacomBackpackWorkspaceLayoutSolver::ClampCardCenterToVisibleBounds(
				Strip[CarryIndex].Transform.CardCenter,
				WorkspaceSize,
				CardDisplaySize,
				Style->MinimumVisibleFraction);
			FWacomBackpackWorkspaceLayoutEntry Entry;
			Entry.NormalizedPosition = FVector2D(
				WorkspaceSize.X > 1.0f ? ClampedCenter.X / WorkspaceSize.X : 0.5f,
				WorkspaceSize.Y > 1.0f ? ClampedCenter.Y / WorkspaceSize.Y : 0.5f);
			Entry.AngleDegrees = Strip[CarryIndex].Transform.AngleDegrees;
			Entry.LayerRank = Strip[CarryIndex].Transform.LayerRank;
			Entry.bHasManualPlacement = true;
			StateStore.SetLayout(FluxZone, InstanceId, Entry);
		}
	};
	auto ApplyFluxLayoutPolicy = [&]()
	{
		if (!bSemanticFlux)
		{
			SaveFluxLayouts();
			return;
		}
		for (const FGuid InstanceId : Intent.InstanceIds)
		{
			StateStore.ClearLayout(FluxZone, InstanceId);
		}
	};
	if (!(Carry.SourceZone == FluxZone))
	{
		const FRunDeckBatchMoveRequest Request = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
			Carry,
			FluxZone,
			Intent.InstanceIds);
		BeginWorkspaceMutationRefreshDeferral();
		const FRunDeckBatchOperationResult Result = FWacomBackpackCommandFlow::SubmitBatchMove(
			*this, Run, Request);
		if (!Result.bSucceeded)
		{
			EndWorkspaceMutationRefreshDeferral(false);
			return;
		}
		ApplyFluxLayoutPolicy();
		WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
		WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(Result.StorageRevision);
		ResetBackpackRefreshDirtyGate();
		EndWorkspaceMutationRefreshDeferral(true);
		return;
	}
	ApplyFluxLayoutPolicy();
	WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandleWorkspacePileReleaseIntent(
	const FWacomBackpackWorkspaceReleaseIntent& Intent,
	const FWacomBackpackZoneKey& PileTarget)
{
	URunSession* Run = GetRunSession();
	if (!Run || !WorkspaceWidget || !WorkspaceInteractionModel
		|| !WorkspaceInteractionModel->IsCarrying() || Intent.InstanceIds.IsEmpty())
	{
		return;
	}

	const FWacomBackpackWorkspaceCarryState& Carry = WorkspaceInteractionModel->GetCarry();
	if (Carry.SourceZone == PileTarget)
	{
		FWacomBackpackWorkspaceStateStore& Store = GetWorkspaceStateStore(Run);
		FWacomBackpackCommandFlow::CollectSameZone(
			Store,
			Carry.SourceZone,
			PileTarget,
			Intent.InstanceIds);
		WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
		RebuildWorkspaceFromCachedSnapshot();
		return;
	}

	const FRunDeckBatchMoveRequest Request = FWacomBackpackCommandFlow::BuildBatchMoveRequest(
		Carry,
		PileTarget,
		Intent.InstanceIds);
	const FRunDeckBatchOperationValidation Validation =
		Run->ValidateMoveInstancesAtomic(Request);
	if (!Validation.bCanExecute)
	{
		return;
	}
	BeginWorkspaceMutationRefreshDeferral();
	const FRunDeckBatchOperationResult Result = FWacomBackpackCommandFlow::SubmitBatchMove(*this, Run, Request);
	if (Result.bSucceeded)
	{
		WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
		WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(Result.StorageRevision);
		ResetBackpackRefreshDirtyGate();
	}
	EndWorkspaceMutationRefreshDeferral(Result.bSucceeded);
}

void UWacomBackpackScreen::CancelWorkspaceInteraction()
{
	const bool bWasSwitchingExpandedPile = bHasPendingPileExpansionAfterCollapse;
	bHasPendingPileExpansionAfterCollapse = false;
	bPendingPileExpansionRequiresCarryHover = false;
	if (WorkspaceWidget)
	{
		WorkspaceWidget->CancelInteraction();
	}
	else if (WorkspaceInteractionModel)
	{
		WorkspaceInteractionModel->CancelTransientState();
	}

	// A pile switch animates the old pile closed before committing the new expanded pile.
	// Cancelling during that interval must restore the still-authoritative old pile layout;
	// otherwise its cards remain visually stacked at the collapsed origin while the state
	// store continues to report the old pile as expanded.
	if (bWasSwitchingExpandedPile)
	{
		RebuildWorkspaceFromCachedSnapshot();
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
	FWacomBackpackCommandFlow::ArrangeAll(
		StateStore,
		FWacomBackpackZoneKey::Make(EZoneKind::Backpack));
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::HandleResetPilePositionsClicked()
{
	URunSession* Run = GetRunSession();
	if (!Run)
	{
		return;
	}
	FWacomBackpackWorkspaceStateStore& StateStore = GetWorkspaceStateStore(Run);
	bHasPendingPileExpansionAfterCollapse = false;
	bPendingPileExpansionRequiresCarryHover = false;
	StateStore.ResetPileLayouts();
	StateStore.SetExpandedPile(TOptional<FWacomBackpackZoneKey>());
	RebuildWorkspaceFromCachedSnapshot();
}

void UWacomBackpackScreen::ResetBackpackRefreshDirtyGate()
{
	GetStorageRefreshGate().Reset();
}

void UWacomBackpackScreen::RebuildTopStats(UWacomRunViewModel* VM)
{
	if (VM && GoldText)
	{
		GoldText->SetText(FText::Format(
			LOCTEXT("GoldFmt", "金币：{0}"),
			FText::AsNumber(VM->GetGold())));
	}
}

void UWacomBackpackScreen::HandleBattleEnabledToggle(FGuid InstanceId)
{
	FWacomBackpackCommandFlow::HandleBattleEnabledToggle(*this, GetRunSession(), InstanceId);
}

void UWacomBackpackScreen::HandleCardHovered(UWacomDeckCardWidget* SourceWidget)
{
	if (WorkspaceWidget && SourceWidget)
	{
		WorkspaceWidget->SetHoveredCard(SourceWidget);
	}
	ShowCardDetailForCardWidget(SourceWidget);
}

void UWacomBackpackScreen::HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget)
{
	if (WorkspaceWidget && SourceWidget)
	{
		WorkspaceWidget->ClearHoveredCard(SourceWidget);
	}
	HideCardDetailPanelIfSourceRemoved(SourceWidget);
}

void UWacomBackpackScreen::HandleWorkspaceBrowseFocusChanged(
	UWacomDeckCardWidget* SourceWidget)
{
	if (SourceWidget)
	{
		WorkspaceBrowseFocusDetailSource = SourceWidget;
		ShowCardDetailForCardWidget(SourceWidget);
	}
	else
	{
		if (UWacomDeckCardWidget* PreviousSource = WorkspaceBrowseFocusDetailSource.Get())
		{
			HideCardDetailPanelIfSourceRemoved(PreviousSource);
		}
		WorkspaceBrowseFocusDetailSource.Reset();
	}
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
	WorkspaceBrowseFocusDetailSource.Reset();
	GetCardDetailController().Hide();
}

void UWacomBackpackScreen::HideCardDetailPanelIfSourceRemoved(UWacomDeckCardWidget* RemovedWidget)
{
	GetCardDetailController().HideIfSourceRemoved(RemovedWidget);
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
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::F1)
	{
		HandleControlsHelpClicked();
		return FReply::Handled();
	}
	if (IsControlsHelpVisible()
		&& (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right))
	{
		HideControlsHelp(true);
		return FReply::Handled();
	}

	// B 键关闭：和打开是同一个键，CommonUI Menu 模式下 EnhancedInput IA 被屏蔽，
	// 必须在 widget 层自己拦。父类已经处理 ESC。
	if (Key == EKeys::B)
	{
		CancelWorkspaceInteraction();
		DeactivateWidget();
		return FReply::Handled();
	}
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		if ((WorkspaceInteractionModel
				&& WorkspaceInteractionModel->GetMode() != EWacomBackpackWorkspaceInteractionMode::Idle))
		{
			CancelWorkspaceInteraction();
			return FReply::Handled();
		}
		URunSession* Run = GetRunSession();
		if (Run && GetWorkspaceStateStore(Run).GetExpandedPile().IsSet())
		{
			HandleCollapseExpandedPileRequested();
			return FReply::Handled();
		}
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
