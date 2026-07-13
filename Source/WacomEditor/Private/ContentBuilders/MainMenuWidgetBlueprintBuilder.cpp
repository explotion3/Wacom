// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/MainMenuWidgetBlueprintBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Menus/WacomMainMenuScreen.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr const TCHAR* MainMenuAssetRoot = TEXT("/Game/Wacom/UI/Menus");
	constexpr const TCHAR* NavButtonAssetName = TEXT("WBP_MainMenuNavButton");
	constexpr const TCHAR* ScreenAssetName = TEXT("WBP_MainMenuScreen");

	const FLinearColor Ink(0.018f, 0.025f, 0.040f, 0.96f);
	const FLinearColor Panel(0.025f, 0.038f, 0.060f, 0.88f);
	const FLinearColor PanelSoft(0.035f, 0.052f, 0.072f, 0.78f);
	const FLinearColor Cyan(0.30f, 0.88f, 0.82f, 1.0f);
	const FLinearColor Amber(0.98f, 0.78f, 0.32f, 1.0f);
	const FLinearColor Paper(0.91f, 0.90f, 0.82f, 1.0f);
	const FLinearColor Muted(0.57f, 0.63f, 0.65f, 1.0f);

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	FWidgetBlueprintAsset LoadOrCreateWidgetBlueprint(
		const TCHAR* AssetName,
		UClass* ParentClass)
	{
		FWidgetBlueprintAsset Result;
		Result.PackagePath = MakePackagePath(MainMenuAssetRoot, AssetName);
		const FString ObjectPath = MakeObjectPath(Result.PackagePath);

		if (UObject* ExistingObject = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(ExistingObject);
			if (!Result.Blueprint)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[MainMenuWidgetBlueprintBuilder] Existing asset is not a Widget Blueprint: %s"),
					*ObjectPath);
				return Result;
			}
			if (!Result.Blueprint->ParentClass
				|| !Result.Blueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[MainMenuWidgetBlueprintBuilder] Existing asset has an incompatible parent: %s"),
					*ObjectPath);
				Result.Blueprint = nullptr;
			}
			return Result;
		}

		UPackage* Package = FindOrCreatePackage(Result.PackagePath);
		if (!Package)
		{
			return Result;
		}

		Result.Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
			ParentClass,
			Package,
			AssetName,
			BPTYPE_Normal,
			UWidgetBlueprint::StaticClass(),
			UWidgetBlueprintGeneratedClass::StaticClass()));
		Result.bCreated = Result.Blueprint != nullptr;
		if (!Result.Blueprint)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[MainMenuWidgetBlueprintBuilder] Failed to create %s"),
				*Result.PackagePath);
		}
		return Result;
	}

	void ResetWidgetBlueprint(UWidgetBlueprint* Blueprint, const FString& Description)
	{
		check(Blueprint);
		Blueprint->Modify();
		if (UWidgetTree* PreviousTree = Blueprint->WidgetTree)
		{
			const FName TransientTreeName = MakeUniqueObjectName(
				GetTransientPackage(),
				UWidgetTree::StaticClass(),
				*FString::Printf(TEXT("%s_PreviousWidgetTree"), *Blueprint->GetName()));
			PreviousTree->Rename(
				*TransientTreeName.ToString(),
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Blueprint->WidgetTree = NewObject<UWidgetTree>(
			Blueprint,
			TEXT("WidgetTree"),
			RF_Transactional);
		Blueprint->Bindings.Reset();
		Blueprint->Animations.Reset();
		Blueprint->WidgetVariableNameToGuidMap.Reset();
		Blueprint->BlueprintDescription = Description;
		Blueprint->bCanCallInitializedWithoutPlayerContext = true;
	}

	void RegisterWidgetGuid(UWidgetBlueprint* Blueprint, const UWidget* Widget)
	{
		check(Blueprint);
		check(Widget);
		const FString StableWidgetPath = FString::Printf(
			TEXT("%s:%s"),
			*Blueprint->GetPathName(),
			*Widget->GetName());
		Blueprint->WidgetVariableNameToGuidMap.FindOrAdd(Widget->GetFName()) =
			FGuid::NewDeterministicGuid(StableWidgetPath);
	}

	void MarkWidgetVariable(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		check(Blueprint);
		check(Widget);
		Widget->bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void StyleText(
		UTextBlock* Text,
		const FText& Value,
		int32 FontSize,
		const FLinearColor& Color,
		FName Typeface = TEXT("Regular"))
	{
		check(Text);
		Text->SetText(Value);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = Typeface;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.8f));
	}

	void StretchCanvasWidget(UCanvasPanelSlot* Slot, const FAnchors& Anchors)
	{
		check(Slot);
		Slot->SetAnchors(Anchors);
		Slot->SetOffsets(FMargin(0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
	}

	bool CompileAndSaveWidgetBlueprint(const FWidgetBlueprintAsset& Asset)
	{
		if (!Asset.Blueprint)
		{
			return false;
		}

		// UE 5.8 要求设计器树里的每个 source widget 都有稳定 GUID，
		// 即使它不勾选 Is Variable；否则编译器会在 GUID 修复阶段触发 ensure。
		TArray<UWidget*> SourceWidgets;
		Asset.Blueprint->WidgetTree->GetAllWidgets(SourceWidgets);
		for (UWidget* Widget : SourceWidgets)
		{
			if (Widget)
			{
				RegisterWidgetGuid(Asset.Blueprint, Widget);
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Asset.Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Asset.Blueprint);
		if (Asset.Blueprint->Status == BS_Error)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[MainMenuWidgetBlueprintBuilder] Blueprint compile failed: %s"),
				*Asset.PackagePath);
			return false;
		}

		if (Asset.bCreated)
		{
			FAssetRegistryModule::AssetCreated(Asset.Blueprint);
		}
		UPackage* Package = Asset.Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		Asset.Blueprint->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset.PackagePath,
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), /*Tree*/true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset.Blueprint, *Filename, Args);
	}

	bool BuildNavButtonBlueprint(FWidgetBlueprintAsset& Asset)
	{
		ResetWidgetBlueprint(
			Asset.Blueprint,
			TEXT("主菜单 CommonUI 导航按钮。视觉状态由 UWacomMainMenuButtonWidget 驱动。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;

		USizeBox* Root = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonSize"));
		Root->SetMinDesiredWidth(420.0f);
		Root->SetMinDesiredHeight(58.0f);
		Tree->RootWidget = Root;

		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ButtonOverlay"));
		Root->AddChild(Overlay);

		UBorder* Backdrop = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ButtonBackdrop"));
		Backdrop->SetBrushColor(Panel);
		Backdrop->SetPadding(FMargin(0.0f));
		MarkWidgetVariable(Asset.Blueprint, Backdrop);
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Backdrop))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* AccentSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ButtonAccentSize"));
		AccentSize->SetWidthOverride(5.0f);
		UBorder* Accent = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ButtonAccent"));
		Accent->SetBrushColor(Cyan);
		AccentSize->AddChild(Accent);
		MarkWidgetVariable(Asset.Blueprint, Accent);
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(AccentSize))
		{
			Slot->SetHorizontalAlignment(HAlign_Left);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UHorizontalBox* Content = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("ButtonContent"));
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Content))
		{
			Slot->SetPadding(FMargin(18.0f, 8.0f, 14.0f, 8.0f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		UCommonTextBlock* Glyph = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonGlyph"));
		StyleText(Glyph, FText::FromString(TEXT(">")), 19, Cyan, TEXT("Bold"));
		MarkWidgetVariable(Asset.Blueprint, Glyph);
		if (UHorizontalBoxSlot* Slot = Content->AddChildToHorizontalBox(Glyph))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 13.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		UCommonTextBlock* ButtonText = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonText"));
		StyleText(ButtonText, FText::FromString(TEXT("MENU ITEM")), 20, Paper, TEXT("Bold"));
		ButtonText->SetJustification(ETextJustify::Left);
		MarkWidgetVariable(Asset.Blueprint, ButtonText);
		if (UHorizontalBoxSlot* Slot = Content->AddChildToHorizontalBox(ButtonText))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		UCommonTextBlock* EndCap = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonEndCap"));
		StyleText(EndCap, FText::FromString(TEXT("[ ]")), 12, Muted);
		if (UHorizontalBoxSlot* Slot = Content->AddChildToHorizontalBox(EndCap))
		{
			Slot->SetPadding(FMargin(12.0f, 0.0f, 0.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* ScanlineSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ButtonScanlineSize"));
		ScanlineSize->SetHeightOverride(1.0f);
		UBorder* Scanline = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ButtonScanline"));
		Scanline->SetBrushColor(FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.20f));
		ScanlineSize->AddChild(Scanline);
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(ScanlineSize))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Bottom);
		}

		return CompileAndSaveWidgetBlueprint(Asset);
	}

	UWacomMainMenuButtonWidget* AddMenuButton(
		UWidgetBlueprint* ScreenBlueprint,
		UVerticalBox* Parent,
		UClass* ButtonClass,
		FName Name,
		const FText& Label)
	{
		UWacomMainMenuButtonWidget* Button =
			ScreenBlueprint->WidgetTree->ConstructWidget<UWacomMainMenuButtonWidget>(
				ButtonClass, Name);
		if (!Button)
		{
			return nullptr;
		}
		UE_LOG(LogTemp, Verbose,
			TEXT("[MainMenuWidgetBlueprintBuilder] Added %s as %s (requested %s)"),
			*Name.ToString(),
			*GetNameSafe(Button->GetClass()),
			*GetNameSafe(ButtonClass));
		Button->SetButtonText(Label);
		MarkWidgetVariable(ScreenBlueprint, Button);
		if (UVerticalBoxSlot* Slot = Parent->AddChildToVerticalBox(Button))
		{
			Slot->SetPadding(FMargin(0.0f, 5.0f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Button;
	}

	bool BuildMainMenuScreenBlueprint(FWidgetBlueprintAsset& Asset, UClass* NavButtonClass)
	{
		if (!NavButtonClass || !NavButtonClass->IsChildOf(UWacomMainMenuButtonWidget::StaticClass()))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[MainMenuWidgetBlueprintBuilder] Nav button class is invalid"));
			return false;
		}

		ResetWidgetBlueprint(
			Asset.Blueprint,
			TEXT("L_MainMenu 正式被动 Screen。只显示 ViewData 并上报 Action。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;

		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;

		UBorder* LeftShade = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("LeftSceneShade"));
		LeftShade->SetBrushColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.76f));
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(LeftShade))
		{
			StretchCanvasWidget(Slot, FAnchors(0.0f, 0.0f, 0.57f, 1.0f));
		}

		UBorder* MidShade = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("MiddleSceneShade"));
		MidShade->SetBrushColor(FLinearColor(Ink.R, Ink.G, Ink.B, 0.28f));
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(MidShade))
		{
			StretchCanvasWidget(Slot, FAnchors(0.57f, 0.0f, 0.63f, 1.0f));
		}

		UBorder* TopRule = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("TopPixelRule"));
		TopRule->SetBrushColor(FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.72f));
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(TopRule))
		{
			Slot->SetAnchors(FAnchors(0.055f, 0.065f, 0.42f, 0.065f));
			Slot->SetOffsets(FMargin(0.0f, 0.0f, 0.0f, 3.0f));
		}

		UTextBlock* BuildLabel = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("BuildLabel"));
		StyleText(BuildLabel, FText::FromString(TEXT("PRE-ALPHA // LOCAL PROFILE")), 11, Muted, TEXT("Bold"));
		BuildLabel->SetJustification(ETextJustify::Right);
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(BuildLabel))
		{
			StretchCanvasWidget(Slot, FAnchors(0.60f, 0.035f, 0.945f, 0.075f));
		}

		UHorizontalBox* ContentRoot = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("MenuContentRoot"));
		MarkWidgetVariable(Asset.Blueprint, ContentRoot);
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(ContentRoot))
		{
			StretchCanvasWidget(Slot, FAnchors(0.055f, 0.105f, 0.945f, 0.865f));
		}

		UVerticalBox* Navigation = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("MainMenuNavigationBox"));
		if (UHorizontalBoxSlot* Slot = ContentRoot->AddChildToHorizontalBox(Navigation))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(0.0f, 0.0f, 28.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UTextBlock* Eyebrow = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("GameEyebrowText"));
		StyleText(Eyebrow, FText::FromString(TEXT("FIRST-PERSON CARD ADVENTURE")), 12, Cyan, TEXT("Bold"));
		if (UVerticalBoxSlot* Slot = Navigation->AddChildToVerticalBox(Eyebrow))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 4.0f));
		}

		UTextBlock* GameTitle = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("GameTitleText"));
		StyleText(GameTitle, FText::FromString(TEXT("WACOM")), 54, Paper, TEXT("Bold"));
		if (UVerticalBoxSlot* Slot = Navigation->AddChildToVerticalBox(GameTitle))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 1.0f));
		}

		UTextBlock* Subtitle = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("GameSubtitleText"));
		StyleText(Subtitle, FText::FromString(TEXT("失落虫巢 · 旅程档案")), 17, Amber, TEXT("Bold"));
		if (UVerticalBoxSlot* Slot = Navigation->AddChildToVerticalBox(Subtitle))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 24.0f));
		}

		USizeBox* BrandRuleSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("BrandRuleSize"));
		BrandRuleSize->SetHeightOverride(3.0f);
		UBorder* BrandRule = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("BrandRule"));
		BrandRule->SetBrushColor(Amber);
		BrandRuleSize->AddChild(BrandRule);
		if (UVerticalBoxSlot* Slot = Navigation->AddChildToVerticalBox(BrandRuleSize))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 120.0f, 20.0f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
		}

		if (!AddMenuButton(Asset.Blueprint, Navigation, NavButtonClass,
			TEXT("ContinueButton"), FText::FromString(TEXT("继续旅程")))
			|| !AddMenuButton(Asset.Blueprint, Navigation, NavButtonClass,
				TEXT("NewJourneyButton"), FText::FromString(TEXT("开始新旅程")))
			|| !AddMenuButton(Asset.Blueprint, Navigation, NavButtonClass,
				TEXT("JourneyHistoryButton"), FText::FromString(TEXT("旅程记录")))
			|| !AddMenuButton(Asset.Blueprint, Navigation, NavButtonClass,
				TEXT("SettingsButton"), FText::FromString(TEXT("设置")))
			|| !AddMenuButton(Asset.Blueprint, Navigation, NavButtonClass,
				TEXT("CreditsButton"), FText::FromString(TEXT("制作人员")))
			|| !AddMenuButton(Asset.Blueprint, Navigation, NavButtonClass,
				TEXT("QuitButton"), FText::FromString(TEXT("退出游戏"))))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[MainMenuWidgetBlueprintBuilder] Failed to add menu buttons"));
			return false;
		}

		USpacer* ColumnSpacer = Tree->ConstructWidget<USpacer>(
			USpacer::StaticClass(), TEXT("NavigationColumnSpacer"));
		if (UVerticalBoxSlot* Slot = Navigation->AddChildToVerticalBox(ColumnSpacer))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* ProfileLabel = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ProfileLabel"));
		StyleText(ProfileLabel, FText::FromString(TEXT("PLAYER PROFILE 01  //  OFFLINE")), 11, Muted, TEXT("Bold"));
		Navigation->AddChildToVerticalBox(ProfileLabel);

		USpacer* ColumnGap = Tree->ConstructWidget<USpacer>(
			USpacer::StaticClass(), TEXT("MainMenuColumnGap"));
		ColumnGap->SetSize(FVector2D(56.0f, 1.0f));
		if (UHorizontalBoxSlot* Slot = ContentRoot->AddChildToHorizontalBox(ColumnGap))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Automatic));
		}

		UBorder* SummaryPanel = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("JourneySummaryPanel"));
		SummaryPanel->SetBrushColor(PanelSoft);
		SummaryPanel->SetPadding(FMargin(30.0f, 28.0f));
		MarkWidgetVariable(Asset.Blueprint, SummaryPanel);
		if (UHorizontalBoxSlot* Slot = ContentRoot->AddChildToHorizontalBox(SummaryPanel))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetPadding(FMargin(28.0f, 26.0f, 0.0f, 26.0f));
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* SummaryContent = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("JourneySummaryContent"));
		SummaryPanel->AddChild(SummaryContent);

		UTextBlock* SummaryEyebrow = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("JourneySummaryEyebrow"));
		StyleText(SummaryEyebrow, FText::FromString(TEXT("JOURNEY ARCHIVE // 00")), 12, Cyan, TEXT("Bold"));
		if (UVerticalBoxSlot* Slot = SummaryContent->AddChildToVerticalBox(SummaryEyebrow))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 14.0f));
		}

		USizeBox* SummaryRuleSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("SummaryRuleSize"));
		SummaryRuleSize->SetHeightOverride(2.0f);
		UBorder* SummaryRule = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("SummaryRule"));
		SummaryRule->SetBrushColor(FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.65f));
		SummaryRuleSize->AddChild(SummaryRule);
		if (UVerticalBoxSlot* Slot = SummaryContent->AddChildToVerticalBox(SummaryRuleSize))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 28.0f));
		}

		UTextBlock* SummaryTitle = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ActiveJourneyTitleText"));
		StyleText(SummaryTitle, FText::FromString(TEXT("准备启程")), 32, Paper, TEXT("Bold"));
		MarkWidgetVariable(Asset.Blueprint, SummaryTitle);
		if (UVerticalBoxSlot* Slot = SummaryContent->AddChildToVerticalBox(SummaryTitle))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}

		UTextBlock* SummaryBody = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ActiveJourneySummaryText"));
		StyleText(SummaryBody, FText::FromString(TEXT("开始一段新的旅程。")), 16, Muted);
		SummaryBody->SetAutoWrapText(true);
		SummaryBody->SetWrapTextAt(480.0f);
		MarkWidgetVariable(Asset.Blueprint, SummaryBody);
		if (UVerticalBoxSlot* Slot = SummaryContent->AddChildToVerticalBox(SummaryBody))
		{
			Slot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 20.0f));
		}

		USpacer* SummarySpacer = Tree->ConstructWidget<USpacer>(
			USpacer::StaticClass(), TEXT("JourneySummarySpacer"));
		if (UVerticalBoxSlot* Slot = SummaryContent->AddChildToVerticalBox(SummarySpacer))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* StatusText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("JourneyStatusText"));
		StyleText(StatusText, FText::FromString(TEXT("● 本地档案离线  /  新旅程可用")), 12, Amber, TEXT("Bold"));
		SummaryContent->AddChildToVerticalBox(StatusText);

		UTextBlock* InputHint = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("MainMenuInputHint"));
		StyleText(InputHint, FText::FromString(TEXT("[ENTER] 确认     [ESC] 返回     [↑↓] 选择")), 12, Muted, TEXT("Bold"));
		if (UCanvasPanelSlot* Slot = Root->AddChildToCanvas(InputHint))
		{
			StretchCanvasWidget(Slot, FAnchors(0.055f, 0.915f, 0.945f, 0.955f));
		}

		return CompileAndSaveWidgetBlueprint(Asset);
	}
}

namespace Wacom::ContentBuilder
{
	bool BuildMainMenuWidgetBlueprintContent()
	{
		FWidgetBlueprintAsset NavButton = LoadOrCreateWidgetBlueprint(
			NavButtonAssetName,
			UWacomMainMenuButtonWidget::StaticClass());
		if (!NavButton.Blueprint || !BuildNavButtonBlueprint(NavButton))
		{
			return false;
		}

		UClass* NavButtonClass = NavButton.Blueprint->GeneratedClass;
		FWidgetBlueprintAsset Screen = LoadOrCreateWidgetBlueprint(
			ScreenAssetName,
			UWacomMainMenuScreen::StaticClass());
		if (!Screen.Blueprint || !BuildMainMenuScreenBlueprint(Screen, NavButtonClass))
		{
			return false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[MainMenuWidgetBlueprintBuilder] Built %s and %s"),
			NavButtonAssetName,
			ScreenAssetName);
		return true;
	}
}
