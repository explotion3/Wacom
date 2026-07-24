// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/BattlePileDetailsUIBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/FileManager.h"
#include "Engine/Texture2D.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"
#include "UI/Battle/WacomBattleCardPileDetailsStyle.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardView.h"
#include "Materials/MaterialInterface.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr TCHAR AssetRoot[] = TEXT("/Game/Wacom/UI/Battle/PileDetails");
	constexpr TCHAR ScreenAssetName[] = TEXT("WBP_BattleCardPileDetailsScreen");
	constexpr TCHAR EntryAssetName[] = TEXT("WBP_BattleCardPileEntry");
	constexpr TCHAR StyleAssetName[] = TEXT("DA_BattleCardPileDetailsStyle_Default");
	constexpr TCHAR CardViewClassPath[] =
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");
	constexpr TCHAR CardDetailPanelClassPath[] =
		TEXT("/Game/Wacom/UI/Card/WBP_CardDetailPanel.WBP_CardDetailPanel_C");
	constexpr TCHAR SelectionOutlineMaterialPath[] =
		TEXT("/Game/DreamMaterials/UI/MI_WacomBattleCardPileSelectionOutline_Default.MI_WacomBattleCardPileSelectionOutline_Default");
	constexpr TCHAR DrawIconPath[] =
		TEXT("/Game/Wacom/UI/Battle/Textures/T_UI_BattlePile_Draw_Image2_NoPlate.T_UI_BattlePile_Draw_Image2_NoPlate");
	constexpr TCHAR DiscardIconPath[] =
		TEXT("/Game/Wacom/UI/Battle/Textures/T_UI_BattlePile_Discard_Image2_NoPlate.T_UI_BattlePile_Discard_Image2_NoPlate");
	constexpr TCHAR ExhaustIconPath[] =
		TEXT("/Game/Wacom/UI/Battle/Textures/T_UI_BattlePile_Exhaust_Image2_NoPlate.T_UI_BattlePile_Exhaust_Image2_NoPlate");
	constexpr TCHAR ContractMarker[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=7");
	constexpr TCHAR LegacyContractMarkerV6[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=6");
	constexpr TCHAR LegacyContractMarkerV5[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=5");
	constexpr TCHAR LegacyContractMarkerV4[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=4");
	constexpr TCHAR LegacyContractMarkerV3[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=3");
	constexpr TCHAR LegacyContractMarkerV2[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=2");
	constexpr TCHAR LegacyContractMarkerV1[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=1");

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	void RegisterWidgetGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(TEXT("%s:%s"),
			*Blueprint.GetPathName(), *Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(UWidgetBlueprint& Blueprint, UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	bool HasManagedContract(const UWidgetBlueprint& Blueprint)
	{
		return Blueprint.BlueprintDescription.Contains(ContractMarker)
			|| Blueprint.BlueprintDescription.Contains(LegacyContractMarkerV6)
			|| Blueprint.BlueprintDescription.Contains(LegacyContractMarkerV5)
			|| Blueprint.BlueprintDescription.Contains(LegacyContractMarkerV4)
			|| Blueprint.BlueprintDescription.Contains(LegacyContractMarkerV3)
			|| Blueprint.BlueprintDescription.Contains(LegacyContractMarkerV2)
			|| Blueprint.BlueprintDescription.Contains(LegacyContractMarkerV1);
	}

	void RegisterAllWidgetGuids(UWidgetBlueprint& Blueprint)
	{
		TSet<FName> LiveNames;
		Blueprint.WidgetTree->ForEachWidget([&](UWidget* Widget)
		{
			if (Widget)
			{
				LiveNames.Add(Widget->GetFName());
				RegisterWidgetGuid(Blueprint, *Widget);
			}
		});
		for (auto It = Blueprint.WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!LiveNames.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}

	FWidgetBlueprintAsset LoadOrCreateWidgetBlueprint(
		const TCHAR* AssetName,
		UClass* ParentClass,
		bool bAllowCreate)
	{
		FWidgetBlueprintAsset Result;
		Result.PackagePath = MakePackagePath(AssetRoot, AssetName);
		const FString ObjectPath = MakeObjectPath(Result.PackagePath);
		if (UObject* Existing = StaticLoadObject(UObject::StaticClass(), nullptr, *ObjectPath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(Existing);
			if (!Result.Blueprint || !Result.Blueprint->ParentClass
				|| !Result.Blueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[BattlePileDetailsUIBuilder] Incompatible WBP class: %s"),
					*ObjectPath);
				Result.Blueprint = nullptr;
			}
			return Result;
		}
		if (!bAllowCreate)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Missing WBP: %s"), *ObjectPath);
			return Result;
		}
		if (UPackage* Package = FindOrCreatePackage(Result.PackagePath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(FKismetEditorUtilities::CreateBlueprint(
				ParentClass,
				Package,
				AssetName,
				BPTYPE_Normal,
				UWidgetBlueprint::StaticClass(),
				UWidgetBlueprintGeneratedClass::StaticClass()));
			Result.bCreated = Result.Blueprint != nullptr;
		}
		return Result;
	}

	void ResetWidgetBlueprint(UWidgetBlueprint& Blueprint, const TCHAR* Description)
	{
		Blueprint.Modify();
		if (UWidgetTree* PreviousTree = Blueprint.WidgetTree)
		{
			PreviousTree->Rename(nullptr, GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Blueprint.WidgetTree = NewObject<UWidgetTree>(&Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint.Bindings.Reset();
		Blueprint.Animations.Reset();
		Blueprint.WidgetVariableNameToGuidMap.Reset();
		Blueprint.BlueprintDescription = FString(Description) + TEXT("\n") + ContractMarker;
		Blueprint.bCanCallInitializedWithoutPlayerContext = true;
	}

	bool CompileAndSave(UWidgetBlueprint& Blueprint)
	{
		RegisterAllWidgetGuids(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
		FKismetEditorUtilities::CompileBlueprint(&Blueprint);
		if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Compile failed: %s"),
				*Blueprint.GetPathName());
			return false;
		}
		FAssetRegistryModule::AssetCreated(&Blueprint);
		UPackage* Package = Blueprint.GetOutermost();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}

	bool SaveDefaults(UWidgetBlueprint& Blueprint)
	{
		if (!Blueprint.GeneratedClass)
		{
			return false;
		}
		UPackage* Package = Blueprint.GetOutermost();
		Package->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}

	void StyleText(UTextBlock& Text, int32 Size, FLinearColor Color)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = TEXT("Bold");
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.9f));
	}

	UButton* AddTextButton(
		UWidgetBlueprint& Blueprint,
		UHorizontalBox& Parent,
		FName Name,
		const FText& Label)
	{
		UButton* Button = Blueprint.WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), Name);
		MarkWidgetVariable(Blueprint, *Button);
		UTextBlock* Text = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("%s_Label"), *Name.ToString()));
		Text->SetText(Label);
		StyleText(*Text, 16, FLinearColor(0.87f, 0.92f, 1.0f, 1.0f));
		Button->SetContent(Text);
		if (UHorizontalBoxSlot* ButtonSlot = Parent.AddChildToHorizontalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));
		}
		return Button;
	}

	UButton* AddNavigationButton(
		UWidgetBlueprint& Blueprint,
		UVerticalBox& Parent,
		FName ButtonName,
		FName IconName,
		const FText& Label,
		const FText& Tooltip)
	{
		UButton* Button = Blueprint.WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), ButtonName);
		MarkWidgetVariable(Blueprint, *Button);
		Button->SetToolTipText(Tooltip);
		UVerticalBox* Content = Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			*FString::Printf(TEXT("%s_Content"), *ButtonName.ToString()));
		USizeBox* IconSize = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString::Printf(TEXT("%s_Size"), *ButtonName.ToString()));
		IconSize->SetWidthOverride(36.0f);
		IconSize->SetHeightOverride(36.0f);
		UImage* Icon = Blueprint.WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		MarkWidgetVariable(Blueprint, *Icon);
		IconSize->SetContent(Icon);
		if (UVerticalBoxSlot* IconSlot = Content->AddChildToVerticalBox(IconSize))
		{
			IconSlot->SetHorizontalAlignment(HAlign_Center);
		}
		const FString Prefix = ButtonName.ToString().LeftChop(6);
		UTextBlock* LabelText = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("%sLabelText"), *Prefix));
		LabelText->SetText(Label);
		LabelText->SetJustification(ETextJustify::Center);
		StyleText(*LabelText, 14, FLinearColor(0.72f, 0.80f, 0.90f, 1.0f));
		MarkWidgetVariable(Blueprint, *LabelText);
		if (UVerticalBoxSlot* LabelSlot = Content->AddChildToVerticalBox(LabelText))
		{
			LabelSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 0.0f));
			LabelSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		UTextBlock* CountText = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), *FString::Printf(TEXT("%sCountText"), *Prefix));
		CountText->SetText(FText::AsNumber(0));
		CountText->SetJustification(ETextJustify::Center);
		StyleText(*CountText, 12, FLinearColor(0.52f, 0.62f, 0.72f, 1.0f));
		MarkWidgetVariable(Blueprint, *CountText);
		if (UVerticalBoxSlot* CountSlot = Content->AddChildToVerticalBox(CountText))
		{
			CountSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		Button->SetContent(Content);
		if (UVerticalBoxSlot* ButtonSlot = Parent.AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(8.0f, 8.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		}
		return Button;
	}

	bool IsBrushAssigned(const FSlateBrush& Brush)
	{
		return Brush.GetResourceObject() != nullptr
			&& Brush.ImageSize.X > 0.0f
			&& Brush.ImageSize.Y > 0.0f;
	}

	FSlateBrush MakeIconBrush(UTexture2D& Texture)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(&Texture);
		Brush.ImageSize = FVector2D(64.0f, 64.0f);
		Brush.DrawAs = ESlateBrushDrawType::Image;
		return Brush;
	}

	bool BuildEntryBlueprint(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(Blueprint, TEXT("Virtualized Battle pile card entry."));
		USizeBox* EntrySize = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("EntrySizeBox"));
		EntrySize->SetWidthOverride(198.0f);
		EntrySize->SetHeightOverride(274.0f);
		MarkWidgetVariable(Blueprint, *EntrySize);
		Blueprint.WidgetTree->RootWidget = EntrySize;

		UOverlay* EntryOverlay = Blueprint.WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("EntryOverlay"));
		EntrySize->SetContent(EntryOverlay);
		UImage* SelectionOutline = Blueprint.WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("SelectionOutlineImage"));
		SelectionOutline->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, *SelectionOutline);
		if (UOverlaySlot* SelectionSlot = EntryOverlay->AddChildToOverlay(SelectionOutline))
		{
			SelectionSlot->SetPadding(FMargin(8.0f, 10.0f));
			SelectionSlot->SetHorizontalAlignment(HAlign_Fill);
			SelectionSlot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* CardHost = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("CardHost"));
		CardHost->SetWidthOverride(178.0f);
		CardHost->SetHeightOverride(252.0f);
		CardHost->SetVisibility(ESlateVisibility::HitTestInvisible);
		MarkWidgetVariable(Blueprint, *CardHost);
		UScaleBox* CardScaleBox = Blueprint.WidgetTree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("CardScaleBox"));
		CardScaleBox->SetStretch(EStretch::ScaleToFit);
		CardScaleBox->SetStretchDirection(EStretchDirection::DownOnly);
		CardScaleBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		MarkWidgetVariable(Blueprint, *CardScaleBox);
		CardHost->SetContent(CardScaleBox);
		if (UOverlaySlot* CardSlot = EntryOverlay->AddChildToOverlay(CardHost))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Center);
			CardSlot->SetVerticalAlignment(VAlign_Center);
		}
		return CompileAndSave(Blueprint);
	}

	bool BuildScreenBlueprint(UWidgetBlueprint& Blueprint, UClass* EntryClass)
	{
		ResetWidgetBlueprint(Blueprint, TEXT("Battle pile details secondary panel."));
		UCanvasPanel* FullScreen = Blueprint.WidgetTree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("FullScreenOverlay"));
		Blueprint.WidgetTree->RootWidget = FullScreen;

		UButton* Backdrop = Blueprint.WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), TEXT("BackdropButton"));
		Backdrop->SetBackgroundColor(FLinearColor(0.005f, 0.009f, 0.016f, 0.44f));
		PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Backdrop->IsFocusable = false;
		PRAGMA_ENABLE_DEPRECATION_WARNINGS
		MarkWidgetVariable(Blueprint, *Backdrop);
		if (UCanvasPanelSlot* BackdropSlot = FullScreen->AddChildToCanvas(Backdrop))
		{
			BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropSlot->SetOffsets(FMargin(0.0f));
		}

		USizeBox* PanelSize = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("PanelSizeBox"));
		MarkWidgetVariable(Blueprint, *PanelSize);
		if (UCanvasPanelSlot* PanelSlot = FullScreen->AddChildToCanvas(PanelSize))
		{
			PanelSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			PanelSlot->SetOffsets(FMargin(24.0f, 24.0f, -24.0f, -24.0f));
		}

		USizeBox* DetailPanelHost = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DetailPanelHost"));
		DetailPanelHost->SetWidthOverride(360.0f);
		DetailPanelHost->SetHeightOverride(420.0f);
		DetailPanelHost->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, *DetailPanelHost);
		if (UCanvasPanelSlot* DetailSlot = FullScreen->AddChildToCanvas(DetailPanelHost))
		{
			DetailSlot->SetAnchors(FAnchors(0.0f));
			DetailSlot->SetPosition(FVector2D::ZeroVector);
			DetailSlot->SetSize(FVector2D(360.0f, 420.0f));
			DetailSlot->SetZOrder(20);
		}

		UBorder* PanelRoot = Blueprint.WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("PanelRoot"));
		PanelRoot->SetBrushColor(FLinearColor(0.012f, 0.022f, 0.034f, 0.96f));
		PanelRoot->SetPadding(FMargin(0.0f));
		MarkWidgetVariable(Blueprint, *PanelRoot);
		PanelSize->SetContent(PanelRoot);

		UHorizontalBox* SafeRoot = Blueprint.WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("SafeRoot"));
		PanelRoot->SetContent(SafeRoot);

		USizeBox* NavigationRail = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("NavigationRail"));
		NavigationRail->SetWidthOverride(96.0f);
		MarkWidgetVariable(Blueprint, *NavigationRail);
		SafeRoot->AddChildToHorizontalBox(NavigationRail);
		UBorder* NavigationBackground = Blueprint.WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("NavigationBackground"));
		NavigationBackground->SetBrushColor(FLinearColor(0.018f, 0.030f, 0.043f, 0.98f));
		NavigationRail->SetContent(NavigationBackground);
		UVerticalBox* NavigationButtons = Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("NavigationButtons"));
		NavigationBackground->SetContent(NavigationButtons);
		AddNavigationButton(
			Blueprint,
			*NavigationButtons,
			TEXT("DrawTabButton"),
			TEXT("DrawTabIcon"),
			NSLOCTEXT("WacomBattlePileDetails", "DrawNavigationLabel", "抽牌"),
			NSLOCTEXT("WacomBattlePileDetails", "DrawNavigationTooltip", "查看抽牌堆"));
		AddNavigationButton(
			Blueprint,
			*NavigationButtons,
			TEXT("DiscardTabButton"),
			TEXT("DiscardTabIcon"),
			NSLOCTEXT("WacomBattlePileDetails", "DiscardNavigationLabel", "弃牌"),
			NSLOCTEXT("WacomBattlePileDetails", "DiscardNavigationTooltip", "查看弃牌堆与本回合已使用卡牌"));
		AddNavigationButton(
			Blueprint,
			*NavigationButtons,
			TEXT("ExhaustTabButton"),
			TEXT("ExhaustTabIcon"),
			NSLOCTEXT("WacomBattlePileDetails", "ExhaustNavigationLabel", "消耗"),
			NSLOCTEXT("WacomBattlePileDetails", "ExhaustNavigationTooltip", "查看消耗区"));

		UBorder* ContentRoot = Blueprint.WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("ContentRoot"));
		ContentRoot->SetBrushColor(FLinearColor(0.018f, 0.032f, 0.046f, 0.94f));
		ContentRoot->SetPadding(FMargin(24.0f, 16.0f, 18.0f, 18.0f));
		if (UHorizontalBoxSlot* ContentSlot = SafeRoot->AddChildToHorizontalBox(ContentRoot))
		{
			ContentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UVerticalBox* RootBox = Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("ContentColumn"));
		ContentRoot->SetContent(RootBox);

		UHorizontalBox* Header = Blueprint.WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("Header"));
		if (UVerticalBoxSlot* HeaderSlot = RootBox->AddChildToVerticalBox(Header))
		{
			HeaderSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
		UTextBlock* Title = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TitleText"));
		Title->SetText(NSLOCTEXT("WacomBattlePileDetails", "DrawTitle", "抽牌堆 · 0"));
		StyleText(*Title, 30, FLinearColor(0.94f, 0.97f, 1.0f, 1.0f));
		MarkWidgetVariable(Blueprint, *Title);
		Header->AddChildToHorizontalBox(Title);
		USpacer* HeaderSpacer = Blueprint.WidgetTree->ConstructWidget<USpacer>(
			USpacer::StaticClass(), TEXT("HeaderSpacer"));
		if (UHorizontalBoxSlot* SpacerSlot = Header->AddChildToHorizontalBox(HeaderSpacer))
		{
			SpacerSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		AddTextButton(Blueprint, *Header, TEXT("CloseButton"), NSLOCTEXT("WacomBattlePileDetails", "Close", "×"));

		UHorizontalBox* DiscardSections = Blueprint.WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("DiscardSectionRoot"));
		MarkWidgetVariable(Blueprint, *DiscardSections);
		if (UVerticalBoxSlot* DiscardSlot = RootBox->AddChildToVerticalBox(DiscardSections))
		{
			DiscardSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}
		AddTextButton(Blueprint, *DiscardSections, TEXT("DiscardSectionButton"), NSLOCTEXT("WacomBattlePileDetails", "DiscardSection", "弃牌堆"));
		AddTextButton(Blueprint, *DiscardSections, TEXT("PlayedSectionButton"), NSLOCTEXT("WacomBattlePileDetails", "PlayedSection", "本回合已使用"));

		USizeBox* CardGridSize = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("CardGridSizeBox"));
		MarkWidgetVariable(Blueprint, *CardGridSize);
		UWacomBattleCardPileTileView* TileView = Blueprint.WidgetTree->ConstructWidget<UWacomBattleCardPileTileView>(
			UWacomBattleCardPileTileView::StaticClass(), TEXT("VirtualizedCardTileView"));
		TileView->SetEntryWidth(198.0f);
		TileView->SetEntryHeight(274.0f);
		TileView->SetSelectionMode(ESelectionMode::Single);
		TileView->SetScrollbarVisibility(ESlateVisibility::Visible);
		TileView->SetRuntimeEntryWidgetClass(EntryClass);
		MarkWidgetVariable(Blueprint, *TileView);
		CardGridSize->SetContent(TileView);
		if (UVerticalBoxSlot* TileSlot = RootBox->AddChildToVerticalBox(CardGridSize))
		{
			TileSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UTextBlock* Empty = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EmptyText"));
		Empty->SetText(NSLOCTEXT("WacomBattlePileDetails", "DrawEmpty", "抽牌堆为空"));
		StyleText(*Empty, 18, FLinearColor(0.58f, 0.64f, 0.72f, 1.0f));
		Empty->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, *Empty);
		if (UVerticalBoxSlot* EmptySlot = RootBox->AddChildToVerticalBox(Empty))
		{
			EmptySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			EmptySlot->SetHorizontalAlignment(HAlign_Center);
			EmptySlot->SetVerticalAlignment(VAlign_Center);
		}
		return CompileAndSave(Blueprint);
	}

	bool HasWidget(UWidgetBlueprint& Blueprint, FName Name, UClass* Class)
	{
		UWidget* Widget = Blueprint.WidgetTree ? Blueprint.WidgetTree->FindWidget(Name) : nullptr;
		return Widget && Widget->IsA(Class);
	}

	bool ValidateEntry(UWidgetBlueprint& Blueprint)
	{
		const USizeBox* EntrySize = Cast<USizeBox>(
			Blueprint.WidgetTree->FindWidget(TEXT("EntrySizeBox")));
		const USizeBox* CardHost = Cast<USizeBox>(
			Blueprint.WidgetTree->FindWidget(TEXT("CardHost")));
		const UScaleBox* CardScaleBox = Cast<UScaleBox>(
			Blueprint.WidgetTree->FindWidget(TEXT("CardScaleBox")));
		return Blueprint.BlueprintDescription.Contains(ContractMarker)
			&& EntrySize
			&& FMath::IsNearlyEqual(EntrySize->GetWidthOverride(), 198.0f)
			&& FMath::IsNearlyEqual(EntrySize->GetHeightOverride(), 274.0f)
			&& HasWidget(Blueprint, TEXT("SelectionOutlineImage"), UImage::StaticClass())
			&& CardHost
			&& FMath::IsNearlyEqual(CardHost->GetWidthOverride(), 178.0f)
			&& FMath::IsNearlyEqual(CardHost->GetHeightOverride(), 252.0f)
			&& CardHost->GetVisibility() == ESlateVisibility::HitTestInvisible
			&& CardScaleBox
			&& CardScaleBox->GetStretch() == EStretch::ScaleToFit
			&& CardScaleBox->GetStretchDirection() == EStretchDirection::DownOnly
			&& CardScaleBox->GetVisibility() == ESlateVisibility::HitTestInvisible;
	}

	bool ValidateScreen(UWidgetBlueprint& Blueprint)
	{
		static const TPair<FName, UClass*> Required[] = {
			{ TEXT("BackdropButton"), UButton::StaticClass() },
			{ TEXT("PanelSizeBox"), USizeBox::StaticClass() },
			{ TEXT("PanelRoot"), UBorder::StaticClass() },
			{ TEXT("CloseButton"), UButton::StaticClass() },
			{ TEXT("NavigationRail"), USizeBox::StaticClass() },
			{ TEXT("DrawTabButton"), UButton::StaticClass() },
			{ TEXT("DiscardTabButton"), UButton::StaticClass() },
			{ TEXT("ExhaustTabButton"), UButton::StaticClass() },
			{ TEXT("DrawTabIcon"), UImage::StaticClass() },
			{ TEXT("DiscardTabIcon"), UImage::StaticClass() },
			{ TEXT("ExhaustTabIcon"), UImage::StaticClass() },
			{ TEXT("DrawTabLabelText"), UTextBlock::StaticClass() },
			{ TEXT("DiscardTabLabelText"), UTextBlock::StaticClass() },
			{ TEXT("ExhaustTabLabelText"), UTextBlock::StaticClass() },
			{ TEXT("DrawTabCountText"), UTextBlock::StaticClass() },
			{ TEXT("DiscardTabCountText"), UTextBlock::StaticClass() },
			{ TEXT("ExhaustTabCountText"), UTextBlock::StaticClass() },
			{ TEXT("DiscardSectionRoot"), UHorizontalBox::StaticClass() },
			{ TEXT("DetailPanelHost"), USizeBox::StaticClass() },
			{ TEXT("CardGridSizeBox"), USizeBox::StaticClass() },
			{ TEXT("VirtualizedCardTileView"), UWacomBattleCardPileTileView::StaticClass() }
		};
		if (!Blueprint.BlueprintDescription.Contains(ContractMarker))
		{
			return false;
		}
		for (const TPair<FName, UClass*>& Pair : Required)
		{
			if (!HasWidget(Blueprint, Pair.Key, Pair.Value))
			{
				return false;
			}
		}
		const USizeBox* NavigationRail = Cast<USizeBox>(
			Blueprint.WidgetTree->FindWidget(TEXT("NavigationRail")));
		const UWacomBattleCardPileTileView* TileView = Cast<UWacomBattleCardPileTileView>(
			Blueprint.WidgetTree->FindWidget(TEXT("VirtualizedCardTileView")));
		const UButton* DrawButton = Cast<UButton>(
			Blueprint.WidgetTree->FindWidget(TEXT("DrawTabButton")));
		const UButton* DiscardButton = Cast<UButton>(
			Blueprint.WidgetTree->FindWidget(TEXT("DiscardTabButton")));
		const UButton* ExhaustButton = Cast<UButton>(
			Blueprint.WidgetTree->FindWidget(TEXT("ExhaustTabButton")));
		return NavigationRail
			&& FMath::IsNearlyEqual(NavigationRail->GetWidthOverride(), 96.0f)
			&& TileView
			&& FMath::IsNearlyEqual(TileView->GetEntryWidth(), 198.0f)
			&& FMath::IsNearlyEqual(TileView->GetEntryHeight(), 274.0f)
			&& DrawButton && !DrawButton->GetToolTipText().IsEmpty()
			&& DiscardButton && !DiscardButton->GetToolTipText().IsEmpty()
			&& ExhaustButton && !ExhaustButton->GetToolTipText().IsEmpty();
	}

	UWacomBattleCardPileDetailsStyle* LoadOrBuildStyle(
		UClass* EntryClass,
		UClass* CardViewClass,
		UClass* CardDetailPanelClass,
		UMaterialInterface* SelectionOutlineMaterial,
		bool bBuild)
	{
		UTexture2D* DrawIcon = LoadObject<UTexture2D>(nullptr, DrawIconPath);
		UTexture2D* DiscardIcon = LoadObject<UTexture2D>(nullptr, DiscardIconPath);
		UTexture2D* ExhaustIcon = LoadObject<UTexture2D>(nullptr, ExhaustIconPath);
		if (!DrawIcon || !DiscardIcon || !ExhaustIcon)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Missing one or more formal pile navigation icons."));
			return nullptr;
		}
		const FString PackagePath = MakePackagePath(AssetRoot, StyleAssetName);
		const FString ObjectPath = MakeObjectPath(PackagePath);
		UWacomBattleCardPileDetailsStyle* Style = Cast<UWacomBattleCardPileDetailsStyle>(
			StaticLoadObject(UWacomBattleCardPileDetailsStyle::StaticClass(), nullptr, *ObjectPath));
		if (!Style)
		{
			if (!bBuild)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[BattlePileDetailsUIBuilder] Missing style: %s"),
					*ObjectPath);
				return nullptr;
			}
			bool bChanged = false;
			TArray<FString> Errors;
			Style = BuildDataAsset<UWacomBattleCardPileDetailsStyle>(
				PackagePath,
				StyleAssetName,
				[EntryClass, CardViewClass, CardDetailPanelClass, SelectionOutlineMaterial,
					DrawIcon, DiscardIcon, ExhaustIcon](UWacomBattleCardPileDetailsStyle& Expected)
				{
					Expected.EntryWidgetClass = EntryClass;
					Expected.CardViewClass = CardViewClass;
					Expected.CardDetailPanelClass = CardDetailPanelClass;
					Expected.SelectionOutlineMaterialInstance = SelectionOutlineMaterial;
					Expected.DrawPileIconBrush = MakeIconBrush(*DrawIcon);
					Expected.DiscardPileIconBrush = MakeIconBrush(*DiscardIcon);
					Expected.ExhaustPileIconBrush = MakeIconBrush(*ExhaustIcon);
				},
				bChanged,
				Errors);
			for (const FString& Error : Errors)
			{
				UE_LOG(LogTemp, Error, TEXT("[BattlePileDetailsUIBuilder] %s"), *Error);
			}
			if (!Errors.IsEmpty() || !Style)
			{
				return nullptr;
			}
		}

		const bool bMissingClasses = !Style->EntryWidgetClass
			|| !Style->CardViewClass
			|| !Style->CardDetailPanelClass;
		const bool bMissingOutline = !Style->SelectionOutlineMaterialInstance;
		const bool bMissingIcons = !IsBrushAssigned(Style->DrawPileIconBrush)
			|| !IsBrushAssigned(Style->DiscardPileIconBrush)
			|| !IsBrushAssigned(Style->ExhaustPileIconBrush);
		const bool bLayoutMismatch =
			!FMath::IsNearlyEqual(Style->CardWidthPixels, 178.0f)
			|| !FMath::IsNearlyEqual(Style->CardHeightPixels, 252.0f)
			|| !Style->ResponsiveReferenceViewportPixels.Equals(
				FVector2D(1920.0f, 1080.0f),
				KINDA_SMALL_NUMBER)
			|| !FMath::IsNearlyEqual(Style->MinimumCardPhysicalScale, 0.90f)
			|| !FMath::IsNearlyEqual(Style->MaximumCardPhysicalScale, 1.15f)
			|| !FMath::IsNearlyEqual(Style->CardEntryPaddingPixels, 4.0f)
			|| !FMath::IsNearlyEqual(Style->CardHorizontalSpacingPixels, 12.0f)
			|| !FMath::IsNearlyEqual(Style->CardVerticalSpacingPixels, 14.0f)
			|| !FMath::IsNearlyEqual(Style->NavigationRailWidthPixels, 96.0f);
		if (bMissingClasses || bMissingOutline || bMissingIcons || bLayoutMismatch)
		{
			if (!bBuild)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[BattlePileDetailsUIBuilder] Style is missing required fields or does not match the v7 responsive layout contract: %s"),
					*ObjectPath);
				return nullptr;
			}
			Style->Modify();
			if (!Style->EntryWidgetClass) { Style->EntryWidgetClass = EntryClass; }
			if (!Style->CardViewClass) { Style->CardViewClass = CardViewClass; }
			if (!Style->CardDetailPanelClass) { Style->CardDetailPanelClass = CardDetailPanelClass; }
			if (!Style->SelectionOutlineMaterialInstance)
			{
				Style->SelectionOutlineMaterialInstance = SelectionOutlineMaterial;
			}
			if (!IsBrushAssigned(Style->DrawPileIconBrush))
			{
				Style->DrawPileIconBrush = MakeIconBrush(*DrawIcon);
			}
			if (!IsBrushAssigned(Style->DiscardPileIconBrush))
			{
				Style->DiscardPileIconBrush = MakeIconBrush(*DiscardIcon);
			}
			if (!IsBrushAssigned(Style->ExhaustPileIconBrush))
			{
				Style->ExhaustPileIconBrush = MakeIconBrush(*ExhaustIcon);
			}
			Style->CardWidthPixels = 178.0f;
			Style->CardHeightPixels = 252.0f;
			Style->ResponsiveReferenceViewportPixels = FVector2D(1920.0f, 1080.0f);
			Style->MinimumCardPhysicalScale = 0.90f;
			Style->MaximumCardPhysicalScale = 1.15f;
			Style->CardEntryPaddingPixels = 4.0f;
			Style->CardHorizontalSpacingPixels = 12.0f;
			Style->CardVerticalSpacingPixels = 14.0f;
			Style->NavigationRailWidthPixels = 96.0f;
			if (!SaveAssetPackage(Style->GetOutermost(), Style, PackagePath))
			{
				return nullptr;
			}
		}
		return Style;
	}
}

namespace Wacom::ContentBuilder
{
	bool ProcessBattlePileDetailsUI(bool bBuild, bool bInspectOnly)
	{
		if (bBuild == bInspectOnly)
		{
			return false;
		}

		FWidgetBlueprintAsset EntryAsset = LoadOrCreateWidgetBlueprint(
			EntryAssetName, UBattleCardPileEntryWidget::StaticClass(), bBuild);
		if (!EntryAsset.Blueprint)
		{
			return false;
		}
		if (!EntryAsset.bCreated
			&& !HasManagedContract(*EntryAsset.Blueprint))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Unknown manual Entry WBP; no overwrite: %s"),
				*EntryAsset.Blueprint->GetPathName());
			return false;
		}
		if (!ValidateEntry(*EntryAsset.Blueprint))
		{
			if (!bBuild || !BuildEntryBlueprint(*EntryAsset.Blueprint)
				|| !ValidateEntry(*EntryAsset.Blueprint))
			{
				return false;
			}
		}

		UClass* CardViewClass = LoadClass<UWacomCardView>(nullptr, CardViewClassPath);
		if (!CardViewClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Missing formal card WBP class: %s"),
				CardViewClassPath);
			return false;
		}
		UClass* CardDetailPanelClass = LoadClass<UWacomCardDetailPanel>(
			nullptr,
			CardDetailPanelClassPath);
		UMaterialInterface* SelectionOutlineMaterial = LoadObject<UMaterialInterface>(
			nullptr,
			SelectionOutlineMaterialPath);
		if (!CardDetailPanelClass || !SelectionOutlineMaterial)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Missing CardDetailPanel class or selection outline MI."));
			return false;
		}

		UWacomBattleCardPileDetailsStyle* Style = LoadOrBuildStyle(
			EntryAsset.Blueprint->GeneratedClass,
			CardViewClass,
			CardDetailPanelClass,
			SelectionOutlineMaterial,
			bBuild);
		if (!Style)
		{
			return false;
		}

		FWidgetBlueprintAsset ScreenAsset = LoadOrCreateWidgetBlueprint(
			ScreenAssetName, UWacomBattleCardPileDetailsScreen::StaticClass(), bBuild);
		if (!ScreenAsset.Blueprint)
		{
			return false;
		}
		if (!ScreenAsset.bCreated
			&& !HasManagedContract(*ScreenAsset.Blueprint))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[BattlePileDetailsUIBuilder] Unknown manual Screen WBP; no overwrite: %s"),
				*ScreenAsset.Blueprint->GetPathName());
			return false;
		}
		if (!ValidateScreen(*ScreenAsset.Blueprint))
		{
			if (!bBuild || !BuildScreenBlueprint(
					*ScreenAsset.Blueprint,
					EntryAsset.Blueprint->GeneratedClass)
				|| !ValidateScreen(*ScreenAsset.Blueprint))
			{
				return false;
			}
		}

		UWacomBattleCardPileDetailsScreen* Defaults = ScreenAsset.Blueprint->GeneratedClass
			? Cast<UWacomBattleCardPileDetailsScreen>(ScreenAsset.Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Defaults)
		{
			return false;
		}
		const bool bDefaultsValid = Defaults->GetPileDetailsStyle() == Style
			&& Defaults->GetEntryWidgetClass() == EntryAsset.Blueprint->GeneratedClass;
		if (!bDefaultsValid)
		{
			if (!bBuild)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[BattlePileDetailsUIBuilder] Screen class defaults are incomplete."));
				return false;
			}
			Defaults->SetAuthoringDefaults(
				Style,
				TSubclassOf<UBattleCardPileEntryWidget>(EntryAsset.Blueprint->GeneratedClass));
			if (!SaveDefaults(*ScreenAsset.Blueprint))
			{
				return false;
			}
		}

		UE_LOG(LogTemp, Display,
			TEXT("[BattlePileDetailsUIBuilder] %s complete. Entry=%s Screen=%s Style=%s"),
			bBuild ? TEXT("Build") : TEXT("InspectOnly"),
			*EntryAsset.Blueprint->GetPathName(),
			*ScreenAsset.Blueprint->GetPathName(),
			*Style->GetPathName());
		return true;
	}
}
