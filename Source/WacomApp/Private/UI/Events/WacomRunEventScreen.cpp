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
#include "UI/Events/WacomRunEventChoiceButton.h"
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

	const TCHAR* ToRunMenuCardDropIntentDebugString(EWacomRunMenuCardDropIntentKind Intent)
	{
		switch (Intent)
		{
		case EWacomRunMenuCardDropIntentKind::ProbeZoneTarget:
			return TEXT("ProbeZoneTarget");
		case EWacomRunMenuCardDropIntentKind::SubmitZoneTarget:
			return TEXT("SubmitZoneTarget");
		case EWacomRunMenuCardDropIntentKind::Reject:
			return TEXT("Reject");
		case EWacomRunMenuCardDropIntentKind::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropRejectDebugString(EWacomRunMenuCardDropRejectReason Reason)
	{
		switch (Reason)
		{
		case EWacomRunMenuCardDropRejectReason::NotInExploration:
			return TEXT("NotInExploration");
		case EWacomRunMenuCardDropRejectReason::MissingGameMenu:
			return TEXT("MissingGameMenu");
		case EWacomRunMenuCardDropRejectReason::MissingMenuLease:
			return TEXT("MissingMenuLease");
		case EWacomRunMenuCardDropRejectReason::MissingSession:
			return TEXT("MissingSession");
		case EWacomRunMenuCardDropRejectReason::InvalidSourceCard:
			return TEXT("InvalidSourceCard");
		case EWacomRunMenuCardDropRejectReason::MissingZoneTarget:
			return TEXT("MissingZoneTarget");
		case EWacomRunMenuCardDropRejectReason::UnsupportedTargetKind:
			return TEXT("UnsupportedTargetKind");
		case EWacomRunMenuCardDropRejectReason::MenuNotFound:
			return TEXT("MenuNotFound");
		case EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept:
			return TEXT("MenuDoesNotAccept");
		case EWacomRunMenuCardDropRejectReason::CardNotOwned:
			return TEXT("CardNotOwned");
		case EWacomRunMenuCardDropRejectReason::RunValidationFailed:
			return TEXT("RunValidationFailed");
		case EWacomRunMenuCardDropRejectReason::SubmitFailed:
			return TEXT("SubmitFailed");
		case EWacomRunMenuCardDropRejectReason::None:
		default:
			return TEXT("None");
		}
	}

	const TCHAR* ToRunMenuCardDropSubmitPolicyDebugString(EWacomRunMenuCardDropSubmitPolicy Policy)
	{
		switch (Policy)
		{
		case EWacomRunMenuCardDropSubmitPolicy::ControllerDestroyOwnedCard:
			return TEXT("ControllerDestroyOwnedCard");
		case EWacomRunMenuCardDropSubmitPolicy::MenuHandled:
			return TEXT("MenuHandled");
		case EWacomRunMenuCardDropSubmitPolicy::None:
		default:
			return TEXT("None");
		}
	}

	FString BuildRunEventScreenDropResultDebugSummary(
		const TCHAR* Prefix,
		const FWacomRunMenuCardDropResolveResult& Result)
	{
		return FString::Printf(
			TEXT("%s{CardId=%s ZoneId=%s Intent=%s Reject=%s SubmitPolicy=%s SubmitReason=%s RunValidation=%s CanSubmit=%s Submitted=%s LeaseId=%s}"),
			Prefix,
			*Result.SourceCardInstanceId.ToString(EGuidFormats::DigitsWithHyphens),
			*Result.ZoneId.ToString(),
			ToRunMenuCardDropIntentDebugString(Result.IntentKind),
			ToRunMenuCardDropRejectDebugString(Result.RejectReason),
			ToRunMenuCardDropSubmitPolicyDebugString(Result.SubmitPolicy),
			*Result.SubmitReason.ToString(),
			*Result.RunValidationReason.ToString(),
			Result.bCanSubmit ? TEXT("true") : TEXT("false"),
			Result.bSubmitted ? TEXT("true") : TEXT("false"),
			*Result.LeaseId.ToString());
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

FWacomRunMenuCardDropResolveResult UWacomRunEventScreen::ResolveRunMenuFirstPersonCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Candidate) const
{
	FWacomRunMenuCardDropResolveResult Result = Candidate;
	FRunEventChoiceSnapshot Choice;
	if (!FindPaymentChoiceForZone(Result.ZoneId, Choice))
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::ProbeZoneTarget;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		RecordPaymentDropResolveDebug(Result);
		return Result;
	}

	const URunSession* Run = ResolveRunSession();
	if (!Run)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::MissingSession;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		RecordPaymentDropResolveDebug(Result);
		return Result;
	}

	const FRunDeckOperationValidation Validation =
		Run->ValidateRunEventOptionCardPayment(Choice.ChoiceId, Result.SourceCardInstanceId);
	Result.RunValidationReason = Validation.DisabledReason;
	if (!Validation.bCanExecute)
	{
		Result.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		Result.RejectReason = EWacomRunMenuCardDropRejectReason::RunValidationFailed;
		Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		Result.bCanSubmit = false;
		RecordPaymentDropResolveDebug(Result);
		return Result;
	}

	Result.IntentKind = EWacomRunMenuCardDropIntentKind::SubmitZoneTarget;
	Result.RejectReason = EWacomRunMenuCardDropRejectReason::None;
	Result.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::MenuHandled;
	Result.SubmitReason = Choice.ChoiceId;
	Result.bCanSubmit = true;
	RecordPaymentDropResolveDebug(Result);
	return Result;
}

bool UWacomRunEventScreen::SubmitRunMenuFirstPersonCardDropIntent_Implementation(
	const FWacomRunMenuCardDropResolveResult& Resolved,
	FWacomRunMenuCardDropResolveResult& OutSubmitted)
{
	OutSubmitted = Resolved;
	FRunEventChoiceSnapshot Choice;
	if (!FindPaymentChoiceForZone(Resolved.ZoneId, Choice))
	{
		OutSubmitted.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		OutSubmitted.RejectReason = EWacomRunMenuCardDropRejectReason::MenuDoesNotAccept;
		OutSubmitted.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		OutSubmitted.bCanSubmit = false;
		OutSubmitted.bSubmitted = false;
		RecordPaymentDropSubmitDebug(OutSubmitted);
		return false;
	}

	URunSession* Run = ResolveRunSession();
	if (!Run)
	{
		OutSubmitted.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		OutSubmitted.RejectReason = EWacomRunMenuCardDropRejectReason::MissingSession;
		OutSubmitted.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		OutSubmitted.bCanSubmit = false;
		OutSubmitted.bSubmitted = false;
		RecordPaymentDropSubmitDebug(OutSubmitted);
		return false;
	}

	const FRunEventChoiceResult Result =
		Run->ChooseRunEventOptionWithPaidCardResult(Choice.ChoiceId, Resolved.SourceCardInstanceId);
	FWacomRunEventScreenFlow::ApplyChoiceResult(
		*this,
		Run,
		ResolveToastSubsystem(),
		Result,
		bDidEndRunEvent);

	OutSubmitted.bSubmitted = Result.bSucceeded;
	if (!Result.bSucceeded)
	{
		OutSubmitted.IntentKind = EWacomRunMenuCardDropIntentKind::Reject;
		OutSubmitted.RejectReason = EWacomRunMenuCardDropRejectReason::SubmitFailed;
		OutSubmitted.SubmitPolicy = EWacomRunMenuCardDropSubmitPolicy::None;
		OutSubmitted.bCanSubmit = false;
		if (OutSubmitted.RunValidationReason.IsNone())
		{
			OutSubmitted.RunValidationReason = Result.DisabledReason;
		}
		RecordPaymentDropSubmitDebug(OutSubmitted);
		return false;
	}
	RecordPaymentDropSubmitDebug(OutSubmitted);
	return true;
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
	FWacomRunEventScreenDebugView View;
	const URunSession* Run = ResolveRunSession();
	View.bHasRunSession = Run != nullptr;
	const FRunEventSnapshot Snapshot = Run ? Run->BuildCurrentRunEventSnapshot() : FRunEventSnapshot();
	View.bIsEventActive = Snapshot.bIsActive;
	View.PersistentId = Snapshot.PersistentId;
	View.EventId = Snapshot.EventId;
	View.CurrentNodeId = Snapshot.CurrentNodeId;
	View.CurrentNodeTitleText = Snapshot.TitleText;
	View.CachedChoiceCount = CachedChoices.Num();
	View.PaymentZoneMappingCount = PaymentZoneToChoiceId.Num();
	View.PaymentZoneMappingSummary = BuildPaymentZoneMappingDebugSummary();
	View.LastPaymentResolveSummary = LastPaymentDropResolveDebugSummary;
	View.LastPaymentSubmitSummary = LastPaymentDropSubmitDebugSummary;

	TSet<FGuid> UniqueCandidateIds;
	for (const FRunEventChoiceSnapshot& Choice : CachedChoices)
	{
		if (!Choice.bRequiresOwnedCardPayment)
		{
			continue;
		}

		++View.PaymentChoiceCount;
		for (const FGuid& CandidateId : Choice.PaymentCandidateInstanceIds)
		{
			if (CandidateId.IsValid())
			{
				UniqueCandidateIds.Add(CandidateId);
			}
		}
	}
	View.PaymentCandidateInstanceCount = UniqueCandidateIds.Num();
	return View;
}

FString UWacomRunEventScreen::GetRunEventScreenDebugSummary() const
{
	const FWacomRunEventScreenDebugView View = GetRunEventScreenDebugView();
	return FString::Printf(
		TEXT("RunEventScreen{HasRunSession=%s Active=%s PersistentId=%s EventId=%s Node=%s Title=\"%s\" Choices=%d PaymentChoices=%d Candidates=%d Zones=%d ZoneMap=[%s] LastResolve=%s LastSubmit=%s}"),
		View.bHasRunSession ? TEXT("true") : TEXT("false"),
		View.bIsEventActive ? TEXT("true") : TEXT("false"),
		*View.PersistentId.ToString(),
		*View.EventId.ToString(),
		*View.CurrentNodeId.ToString(),
		*View.CurrentNodeTitleText.ToString(),
		View.CachedChoiceCount,
		View.PaymentChoiceCount,
		View.PaymentCandidateInstanceCount,
		View.PaymentZoneMappingCount,
		*View.PaymentZoneMappingSummary,
		View.LastPaymentResolveSummary.IsEmpty() ? TEXT("None") : *View.LastPaymentResolveSummary,
		View.LastPaymentSubmitSummary.IsEmpty() ? TEXT("None") : *View.LastPaymentSubmitSummary);
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
	CachedChoices.Reset();
	PaymentZoneToChoiceId.Reset();
	PaymentDropTargets.Reset();
	if (ChoiceList)
	{
		ChoiceList->ClearChildren();
	}

	URunSession* Run = ResolveRunSession();
	const FRunEventSnapshot Snapshot = Run ? Run->BuildCurrentRunEventSnapshot() : FRunEventSnapshot();
	CachedChoices = Snapshot.Choices;

	if (EmptyText)
	{
		EmptyText->SetVisibility(Snapshot.Choices.Num() == 0
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (!ChoiceList)
	{
		return;
	}

	for (const FRunEventChoiceSnapshot& Choice : Snapshot.Choices)
	{
		AddChoiceButton(Choice);
	}
}

void UWacomRunEventScreen::AddChoiceButton(const FRunEventChoiceSnapshot& Choice)
{
	if (!WidgetTree || !ChoiceList)
	{
		return;
	}

	UWacomRunEventChoiceButton* ChoiceButtonWidget = WidgetTree->ConstructWidget<UWacomRunEventChoiceButton>(
		UWacomRunEventChoiceButton::StaticClass());
	ChoiceButtonWidget->SetChoiceSnapshot(Choice);
	ChoiceButtonWidget->OnChoiceClickedNative.AddUObject(this, &UWacomRunEventScreen::HandleChoiceClicked);
	UWidget* ChoiceWidget = ChoiceButtonWidget;
	if (Choice.bRequiresOwnedCardPayment && !Choice.PaymentZoneId.IsNone())
	{
		UWacomRunMenuDropTargetWidget* DropTarget =
			WidgetTree->ConstructWidget<UWacomRunMenuDropTargetWidget>(
				UWacomRunMenuDropTargetWidget::StaticClass(),
				FName(*FString::Printf(TEXT("RunEventChoiceDrop_%s"), *Choice.ChoiceId.ToString())));
		if (DropTarget)
		{
			DropTarget->ZoneId = Choice.PaymentZoneId;
			DropTarget->StableTargetId = Choice.PaymentZoneId;
			DropTarget->ProbePreviewScale = 1.025f;

			USizeBox* ChoiceSize = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				FName(*FString::Printf(TEXT("RunEventChoiceDropSize_%s"), *Choice.ChoiceId.ToString())));
			ChoiceSize->SetMinDesiredWidth(420.0f);
			ChoiceSize->SetContent(ChoiceButtonWidget);
			DropTarget->SetDropContent(ChoiceSize);
			ChoiceWidget = DropTarget;
			PaymentZoneToChoiceId.Add(Choice.PaymentZoneId, Choice.ChoiceId);
			PaymentDropTargets.Add(DropTarget);
		}
	}

	if (UVerticalBoxSlot* ChoiceSlot = ChoiceList->AddChildToVerticalBox(ChoiceWidget))
	{
		ChoiceSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 8.f));
	}
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
		CachedChoices,
		bDidEndRunEvent);
}

void UWacomRunEventScreen::RefreshPaymentLeaseFromCachedChoices()
{
	TArray<FGuid> CandidateInstanceIds;
	for (const FRunEventChoiceSnapshot& Choice : CachedChoices)
	{
		if (!Choice.bRequiresOwnedCardPayment)
		{
			continue;
		}
		for (const FGuid& CandidateId : Choice.PaymentCandidateInstanceIds)
		{
			if (CandidateId.IsValid())
			{
				CandidateInstanceIds.AddUnique(CandidateId);
			}
		}
	}
	if (CandidateInstanceIds.IsEmpty())
	{
		ClearOwnedRunFirstPersonCardLayerMenuLease();
		return;
	}

	FWacomRunMenuCardLeaseRequest Request;
	Request.LeaseId = TEXT("RunEventCardPayment");
	Request.SourceId = TEXT("RunEventCardPaymentSource");
	Request.ExplicitCardInstanceIds = MoveTemp(CandidateInstanceIds);
	FWacomRunMenuCardLeaseResult LeaseResult;
	SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(Request, LeaseResult);
}

bool UWacomRunEventScreen::FindPaymentChoiceForZone(
	FName ZoneId,
	FRunEventChoiceSnapshot& OutChoice) const
{
	if (ZoneId.IsNone())
	{
		return false;
	}
	const FName* ChoiceId = PaymentZoneToChoiceId.Find(ZoneId);
	if (!ChoiceId)
	{
		return false;
	}
	for (const FRunEventChoiceSnapshot& Choice : CachedChoices)
	{
		if (Choice.ChoiceId == *ChoiceId)
		{
			OutChoice = Choice;
			return true;
		}
	}
	return false;
}

FString UWacomRunEventScreen::BuildPaymentZoneMappingDebugSummary() const
{
	TArray<FString> Entries;
	Entries.Reserve(PaymentZoneToChoiceId.Num());
	for (const TPair<FName, FName>& Pair : PaymentZoneToChoiceId)
	{
		Entries.Add(FString::Printf(TEXT("%s->%s"), *Pair.Key.ToString(), *Pair.Value.ToString()));
	}
	Entries.Sort();
	return FString::Join(Entries, TEXT(","));
}

void UWacomRunEventScreen::RecordPaymentDropResolveDebug(
	const FWacomRunMenuCardDropResolveResult& Result) const
{
	LastPaymentDropResolveDebugSummary =
		BuildRunEventScreenDropResultDebugSummary(TEXT("Resolve"), Result);
}

void UWacomRunEventScreen::RecordPaymentDropSubmitDebug(
	const FWacomRunMenuCardDropResolveResult& Result) const
{
	LastPaymentDropSubmitDebugSummary =
		BuildRunEventScreenDropResultDebugSummary(TEXT("Submit"), Result);
}

#undef LOCTEXT_NAMESPACE
