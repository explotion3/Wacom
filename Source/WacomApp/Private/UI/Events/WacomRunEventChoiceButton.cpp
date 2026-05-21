// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventChoiceButton.h"

#define LOCTEXT_NAMESPACE "WacomRunEventChoiceButton"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "UI/Events/WacomRunEventPresentationBuilder.h"

namespace
{
	UTextBlock* MakeEventChoiceText(UWidgetTree* Tree, FName Name, const FText& Text, int32 FontSize)
	{
		UTextBlock* Block = Tree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), Name);
		Block->SetText(Text);
		FSlateFontInfo Font = Block->GetFont();
		Font.Size = FontSize;
		Block->SetFont(Font);
		Block->SetColorAndOpacity(FSlateColor(FLinearColor(0.95f, 0.95f, 0.95f, 1.f)));
		return Block;
	}
}

TSharedRef<SWidget> UWacomRunEventChoiceButton::RebuildWidget()
{
	if (!WidgetTree || !WidgetTree->RootWidget)
	{
		if (!WidgetTree)
		{
			WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
		}

		ChoiceButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("ChoiceButton"));
		WidgetTree->RootWidget = ChoiceButton;

		UVerticalBox* RootBox = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("RootBox"));
		ChoiceButton->AddChild(RootBox);

		LabelText = MakeEventChoiceText(WidgetTree, TEXT("LabelText"), LOCTEXT("Choice", "选项"), 18);
		RootBox->AddChildToVerticalBox(LabelText);

		DisabledReasonText = MakeEventChoiceText(WidgetTree, TEXT("DisabledReasonText"), FText::GetEmpty(), 14);
		DisabledReasonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.9f, 0.55f, 0.45f, 1.f)));
		RootBox->AddChildToVerticalBox(DisabledReasonText);
	}
	return Super::RebuildWidget();
}

void UWacomRunEventChoiceButton::NativeConstruct()
{
	Super::NativeConstruct();
	if (ChoiceButton)
	{
		ChoiceButton->OnClicked.AddUniqueDynamic(this, &UWacomRunEventChoiceButton::HandleClicked);
	}
	RefreshVisuals();
}

void UWacomRunEventChoiceButton::SetChoiceSnapshot(const FRunEventChoiceSnapshot& InChoice)
{
	ChoiceSnapshot = InChoice;
	RefreshVisuals();
}

void UWacomRunEventChoiceButton::HandleClicked()
{
	OnChoiceClickedNative.Broadcast(ChoiceSnapshot.ChoiceId);
}

void UWacomRunEventChoiceButton::RefreshVisuals()
{
	if (LabelText)
	{
		LabelText->SetText(ChoiceSnapshot.LabelText.IsEmpty()
			? FText::FromName(ChoiceSnapshot.ChoiceId)
			: ChoiceSnapshot.LabelText);
	}
	if (ChoiceButton)
	{
		ChoiceButton->SetIsEnabled(true);
		ChoiceButton->SetRenderOpacity(ChoiceSnapshot.bAvailable ? 1.f : 0.62f);
	}
	if (DisabledReasonText)
	{
		DisabledReasonText->SetText(ChoiceSnapshot.DisabledReason.IsNone()
			? FText::GetEmpty()
			: FText::Format(LOCTEXT("DisabledReasonFmt", "不可选：{0}"),
				UWacomRunEventPresentationBuilder::FormatDisabledReason(ChoiceSnapshot.DisabledReason)));
		DisabledReasonText->SetVisibility(ChoiceSnapshot.bAvailable || ChoiceSnapshot.DisabledReason.IsNone()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}
}

#undef LOCTEXT_NAMESPACE
