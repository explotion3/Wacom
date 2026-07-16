// Copyright Wacom. All Rights Reserved.

#include "UI/Foundation/WacomMenuButtonWidget.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/SizeBox.h"

UWacomMenuButtonWidget::UWacomMenuButtonWidget(
	const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetIsFocusable(true);
}

void UWacomMenuButtonWidget::InitializeNativeClassData()
{
	Super::InitializeNativeClassData();
	// UCommonButtonBase::Initialize wraps and binds an existing root after
	// UUserWidget::Initialize returns. Build only the native fallback tree here;
	// Blueprint subclasses must remain free to duplicate their authored tree.
	EnsureFallbackWidgetTree();
}

void UWacomMenuButtonWidget::EnsureFallbackWidgetTree()
{
	if (!WidgetTree)
	{
		WidgetTree = NewObject<UWidgetTree>(this, TEXT("WidgetTree_Default"));
	}
	if (!WidgetTree->RootWidget)
	{
		USizeBox* Root = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ButtonSize"));
		Root->SetMinDesiredWidth(144.0f);
		Root->SetMinDesiredHeight(42.0f);
		WidgetTree->RootWidget = Root;

		ButtonBackdrop = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ButtonBackdrop"));
		ButtonBackdrop->SetPadding(FMargin(12.0f, 7.0f));
		Root->AddChild(ButtonBackdrop);

		ButtonText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonText"));
		ButtonText->SetJustification(ETextJustify::Center);
		ButtonText->SetColorAndOpacity(FSlateColor(FLinearColor(0.91f, 0.90f, 0.82f, 1.0f)));
		ButtonBackdrop->AddChild(ButtonText);
	}
}

TSharedRef<SWidget> UWacomMenuButtonWidget::RebuildWidget()
{
	EnsureFallbackWidgetTree();
	return Super::RebuildWidget();
}

void UWacomMenuButtonWidget::NativeConstruct()
{
	Super::NativeConstruct();
	OnFocusReceived().RemoveAll(this);
	OnFocusReceived().AddUObject(
		this,
		&UWacomMenuButtonWidget::HandleCommonButtonFocusReceived);
	OnFocusLost().RemoveAll(this);
	OnFocusLost().AddUObject(
		this,
		&UWacomMenuButtonWidget::HandleCommonButtonFocusLost);
	bPresentationFocused = false;
	bPresentationHovered = false;
	bPresentationPressed = false;
	SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeDestruct()
{
	OnFocusReceived().RemoveAll(this);
	OnFocusLost().RemoveAll(this);
	bPresentationFocused = false;
	bPresentationHovered = false;
	bPresentationPressed = false;
	Super::NativeDestruct();
}

void UWacomMenuButtonWidget::NativeOnHovered()
{
	Super::NativeOnHovered();
	bPresentationHovered = true;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnUnhovered()
{
	Super::NativeOnUnhovered();
	bPresentationHovered = false;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnPressed()
{
	Super::NativeOnPressed();
	bPresentationPressed = true;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnReleased()
{
	Super::NativeOnReleased();
	bPresentationPressed = false;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnSelected(bool bBroadcast)
{
	Super::NativeOnSelected(bBroadcast);
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnDeselected(bool bBroadcast)
{
	Super::NativeOnDeselected(bBroadcast);
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnEnabled()
{
	Super::NativeOnEnabled();
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::NativeOnDisabled()
{
	Super::NativeOnDisabled();
	bPresentationPressed = false;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::HandleCommonButtonFocusReceived()
{
	bPresentationFocused = true;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::HandleCommonButtonFocusLost()
{
	bPresentationFocused = false;
	RefreshPresentationState();
}

void UWacomMenuButtonWidget::RefreshPresentationState()
{
	const bool bInteractable = IsInteractionEnabled();
	const bool bEmphasized = bInteractable
		&& (bPresentationHovered || bPresentationFocused || GetSelected());

	const FLinearColor RestingBackdrop(0.035f, 0.052f, 0.072f, 0.94f);
	const FLinearColor EmphasizedBackdrop(0.075f, 0.115f, 0.145f, 0.98f);
	const FLinearColor DisabledBackdrop(0.020f, 0.026f, 0.036f, 0.62f);
	const FLinearColor RestingText(0.91f, 0.90f, 0.82f, 1.0f);
	const FLinearColor EmphasizedText(0.98f, 0.78f, 0.32f, 1.0f);
	const FLinearColor DisabledText(0.38f, 0.41f, 0.43f, 1.0f);
	const FLinearColor AccentColor(0.98f, 0.62f, 0.18f, 1.0f);

	FLinearColor BackdropColor = bInteractable
		? (bEmphasized ? EmphasizedBackdrop : RestingBackdrop)
		: DisabledBackdrop;
	if (bPresentationPressed && bInteractable)
	{
		BackdropColor *= 0.86f;
		BackdropColor.A = EmphasizedBackdrop.A;
	}
	if (ButtonBackdrop)
	{
		ButtonBackdrop->SetBrushColor(BackdropColor);
	}
	if (Accent)
	{
		Accent->SetBrushColor(AccentColor);
		Accent->SetRenderOpacity(!bInteractable ? 0.10f : (bEmphasized ? 1.0f : 0.25f));
	}
	if (ButtonText)
	{
		ButtonText->SetColorAndOpacity(FSlateColor(
			bInteractable ? (bEmphasized ? EmphasizedText : RestingText) : DisabledText));
	}

	const float Scale = bPresentationPressed && bInteractable ? 0.985f : 1.0f;
	SetRenderScale(FVector2D(Scale));
}
