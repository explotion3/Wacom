// Copyright Wacom. All Rights Reserved.

#include "UI/Settings/WacomSettingsScreen.h"

#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Settings/WacomSettingsSubsystem.h"
#include "TimerManager.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/Settings/WacomSettingsConfirmationDialog.h"
#include "UI/Settings/WacomSettingsOptionRow.h"

#define LOCTEXT_NAMESPACE "WacomSettingsScreen"

namespace
{
	struct FSettingsFieldDescriptor
	{
		EWacomSettingsField Field;
		EWacomSettingsCategory Category;
		FText Label;
		EWacomSettingsOptionKind Kind;
		bool bPreviewable;
	};

	const TArray<EWacomSettingsCategory>& GetAllCategories()
	{
		static const TArray<EWacomSettingsCategory> Categories = {
			EWacomSettingsCategory::Display,
			EWacomSettingsCategory::Graphics,
			EWacomSettingsCategory::Audio,
			EWacomSettingsCategory::View,
			EWacomSettingsCategory::Accessibility
		};
		return Categories;
	}

	FText GetCategoryLabel(EWacomSettingsCategory Category)
	{
		switch (Category)
		{
		case EWacomSettingsCategory::Display: return LOCTEXT("CategoryDisplay", "显示");
		case EWacomSettingsCategory::Graphics: return LOCTEXT("CategoryGraphics", "图形");
		case EWacomSettingsCategory::Audio: return LOCTEXT("CategoryAudio", "音频");
		case EWacomSettingsCategory::View: return LOCTEXT("CategoryView", "视角");
		case EWacomSettingsCategory::Accessibility: return LOCTEXT("CategoryAccessibility", "辅助");
		default: return FText::GetEmpty();
		}
	}

	FSettingsFieldDescriptor GetFieldDescriptor(EWacomSettingsField Field)
	{
		switch (Field)
		{
		case EWacomSettingsField::ScreenResolution:
			return { Field, EWacomSettingsCategory::Display, LOCTEXT("Resolution", "分辨率"), EWacomSettingsOptionKind::Discrete, false };
		case EWacomSettingsField::WindowMode:
			return { Field, EWacomSettingsCategory::Display, LOCTEXT("WindowMode", "窗口模式"), EWacomSettingsOptionKind::Discrete, false };
		case EWacomSettingsField::VSync:
			return { Field, EWacomSettingsCategory::Display, LOCTEXT("VSync", "垂直同步"), EWacomSettingsOptionKind::Discrete, false };
		case EWacomSettingsField::FrameRateLimit:
			return { Field, EWacomSettingsCategory::Display, LOCTEXT("FrameRate", "帧率上限"), EWacomSettingsOptionKind::Discrete, false };
		case EWacomSettingsField::GraphicsQuality:
			return { Field, EWacomSettingsCategory::Graphics, LOCTEXT("GraphicsQuality", "整体画质"), EWacomSettingsOptionKind::Discrete, false };
		case EWacomSettingsField::MasterVolume:
			return { Field, EWacomSettingsCategory::Audio, LOCTEXT("MasterVolume", "主音量"), EWacomSettingsOptionKind::Continuous, true };
		case EWacomSettingsField::MusicVolume:
			return { Field, EWacomSettingsCategory::Audio, LOCTEXT("MusicVolume", "音乐"), EWacomSettingsOptionKind::Continuous, true };
		case EWacomSettingsField::SFXVolume:
			return { Field, EWacomSettingsCategory::Audio, LOCTEXT("SFXVolume", "音效"), EWacomSettingsOptionKind::Continuous, true };
		case EWacomSettingsField::UISoundVolume:
			return { Field, EWacomSettingsCategory::Audio, LOCTEXT("UISoundVolume", "UI 音效"), EWacomSettingsOptionKind::Continuous, true };
		case EWacomSettingsField::LookResponseStrength:
			return { Field, EWacomSettingsCategory::View, LOCTEXT("LookResponse", "视角响应强度"), EWacomSettingsOptionKind::Continuous, true };
		case EWacomSettingsField::InvertLookY:
			return { Field, EWacomSettingsCategory::View, LOCTEXT("InvertLookY", "反转 Y 轴"), EWacomSettingsOptionKind::Discrete, true };
		case EWacomSettingsField::CameraMotionStrength:
			return { Field, EWacomSettingsCategory::View, LOCTEXT("CameraMotion", "镜头运动强度"), EWacomSettingsOptionKind::Continuous, true };
		case EWacomSettingsField::FlashEffectMode:
			return { Field, EWacomSettingsCategory::Accessibility, LOCTEXT("FlashMode", "闪光效果"), EWacomSettingsOptionKind::Discrete, true };
		case EWacomSettingsField::UIMotionMode:
			return { Field, EWacomSettingsCategory::Accessibility, LOCTEXT("UIMotion", "UI 动效"), EWacomSettingsOptionKind::Discrete, true };
		default:
			return { Field, EWacomSettingsCategory::Display, FText::GetEmpty(), EWacomSettingsOptionKind::Discrete, false };
		}
	}

	TArray<EWacomSettingsField> GetFieldsForCategory(EWacomSettingsCategory Category)
	{
		switch (Category)
		{
		case EWacomSettingsCategory::Display:
			return { EWacomSettingsField::ScreenResolution, EWacomSettingsField::WindowMode,
				EWacomSettingsField::VSync, EWacomSettingsField::FrameRateLimit };
		case EWacomSettingsCategory::Graphics:
			return { EWacomSettingsField::GraphicsQuality };
		case EWacomSettingsCategory::Audio:
			return { EWacomSettingsField::MasterVolume, EWacomSettingsField::MusicVolume,
				EWacomSettingsField::SFXVolume, EWacomSettingsField::UISoundVolume };
		case EWacomSettingsCategory::View:
			return { EWacomSettingsField::LookResponseStrength, EWacomSettingsField::InvertLookY,
				EWacomSettingsField::CameraMotionStrength };
		case EWacomSettingsCategory::Accessibility:
			return { EWacomSettingsField::FlashEffectMode, EWacomSettingsField::UIMotionMode };
		default:
			return {};
		}
	}

	TArray<float> BuildFrameRateOptions(float CurrentValue)
	{
		TArray<float> Values = { 0.0f, 30.0f, 60.0f, 90.0f, 120.0f, 144.0f, 165.0f, 240.0f };
		if (CurrentValue > 0.0f && !Values.ContainsByPredicate(
			[CurrentValue](float Value) { return FMath::IsNearlyEqual(Value, CurrentValue, 0.1f); }))
		{
			Values.Add(CurrentValue);
			Values.Sort();
		}
		return Values;
	}

	FText FormatPercent(float Value)
	{
		return FText::Format(LOCTEXT("PercentFormat", "{0}%"), FText::AsNumber(FMath::RoundToInt(Value * 100.0f)));
	}

	FText FormatResolution(FIntPoint Resolution, bool bDesktopManaged)
	{
		// Display modes conventionally omit locale thousands separators (1920 × 1080).
		const FText Dimensions = FText::FromString(FString::Printf(
			TEXT("%d × %d"), Resolution.X, Resolution.Y));
		return bDesktopManaged
			? FText::Format(LOCTEXT("DesktopResolutionFormat", "跟随桌面（{0}）"), Dimensions)
			: Dimensions;
	}

	FText FormatWindowMode(EWindowMode::Type Mode)
	{
		switch (Mode)
		{
		case EWindowMode::Fullscreen: return LOCTEXT("Fullscreen", "全屏");
		case EWindowMode::Windowed: return LOCTEXT("Windowed", "窗口");
		case EWindowMode::WindowedFullscreen: return LOCTEXT("Borderless", "无边框窗口");
		default: return LOCTEXT("UnknownWindowMode", "未知");
		}
	}

	bool IsResolutionLess(const FIntPoint& A, const FIntPoint& B)
	{
		return A.X == B.X ? A.Y < B.Y : A.X < B.X;
	}

	FText FormatFieldValue(
		EWacomSettingsField Field,
		const FWacomLocalSettingsSnapshot& Draft,
		const FWacomScreenResolutionOptions& ResolutionOptions)
	{
		switch (Field)
		{
		case EWacomSettingsField::ScreenResolution:
		{
			const bool bDesktopManaged = Draft.WindowMode == EWindowMode::WindowedFullscreen;
			const FIntPoint DisplayResolution = bDesktopManaged
				&& ResolutionOptions.DesktopResolution.X > 0
				&& ResolutionOptions.DesktopResolution.Y > 0
				? ResolutionOptions.DesktopResolution
				: Draft.ScreenResolution;
			return FormatResolution(DisplayResolution, bDesktopManaged);
		}
		case EWacomSettingsField::WindowMode:
			return FormatWindowMode(Draft.WindowMode);
		case EWacomSettingsField::VSync:
			return Draft.bVSyncEnabled ? LOCTEXT("On", "开") : LOCTEXT("Off", "关");
		case EWacomSettingsField::FrameRateLimit:
			return Draft.FrameRateLimit <= 0.0f
				? LOCTEXT("Unlimited", "无限制")
				: FText::Format(LOCTEXT("FPSFormat", "{0} FPS"), FText::AsNumber(FMath::RoundToInt(Draft.FrameRateLimit)));
		case EWacomSettingsField::GraphicsQuality:
		{
			static const FText Labels[] = {
				LOCTEXT("QualityLow", "低"), LOCTEXT("QualityMedium", "中"),
				LOCTEXT("QualityHigh", "高"), LOCTEXT("QualityEpic", "史诗")
			};
			return Labels[FMath::Clamp(Draft.GraphicsQuality, 0, 3)];
		}
		case EWacomSettingsField::MasterVolume: return FormatPercent(Draft.MasterVolume);
		case EWacomSettingsField::MusicVolume: return FormatPercent(Draft.MusicVolume);
		case EWacomSettingsField::SFXVolume: return FormatPercent(Draft.SFXVolume);
		case EWacomSettingsField::UISoundVolume: return FormatPercent(Draft.UISoundVolume);
		case EWacomSettingsField::LookResponseStrength: return FormatPercent(Draft.LookResponseStrength);
		case EWacomSettingsField::InvertLookY:
			return Draft.bInvertLookY ? LOCTEXT("Inverted", "反转") : LOCTEXT("Normal", "正常");
		case EWacomSettingsField::CameraMotionStrength: return FormatPercent(Draft.CameraMotionStrength);
		case EWacomSettingsField::FlashEffectMode:
			switch (Draft.FlashEffectMode)
			{
			case EWacomFlashEffectMode::Full: return LOCTEXT("FlashFull", "完整");
			case EWacomFlashEffectMode::Reduced: return LOCTEXT("FlashReduced", "降低");
			case EWacomFlashEffectMode::Off: return LOCTEXT("FlashOff", "关闭");
			default: return LOCTEXT("FlashUnknown", "未知");
			}
		case EWacomSettingsField::UIMotionMode:
			return Draft.UIMotionMode == EWacomUIMotionMode::Simplified
				? LOCTEXT("MotionSimplified", "简化")
				: LOCTEXT("MotionFull", "完整");
		default:
			return FText::GetEmpty();
		}
	}

	float GetNormalizedFieldValue(EWacomSettingsField Field, const FWacomLocalSettingsSnapshot& Draft)
	{
		switch (Field)
		{
		case EWacomSettingsField::MasterVolume: return Draft.MasterVolume;
		case EWacomSettingsField::MusicVolume: return Draft.MusicVolume;
		case EWacomSettingsField::SFXVolume: return Draft.SFXVolume;
		case EWacomSettingsField::UISoundVolume: return Draft.UISoundVolume;
		case EWacomSettingsField::LookResponseStrength: return Draft.LookResponseStrength / 3.0f;
		case EWacomSettingsField::CameraMotionStrength: return Draft.CameraMotionStrength;
		default: return 0.0f;
		}
	}
}

UWacomSettingsScreen::UWacomSettingsScreen(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRestoreFocus = true;
	SettingsButtonClass = UWacomMenuButtonWidget::StaticClass();
	OptionRowClass = UWacomSettingsOptionRow::StaticClass();
	ConfirmationDialogClass = UWacomSettingsConfirmationDialog::StaticClass();
	ActiveConfirmationMode = EWacomSettingsConfirmationMode::DiscardChanges;
}

TSharedRef<SWidget> UWacomSettingsScreen::RebuildWidget()
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
			UBorder::StaticClass(), TEXT("SettingsPanel"));
		Panel->SetBrushColor(FLinearColor(0.018f, 0.025f, 0.040f, 0.96f));
		Panel->SetPadding(FMargin(28.0f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(Panel))
		{
			PanelSlot->SetAnchors(FAnchors(0.08f, 0.08f, 0.92f, 0.92f));
			PanelSlot->SetOffsets(FMargin(0.0f));
		}

		UVerticalBox* Page = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("Page"));
		Panel->AddChild(Page);

		UCommonTextBlock* Title = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("TitleText"));
		Title->SetText(LOCTEXT("SettingsTitle", "设置"));
		Title->SetColorAndOpacity(FSlateColor(FLinearColor(0.98f, 0.78f, 0.32f, 1.0f)));
		if (UVerticalBoxSlot* TitleSlot = Page->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}

		UHorizontalBox* Body = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Body"));
		if (UVerticalBoxSlot* BodySlot = Page->AddChildToVerticalBox(Body))
		{
			BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		USizeBox* CategorySize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("CategorySize"));
		CategorySize->SetWidthOverride(220.0f);
		CategoryContainer = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("CategoryContainer"));
		CategorySize->AddChild(CategoryContainer);
		if (UHorizontalBoxSlot* CategorySlot = Body->AddChildToHorizontalBox(CategorySize))
		{
			CategorySlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
		}

		UVerticalBox* OptionColumn = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("OptionColumn"));
		if (UHorizontalBoxSlot* OptionColumnSlot = Body->AddChildToHorizontalBox(OptionColumn))
		{
			OptionColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		CategoryTitleText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("CategoryTitleText"));
		CategoryTitleText->SetColorAndOpacity(FSlateColor(FLinearColor(0.30f, 0.88f, 0.82f, 1.0f)));
		if (UVerticalBoxSlot* CategoryTitleSlot = OptionColumn->AddChildToVerticalBox(CategoryTitleText))
		{
			CategoryTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		UScrollBox* Scroll = WidgetTree->ConstructWidget<UScrollBox>(
			UScrollBox::StaticClass(), TEXT("OptionsScroll"));
		if (UVerticalBoxSlot* ScrollSlot = OptionColumn->AddChildToVerticalBox(Scroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		OptionsContainer = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("OptionsContainer"));
		Scroll->AddChild(OptionsContainer);

		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Footer"));
		if (UVerticalBoxSlot* FooterSlot = Page->AddChildToVerticalBox(Footer))
		{
			FooterSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
		}

		StatusText = WidgetTree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("StatusText"));
		StatusText->SetColorAndOpacity(FSlateColor(FLinearColor(0.57f, 0.63f, 0.65f, 1.0f)));
		if (UHorizontalBoxSlot* StatusSlot = Footer->AddChildToHorizontalBox(StatusText))
		{
			StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			StatusSlot->SetVerticalAlignment(VAlign_Center);
		}

		RestoreDefaultsButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("RestoreDefaultsButton"));
		if (UHorizontalBoxSlot* RestoreSlot = Footer->AddChildToHorizontalBox(RestoreDefaultsButton))
		{
			RestoreSlot->SetPadding(FMargin(8.0f, 0.0f));
		}

		ApplyButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("ApplyButton"));
		if (UHorizontalBoxSlot* ApplySlot = Footer->AddChildToHorizontalBox(ApplyButton))
		{
			ApplySlot->SetPadding(FMargin(8.0f, 0.0f));
		}
		BackButton = WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), TEXT("BackButton"));
		Footer->AddChildToHorizontalBox(BackButton);
	}
	return Super::RebuildWidget();
}

void UWacomSettingsScreen::NativeConstruct()
{
	Super::NativeConstruct();
	if (RestoreDefaultsButton)
	{
		RestoreDefaultsButton->SetButtonText(LOCTEXT("RestoreDefaults", "恢复默认"));
		RestoreDefaultsButton->OnClicked().RemoveAll(this);
		RestoreDefaultsButton->OnClicked().AddUObject(
			this, &UWacomSettingsScreen::HandleRestoreDefaultsClicked);
	}
	if (ApplyButton)
	{
		ApplyButton->SetButtonText(LOCTEXT("Apply", "应用"));
		ApplyButton->OnClicked().RemoveAll(this);
		ApplyButton->OnClicked().AddUObject(this, &UWacomSettingsScreen::HandleApplyClicked);
	}
	if (BackButton)
	{
		BackButton->SetButtonText(LOCTEXT("Back", "返回"));
		BackButton->OnClicked().RemoveAll(this);
		BackButton->OnClicked().AddUObject(this, &UWacomSettingsScreen::HandleBackClicked);
	}
	BuildCategoryButtons();
	SelectCategory(SelectedCategory);
}

void UWacomSettingsScreen::NativeDestruct()
{
	if (RestoreDefaultsButton)
	{
		RestoreDefaultsButton->OnClicked().RemoveAll(this);
	}
	if (ApplyButton)
	{
		ApplyButton->OnClicked().RemoveAll(this);
	}
	if (BackButton)
	{
		BackButton->OnClicked().RemoveAll(this);
	}
	for (int32 Index = 0; Index < CategoryButtons.Num(); ++Index)
	{
		if (CategoryButtons[Index] && CategoryButtonDelegateHandles.IsValidIndex(Index))
		{
			CategoryButtons[Index]->OnClicked().Remove(CategoryButtonDelegateHandles[Index]);
		}
	}
	CategoryButtonDelegateHandles.Reset();
	CategoryButtons.Reset();
	ActiveOptionRows.Reset();
	OptionRowsByField.Reset();
	if (!bTearingDown)
	{
		bTearingDown = true;
		CloseActiveDialogWithoutDecision();
		EndOwnedSessionForTeardown();
		UnbindRuntimeSettings();
		bTearingDown = false;
	}
	Super::NativeDestruct();
}

void UWacomSettingsScreen::NativeOnActivated()
{
	bTearingDown = false;
	bFatalSessionFailureScheduled = false;
	Super::NativeOnActivated();
	if (!EditToken.IsValid() && !bAwaitingVideoConfirmation)
	{
		BeginEditSession();
	}
	RefreshFromDraft();
}

void UWacomSettingsScreen::NativeOnDeactivated()
{
	bTearingDown = true;
	CloseActiveDialogWithoutDecision();
	EndOwnedSessionForTeardown();
	UnbindRuntimeSettings();
	Super::NativeOnDeactivated();
	bTearingDown = false;
}

FReply UWacomSettingsScreen::NativeHandleBackRequested()
{
	RequestClose();
	return FReply::Handled();
}

UWidget* UWacomSettingsScreen::NativeGetDesiredFocusTarget() const
{
	const int32 CategoryIndex = static_cast<int32>(SelectedCategory);
	if (CategoryButtons.IsValidIndex(CategoryIndex) && CategoryButtons[CategoryIndex])
	{
		return CategoryButtons[CategoryIndex];
	}
	return ApplyButton ? static_cast<UWidget*>(ApplyButton) : static_cast<UWidget*>(BackButton);
}

void UWacomSettingsScreen::BuildCategoryButtons()
{
	if (!CategoryContainer)
	{
		return;
	}
	for (int32 Index = 0; Index < CategoryButtons.Num(); ++Index)
	{
		if (CategoryButtons[Index] && CategoryButtonDelegateHandles.IsValidIndex(Index))
		{
			CategoryButtons[Index]->OnClicked().Remove(CategoryButtonDelegateHandles[Index]);
		}
	}
	CategoryContainer->ClearChildren();
	CategoryButtons.Reset();
	CategoryButtonDelegateHandles.Reset();

	TSubclassOf<UWacomMenuButtonWidget> ButtonClass = SettingsButtonClass;
	if (!ButtonClass)
	{
		ButtonClass = UWacomMenuButtonWidget::StaticClass();
	}
	for (EWacomSettingsCategory Category : GetAllCategories())
	{
		UWacomMenuButtonWidget* Button = CreateWidget<UWacomMenuButtonWidget>(this, ButtonClass);
		if (!Button)
		{
			continue;
		}
		Button->SetButtonText(GetCategoryLabel(Category));
		if (UVerticalBoxSlot* ButtonSlot = CategoryContainer->AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		CategoryButtons.Add(Button);
		CategoryButtonDelegateHandles.Add(Button->OnClicked().AddWeakLambda(
			this,
			[this, Category]() { SelectCategory(Category, true); }));
	}
}

void UWacomSettingsScreen::RebuildOptionRows()
{
	if (!OptionsContainer)
	{
		return;
	}
	OptionsContainer->ClearChildren();
	ActiveOptionRows.Reset();
	OptionRowsByField.Reset();

	TSubclassOf<UWacomSettingsOptionRow> RowClass = OptionRowClass;
	if (!RowClass)
	{
		RowClass = UWacomSettingsOptionRow::StaticClass();
	}
	for (EWacomSettingsField Field : GetFieldsForCategory(SelectedCategory))
	{
		UWacomSettingsOptionRow* Row = CreateWidget<UWacomSettingsOptionRow>(this, RowClass);
		if (!Row)
		{
			continue;
		}
		Row->OnStepRequestedNative.AddWeakLambda(
			this,
			[this, Field](int32 Direction) { HandleOptionStep(Field, Direction); });
		Row->OnNormalizedValueRequestedNative.AddWeakLambda(
			this,
			[this, Field](float Value) { HandleOptionNormalizedValue(Field, Value); });
		if (UVerticalBoxSlot* RowSlot = OptionsContainer->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		ActiveOptionRows.Add(Row);
		OptionRowsByField.Add(Field, Row);
	}
	RefreshOptionRows();
}

void UWacomSettingsScreen::RefreshFromDraft()
{
	if (CategoryTitleText)
	{
		CategoryTitleText->SetText(GetCategoryLabel(SelectedCategory));
	}
	RefreshOptionRows();
	RefreshInteractionState();
}

void UWacomSettingsScreen::RefreshOptionRows()
{
	const bool bScreenAllowsInteraction = EditToken.IsValid()
		&& !bAwaitingVideoConfirmation && !bTearingDown;
	for (const TPair<EWacomSettingsField, TWeakObjectPtr<UWacomSettingsOptionRow>>& Pair : OptionRowsByField)
	{
		UWacomSettingsOptionRow* Row = Pair.Value.Get();
		if (!Row)
		{
			continue;
		}
		const FSettingsFieldDescriptor Descriptor = GetFieldDescriptor(Pair.Key);
		FWacomSettingsOptionRowViewData ViewData;
		ViewData.Label = Descriptor.Label;
		ViewData.Value = FormatFieldValue(Pair.Key, Draft, ScreenResolutionOptions);
		ViewData.Kind = Descriptor.Kind;
		ViewData.NormalizedValue = GetNormalizedFieldValue(Pair.Key, Draft);
		ViewData.bEnabled = bScreenAllowsInteraction
			&& (Pair.Key != EWacomSettingsField::ScreenResolution
				|| ScreenResolutionOptions.bCanSelectResolution);
		Row->ApplyViewData(ViewData);
	}
}

void UWacomSettingsScreen::RefreshInteractionState()
{
	const bool bCanInteract = EditToken.IsValid() && !bAwaitingVideoConfirmation && !bTearingDown;
	for (int32 Index = 0; Index < CategoryButtons.Num(); ++Index)
	{
		if (!CategoryButtons[Index])
		{
			continue;
		}
		CategoryButtons[Index]->SetIsInteractionEnabled(bCanInteract);
		CategoryButtons[Index]->SetIsSelected(Index == static_cast<int32>(SelectedCategory));
		CategoryButtons[Index]->RefreshPresentationState();
	}
	if (ApplyButton)
	{
		ApplyButton->SetIsInteractionEnabled(bCanInteract && IsDirty());
		ApplyButton->RefreshPresentationState();
	}
	if (RestoreDefaultsButton)
	{
		RestoreDefaultsButton->SetIsInteractionEnabled(
			bCanInteract && !Draft.IsEquivalentTo(DefaultSnapshot));
		RestoreDefaultsButton->RefreshPresentationState();
	}
	if (BackButton)
	{
		BackButton->SetIsInteractionEnabled(!bAwaitingVideoConfirmation && !bTearingDown);
		BackButton->RefreshPresentationState();
	}
	if (StatusText)
	{
		if (!StatusMessage.IsEmpty())
		{
			StatusText->SetText(StatusMessage);
		}
		else
		{
			StatusText->SetText(IsDirty()
				? LOCTEXT("DirtyStatus", "有未应用的修改")
				: LOCTEXT("SavedStatus", "所有更改已应用"));
		}
	}
	RefreshOptionRows();
}

bool UWacomSettingsScreen::RefreshScreenResolutionOptions(bool bNormalizeDraft)
{
	if (!SettingsSubsystem)
	{
		ScreenResolutionOptions = FWacomScreenResolutionOptions();
		return false;
	}

	ScreenResolutionOptions = SettingsSubsystem->GetScreenResolutionOptions(
		Draft.WindowMode,
		Draft.ScreenResolution);
	return bNormalizeDraft && NormalizeDraftResolutionForCurrentMode();
}

bool UWacomSettingsScreen::NormalizeDraftResolutionForCurrentMode()
{
	FIntPoint NormalizedResolution = Draft.ScreenResolution;
	if (Draft.WindowMode == EWindowMode::WindowedFullscreen)
	{
		if (ScreenResolutionOptions.DesktopResolution.X <= 0
			|| ScreenResolutionOptions.DesktopResolution.Y <= 0)
		{
			return false;
		}
		NormalizedResolution = ScreenResolutionOptions.DesktopResolution;
	}
	else
	{
		const TArray<FIntPoint>& Resolutions = ScreenResolutionOptions.SelectableResolutions;
		if (!ScreenResolutionOptions.bCanSelectResolution
			|| Resolutions.Contains(Draft.ScreenResolution))
		{
			return false;
		}

		int64 BestDistance = MAX_int64;
		for (const FIntPoint Resolution : Resolutions)
		{
			const int64 DeltaX = static_cast<int64>(Resolution.X) - Draft.ScreenResolution.X;
			const int64 DeltaY = static_cast<int64>(Resolution.Y) - Draft.ScreenResolution.Y;
			const int64 Distance = DeltaX * DeltaX + DeltaY * DeltaY;
			if (Distance < BestDistance
				|| (Distance == BestDistance && IsResolutionLess(Resolution, NormalizedResolution)))
			{
				BestDistance = Distance;
				NormalizedResolution = Resolution;
			}
		}
	}

	if (NormalizedResolution == Draft.ScreenResolution)
	{
		return false;
	}
	Draft.ScreenResolution = NormalizedResolution;
	return true;
}

void UWacomSettingsScreen::SelectCategory(
	EWacomSettingsCategory Category,
	bool bMoveFocusToFirstRow)
{
	SelectedCategory = Category;
	if (CategoryTitleText)
	{
		CategoryTitleText->SetText(GetCategoryLabel(SelectedCategory));
	}
	RebuildOptionRows();
	RefreshInteractionState();
	if (bMoveFocusToFirstRow && !ActiveOptionRows.IsEmpty() && ActiveOptionRows[0])
	{
		ActiveOptionRows[0]->SetKeyboardFocus();
	}
}

void UWacomSettingsScreen::HandleOptionStep(EWacomSettingsField Field, int32 Direction)
{
	if (!EditToken.IsValid() || bAwaitingVideoConfirmation || Direction == 0)
	{
		return;
	}
	const FWacomLocalSettingsSnapshot PreviousDraft = Draft;
	const int32 Step = Direction < 0 ? -1 : 1;
	bool bResolutionAdjustedForMode = false;
	if (Field == EWacomSettingsField::ScreenResolution)
	{
		const TArray<FIntPoint>& Resolutions = ScreenResolutionOptions.SelectableResolutions;
		if (!ScreenResolutionOptions.bCanSelectResolution || Resolutions.IsEmpty())
		{
			return;
		}
		const int32 CurrentIndex = Resolutions.IndexOfByKey(Draft.ScreenResolution);
		int32 NextIndex = CurrentIndex;
		if (CurrentIndex != INDEX_NONE)
		{
			NextIndex = FMath::Clamp(CurrentIndex + Step, 0, Resolutions.Num() - 1);
		}
		else if (Step > 0)
		{
			NextIndex = Resolutions.IndexOfByPredicate([this](const FIntPoint Resolution)
			{
				return IsResolutionLess(Draft.ScreenResolution, Resolution);
			});
			if (NextIndex == INDEX_NONE)
			{
				NextIndex = Resolutions.Num() - 1;
			}
		}
		else
		{
			NextIndex = Resolutions.Num() - 1;
			while (NextIndex > 0
				&& !IsResolutionLess(Resolutions[NextIndex], Draft.ScreenResolution))
			{
				--NextIndex;
			}
		}
		Draft.ScreenResolution = Resolutions[NextIndex];
	}
	else if (Field == EWacomSettingsField::WindowMode)
	{
		const TArray<EWindowMode::Type> Modes = {
			EWindowMode::WindowedFullscreen, EWindowMode::Fullscreen, EWindowMode::Windowed };
		const int32 CurrentIndex = FMath::Max(0, Modes.IndexOfByKey(Draft.WindowMode));
		Draft.WindowMode = Modes[FMath::Clamp(CurrentIndex + Step, 0, Modes.Num() - 1)];
		if (Draft.WindowMode != PreviousDraft.WindowMode)
		{
			bResolutionAdjustedForMode = RefreshScreenResolutionOptions(true);
		}
	}
	else if (Field == EWacomSettingsField::VSync)
	{
		Draft.bVSyncEnabled = !Draft.bVSyncEnabled;
	}
	else if (Field == EWacomSettingsField::FrameRateLimit)
	{
		const TArray<float> Values = BuildFrameRateOptions(Draft.FrameRateLimit);
		int32 CurrentIndex = Values.IndexOfByPredicate([this](float Value)
		{
			return FMath::IsNearlyEqual(Value, Draft.FrameRateLimit, 0.1f);
		});
		CurrentIndex = FMath::Max(0, CurrentIndex);
		Draft.FrameRateLimit = Values[FMath::Clamp(CurrentIndex + Step, 0, Values.Num() - 1)];
	}
	else if (Field == EWacomSettingsField::GraphicsQuality)
	{
		Draft.GraphicsQuality = FMath::Clamp(Draft.GraphicsQuality + Step, 0, 3);
	}
	else if (Field == EWacomSettingsField::MasterVolume)
	{
		Draft.MasterVolume = FMath::Clamp(Draft.MasterVolume + Step * 0.05f, 0.0f, 1.0f);
	}
	else if (Field == EWacomSettingsField::MusicVolume)
	{
		Draft.MusicVolume = FMath::Clamp(Draft.MusicVolume + Step * 0.05f, 0.0f, 1.0f);
	}
	else if (Field == EWacomSettingsField::SFXVolume)
	{
		Draft.SFXVolume = FMath::Clamp(Draft.SFXVolume + Step * 0.05f, 0.0f, 1.0f);
	}
	else if (Field == EWacomSettingsField::UISoundVolume)
	{
		Draft.UISoundVolume = FMath::Clamp(Draft.UISoundVolume + Step * 0.05f, 0.0f, 1.0f);
	}
	else if (Field == EWacomSettingsField::LookResponseStrength)
	{
		Draft.LookResponseStrength = FMath::Clamp(Draft.LookResponseStrength + Step * 0.1f, 0.0f, 3.0f);
	}
	else if (Field == EWacomSettingsField::InvertLookY)
	{
		Draft.bInvertLookY = !Draft.bInvertLookY;
	}
	else if (Field == EWacomSettingsField::CameraMotionStrength)
	{
		Draft.CameraMotionStrength = FMath::Clamp(Draft.CameraMotionStrength + Step * 0.05f, 0.0f, 1.0f);
	}
	else if (Field == EWacomSettingsField::FlashEffectMode)
	{
		Draft.FlashEffectMode = static_cast<EWacomFlashEffectMode>(FMath::Clamp(
			static_cast<int32>(Draft.FlashEffectMode) + Step, 0, 2));
	}
	else if (Field == EWacomSettingsField::UIMotionMode)
	{
		Draft.UIMotionMode = static_cast<EWacomUIMotionMode>(FMath::Clamp(
			static_cast<int32>(Draft.UIMotionMode) + Step, 0, 1));
	}
	Draft.Sanitize();
	CommitDraftMutation(Field, PreviousDraft);
	if (bResolutionAdjustedForMode)
	{
		StatusMessage = LOCTEXT(
			"ResolutionAdjustedForMode",
			"已调整为当前窗口模式可用的分辨率。");
		RefreshFromDraft();
	}
}

void UWacomSettingsScreen::HandleOptionNormalizedValue(
	EWacomSettingsField Field,
	float NormalizedValue)
{
	if (!EditToken.IsValid() || bAwaitingVideoConfirmation)
	{
		return;
	}
	const FWacomLocalSettingsSnapshot PreviousDraft = Draft;
	const float Value = FMath::Clamp(NormalizedValue, 0.0f, 1.0f);
	switch (Field)
	{
	case EWacomSettingsField::MasterVolume: Draft.MasterVolume = Value; break;
	case EWacomSettingsField::MusicVolume: Draft.MusicVolume = Value; break;
	case EWacomSettingsField::SFXVolume: Draft.SFXVolume = Value; break;
	case EWacomSettingsField::UISoundVolume: Draft.UISoundVolume = Value; break;
	case EWacomSettingsField::LookResponseStrength: Draft.LookResponseStrength = Value * 3.0f; break;
	case EWacomSettingsField::CameraMotionStrength: Draft.CameraMotionStrength = Value; break;
	default: return;
	}
	Draft.Sanitize();
	CommitDraftMutation(Field, PreviousDraft);
}

void UWacomSettingsScreen::CommitDraftMutation(
	EWacomSettingsField Field,
	const FWacomLocalSettingsSnapshot& PreviousDraft)
{
	if (Draft.IsEquivalentTo(PreviousDraft))
	{
		RefreshFromDraft();
		return;
	}
	if (GetFieldDescriptor(Field).bPreviewable)
	{
		const FWacomSettingsOperationResult Result = SettingsSubsystem
			? SettingsSubsystem->Preview(EditToken, Draft)
			: FWacomSettingsOperationResult::Failure(LOCTEXT("MissingSubsystemPreview", "本地设置服务不可用。"));
		if (!Result.bSucceeded)
		{
			Draft = PreviousDraft;
			SetStatus(Result.FailureReason);
			RefreshFromDraft();
			return;
		}
	}
	StatusMessage = FText::GetEmpty();
	RefreshFromDraft();
}

void UWacomSettingsScreen::HandleApplyClicked()
{
	if (!SettingsSubsystem || !EditToken.IsValid() || bAwaitingVideoConfirmation || !IsDirty())
	{
		return;
	}
	const FWacomSettingsOperationResult Result = SettingsSubsystem->Apply(EditToken, Draft);
	if (!Result.bSucceeded)
	{
		SetStatus(Result.FailureReason);
		RefreshInteractionState();
		return;
	}
	if (!Result.bVideoConfirmationRequired)
	{
		EditToken.Invalidate();
		RestartEditSession(LOCTEXT("Applied", "设置已应用。"));
		return;
	}

	bAwaitingVideoConfirmation = true;
	RefreshInteractionState();
	if (!ShowConfirmationDialog(EWacomSettingsConfirmationMode::VideoMode))
	{
		const FWacomSettingsOperationResult RevertResult = SettingsSubsystem->RevertVideoMode(EditToken);
		if (bAwaitingVideoConfirmation && RevertResult.bSucceeded)
		{
			CompleteVideoConfirmation(EWacomRuntimeSettingsChangeReason::VideoReverted);
		}
		else if (!RevertResult.bSucceeded)
		{
			HandleFatalSessionFailure(RevertResult.FailureReason);
		}
	}
}

void UWacomSettingsScreen::HandleRestoreDefaultsClicked()
{
	if (!SettingsSubsystem || !EditToken.IsValid()
		|| bAwaitingVideoConfirmation || bTearingDown)
	{
		return;
	}

	DefaultSnapshot = SettingsSubsystem->GetDefaultSnapshot();
	if (Draft.IsEquivalentTo(DefaultSnapshot))
	{
		RefreshInteractionState();
		return;
	}

	const FWacomSettingsOperationResult Result = SettingsSubsystem->Preview(
		EditToken, DefaultSnapshot);
	if (!Result.bSucceeded)
	{
		return;
	}

	Draft = DefaultSnapshot;
	RefreshScreenResolutionOptions(true);
	StatusMessage = LOCTEXT(
		"DefaultsLoaded",
		"已载入默认设置，选择应用以保存。");
	RefreshFromDraft();

	if (ApplyButton && ApplyButton->IsInteractionEnabled())
	{
		ApplyButton->SetKeyboardFocus();
	}
	else if (BackButton)
	{
		BackButton->SetKeyboardFocus();
	}
}

void UWacomSettingsScreen::HandleBackClicked()
{
	RequestClose();
}

void UWacomSettingsScreen::RequestClose()
{
	if (bAwaitingVideoConfirmation || bTearingDown)
	{
		return;
	}
	if (IsDirty())
	{
		if (!ShowConfirmationDialog(EWacomSettingsConfirmationMode::DiscardChanges))
		{
			SetStatus(LOCTEXT("DiscardDialogFailed", "无法打开确认框；设置仍保持未应用状态。"));
		}
		return;
	}
	if (SettingsSubsystem && EditToken.IsValid())
	{
		SettingsSubsystem->Cancel(EditToken);
	}
	EditToken.Invalidate();
	DeactivateWidget();
}

bool UWacomSettingsScreen::BeginEditSession()
{
	if (!SettingsSubsystem)
	{
		SettingsSubsystem = GetGameInstance()
			? GetGameInstance()->GetSubsystem<UWacomSettingsSubsystem>()
			: nullptr;
	}
	if (!SettingsSubsystem)
	{
		HandleFatalSessionFailure(LOCTEXT("MissingSettingsSubsystem", "本地设置服务不可用。"));
		return false;
	}
	BindRuntimeSettings();
	const FWacomSettingsEditSession Session = SettingsSubsystem->BeginEdit();
	if (!Session.IsValid())
	{
		HandleFatalSessionFailure(LOCTEXT("OverlappingSettingsEdit", "另一个设置编辑或视频确认仍在进行。"));
		return false;
	}
	EditToken = Session.Token;
	Baseline = Session.Snapshot;
	Draft = Session.Snapshot;
	DefaultSnapshot = SettingsSubsystem->GetDefaultSnapshot();
	RefreshScreenResolutionOptions(false);
	StatusMessage = FText::GetEmpty();
	RebuildOptionRows();
	RefreshFromDraft();
	return true;
}

void UWacomSettingsScreen::RestartEditSession(const FText& SuccessMessage)
{
	if (!SettingsSubsystem || bTearingDown)
	{
		return;
	}
	const FWacomSettingsEditSession Session = SettingsSubsystem->BeginEdit();
	if (!Session.IsValid())
	{
		HandleFatalSessionFailure(LOCTEXT("RestartEditFailed", "设置已处理，但无法继续开启新的编辑事务。"));
		return;
	}
	EditToken = Session.Token;
	Baseline = Session.Snapshot;
	Draft = Session.Snapshot;
	DefaultSnapshot = SettingsSubsystem->GetDefaultSnapshot();
	RefreshScreenResolutionOptions(false);
	StatusMessage = SuccessMessage;
	RebuildOptionRows();
	RefreshFromDraft();
}

void UWacomSettingsScreen::EndOwnedSessionForTeardown()
{
	if (!SettingsSubsystem || !EditToken.IsValid())
	{
		EditToken.Invalidate();
		bAwaitingVideoConfirmation = false;
		return;
	}
	if (bAwaitingVideoConfirmation)
	{
		SettingsSubsystem->RevertVideoMode(EditToken);
	}
	else
	{
		SettingsSubsystem->Cancel(EditToken);
	}
	EditToken.Invalidate();
	bAwaitingVideoConfirmation = false;
}

void UWacomSettingsScreen::BindRuntimeSettings()
{
	if (!SettingsSubsystem || RuntimeSettingsChangedHandle.IsValid())
	{
		return;
	}
	RuntimeSettingsChangedHandle = SettingsSubsystem->OnRuntimeSettingsChangedNative().AddUObject(
		this,
		&UWacomSettingsScreen::HandleRuntimeSettingsChanged);
}

void UWacomSettingsScreen::UnbindRuntimeSettings()
{
	if (SettingsSubsystem && RuntimeSettingsChangedHandle.IsValid())
	{
		SettingsSubsystem->OnRuntimeSettingsChangedNative().Remove(RuntimeSettingsChangedHandle);
	}
	RuntimeSettingsChangedHandle.Reset();
}

void UWacomSettingsScreen::HandleRuntimeSettingsChanged(
	const FWacomLocalSettingsSnapshot& /*Snapshot*/,
	EWacomRuntimeSettingsChangeReason Reason)
{
	if (bTearingDown || !bAwaitingVideoConfirmation)
	{
		return;
	}
	if (Reason == EWacomRuntimeSettingsChangeReason::VideoConfirmed
		|| Reason == EWacomRuntimeSettingsChangeReason::VideoReverted)
	{
		CompleteVideoConfirmation(Reason);
	}
}

bool UWacomSettingsScreen::ShowConfirmationDialog(EWacomSettingsConfirmationMode Mode)
{
	if (ActiveConfirmationDialog.IsValid())
	{
		return false;
	}
	UWacomGameUIManagerSubsystem* UIManager = GetGameInstance()
		? GetGameInstance()->GetSubsystem<UWacomGameUIManagerSubsystem>()
		: nullptr;
	if (!UIManager)
	{
		return false;
	}
	TSubclassOf<UWacomSettingsConfirmationDialog> DialogClass = ConfirmationDialogClass;
	if (!DialogClass)
	{
		DialogClass = UWacomSettingsConfirmationDialog::StaticClass();
	}
	UWacomSettingsConfirmationDialog* Dialog = Cast<UWacomSettingsConfirmationDialog>(
		UIManager->PushContentToLayer(WacomUITags::UI_Layer_Modal.GetTag(), DialogClass));
	if (!Dialog)
	{
		return false;
	}
	ActiveConfirmationMode = Mode;
	ActiveConfirmationDialog = Dialog;
	FWacomSettingsConfirmationDecisionDelegate Callback;
	Callback.BindUObject(this, &UWacomSettingsScreen::HandleConfirmationDecision);
	Dialog->Configure(
		Mode,
		Mode == EWacomSettingsConfirmationMode::VideoMode && SettingsSubsystem
			? SettingsSubsystem->GetRemainingVideoModeConfirmationSeconds()
			: 0.0f,
		MoveTemp(Callback));
	return true;
}

void UWacomSettingsScreen::HandleConfirmationDecision(
	EWacomSettingsConfirmationDecision Decision)
{
	ActiveConfirmationDialog.Reset();
	if (ActiveConfirmationMode == EWacomSettingsConfirmationMode::DiscardChanges)
	{
		if (Decision == EWacomSettingsConfirmationDecision::Confirm)
		{
			if (SettingsSubsystem && EditToken.IsValid())
			{
				SettingsSubsystem->Cancel(EditToken);
			}
			EditToken.Invalidate();
			DeactivateWidget();
		}
		else if (UWidget* FocusTarget = NativeGetDesiredFocusTarget())
		{
			FocusTarget->SetKeyboardFocus();
		}
		return;
	}

	if (!SettingsSubsystem || !EditToken.IsValid() || !bAwaitingVideoConfirmation)
	{
		return;
	}
	const FWacomSettingsOperationResult Result = Decision == EWacomSettingsConfirmationDecision::Confirm
		? SettingsSubsystem->ConfirmVideoMode(EditToken)
		: SettingsSubsystem->RevertVideoMode(EditToken);
	if (!Result.bSucceeded)
	{
		SetStatus(Result.FailureReason);
		if (!SettingsSubsystem->IsVideoModeConfirmationPending() && bAwaitingVideoConfirmation)
		{
			CompleteVideoConfirmation(EWacomRuntimeSettingsChangeReason::VideoReverted);
		}
	}
}

void UWacomSettingsScreen::CompleteVideoConfirmation(EWacomRuntimeSettingsChangeReason Reason)
{
	if (!bAwaitingVideoConfirmation)
	{
		return;
	}
	bAwaitingVideoConfirmation = false;
	EditToken.Invalidate();
	CloseActiveDialogWithoutDecision();
	RestartEditSession(Reason == EWacomRuntimeSettingsChangeReason::VideoConfirmed
		? LOCTEXT("VideoConfirmed", "显示设置已保留。")
		: LOCTEXT("VideoReverted", "显示设置已恢复，其它更改已保存。"));
}

void UWacomSettingsScreen::CloseActiveDialogWithoutDecision()
{
	if (UWacomSettingsConfirmationDialog* Dialog = ActiveConfirmationDialog.Get())
	{
		ActiveConfirmationDialog.Reset();
		Dialog->CloseWithoutDecision();
	}
}

void UWacomSettingsScreen::SetStatus(const FText& Message)
{
	StatusMessage = Message;
	RefreshInteractionState();
}

void UWacomSettingsScreen::HandleFatalSessionFailure(const FText& Message)
{
	EditToken.Invalidate();
	bAwaitingVideoConfirmation = false;
	SetStatus(Message);
	if (bFatalSessionFailureScheduled)
	{
		return;
	}
	bFatalSessionFailureScheduled = true;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimerForNextTick(
			FTimerDelegate::CreateWeakLambda(this, [this]()
			{
				if (IsActivated())
				{
					DeactivateWidget();
				}
			}));
	}
}

bool UWacomSettingsScreen::IsDirty() const
{
	return EditToken.IsValid() && !Draft.IsEquivalentTo(Baseline);
}

#if WITH_AUTOMATION_TESTS
FWacomSettingsScreenAutomationTestView UWacomSettingsScreen::GetAutomationTestViewForTest() const
{
	FWacomSettingsScreenAutomationTestView View;
	View.bHasValidEditSession = EditToken.IsValid();
	View.bDirty = IsDirty();
	View.bAwaitingVideoConfirmation = bAwaitingVideoConfirmation;
	View.bRestoreDefaultsEnabled = RestoreDefaultsButton
		&& RestoreDefaultsButton->IsInteractionEnabled();
	View.SelectedCategory = SelectedCategory;
	View.VisibleOptionCount = ActiveOptionRows.Num();
	View.Token = EditToken;
	View.Baseline = Baseline;
	View.Draft = Draft;
	View.DefaultSnapshot = DefaultSnapshot;
	View.ScreenResolutionOptions = ScreenResolutionOptions;
	return View;
}
#endif

#undef LOCTEXT_NAMESPACE
