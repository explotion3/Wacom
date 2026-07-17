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
#include "Settings/WacomSettingsSubsystem.h"
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

#if WITH_EDITOR
#include "UI/Backpack/WacomBackpackPIEValidationState.h"
#endif

namespace
{
template <typename TWidget>
TSubclassOf<TWidget> LoadOptionalWidgetClass(const TCHAR* ClassPath);

TSubclassOf<UWacomSpecialZoneWidget> LoadSpecialZoneWidgetClassWithDiagnostic(
	const TCHAR* ClassPath);

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
		SpecialZoneWidgetClass = UWacomSpecialZoneWidget::StaticClass();
		CardDetailPanelClass = UWacomCardDetailPanel::StaticClass();
		WorkspaceWidgetClass = UWacomBackpackWorkspaceWidget::StaticClass();
		DeleteConfirmWidgetClass = UWacomBackpackDeleteConfirmWidget::StaticClass();
		DeleteZoneSectionWidgetClass = UWacomBackpackZoneSectionWidget::StaticClass();
		BattleDeckZoneSectionWidgetClass = UWacomBackpackZoneSectionWidget::StaticClass();
		FluxMainZoneSectionWidgetClass = UWacomBackpackZoneSectionWidget::StaticClass();
		FluxContentZoneSectionWidgetClass = UWacomBackpackZoneSectionWidget::StaticClass();
		SpecialZonesSectionWidgetClass = UWacomBackpackZoneSectionWidget::StaticClass();
		BurdenZoneSectionWidgetClass = UWacomBackpackZoneSectionWidget::StaticClass();
		return;
	}
#endif

	if (!CardWidgetClass)
	{
		CardWidgetClass = LoadOptionalWidgetClass<UWacomDeckCardWidget>(
			TEXT("/Game/Wacom/UI/Card/WBP_WacomDeckCardWidget.WBP_WacomDeckCardWidget_C"));
	}

	if (!SpecialZoneWidgetClass)
	{
		SpecialZoneWidgetClass = LoadSpecialZoneWidgetClassWithDiagnostic(
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

TSubclassOf<UWacomSpecialZoneWidget> LoadSpecialZoneWidgetClassWithDiagnostic(
	const TCHAR* ClassPath)
{
	const FString ObjectPath(ClassPath);
	const FString PackagePath = FPackageName::ObjectPathToPackageName(ObjectPath);
	if (!FPackageName::DoesPackageExist(PackagePath))
	{
		static bool bLoggedMissingAsset = false;
		if (!bLoggedMissingAsset)
		{
			bLoggedMissingAsset = true;
			UE_LOG(
				LogTemp,
				Warning,
				TEXT("[Backpack] Compatibility SpecialZone asset is missing: %s. Using UWacomSpecialZoneWidget C++ fallback."),
				ClassPath);
		}
		return UWacomSpecialZoneWidget::StaticClass();
	}

	UClass* LoadedClass = LoadObject<UClass>(nullptr, ClassPath);
	if (LoadedClass && LoadedClass->IsChildOf(UWacomSpecialZoneWidget::StaticClass()))
	{
		return LoadedClass;
	}

	static bool bLoggedInvalidAsset = false;
	if (!bLoggedInvalidAsset)
	{
		bLoggedInvalidAsset = true;
		UE_LOG(
			LogTemp,
			Warning,
			TEXT("[Backpack] Compatibility SpecialZone asset has an invalid class: %s. Using UWacomSpecialZoneWidget C++ fallback."),
			ClassPath);
	}
	return UWacomSpecialZoneWidget::StaticClass();
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
			&DeleteTargetHost,
			&DeleteConfirmHost,
			&ArrangeAllButton,
			&ResetPilePositionsButton,
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
	BindRuntimeSettings();

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

	TrySubscribeAndRefresh();
}

void UWacomBackpackScreen::NativeDestruct()
{
	CancelWorkspaceInteraction();
	// Workspace children deliberately release transient bindings in NativeDestruct.
	// The next construction must therefore perform an authoritative reconcile even
	// when the Run storage revision itself has not changed.
	ResetBackpackRefreshDirtyGate();
	UnbindOwningLayerTransition();
	UnbindRuntimeSettings();
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
	if (ResetPilePositionsButton)
	{
		ResetPilePositionsButton->OnClicked.RemoveDynamic(
			this, &UWacomBackpackScreen::HandleResetPilePositionsClicked);
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
	View.WorkspaceCardCount = ActiveWorkspaceCardWidgets.Num();
	if (WorkspaceWidget)
	{
		const FWacomBackpackWorkspaceAutomationTestView WorkspaceView = WorkspaceWidget->GetAutomationTestView();
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
		ActiveWorkspaceCardWidgets.Reset();
		if (WorkspaceWidget && WorkspaceWidget->GetCardCanvas())
		{
			WorkspaceWidget->GetCardCanvas()->ClearChildren();
			WorkspaceWidget->SetEmptyStateVisible(true);
		}
		ClearCardBoxes();
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

		bHasLastAppliedStorageSnapshot = true;
		LastAppliedStorageSnapshot = EmptySnapshot;
		RebuildWorkspaceChrome(EmptySnapshot);
		RebuildBattleDeckZone(EmptySnapshot);
		RebuildBackpackZone(EmptySnapshot);
		RebuildSpecialZones(EmptySnapshot);
		RebuildBurdenZone(EmptySnapshot);
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
	RebuildBattleDeckZone(Snapshot);
	RebuildBackpackZone(Snapshot);
	RebuildSpecialZones(Snapshot);
	RebuildBurdenZone(Snapshot);
}

void UWacomBackpackScreen::RebuildWorkspaceChrome(const FRunBackpackStorageSnapshot& Snapshot)
{
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
	if (WorkspaceInteractionModel && WorkspaceInteractionModel->IsCarrying())
	{
		HideCardDetailPanel();
		FWacomBackpackZoneKey Target;
		if (ResolveWorkspacePileTarget(Target) && WorkspaceWidget)
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
			WorkspaceWidget->SetPileDropPreview(
				Target.Zone, Target.OwnerInstanceId, true, bRejected);
			return;
		}
	}
	if (WorkspaceWidget)
	{
		WorkspaceWidget->SetPileDropPreview(EZoneKind::Backpack, FGuid(), false, false);
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

void UWacomBackpackScreen::BindRuntimeSettings()
{
	UWacomSettingsSubsystem* Settings = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
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

bool UWacomBackpackScreen::ResolveWorkspacePileTarget(FWacomBackpackZoneKey& OutTarget) const
{
	if (!WorkspaceWidget || !WorkspaceInteractionModel || !WorkspaceInteractionModel->IsCarrying())
	{
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
		// Atomic rejection caused by an unrelated storage revision must not strand the
		// restored strip on an obsolete revision forever. Reconcile has already verified
		// that the carried cards still exist in their source zone, so the same carry can
		// be retried against the current snapshot without partially committing anything.
		if (URunSession* Run = GetRunSession())
		{
			WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(
				Run->GetBackpackStorageSnapshotRevision());
		}
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
	FWacomBackpackZoneKey PileTarget;
	if (ResolveWorkspacePileTarget(PileTarget))
	{
		HandleWorkspacePileReleaseIntent(Intent, PileTarget);
		return;
	}
	const FVector2D AbsolutePointer = WorkspaceWidget->GetCachedGeometry().LocalToAbsolute(
		Carry.PointerPosition);
	if (!WorkspaceWidget->GetCachedGeometry().IsUnderLocation(AbsolutePointer))
	{
		// 工作台外既不是通量空白，也不是合法目标；保持携带并显示拒绝状态。
		WorkspaceWidget->RefreshInteractionPresentation();
		return;
	}
	const FVector2D WorkspaceSize = WorkspaceWidget->GetLayoutSpaceSize();
	const float AvailableStripWidth = FMath::Max(
		Style->CardRenderSize.X,
		WorkspaceSize.X - FMath::Max(0.0f, Style->PileEdgeMarginPixels) * 2.0f);
	const TArray<FWacomBackpackCarriedStripLayout> Strip =
		FWacomBackpackWorkspaceLayoutSolver::BuildCarriedStripLayout(
			Carry.RemainingInstanceIds.Num(),
			Carry.CurrentIndex,
			Carry.DefaultIndex,
			Carry.PointerPosition,
			AvailableStripWidth,
			Style->CardRenderSize.X,
			Style->AdaptiveStripExposurePixels,
			Style->AdaptiveStripFocusSeparationPixels,
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
				Style->CardRenderSize,
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
			WorkspaceWidget->RefreshInteractionPresentation();
			return;
		}
		SaveFluxLayouts();
		WorkspaceInteractionModel->CommitReleasedCards(Intent.InstanceIds);
		WorkspaceInteractionModel->UpdateCarrySourceStorageRevision(Result.StorageRevision);
		ResetBackpackRefreshDirtyGate();
		EndWorkspaceMutationRefreshDeferral(true);
		return;
	}
	SaveFluxLayouts();
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
		CancelWorkspaceInteraction();
		DeactivateWidget();
		return FReply::Handled();
	}
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		if (PendingDeleteConfirmation && PendingDeleteConfirmation->bPending)
		{
			HandleWorkspaceDeleteCancelled();
			return FReply::Handled();
		}
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
