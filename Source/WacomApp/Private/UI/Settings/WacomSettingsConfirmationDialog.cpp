// Copyright Wacom. All Rights Reserved.

#include "UI/Settings/WacomSettingsConfirmationDialog.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Input/Events.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"

#define LOCTEXT_NAMESPACE "WacomSettingsConfirmationDialog"

void UWacomSettingsConfirmationDialog::Configure(
	EWacomSettingsConfirmationMode InMode,
	float InRemainingSeconds,
	FWacomSettingsConfirmationDecisionDelegate InOnDecision)
{
	Mode = InMode;
	InitialRemainingSeconds = FMath::Max(0.0f, InRemainingSeconds);
	OnDecision = MoveTemp(InOnDecision);
	RefreshContent();
	if (IsActivated())
	{
		StartCountdown();
	}
}

TSharedRef<SWidget> UWacomSettingsConfirmationDialog::RebuildWidget()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* Panel = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("DialogPanel"));
		Panel->SetBrushColor(FLinearColor(0.018f, 0.025f, 0.040f, 0.98f));
		Panel->SetPadding(FMargin(28.0f, 22.0f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetOffsets(FMargin(-280.0f, -125.0f, 560.0f, 250.0f));
		}

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("DialogColumn"));
		Panel->AddChild(Column);

		TitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetJustification(ETextJustify::Center);
		TitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.78f, 0.32f, 1.0f)));
		if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 16.0f));
		}

		MessageText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("MessageText"));
		MessageText->SetJustification(ETextJustify::Center);
		MessageText->SetAutoWrapText(true);
		MessageText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.90f, 0.82f, 1.0f)));
		if (UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(MessageText))
		{
			MessageSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 22.0f));
		}

		UHorizontalBox* Buttons = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("ButtonRow"));
		if (UVerticalBoxSlot* ButtonRowSlot = Column->AddChildToVerticalBox(Buttons))
		{
			ButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
		}

		ConfirmButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("ConfirmButton"));
		if (UHorizontalBoxSlot* ConfirmSlot = Buttons->AddChildToHorizontalBox(ConfirmButton))
		{
			ConfirmSlot->SetPadding(FMargin(0.0f, 0.0f, 14.0f, 0.0f));
		}
		CancelButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("CancelButton"));
		Buttons->AddChildToHorizontalBox(CancelButton);
	}
	return Super::RebuildWidget();
}

void UWacomSettingsConfirmationDialog::NativeConstruct()
{
	Super::NativeConstruct();
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked().RemoveAll(this);
		ConfirmButton->OnClicked().AddUObject(this, &UWacomSettingsConfirmationDialog::HandleConfirmClicked);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked().RemoveAll(this);
		CancelButton->OnClicked().AddUObject(this, &UWacomSettingsConfirmationDialog::HandleCancelClicked);
	}
	RefreshContent();
}

void UWacomSettingsConfirmationDialog::NativeDestruct()
{
	StopCountdown();
	if (ConfirmButton)
	{
		ConfirmButton->OnClicked().RemoveAll(this);
	}
	if (CancelButton)
	{
		CancelButton->OnClicked().RemoveAll(this);
	}
	OnDecision.Unbind();
	Super::NativeDestruct();
}

void UWacomSettingsConfirmationDialog::NativeOnActivated()
{
	Super::NativeOnActivated();
	RefreshContent();
	StartCountdown();
}

void UWacomSettingsConfirmationDialog::NativeOnDeactivated()
{
	StopCountdown();
	FWacomSettingsConfirmationDecisionDelegate Callback;
	if (!bClosingWithoutDecision && OnDecision.IsBound())
	{
		Callback = MoveTemp(OnDecision);
	}
	OnDecision.Unbind();
	Super::NativeOnDeactivated();
	if (Callback.IsBound())
	{
		Callback.Execute(EWacomSettingsConfirmationDecision::Cancel);
	}
}

FReply UWacomSettingsConfirmationDialog::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape
		|| InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		ResolveDecision(EWacomSettingsConfirmationDecision::Cancel);
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

UWidget* UWacomSettingsConfirmationDialog::NativeGetDesiredFocusTarget() const
{
	return Mode == EWacomSettingsConfirmationMode::DiscardChanges
		? static_cast<UWidget*>(CancelButton)
		: static_cast<UWidget*>(ConfirmButton);
}

void UWacomSettingsConfirmationDialog::CloseWithoutDecision()
{
	bClosingWithoutDecision = true;
	OnDecision.Unbind();
	StopCountdown();
	DeactivateWidget();
	bClosingWithoutDecision = false;
}

void UWacomSettingsConfirmationDialog::HandleConfirmClicked()
{
	ResolveDecision(EWacomSettingsConfirmationDecision::Confirm);
}

void UWacomSettingsConfirmationDialog::HandleCancelClicked()
{
	ResolveDecision(EWacomSettingsConfirmationDecision::Cancel);
}

void UWacomSettingsConfirmationDialog::RefreshContent()
{
	if (TitleText)
	{
		TitleText->SetText(Mode == EWacomSettingsConfirmationMode::VideoMode
			? LOCTEXT("VideoTitle", "保留显示设置？")
			: LOCTEXT("DiscardTitle", "放弃未保存的修改？"));
	}
	if (ConfirmButton)
	{
		ConfirmButton->SetButtonText(Mode == EWacomSettingsConfirmationMode::VideoMode
			? LOCTEXT("KeepVideo", "保留设置")
			: LOCTEXT("DiscardChanges", "放弃更改"));
	}
	if (CancelButton)
	{
		CancelButton->SetButtonText(Mode == EWacomSettingsConfirmationMode::VideoMode
			? LOCTEXT("RevertVideo", "恢复设置")
			: LOCTEXT("ContinueEditing", "继续编辑"));
	}

	if (!MessageText)
	{
		return;
	}
	if (Mode == EWacomSettingsConfirmationMode::VideoMode)
	{
		const float Remaining = CountdownDeadlineSeconds > 0.0
			? static_cast<float>(FMath::Max(0.0, CountdownDeadlineSeconds - FPlatformTime::Seconds()))
			: InitialRemainingSeconds;
		MessageText->SetText(FText::Format(
			LOCTEXT("VideoMessage", "请确认新的分辨率与窗口模式。{0} 秒后将自动恢复。"),
			FText::AsNumber(FMath::CeilToInt(Remaining))));
	}
	else
	{
		MessageText->SetText(LOCTEXT(
			"DiscardMessage",
			"返回将恢复本次编辑前的预览值，并丢弃尚未应用的修改。"));
	}
}

void UWacomSettingsConfirmationDialog::StartCountdown()
{
	StopCountdown();
	if (Mode != EWacomSettingsConfirmationMode::VideoMode || InitialRemainingSeconds <= 0.0f)
	{
		return;
	}
	CountdownDeadlineSeconds = FPlatformTime::Seconds() + InitialRemainingSeconds;
	CountdownTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateUObject(this, &UWacomSettingsConfirmationDialog::TickCountdown),
		0.1f);
	RefreshContent();
}

void UWacomSettingsConfirmationDialog::StopCountdown()
{
	if (CountdownTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(CountdownTickerHandle);
		CountdownTickerHandle.Reset();
	}
	CountdownDeadlineSeconds = 0.0;
}

bool UWacomSettingsConfirmationDialog::TickCountdown(float /*DeltaTime*/)
{
	if (CountdownDeadlineSeconds <= 0.0)
	{
		CountdownTickerHandle.Reset();
		return false;
	}
	if (FPlatformTime::Seconds() >= CountdownDeadlineSeconds)
	{
		CountdownTickerHandle.Reset();
		CountdownDeadlineSeconds = 0.0;
		ResolveDecision(EWacomSettingsConfirmationDecision::TimedOut);
		return false;
	}
	RefreshContent();
	return true;
}

void UWacomSettingsConfirmationDialog::ResolveDecision(
	EWacomSettingsConfirmationDecision Decision)
{
	if (!OnDecision.IsBound())
	{
		return;
	}
	FWacomSettingsConfirmationDecisionDelegate Callback = MoveTemp(OnDecision);
	OnDecision.Unbind();
	StopCountdown();
	bClosingWithoutDecision = true;
	DeactivateWidget();
	bClosingWithoutDecision = false;
	Callback.ExecuteIfBound(Decision);
}

#undef LOCTEXT_NAMESPACE
