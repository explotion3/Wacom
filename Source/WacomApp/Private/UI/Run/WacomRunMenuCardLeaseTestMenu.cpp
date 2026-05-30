// Copyright Wacom. All Rights Reserved.

#include "UI/Run/WacomRunMenuCardLeaseTestMenu.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "UI/Run/WacomRunMenuDropTargetWidget.h"

#define LOCTEXT_NAMESPACE "WacomRunMenuCardLeaseTestMenu"

namespace
{
	UTextBlock* CreateLeaseTestText(
		UWidgetTree* Tree,
		FName Name,
		const FText& Text,
		int32 FontSize,
		const FLinearColor& Color = FLinearColor::White)
	{
		UTextBlock* TextBlock = Tree
			? Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name)
			: nullptr;
		if (TextBlock)
		{
			TextBlock->SetText(Text);
			TextBlock->SetColorAndOpacity(FSlateColor(Color));
			FSlateFontInfo Font = TextBlock->GetFont();
			Font.Size = FontSize;
			TextBlock->SetFont(Font);
			TextBlock->SetAutoWrapText(true);
		}
		return TextBlock;
	}

	UButton* CreateTextButton(
		UWidgetTree* Tree,
		FName ButtonName,
		FName TextName,
		const FText& Label)
	{
		UButton* Button = Tree
			? Tree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName)
			: nullptr;
		UTextBlock* Text = CreateLeaseTestText(
			Tree,
			TextName,
			Label,
			18,
			FLinearColor(0.92f, 0.95f, 1.0f, 1.0f));
		if (Button && Text)
		{
			Button->SetContent(Text);
		}
		return Button;
	}
}

UWacomRunMenuCardLeaseTestMenu::UWacomRunMenuCardLeaseTestMenu(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	LeaseRequest.LeaseId = TEXT("RunMenuCardLeaseTest");
	LeaseRequest.SourceId = TEXT("RunMenuCardLeaseTestSource");
	LeaseRequest.AllowedCardIds.Add(TEXT("PoisonFang"));
}

TSharedRef<SWidget> UWacomRunMenuCardLeaseTestMenu::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}

	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(),
			TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* DimBg = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("DimBg"));
		DimBg->SetBrushColor(FLinearColor(0.02f, 0.02f, 0.03f, 0.58f));
		if (UCanvasPanelSlot* DimSlot = Root->AddChildToCanvas(DimBg))
		{
			DimSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			DimSlot->SetOffsets(FMargin(0.0f));
		}

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(),
			TEXT("Panel"));
		Panel->SetBrushColor(FLinearColor(0.055f, 0.065f, 0.08f, 0.94f));
		Panel->SetPadding(FMargin(24.0f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetSize(FVector2D(640.0f, 420.0f));
		}

		UVerticalBox* Body = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("Body"));
		Panel->SetContent(Body);

		if (UTextBlock* Title = CreateLeaseTestText(
			WidgetTree,
			TEXT("Title"),
			LOCTEXT("Title", "Run Menu Card Lease Test"),
			26,
			FLinearColor(1.0f, 0.96f, 0.82f, 1.0f)))
		{
			Body->AddChildToVerticalBox(Title);
		}

		HintText = CreateLeaseTestText(
			WidgetTree,
			TEXT("Hint"),
			LOCTEXT("Hint", "Drag a leased first-person card onto the zone below. Releasing on the zone pays that exact owned card instance."),
			15,
			FLinearColor(0.75f, 0.8f, 0.88f, 1.0f));
		if (HintText)
		{
			if (UVerticalBoxSlot* HintSlot = Body->AddChildToVerticalBox(HintText))
			{
				HintSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 16.0f));
			}
		}

		DropTargetWidget = WidgetTree->ConstructWidget<UWacomRunMenuDropTargetWidget>(
			UWacomRunMenuDropTargetWidget::StaticClass(),
			TEXT("LeaseProbeZone"));
		if (DropTargetWidget)
		{
			DropTargetWidget->ZoneId = TestZoneId;
			DropTargetWidget->StableTargetId = TestStableTargetId;
			DropTargetWidget->ProbePreviewScale = 1.04f;

			USizeBox* ZoneSize = WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(),
				TEXT("ZoneSize"));
			ZoneSize->SetWidthOverride(560.0f);
			ZoneSize->SetHeightOverride(150.0f);

			UTextBlock* ZoneLabel = CreateLeaseTestText(
				WidgetTree,
				TEXT("ZoneLabel"),
				LOCTEXT("ZoneLabel", "ZONE: RunEvent.Pay.Fang"),
				22,
				FLinearColor(0.72f, 0.92f, 1.0f, 1.0f));
			if (ZoneLabel)
			{
				ZoneLabel->SetJustification(ETextJustify::Center);
			}
			ZoneSize->SetContent(ZoneLabel);
			DropTargetWidget->SetDropContent(ZoneSize);

			if (UVerticalBoxSlot* ZoneSlot = Body->AddChildToVerticalBox(DropTargetWidget))
			{
				ZoneSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
				ZoneSlot->SetHorizontalAlignment(HAlign_Center);
			}
		}

		StatusText = CreateLeaseTestText(
			WidgetTree,
			TEXT("Status"),
			FText::GetEmpty(),
			14,
			FLinearColor(0.86f, 0.9f, 0.96f, 1.0f));
		if (StatusText)
		{
			if (UVerticalBoxSlot* StatusSlot = Body->AddChildToVerticalBox(StatusText))
			{
				StatusSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
			}
		}

		UVerticalBox* Buttons = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			TEXT("Buttons"));
		Body->AddChildToVerticalBox(Buttons);

		RefreshButton = CreateTextButton(
			WidgetTree,
			TEXT("RefreshButton"),
			TEXT("RefreshButtonText"),
			LOCTEXT("RefreshButton", "Refresh Lease"));
		if (RefreshButton)
		{
			if (UVerticalBoxSlot* RefreshSlot = Buttons->AddChildToVerticalBox(RefreshButton))
			{
				RefreshSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
			}
		}

		CloseButton = CreateTextButton(
			WidgetTree,
			TEXT("CloseButton"),
			TEXT("CloseButtonText"),
			LOCTEXT("CloseButton", "Close"));
		if (CloseButton)
		{
			Buttons->AddChildToVerticalBox(CloseButton);
		}
	}

	return Super::RebuildWidget();
}

void UWacomRunMenuCardLeaseTestMenu::NativeOnActivated()
{
	Super::NativeOnActivated();

	if (DropTargetWidget)
	{
		DropTargetWidget->ZoneId = TestZoneId;
		DropTargetWidget->StableTargetId = TestStableTargetId;
	}

	if (RefreshButton)
	{
		RefreshButton->OnClicked.RemoveAll(this);
		RefreshButton->OnClicked.AddDynamic(
			this,
			&UWacomRunMenuCardLeaseTestMenu::HandleRefreshClicked);
	}
	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveAll(this);
		CloseButton->OnClicked.AddDynamic(
			this,
			&UWacomRunMenuCardLeaseTestMenu::HandleCloseClicked);
	}

	if (bRequestLeaseOnActivate)
	{
		RequestOwnedLeaseNow();
	}
	else
	{
		UpdateStatusText();
	}
}

bool UWacomRunMenuCardLeaseTestMenu::RequestOwnedLeaseNow()
{
	LastPaymentResult = FWacomRunMenuCardDropResolveResult();
	const bool bSet =
		SetOwnedRunFirstPersonCardLayerMenuLeaseFromRunCards(LeaseRequest, LastLeaseResult);
	UpdateStatusText();
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunMenuCardLeaseTestMenu] %s"),
		*GetLeaseTestDebugSummary());
	return bSet;
}

FString UWacomRunMenuCardLeaseTestMenu::GetLeaseTestDebugSummary() const
{
	return FString::Printf(
		TEXT("RunMenuCardLeaseTestMenu{LeaseSet=%s Reject=%s Candidates=%d Considered=%d LeaseId=%s SourceId=%s ZoneId=%s Result=%s}"),
		LastLeaseResult.bLeaseSet ? TEXT("true") : TEXT("false"),
		*LastLeaseResult.RejectReason.ToString(),
		LastLeaseResult.CandidateCount,
		LastLeaseResult.ConsideredCount,
		*LastLeaseResult.LeaseId.ToString(),
		*LastLeaseResult.SourceId.ToString(),
		*TestZoneId.ToString(),
		*LastLeaseResult.DebugSummary);
}

bool UWacomRunMenuCardLeaseTestMenu::CanAcceptOwnedRunFirstPersonCardPayment_Implementation(
	const FWacomRunMenuCardDropResolveResult& DropResult) const
{
	return DropResult.ZoneId == TestZoneId
		&& DropResult.LeaseId == GetOwnedRunFirstPersonCardLayerMenuLeaseId();
}

void UWacomRunMenuCardLeaseTestMenu::OnOwnedRunFirstPersonCardPaymentResolved_Implementation(
	const FWacomRunMenuCardDropResolveResult& DropResult)
{
	LastPaymentResult = DropResult;
	if (DropResult.bSubmitted)
	{
		RequestOwnedLeaseNow();
	}
	else
	{
		UpdateStatusText();
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRunMenuCardLeaseTestMenu] Payment %s"),
		*DropResult.DebugSummary);
}

void UWacomRunMenuCardLeaseTestMenu::UpdateStatusText()
{
	if (!StatusText)
	{
		return;
	}

	const FText Status = FText::Format(
		LOCTEXT(
			"StatusFormat",
			"LeaseSet={0}  Reject={1}  Candidates={2}  Considered={3}\nLeaseId={4}  SourceId={5}\nZoneId={6}\nLastPayment={7}  Submitted={8}"),
		LastLeaseResult.bLeaseSet ? LOCTEXT("True", "true") : LOCTEXT("False", "false"),
		FText::FromName(LastLeaseResult.RejectReason),
		FText::AsNumber(LastLeaseResult.CandidateCount),
		FText::AsNumber(LastLeaseResult.ConsideredCount),
		FText::FromName(LastLeaseResult.LeaseId),
		FText::FromName(LastLeaseResult.SourceId),
		FText::FromName(TestZoneId),
		FText::FromString(LastPaymentResult.DebugSummary),
		LastPaymentResult.bSubmitted ? LOCTEXT("PaymentSubmittedTrue", "true") : LOCTEXT("PaymentSubmittedFalse", "false"));
	StatusText->SetText(Status);
}

void UWacomRunMenuCardLeaseTestMenu::HandleRefreshClicked()
{
	RequestOwnedLeaseNow();
}

void UWacomRunMenuCardLeaseTestMenu::HandleCloseClicked()
{
	DeactivateWidget();
}

#undef LOCTEXT_NAMESPACE
