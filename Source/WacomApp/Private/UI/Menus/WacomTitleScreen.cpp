// Copyright Wacom. All Rights Reserved.

#include "UI/Menus/WacomTitleScreen.h"

#define LOCTEXT_NAMESPACE "WacomTitleScreen"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Input/Events.h"
#include "InputCoreTypes.h"
#include "Settings/WacomLocalSettingsTypes.h"
#include "Settings/WacomSettingsSubsystem.h"

namespace
{
	void StyleTitleText(
		UTextBlock& Text,
		const FText& Value,
		int32 FontSize,
		const FLinearColor& Color,
		FName Typeface = TEXT("Regular"))
	{
		Text.SetText(Value);
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = Typeface;
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetJustification(ETextJustify::Center);
		Text.SetShadowOffset(FVector2D(2.0f, 2.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	}
}

UWacomTitleScreen::UWacomTitleScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
}

TSharedRef<SWidget> UWacomTitleScreen::RebuildWidget()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return Super::RebuildWidget();
	}

	UCanvasPanel* Root = WidgetTree->ConstructWidget<UCanvasPanel>(
		UCanvasPanel::StaticClass(), TEXT("TitleScreenRoot"));
	WidgetTree->RootWidget = Root;

	UBorder* SceneShade = WidgetTree->ConstructWidget<UBorder>(
		UBorder::StaticClass(), TEXT("TitleSceneShade"));
	SceneShade->SetBrushColor(FLinearColor(0.012f, 0.020f, 0.034f, 0.42f));
	if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(SceneShade))
	{
		CanvasSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		CanvasSlot->SetOffsets(FMargin(0.0f));
	}

	UVerticalBox* Content = WidgetTree->ConstructWidget<UVerticalBox>(
		UVerticalBox::StaticClass(), TEXT("TitleContentRoot"));
	TitleContentRoot = Content;
	if (UCanvasPanelSlot* CanvasSlot = Root->AddChildToCanvas(Content))
	{
		CanvasSlot->SetAnchors(FAnchors(0.5f, 0.47f));
		CanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
		CanvasSlot->SetAutoSize(true);
	}

	UTextBlock* Eyebrow = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TitleEyebrowText"));
	StyleTitleText(
		*Eyebrow,
		LOCTEXT("TitleEyebrow", "FIRST-PERSON CARD ADVENTURE"),
		15,
		FLinearColor(0.30f, 0.88f, 0.82f, 1.0f),
		TEXT("Bold"));
	Content->AddChildToVerticalBox(Eyebrow);

	UTextBlock* GameTitle = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TitleGameNameText"));
	StyleTitleText(
		*GameTitle,
		LOCTEXT("GameTitle", "WACOM"),
		82,
		FLinearColor(0.91f, 0.90f, 0.82f, 1.0f),
		TEXT("Bold"));
	if (UVerticalBoxSlot* BoxSlot = Content->AddChildToVerticalBox(GameTitle))
	{
		BoxSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 4.0f));
	}

	UTextBlock* Subtitle = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("TitleSubtitleText"));
	StyleTitleText(
		*Subtitle,
		LOCTEXT("GameSubtitle", "失落虫巢 · 旅程档案"),
		20,
		FLinearColor(0.98f, 0.78f, 0.32f, 1.0f),
		TEXT("Bold"));
	if (UVerticalBoxSlot* BoxSlot = Content->AddChildToVerticalBox(Subtitle))
	{
		BoxSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 52.0f));
	}

	PressAnyKeyText = WidgetTree->ConstructWidget<UTextBlock>(
		UTextBlock::StaticClass(), TEXT("PressAnyKeyText"));
	StyleTitleText(
		*PressAnyKeyText,
		LOCTEXT("PressAnyKey", "按任意键继续"),
		20,
		FLinearColor(0.91f, 0.90f, 0.82f, 1.0f),
		TEXT("Bold"));
	Content->AddChildToVerticalBox(PressAnyKeyText);

	return Super::RebuildWidget();
}

void UWacomTitleScreen::NativeConstruct()
{
	Super::NativeConstruct();
	BindRuntimeSettings();
}

void UWacomTitleScreen::NativeDestruct()
{
	UnbindRuntimeSettings();
	StopPromptPulse();
	Super::NativeDestruct();
}

void UWacomTitleScreen::NativeOnActivated()
{
	Super::NativeOnActivated();
	bAdvanceRequestInFlight = false;
	StartPromptPulse();
}

void UWacomTitleScreen::NativeOnDeactivated()
{
	StopPromptPulse();
	if (PressAnyKeyText)
	{
		PressAnyKeyText->SetRenderOpacity(1.0f);
	}
	Super::NativeOnDeactivated();
}

FReply UWacomTitleScreen::NativeOnKeyDown(
	const FGeometry& InGeometry,
	const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();
	if (Key == EKeys::Escape || Key == EKeys::Gamepad_FaceButton_Right)
	{
		return NativeHandleBackRequested();
	}

	if (Key.IsValid() && !Key.IsMouseButton())
	{
		RequestAdvance();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

FReply UWacomTitleScreen::NativeOnMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		RequestAdvance();
		return FReply::Handled();
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UWacomTitleScreen::NativeHandleBackRequested()
{
	// TitleScreen 是 GameMenu layer 的稳定根。Back 在这里没有上一级，必须消费。
	return FReply::Handled();
}

void UWacomTitleScreen::RequestAdvance()
{
	if (bAdvanceRequestInFlight || !IsActivated())
	{
		return;
	}

	bAdvanceRequestInFlight = true;
	OnAdvanceRequestedNative.Broadcast();

	// 正常 flow 会同步 Push MainMenu 并停用 Title。Push 失败时 Title 仍 active，
	// 允许下一次输入重试，避免一次资产加载失败永久锁死启动页。
	if (IsActivated())
	{
		bAdvanceRequestInFlight = false;
	}
}

void UWacomTitleScreen::StartPromptPulse()
{
	StopPromptPulse();
	PromptPulseElapsedSeconds = 0.0f;
	if (!PressAnyKeyText || bRuntimeSimplifiedMotion)
	{
		if (PressAnyKeyText)
		{
			PressAnyKeyText->SetRenderOpacity(1.0f);
		}
		return;
	}

	PromptPulseTickerHandle = FTSTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(this, [this](float DeltaTime)
		{
			return TickPromptPulse(DeltaTime);
		}));
}

void UWacomTitleScreen::StopPromptPulse()
{
	if (PromptPulseTickerHandle.IsValid())
	{
		FTSTicker::GetCoreTicker().RemoveTicker(PromptPulseTickerHandle);
		PromptPulseTickerHandle.Reset();
	}
}

bool UWacomTitleScreen::TickPromptPulse(float DeltaTime)
{
	if (!PressAnyKeyText || !IsActivated() || bRuntimeSimplifiedMotion)
	{
		PromptPulseTickerHandle.Reset();
		return false;
	}

	PromptPulseElapsedSeconds += FMath::Max(0.0f, DeltaTime);
	const float Wave = 0.5f + 0.5f * FMath::Sin(
		PromptPulseElapsedSeconds * (2.0f * PI / 1.6f));
	PressAnyKeyText->SetRenderOpacity(FMath::Lerp(0.46f, 1.0f, Wave));
	return true;
}

void UWacomTitleScreen::BindRuntimeSettings()
{
	UGameInstance* GameInstance = GetGameInstance();
	UWacomSettingsSubsystem* SettingsSubsystem = GameInstance
		? GameInstance->GetSubsystem<UWacomSettingsSubsystem>()
		: nullptr;
	if (!SettingsSubsystem)
	{
		return;
	}

	RuntimeSettingsChangedHandle = SettingsSubsystem->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomTitleScreen::HandleRuntimeSettingsChanged);
	HandleRuntimeSettingsChanged(
		SettingsSubsystem->GetCurrentSnapshot(),
		EWacomRuntimeSettingsChangeReason::Startup);
}

void UWacomTitleScreen::UnbindRuntimeSettings()
{
	if (UGameInstance* GameInstance = GetGameInstance())
	{
		if (UWacomSettingsSubsystem* SettingsSubsystem =
			GameInstance->GetSubsystem<UWacomSettingsSubsystem>())
		{
			SettingsSubsystem->OnRuntimeSettingsChangedNative().Remove(
				RuntimeSettingsChangedHandle);
		}
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomTitleScreen::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& Snapshot,
	EWacomRuntimeSettingsChangeReason /*Reason*/)
{
	bRuntimeSimplifiedMotion = Snapshot.UIMotionMode == EWacomUIMotionMode::Simplified;
	if (IsActivated())
	{
		StartPromptPulse();
	}
}

#undef LOCTEXT_NAMESPACE
