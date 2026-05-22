// Copyright Wacom. All Rights Reserved.

#include "UI/Backpack/WacomBackpackScreen.h"

#define LOCTEXT_NAMESPACE "WacomBackpack"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WrapBox.h"
#include "Components/WrapBoxSlot.h"
#include "Misc/PackageName.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Backpack/BackpackFallbackLayoutBuilder.h"
#include "UI/Backpack/BackpackRuntimeZoneBuilder.h"
#include "UI/Backpack/WacomBackpackZoneSectionWidget.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomDeleteZoneDropTarget.h"
#include "UI/Backpack/WacomBackpackScreenPresenter.h"
#include "UI/Backpack/WacomSpecialZoneWidget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Menus/WacomConfirmDialog.h"
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
const FVector2D CardDetailPanelEstimatedSize(360.f, 420.f);
constexpr float CardDetailPanelPadding = 12.f;

FText GetBackpackCommandCardDisplayName(const UCardDefinition* Card)
{
	if (!Card)
	{
		return LOCTEXT("UnknownCard", "未知卡牌");
	}
	return Card->DisplayName.IsEmpty()
		? FText::FromName(Card->CardId)
		: Card->DisplayName;
}

UWacomAppToastSubsystem* GetBackpackToastSubsystem(const UObject* Context)
{
	const UWorld* World = Context ? Context->GetWorld() : nullptr;
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

void ShowBackpackWarningToast(const UObject* Context, const FText& Message)
{
	if (UWacomAppToastSubsystem* ToastSubsystem = GetBackpackToastSubsystem(Context))
	{
		ToastSubsystem->ShowWarning(Message);
	}
}

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
	HideCardDetailPanel();

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
	return CardDetailPanel && CardDetailPanel->GetVisibility() != ESlateVisibility::Collapsed;
}

FText UWacomBackpackScreen::GetCardDetailPanelNameText() const
{
	return CardDetailPanel ? CardDetailPanel->GetNameText() : FText::GetEmpty();
}

bool UWacomBackpackScreen::HandleZoneDropRequested(const UWacomCardDragOperation& CardOp, EZoneKind TargetZone, FGuid TargetZoneOwnerInstanceId)
{
	UWacomAppToastSubsystem* ToastSubsystem = GetBackpackToastSubsystem(this);

	auto ShowFailureToast = [ToastSubsystem](FName DisabledReason)
	{
		if (ToastSubsystem)
		{
			ToastSubsystem->ShowWarning(UWacomZoneDropTarget::FormatMoveFailureReasonForToast(DisabledReason));
		}
	};

	if (!CardOp.InstanceId.IsValid())
	{
		ShowFailureToast(TEXT("CardNotFound"));
		return false;
	}

	URunSession* Run = GetRunSession();
	if (!Run)
	{
		ShowFailureToast(TEXT("RunSessionMissing"));
		return false;
	}

	const FRunDeckOperationValidation Validation =
		Run->ValidateMoveInstance(CardOp.InstanceId, TargetZone, TargetZoneOwnerInstanceId);
	if (!Validation.bCanExecute)
	{
		ShowFailureToast(Validation.DisabledReason);
		return false;
	}

	const bool bMoved = Run->MoveInstance(CardOp.InstanceId, TargetZone, TargetZoneOwnerInstanceId);
	if (!bMoved)
	{
		ShowFailureToast(TEXT("Unknown"));
		return false;
	}

	if (ToastSubsystem)
	{
		FWacomAppToastView ToastView;
		ToastView.MessageText = FText::Format(
			LOCTEXT("MoveSuccessToast", "移动卡牌：{0} → {1}"),
			GetBackpackCommandCardDisplayName(CardOp.Definition.Get()),
			UWacomZoneDropTarget::FormatZoneNameForToast(TargetZone));
		ToastView.Tone = EWacomAppToastTone::System;
		ToastView.IconKey = TEXT("CardMoved");
		ToastSubsystem->ShowToast(ToastView);
	}

	return true;
}

bool UWacomBackpackScreen::HandleDeleteDropRequested(const UWacomCardDragOperation& CardOp)
{
	UCardDefinition* Card = CardOp.Definition.Get();
	if (!Card)
	{
		ShowBackpackWarningToast(
			this,
			UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(TEXT("MissingCard")));
		return false;
	}

	URunSession* Run = GetRunSession();
	if (!Run)
	{
		ShowBackpackWarningToast(
			this,
			UWacomZoneDropTarget::FormatMoveFailureReasonForToast(TEXT("RunSessionMissing")));
		return false;
	}

	const FRunDeckOperationValidation Validation = Run->ValidateDeleteCardForGold(Card);
	if (!Validation.bCanExecute)
	{
		ShowBackpackWarningToast(
			this,
			UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(Validation.DisabledReason));
		return false;
	}

	const FText CardName = GetBackpackCommandCardDisplayName(Card);
	const int32 GoldReward = UWacomDeleteZoneDropTarget::GetDeleteGoldRewardPreviewForToast(Card);
	const TWeakObjectPtr<UWacomBackpackScreen> WeakScreen(this);
	UWacomConfirmDialog* Dialog = UWacomConfirmDialog::Show(
		this,
		LOCTEXT("DeleteCardTitle", "删除卡牌"),
		FText::Format(
			LOCTEXT("DeleteCardMessage", "确认永久销毁 {0} 并置换金币？"),
			CardName),
		[WeakScreen, Card, CardName, GoldReward]()
		{
			UWacomBackpackScreen* PinnedScreen = WeakScreen.Get();
			URunSession* PinnedRun = PinnedScreen ? PinnedScreen->GetRunSession() : nullptr;
			if (!PinnedScreen || !PinnedRun || !Card)
			{
				return;
			}

			UWacomAppToastSubsystem* ToastSubsystem = GetBackpackToastSubsystem(PinnedScreen);

			const bool bDeleted = PinnedRun->DeleteCardForGold(Card);
			if (bDeleted)
			{
				if (ToastSubsystem)
				{
					FWacomAppToastView ToastView;
					ToastView.MessageText = FText::Format(
						LOCTEXT("DeleteCardSuccessToast", "销毁卡牌：{0}，获得 {1} 金币"),
						CardName,
						FText::AsNumber(GoldReward));
					ToastView.Tone = EWacomAppToastTone::Positive;
					ToastView.IconKey = TEXT("CardDestroyed");
					ToastSubsystem->ShowToast(ToastView);
				}
				return;
			}

			if (ToastSubsystem)
			{
				const FRunDeckOperationValidation RetryValidation = PinnedRun->ValidateDeleteCardForGold(Card);
				ToastSubsystem->ShowWarning(UWacomDeleteZoneDropTarget::FormatDeleteFailureReasonForToast(RetryValidation.DisabledReason));
			}
		});

	return Dialog != nullptr;
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
	if (BattleDeckCardsBox) { BattleDeckCardsBox->ClearChildren(); }
	if (FluxContentCardsBox) { FluxContentCardsBox->ClearChildren(); }
	if (SpecialZonesPanel)  { SpecialZonesPanel->ClearChildren(); }
	if (BurdenCardsBox)     { BurdenCardsBox->ClearChildren(); }
}

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
		ClearCardBoxes();
		return;
	}

	const FRunBackpackStorageSnapshot Snapshot = Run->BuildBackpackStorageSnapshot();
	ClearCardBoxes();
	RebuildBattleDeckZone(Snapshot);
	RebuildBackpackZone(Snapshot);
	RebuildSpecialZones(Snapshot);
	RebuildBurdenZone(Snapshot);
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
		for (const FRunStorageCardView& CardView : Snapshot.BattleDeckPhysicalCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(CardView);
			if (!W) { continue; }
			BattleDeckCardsBox->AddChildToWrapBox(W);
		}

		for (const FRunStorageCardView& ProjectedView : Snapshot.BattleDeckProjectedCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(ProjectedView);
			if (!W)
			{
				continue;
			}

			const FText ProjectedBadgeText = UWacomBackpackScreenPresenter::BuildBattleDeckProjectedFromBadgeText(ProjectedView, Snapshot);
			if (!ProjectedBadgeText.IsEmpty())
			{
				W->SetProjectedFromBadgeText(ProjectedBadgeText);
			}
			W->SetRightClickToggleEnabled(true);
			BattleDeckCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::RebuildBackpackZone(const FRunBackpackStorageSnapshot& Snapshot)
{
	RebuildFluxContentCards(Snapshot);
}

void UWacomBackpackScreen::RebuildFluxMainCards(const FRunBackpackStorageSnapshot& Snapshot)
{
	// 兼容旧 API：通量区已不再有 A 类主卡槽，默认不渲染 Flux.MainCards。
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
		for (const FRunStorageCardView& CardView : Snapshot.Flux.ContentCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(CardView);
			if (!W) { continue; }
			FluxContentCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::RebuildSpecialZones(const FRunBackpackStorageSnapshot& Snapshot)
{
	if (SpecialZonesPanel)
	{
		for (const FRunSpecialStorageView& SpecialView : Snapshot.SpecialZones)
		{
			UClass* ZoneWidgetClass = SpecialZoneWidgetClass ? SpecialZoneWidgetClass.Get() : UWacomSpecialZoneWidget::StaticClass();
			UWacomSpecialZoneWidget* ZoneWidget = CreateWidget<UWacomSpecialZoneWidget>(this, ZoneWidgetClass);
			if (!ZoneWidget)
			{
				continue;
			}
			ZoneWidget->SetSpecialZoneView(SpecialView, this, CardWidgetClass);
			ZoneWidget->OnBattleEnabledToggleRequestedNative.AddUObject(this, &UWacomBackpackScreen::HandleBattleEnabledToggle);
			ZoneWidget->OnCardHoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardHovered);
			ZoneWidget->OnCardUnhoveredNative.AddUObject(this, &UWacomBackpackScreen::HandleCardUnhovered);
			if (UVerticalBoxSlot* ZoneSlot = SpecialZonesPanel->AddChildToVerticalBox(ZoneWidget))
			{
				ZoneSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
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
		for (const FRunStorageCardView& CardView : Snapshot.BurdenCards)
		{
			UWacomDeckCardWidget* W = CreateCardWidget(CardView);
			if (!W) { continue; }
			BurdenCardsBox->AddChildToWrapBox(W);
		}
	}
}

void UWacomBackpackScreen::HandleBattleEnabledToggle(FGuid InstanceId)
{
	URunSession* Run = GetRunSession();
	if (!Run || !InstanceId.IsValid())
	{
		return;
	}

	FCardInstance Inst;
	EZoneKind Zone = EZoneKind::Backpack;
	FGuid ZoneOwner;
	if (!Run->FindInstance(InstanceId, Inst, Zone, ZoneOwner) || Zone != EZoneKind::SpecialZone)
	{
		return;
	}

	Run->SetSpecialZoneCardBattleEnabled(InstanceId, !Inst.bBattleEnabledInSpecialZone);
}

void UWacomBackpackScreen::HandleCardHovered(UWacomDeckCardWidget* SourceWidget)
{
	ShowCardDetailForCardWidget(SourceWidget);
}

void UWacomBackpackScreen::HandleCardUnhovered(UWacomDeckCardWidget* SourceWidget)
{
	HideCardDetailPanel();
}

bool UWacomBackpackScreen::ShowCardDetailForCardWidget(UWacomDeckCardWidget* SourceWidget)
{
	if (!SourceWidget || !SourceWidget->GetCard())
	{
		HideCardDetailPanel();
		return false;
	}

	UWacomCardDetailPanel* Panel = EnsureCardDetailPanel();
	if (!Panel)
	{
		return false;
	}

	Panel->SetCardDetailData(UWacomBackpackScreenPresenter::BuildCardDetailViewData(SourceWidget->GetCard()));
	PositionCardDetailPanelNear(SourceWidget);
	Panel->SetRenderOpacity(1.f);
	Panel->SetVisibility(ESlateVisibility::HitTestInvisible);
	return true;
}

void UWacomBackpackScreen::HideCardDetailPanel()
{
	if (CardDetailPanel)
	{
		CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	}
}

UWacomCardDetailPanel* UWacomBackpackScreen::EnsureCardDetailPanel()
{
	EnsureRuntimeZoneWidgets();
	if (!CardDetailLayer)
	{
		return nullptr;
	}

	if (CardDetailPanel)
	{
		return CardDetailPanel;
	}

	UClass* PanelClass = CardDetailPanelClass
		? CardDetailPanelClass.Get()
		: UWacomCardDetailPanel::StaticClass();
	CardDetailPanel = GetWorld()
		? CreateWidget<UWacomCardDetailPanel>(this, PanelClass)
		: NewObject<UWacomCardDetailPanel>(this, PanelClass);
	if (!CardDetailPanel)
	{
		return nullptr;
	}

	CardDetailPanel->SetVisibility(ESlateVisibility::Collapsed);
	CardDetailPanel->SetIsEnabled(true);
	CardDetailPanel->SetRenderOpacity(1.f);
	if (UCanvasPanelSlot* DetailSlot = CardDetailLayer->AddChildToCanvas(CardDetailPanel))
	{
		DetailSlot->SetAutoSize(false);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
		DetailSlot->SetZOrder(1);
	}
	return CardDetailPanel;
}

void UWacomBackpackScreen::PositionCardDetailPanelNear(UWacomDeckCardWidget* SourceWidget)
{
	if (!SourceWidget || !CardDetailLayer || !CardDetailPanel)
	{
		return;
	}

	const FGeometry& LayerGeometry = CardDetailLayer->GetCachedGeometry();
	const FGeometry& SourceGeometry = SourceWidget->GetCachedGeometry();
	const FVector2D AnchorPosition = LayerGeometry.AbsoluteToLocal(SourceGeometry.GetAbsolutePosition());
	const FVector2D AnchorSize = SourceGeometry.GetLocalSize();
	const FVector2D LayerSize = LayerGeometry.GetLocalSize();
	const FVector2D Position = UWacomBackpackScreenPresenter::ComputeCardDetailPanelPosition(
		AnchorPosition,
		AnchorSize,
		LayerSize,
		CardDetailPanelEstimatedSize,
		CardDetailPanelPadding);

	if (UCanvasPanelSlot* DetailSlot = Cast<UCanvasPanelSlot>(CardDetailPanel->Slot))
	{
		DetailSlot->SetPosition(Position);
		DetailSlot->SetSize(CardDetailPanelEstimatedSize);
	}
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
