// Copyright Wacom. All Rights Reserved.

#include "UI/Menus/WacomConfirmDialog.h"

#define LOCTEXT_NAMESPACE "WacomConfirmDialog"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"

#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomUITags.h"

// ================ 静态工厂 ================

UWacomConfirmDialog* UWacomConfirmDialog::Show(
	UObject* WorldContext,
	const FText& Title,
	const FText& Message,
	TFunction<void()> OnConfirm,
	TFunction<void()> OnCancel)
{
	if (!WorldContext) { return nullptr; }

	UWorld* World = WorldContext->GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UWacomGameUIManagerSubsystem* UIManager =
		GI ? GI->GetSubsystem<UWacomGameUIManagerSubsystem>() : nullptr;
	if (!UIManager)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ConfirmDialog] Show: UIManager 未就位"));
		return nullptr;
	}

	UCommonActivatableWidget* Pushed = UIManager->PushContentToLayer(
		WacomUITags::UI_Layer_Modal.GetTag(),
		UWacomConfirmDialog::StaticClass());

	UWacomConfirmDialog* Dialog = Cast<UWacomConfirmDialog>(Pushed);
	if (!Dialog)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ConfirmDialog] Show: Push 失败"));
		return nullptr;
	}

	Dialog->SetCallbacks(MoveTemp(OnConfirm), MoveTemp(OnCancel));
	Dialog->PendingTitle      = Title;
	Dialog->PendingMessage    = Message;

	// 如果 NativeConstruct 已经跑过了（Widget 立即构造），直接设文本。
	// 否则 NativeConstruct 里会读 PendingTitle/Message。
	Dialog->SetContent(Title, Message);

	return Dialog;
}

// ================ 内容设置 ================

void UWacomConfirmDialog::SetContent(const FText& Title, const FText& Message)
{
	if (TitleText)   { TitleText->SetText(Title); }
	if (MessageText) { MessageText->SetText(Message); }
}

void UWacomConfirmDialog::SetCallbacks(TFunction<void()> OnConfirm, TFunction<void()> OnCancel)
{
	OnConfirmCallback = MoveTemp(OnConfirm);
	OnCancelCallback = MoveTemp(OnCancel);
}

// ================ 默认布局 ================

TSharedRef<SWidget> UWacomConfirmDialog::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// 居中面板
		UVerticalBox* VBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("VBox"));
		if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(VBox))
		{
			CanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			CanvasSlot->SetOffsets(FMargin(-200.f, -80.f, 400.f, 160.f));
			CanvasSlot->SetAutoSize(false);
		}

		// 标题
		TitleText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("TitleText"));
		TitleText->SetText(LOCTEXT("DefaultTitle", "确认"));
		TitleText->SetJustification(ETextJustify::Center);
		FSlateFontInfo TitleFont = TitleText->GetFont();
		TitleFont.Size = 24;
		TitleText->SetFont(TitleFont);
		if (UVerticalBoxSlot* TitleSlot = VBox->AddChildToVerticalBox(TitleText))
		{
			TitleSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 12.f));
			TitleSlot->SetHorizontalAlignment(HAlign_Center);
		}

		// 正文
		MessageText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("MessageText"));
		MessageText->SetText(LOCTEXT("DefaultMsg", "确定吗？"));
		MessageText->SetJustification(ETextJustify::Center);
		MessageText->SetAutoWrapText(true);
		if (UVerticalBoxSlot* MsgSlot = VBox->AddChildToVerticalBox(MessageText))
		{
			MsgSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 20.f));
			MsgSlot->SetHorizontalAlignment(HAlign_Center);
		}

		// 按钮行
		UHorizontalBox* BtnRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("BtnRow"));
		if (UVerticalBoxSlot* RowSlot = VBox->AddChildToVerticalBox(BtnRow))
		{
			RowSlot->SetHorizontalAlignment(HAlign_Center);
		}

		// Confirm 按钮
		ConfirmButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ConfirmButton"));
		{
			UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			BtnText->SetText(LOCTEXT("ConfirmBtn", "确认"));
			BtnText->SetJustification(ETextJustify::Center);
			ConfirmButton->AddChild(BtnText);
		}
		if (UHorizontalBoxSlot* BtnSlot = BtnRow->AddChildToHorizontalBox(ConfirmButton))
		{
			BtnSlot->SetPadding(FMargin(0.f, 0.f, 16.f, 0.f));
		}

		// Cancel 按钮
		CancelButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("CancelButton"));
		{
			UTextBlock* BtnText = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass());
			BtnText->SetText(LOCTEXT("CancelBtn", "取消"));
			BtnText->SetJustification(ETextJustify::Center);
			CancelButton->AddChild(BtnText);
		}
		BtnRow->AddChildToHorizontalBox(CancelButton);
	}
	return Super::RebuildWidget();
}

void UWacomConfirmDialog::NativeConstruct()
{
	Super::NativeConstruct();

	if (ConfirmButton) { ConfirmButton->OnClicked.AddUniqueDynamic(this, &UWacomConfirmDialog::HandleConfirmClicked); }
	if (CancelButton)  { CancelButton ->OnClicked.AddUniqueDynamic(this, &UWacomConfirmDialog::HandleCancelClicked); }

	// 应用 Show 时传入的文本（可能 RebuildWidget 之后才到 NativeConstruct）
	SetContent(PendingTitle, PendingMessage);
}

// ================ 按钮回调 ================

void UWacomConfirmDialog::HandleConfirmClicked()
{
	UE_LOG(LogTemp, Display, TEXT("[ConfirmDialog] Confirmed"));
	TFunction<void()> Cb = MoveTemp(OnConfirmCallback);
	DeactivateWidget();
	if (Cb) { Cb(); }
}

void UWacomConfirmDialog::HandleCancelClicked()
{
	UE_LOG(LogTemp, Display, TEXT("[ConfirmDialog] Cancelled"));
	TFunction<void()> Cb = MoveTemp(OnCancelCallback);
	DeactivateWidget();
	if (Cb) { Cb(); }
}

FReply UWacomConfirmDialog::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// Back = 取消（触发 OnCancel 回调，等价于点 Cancel）
	if (InKeyEvent.GetKey() == EKeys::Escape
		|| InKeyEvent.GetKey() == EKeys::Gamepad_FaceButton_Right)
	{
		HandleCancelClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

#undef LOCTEXT_NAMESPACE
