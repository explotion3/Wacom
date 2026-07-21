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
	constexpr TCHAR ContractMarker[] = TEXT("WacomBattlePileDetailsWBP.ContractVersion=5");
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
		FName IconName)
	{
		UButton* Button = Blueprint.WidgetTree->ConstructWidget<UButton>(
			UButton::StaticClass(), ButtonName);
		MarkWidgetVariable(Blueprint, *Button);
		USizeBox* IconSize = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), *FString::Printf(TEXT("%s_Size"), *ButtonName.ToString()));
		IconSize->SetWidthOverride(72.0f);
		IconSize->SetHeightOverride(72.0f);
		UImage* Icon = Blueprint.WidgetTree->ConstructWidget<UImage>(UImage::StaticClass(), IconName);
		MarkWidgetVariable(Blueprint, *Icon);
		IconSize->SetContent(Icon);
		Button->SetContent(IconSize);
		if (UVerticalBoxSlot* ButtonSlot = Parent.AddChildToVerticalBox(Button))
		{
			ButtonSlot->SetPadding(FMargin(20.0f, 12.0f));
			ButtonSlot->SetHorizontalAlignment(HAlign_Center);
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
		EntrySize->SetWidthOverride(320.0f);
		EntrySize->SetHeightOverride(448.0f);
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
		CardHost->SetWidthOverride(296.0f);
		CardHost->SetHeightOverride(420.0f);
		MarkWidgetVariable(Blueprint, *CardHost);
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
		NavigationRail->SetWidthOverride(128.0f);
		MarkWidgetVariable(Blueprint, *NavigationRail);
		SafeRoot->AddChildToHorizontalBox(NavigationRail);
		UBorder* NavigationBackground = Blueprint.WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("NavigationBackground"));
		NavigationBackground->SetBrushColor(FLinearColor(0.018f, 0.030f, 0.043f, 0.98f));
		NavigationRail->SetContent(NavigationBackground);
		UVerticalBox* NavigationButtons = Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("NavigationButtons"));
		NavigationBackground->SetContent(NavigationButtons);
		AddNavigationButton(Blueprint, *NavigationButtons, TEXT("DrawTabButton"), TEXT("DrawTabIcon"));
		AddNavigationButton(Blueprint, *NavigationButtons, TEXT("DiscardTabButton"), TEXT("DiscardTabIcon"));
		AddNavigationButton(Blueprint, *NavigationButtons, TEXT("ExhaustTabButton"), TEXT("ExhaustTabIcon"));

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
		Title->SetText(NSLOCTEXT("WacomBattlePileDetails", "DrawTitle", "抽牌堆 0"));
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
		TileView->SetEntryWidth(320.0f);
		TileView->SetEntryHeight(448.0f);
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
		Empty->SetText(NSLOCTEXT("WacomBattlePileDetails", "Empty", "这里还没有卡牌"));
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
		return Blueprint.BlueprintDescription.Contains(ContractMarker)
			&& HasWidget(Blueprint, TEXT("EntrySizeBox"), USizeBox::StaticClass())
			&& HasWidget(Blueprint, TEXT("SelectionOutlineImage"), UImage::StaticClass())
			&& HasWidget(Blueprint, TEXT("CardHost"), USizeBox::StaticClass());
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
		return true;
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
		if (bMissingClasses || bMissingOutline || bMissingIcons)
		{
			if (!bBuild)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[BattlePileDetailsUIBuilder] Style is missing required classes, outline material, or navigation icons: %s"),
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
