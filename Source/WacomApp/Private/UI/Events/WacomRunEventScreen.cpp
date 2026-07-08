// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventScreen.h"

#define LOCTEXT_NAMESPACE "WacomRunEventScreen"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "Engine/GameInstance.h"
#include "GameFramework/WacomPlayerController.h"
#include "RunSession.h"
#include "UI/Events/WacomRunEventChoiceListReconciler.h"
#include "UI/Events/WacomRunEventChoiceButton.h"
#include "UI/Events/WacomRunEventPaymentDropFlow.h"
#include "UI/Events/WacomRunEventPaymentLeaseBuilder.h"
#include "UI/Events/WacomRunEventPresentationState.h"
#include "UI/Events/WacomRunEventScreenDebugBuilder.h"
#include "UI/Events/WacomRunEventScreenFlow.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"

namespace
{
	UTextBlock* MakeRunEventText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}

	template <typename WidgetType>
	TSubclassOf<WidgetType> ResolveRunEventWidgetClass(
		TSubclassOf<WidgetType> ConfiguredClass)
	{
		if (ConfiguredClass)
		{
			return ConfiguredClass;
		}
		return TSubclassOf<WidgetType>(WidgetType::StaticClass());
	}

	FWacomRunEventPresentationStateView BuildRunEventPresentationStateView(
		const TArray<FRunEventChoiceSnapshot>& CachedChoices,
		const TMap<FName, FName>& PaymentZoneToChoiceId)
	{
		return FWacomRunEventPresentationStateView{
			&CachedChoices,
			&PaymentZoneToChoiceId
		};
	}

	FWacomRunEventPresentationStateEdit BuildRunEventPresentationStateEdit(
		TArray<FRunEventChoiceSnapshot>& CachedChoices,
		TMap<FName, FName>& PaymentZoneToChoiceId)
	{
		return FWacomRunEventPresentationStateEdit{
			&CachedChoices,
			&PaymentZoneToChoiceId
		};
	}
}

TSharedRef<SWidget> UWacomRunEventScreen::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* PanelBg = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("PanelBg"));
		PanelBg->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.025f, 0.94f));
		PanelBg->SetPadding(FMargin(22.f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBg))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetOffsets(FMargin(-420.f, -280.f, 840.f, 560.f));
			PanelSlot->SetAutoSize(false);
		}

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		PanelBg->AddChild(RootBox);

		TitleText = MakeRunEventText(WidgetTree, TEXT("TitleText"), LOCTEXT("Title", "事件"), 30);
		TitleText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* TitleSlot = RootBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 14.f));
		}

		BodyText = MakeRunEventText(WidgetTree, TEXT("BodyText"), FText::GetEmpty(), 18);
		BodyText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* BodySlot = RootBox->AddChildToVerticalBox(BodyText))
		{
			BodySlot->SetPadding(FMargin(0.f, 0.f, 0.f, 18.f));
		}

		EmptyText = MakeRunEventText(WidgetTree, TEXT("EmptyText"), LOCTEXT("Empty", "暂无可选行动"), 16);
		EmptyText->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* EmptySlot = RootBox->AddChildToVerticalBox(EmptyText))
		{
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetPadding(FMargin(0.f, 8.f));
		}

		UScrollBox* ChoiceScroll = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("ChoiceScroll"));
		if (UVerticalBoxSlot* ScrollSlot = RootBox->AddChildToVerticalBox(ChoiceScroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		ChoiceList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("ChoiceList"));
		ChoiceScroll->AddChild(ChoiceList);

		CloseButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CloseButton"));
		UTextBlock* CloseText = MakeRunEventText(WidgetTree, TEXT("CloseText"), LOCTEXT("Close", "关闭"), 18);
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

void UWacomRunEventScreen::NativeConstruct()
{
	Super::NativeConstruct();
	if (CloseButton)
	{
		CloseButton->OnClicked.AddUniqueDynamic(this, &UWacomRunEventScreen::HandleCloseClicked);
	}
	RefreshEvent();
}

void UWacomRunEventScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	bDidEndRunEvent = false;
	RefreshEvent();
}

void UWacomRunEventScreen::NativeOnDeactivated()
{
	FWacomRunEventScreenFlow::EndRunEventOnDeactivate(ResolveRunSession(), bDidEndRunEvent);
	Super::NativeOnDeactivated();
}

FWacomRunMenuCardDropResolveResult UWacomRunEventScreen::ResolveRunMenuCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Candidate) const
{
	const FWacomRunMenuCardDropResolveResult Result =
		FWacomRunEventPaymentDropFlow::ResolveDropIntent(
			FWacomRunEventPaymentDropFlowContext{
				nullptr,
				ResolveRunSession(),
				nullptr,
				BuildRunEventPresentationStateView(CachedChoices, PaymentZoneToChoiceId),
				nullptr
			},
			Candidate);
	RecordPaymentDropResolveDebug(Result);
	return Result;
}

bool UWacomRunEventScreen::SubmitRunMenuCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Resolved,
	FWacomRunMenuCardDropResolveResult& OutSubmitted)
{
	const bool bSubmitted =
		FWacomRunEventPaymentDropFlow::SubmitDropIntent(
			FWacomRunEventPaymentDropFlowContext{
				this,
				ResolveRunSession(),
				ResolveToastSubsystem(),
				BuildRunEventPresentationStateView(CachedChoices, PaymentZoneToChoiceId),
				&bDidEndRunEvent
			},
			Resolved,
			OutSubmitted);
	RecordPaymentDropSubmitDebug(OutSubmitted);
	return bSubmitted;
}

void UWacomRunEventScreen::RefreshEvent()
{
	if (URunSession* Run = ResolveRunSession())
	{
		const FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
		if (TitleText)
		{
			TitleText->SetText(Snapshot.TitleText.IsEmpty() ? LOCTEXT("Title", "事件") : Snapshot.TitleText);
		}
		if (BodyText)
		{
			BodyText->SetText(Snapshot.BodyText);
		}
	}

	RebuildChoices();
	RefreshPaymentLeaseFromCachedChoices();
}

void UWacomRunEventScreen::SuppressEndRunEventOnNextDeactivate()
{
	bDidEndRunEvent = true;
}

FWacomRunEventScreenDebugView UWacomRunEventScreen::GetRunEventScreenDebugView() const
{
	return FWacomRunEventScreenDebugBuilder::BuildView(
		FWacomRunEventScreenDebugBuildContext{
			ResolveRunSession(),
			BuildRunEventPresentationStateView(CachedChoices, PaymentZoneToChoiceId),
			LastPaymentDropResolveDebugSummary,
			LastPaymentDropSubmitDebugSummary
		});
}

FString UWacomRunEventScreen::GetRunEventScreenDebugSummary() const
{
	return FWacomRunEventScreenDebugBuilder::BuildSummary(GetRunEventScreenDebugView());
}

void UWacomRunEventScreen::LogRunEventScreenDebugSummary() const
{
	UE_LOG(LogTemp, Display, TEXT("[WacomRunEventScreen] %s"), *GetRunEventScreenDebugSummary());
}

void UWacomRunEventScreen::HandleCloseClicked()
{
	DeactivateWidget();
}

#if WITH_AUTOMATION_TESTS
FRunEventChoiceSnapshot UWacomRunEventScreen::GetCachedChoiceSnapshot(int32 Index) const
{
	return CachedChoices.IsValidIndex(Index) ? CachedChoices[Index] : FRunEventChoiceSnapshot();
}

bool UWacomRunEventScreen::ChooseChoiceByIndex(int32 Index)
{
	if (!CachedChoices.IsValidIndex(Index))
	{
		return false;
	}
	return ChooseChoice(CachedChoices[Index].ChoiceId);
}

FText UWacomRunEventScreen::GetDisplayedTitleText() const
{
	return TitleText ? TitleText->GetText() : FText::GetEmpty();
}

FText UWacomRunEventScreen::GetDisplayedBodyText() const
{
	return BodyText ? BodyText->GetText() : FText::GetEmpty();
}

UWacomRunEventChoiceButton* UWacomRunEventScreen::GetChoiceButtonWidgetForTest(int32 Index) const
{
	return ChoiceButtonWidgets.IsValidIndex(Index) ? ChoiceButtonWidgets[Index] : nullptr;
}

UWacomRunMenuDropTargetWidget* UWacomRunEventScreen::GetPaymentDropTargetForTest(int32 Index) const
{
	return PaymentDropTargets.IsValidIndex(Index) ? PaymentDropTargets[Index] : nullptr;
}
#endif

URunSession* UWacomRunEventScreen::ResolveRunSession() const
{
	AWacomPlayerController* WacomPC = Cast<AWacomPlayerController>(GetOwningPlayer());
	return WacomPC ? WacomPC->GetRunSession() : nullptr;
}

UWacomAppToastSubsystem* UWacomRunEventScreen::ResolveToastSubsystem() const
{
	const UGameInstance* GI = GetGameInstance();
	return GI ? GI->GetSubsystem<UWacomAppToastSubsystem>() : nullptr;
}

void UWacomRunEventScreen::RebuildChoices()
{
	URunSession* Run = ResolveRunSession();
	const FRunEventSnapshot Snapshot = Run ? Run->BuildCurrentRunEventSnapshot() : FRunEventSnapshot();
	BuildRunEventPresentationStateEdit(CachedChoices, PaymentZoneToChoiceId)
		.SetChoices(Snapshot.Choices);

	if (EmptyText)
	{
		EmptyText->SetVisibility(Snapshot.Choices.Num() == 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	FWacomRunEventChoiceListReconciler::Reconcile(
		FWacomRunEventChoiceListReconcileContext{
			ChoiceList,
			PaymentChoiceMinDesiredWidth,
			&ChoiceButtonWidgets,
			&PaymentDropTargets,
			BuildRunEventPresentationStateEdit(CachedChoices, PaymentZoneToChoiceId)
		},
		Snapshot.Choices,
		[this](const FRunEventChoiceSnapshot& /*Choice*/) -> UWacomRunEventChoiceButton*
		{
			if (!WidgetTree)
			{
				return nullptr;
			}

			UWacomRunEventChoiceButton* ChoiceButtonWidget =
				WidgetTree->ConstructWidget<UWacomRunEventChoiceButton>(ResolveChoiceButtonWidgetClass());
			if (ChoiceButtonWidget)
			{
				ChoiceButtonWidget->OnChoiceClickedNative.AddUObject(this, &UWacomRunEventScreen::HandleChoiceClicked);
			}
			return ChoiceButtonWidget;
		},
		[this](const FRunEventChoiceSnapshot& Choice) -> UWacomRunMenuDropTargetWidget*
		{
			return WidgetTree
				? WidgetTree->ConstructWidget<UWacomRunMenuDropTargetWidget>(
					ResolvePaymentDropTargetWidgetClass(),
					FName(*FString::Printf(TEXT("RunEventChoiceDrop_%s"), *Choice.ChoiceId.ToString())))
				: nullptr;
		},
		[this](const FRunEventChoiceSnapshot& Choice) -> USizeBox*
		{
			return WidgetTree
				? WidgetTree->ConstructWidget<USizeBox>(
					USizeBox::StaticClass(),
					FName(*FString::Printf(TEXT("RunEventChoiceDropSize_%s"), *Choice.ChoiceId.ToString())))
				: nullptr;
		},
		[](UWacomRunEventChoiceButton& ChoiceButtonWidget, const FRunEventChoiceSnapshot& Choice)
		{
			ChoiceButtonWidget.SetChoiceSnapshot(Choice);
		});
}

TSubclassOf<UWacomRunEventChoiceButton> UWacomRunEventScreen::ResolveChoiceButtonWidgetClass() const
{
	return ResolveRunEventWidgetClass<UWacomRunEventChoiceButton>(ChoiceButtonWidgetClass);
}

TSubclassOf<UWacomRunMenuDropTargetWidget> UWacomRunEventScreen::ResolvePaymentDropTargetWidgetClass() const
{
	return ResolveRunEventWidgetClass<UWacomRunMenuDropTargetWidget>(PaymentDropTargetWidgetClass);
}

void UWacomRunEventScreen::HandleChoiceClicked(FName ChoiceId)
{
	ChooseChoice(ChoiceId);
}

bool UWacomRunEventScreen::ChooseChoice(FName ChoiceId)
{
	URunSession* Run = ResolveRunSession();
	if (!Run)
	{
		return false;
	}

	return FWacomRunEventScreenFlow::ChooseChoice(
		*this,
		Run,
		ResolveToastSubsystem(),
		ChoiceId,
		bDidEndRunEvent);
}

void UWacomRunEventScreen::RefreshPaymentLeaseFromCachedChoices()
{
	const FWacomRunEventPaymentLeaseBuildResult PaymentLease =
		FWacomRunEventPaymentLeaseBuilder::BuildRequest(
			BuildRunEventPresentationStateView(CachedChoices, PaymentZoneToChoiceId).GetChoices());
	if (!PaymentLease.bHasCandidateCards)
	{
		ClearOwnedRunMenuCardLease();
		return;
	}

	FWacomRunMenuCardLeaseResult LeaseResult;
	SetOwnedRunMenuCardLeaseFromRunCards(PaymentLease.Request, LeaseResult);
}

void UWacomRunEventScreen::RecordPaymentDropResolveDebug(
	const FWacomRunMenuCardDropResolveResult& Result) const
{
	LastPaymentDropResolveDebugSummary =
		FWacomRunEventScreenDebugBuilder::BuildDropResultSummary(TEXT("Resolve"), Result);
}

void UWacomRunEventScreen::RecordPaymentDropSubmitDebug(
	const FWacomRunMenuCardDropResolveResult& Result) const
{
	LastPaymentDropSubmitDebugSummary =
		FWacomRunEventScreenDebugBuilder::BuildDropResultSummary(TEXT("Submit"), Result);
}

#undef LOCTEXT_NAMESPACE
