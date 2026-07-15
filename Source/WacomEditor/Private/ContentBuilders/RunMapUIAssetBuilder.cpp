// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunMapUIAssetBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonTextBlock.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/PanelWidget.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Map/WacomRunMapEdgeLayerWidget.h"
#include "UI/Map/WacomRunMapNodeWidget.h"
#include "UI/Map/WacomRunMapScreen.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr const TCHAR* AssetRoot = TEXT("/Game/Wacom/UI/Map");
	constexpr const TCHAR* NodeAssetName = TEXT("WBP_RunMapNode");
	constexpr const TCHAR* ScreenAssetName = TEXT("WBP_RunMapScreen");

	const FLinearColor Ink(0.010f, 0.018f, 0.030f, 0.985f);
	const FLinearColor Panel(0.025f, 0.045f, 0.067f, 0.97f);
	const FLinearColor PanelSoft(0.035f, 0.060f, 0.082f, 0.94f);
	const FLinearColor Cyan(0.24f, 0.90f, 0.84f, 1.0f);
	const FLinearColor Amber(1.0f, 0.70f, 0.22f, 1.0f);
	const FLinearColor Paper(0.91f, 0.91f, 0.84f, 1.0f);
	const FLinearColor Muted(0.52f, 0.62f, 0.66f, 1.0f);

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	FWidgetBlueprintAsset LoadOrCreate(const TCHAR* AssetName, UClass* ParentClass)
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
					TEXT("[RunMapUIAssetBuilder] Incompatible asset: %s"), *ObjectPath);
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
		return Result;
	}

	void Reset(UWidgetBlueprint& Blueprint, const TCHAR* Description)
	{
		Blueprint.Modify();
		if (Blueprint.WidgetTree)
		{
			Blueprint.WidgetTree->Rename(
				*MakeUniqueObjectName(
					GetTransientPackage(), UWidgetTree::StaticClass(), TEXT("PreviousRunMapTree")).ToString(),
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Blueprint.WidgetTree = NewObject<UWidgetTree>(
			&Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint.Bindings.Reset();
		Blueprint.Animations.Reset();
		Blueprint.WidgetVariableNameToGuidMap.Reset();
		Blueprint.BlueprintDescription = Description;
		Blueprint.bCanCallInitializedWithoutPlayerContext = true;
	}

	void RegisterGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"), *Blueprint.GetPathName(), *Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkVariable(UWidgetBlueprint& Blueprint, UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterGuid(Blueprint, Widget);
	}

	void StyleText(UCommonTextBlock& Text, const FText& Value, int32 Size, const FLinearColor& Color)
	{
		Text.SetText(Value);
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = Size;
		Font.TypefaceFontName = TEXT("Bold");
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	}

	bool CompileAndSave(const FWidgetBlueprintAsset& Asset, const bool bCompile = true)
	{
		if (!Asset.Blueprint || !Asset.Blueprint->WidgetTree)
		{
			return false;
		}
		if (bCompile)
		{
			TArray<UWidget*> Widgets;
			Asset.Blueprint->WidgetTree->GetAllWidgets(Widgets);
			for (UWidget* Widget : Widgets)
			{
				if (Widget)
				{
					RegisterGuid(*Asset.Blueprint, *Widget);
				}
			}
			FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Asset.Blueprint);
			FKismetEditorUtilities::CompileBlueprint(Asset.Blueprint);
			if (Asset.Blueprint->Status == BS_Error)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunMapUIAssetBuilder] Compile failed: %s"), *Asset.PackagePath);
				return false;
			}
		}
		if (Asset.bCreated)
		{
			FAssetRegistryModule::AssetCreated(Asset.Blueprint);
		}
		UPackage* Package = Asset.Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		Asset.Blueprint->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset.PackagePath, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Asset.Blueprint, *Filename, Args);
	}

	bool BuildNode(FWidgetBlueprintAsset& Asset)
	{
		Reset(*Asset.Blueprint, TEXT("Run 地图动态节点。只消费 ViewData 并上报稳定 NodeHandle。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;
		USizeBox* Root = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("NodeSize"));
		Root->SetWidthOverride(180.0f);
		Root->SetHeightOverride(72.0f);
		Tree->RootWidget = Root;

		UBorder* Backdrop = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ButtonBackdrop"));
		Backdrop->SetBrushColor(PanelSoft);
		Backdrop->SetPadding(FMargin(8.0f, 6.0f));
		MarkVariable(*Asset.Blueprint, *Backdrop);
		Root->AddChild(Backdrop);

		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("NodeRow"));
		Backdrop->AddChild(Row);
		USizeBox* MarkerSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("MarkerSize"));
		MarkerSize->SetWidthOverride(6.0f);
		UBorder* Marker = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("NodeSemanticMarker"));
		Marker->SetBrushColor(Cyan);
		MarkVariable(*Asset.Blueprint, *Marker);
		MarkerSize->AddChild(Marker);
		if (UHorizontalBoxSlot* MarkerSlot = Row->AddChildToHorizontalBox(MarkerSize))
		{
			MarkerSlot->SetPadding(FMargin(0.0f, 0.0f, 9.0f, 0.0f));
		}

		UVerticalBox* Labels = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Labels"));
		if (UHorizontalBoxSlot* LabelsSlot = Row->AddChildToHorizontalBox(Labels))
		{
			LabelsSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelsSlot->SetVerticalAlignment(VAlign_Center);
		}
		UCommonTextBlock* Title = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonText"));
		StyleText(*Title, FText::FromString(TEXT("节点")), 15, Paper);
		MarkVariable(*Asset.Blueprint, *Title);
		Labels->AddChildToVerticalBox(Title);
		UCommonTextBlock* Type = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("NodeTypeText"));
		StyleText(*Type, FText::FromString(TEXT("未知")), 10, Muted);
		MarkVariable(*Asset.Blueprint, *Type);
		Labels->AddChildToVerticalBox(Type);
		return CompileAndSave(Asset);
	}

	UWacomMenuButtonWidget* AddButton(
		UWidgetBlueprint& Blueprint,
		UPanelWidget& Parent,
		FName Name,
		const FText& Label)
	{
		UWacomMenuButtonWidget* Button = Blueprint.WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			UWacomMenuButtonWidget::StaticClass(), Name);
		Button->SetButtonText(Label);
		MarkVariable(Blueprint, *Button);
		Parent.AddChild(Button);
		return Button;
	}

	bool BuildScreen(FWidgetBlueprintAsset& Asset)
	{
		Reset(*Asset.Blueprint, TEXT("当前 Floor 的像素风被动地图 Screen。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;
		UBorder* Root = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Root"));
		Root->SetBrushColor(Ink);
		Root->SetPadding(FMargin(30.0f, 24.0f));
		Tree->RootWidget = Root;
		UVerticalBox* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Page"));
		Root->AddChild(Page);

		UCommonTextBlock* FloorTitle = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("FloorTitleText"));
		StyleText(*FloorTitle, FText::FromString(TEXT("当前区域 // MAP")), 30, Amber);
		MarkVariable(*Asset.Blueprint, *FloorTitle);
		Page->AddChildToVerticalBox(FloorTitle);

		UHorizontalBox* Body = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Body"));
		if (UVerticalBoxSlot* BodySlot = Page->AddChildToVerticalBox(Body))
		{
			BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			BodySlot->SetPadding(FMargin(0.0f, 16.0f));
		}

		UScaleBox* Scale = Tree->ConstructWidget<UScaleBox>(UScaleBox::StaticClass(), TEXT("MapViewportScaleBox"));
		Scale->SetStretch(EStretch::ScaleToFit);
		MarkVariable(*Asset.Blueprint, *Scale);
		if (UHorizontalBoxSlot* ScaleSlot = Body->AddChildToHorizontalBox(Scale))
		{
			ScaleSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			ScaleSlot->SetPadding(FMargin(0.0f, 0.0f, 22.0f, 0.0f));
		}
		USizeBox* DesignSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DesignSize"));
		DesignSize->SetWidthOverride(1920.0f);
		DesignSize->SetHeightOverride(1080.0f);
		Scale->AddChild(DesignSize);
		UOverlay* MapOverlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("MapOverlay"));
		DesignSize->AddChild(MapOverlay);
		UWacomRunMapEdgeLayerWidget* Edges = Tree->ConstructWidget<UWacomRunMapEdgeLayerWidget>(
			UWacomRunMapEdgeLayerWidget::StaticClass(), TEXT("EdgeLayer"));
		MarkVariable(*Asset.Blueprint, *Edges);
		MapOverlay->AddChild(Edges);
		UCanvasPanel* Canvas = Tree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("MapCanvas"));
		MarkVariable(*Asset.Blueprint, *Canvas);
		MapOverlay->AddChild(Canvas);

		USizeBox* DetailSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DetailSize"));
		DetailSize->SetWidthOverride(360.0f);
		Body->AddChildToHorizontalBox(DetailSize);
		UBorder* DetailPanel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DetailPanel"));
		DetailPanel->SetBrushColor(Panel);
		DetailPanel->SetPadding(FMargin(18.0f));
		DetailSize->AddChild(DetailPanel);
		UVerticalBox* Detail = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Detail"));
		DetailPanel->AddChild(Detail);

		UCommonTextBlock* NodeTitle = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("SelectedNodeTitleText"));
		StyleText(*NodeTitle, FText::FromString(TEXT("节点")), 24, Cyan);
		MarkVariable(*Asset.Blueprint, *NodeTitle);
		Detail->AddChildToVerticalBox(NodeTitle);
		UCommonTextBlock* Description = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("SelectedNodeDescriptionText"));
		StyleText(*Description, FText::GetEmpty(), 14, Paper);
		Description->SetAutoWrapText(true);
		MarkVariable(*Asset.Blueprint, *Description);
		if (UVerticalBoxSlot* DescriptionSlot = Detail->AddChildToVerticalBox(Description))
		{
			DescriptionSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			DescriptionSlot->SetPadding(FMargin(0.0f, 14.0f));
		}
		UCommonTextBlock* Status = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("StatusText"));
		StyleText(*Status, FText::GetEmpty(), 12, Muted);
		Status->SetAutoWrapText(true);
		MarkVariable(*Asset.Blueprint, *Status);
		Detail->AddChildToVerticalBox(Status);
		AddButton(*Asset.Blueprint, *Detail, TEXT("TravelButton"), FText::FromString(TEXT("传送")));
		AddButton(*Asset.Blueprint, *Detail, TEXT("CloseButton"), FText::FromString(TEXT("关闭")));
		return CompileAndSave(Asset);
	}

	bool SetNodeWidgetClass(UWidgetBlueprint& Blueprint, UClass* NodeClass)
	{
		UWacomRunMapScreen* CDO = Blueprint.GeneratedClass
			? Cast<UWacomRunMapScreen>(Blueprint.GeneratedClass->GetDefaultObject())
			: nullptr;
		FClassProperty* Property = CDO
			? FindFProperty<FClassProperty>(CDO->GetClass(), TEXT("NodeWidgetClass"))
			: nullptr;
		if (!CDO || !Property || !NodeClass)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(CDO, NodeClass);
		CDO->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsModified(&Blueprint);
		return true;
	}
}

namespace Wacom::ContentBuilder
{
	bool BuildRunMapUIAssets()
	{
		FWidgetBlueprintAsset Node = LoadOrCreate(
			NodeAssetName, UWacomRunMapNodeWidget::StaticClass());
		if (!Node.Blueprint || !BuildNode(Node) || !Node.Blueprint->GeneratedClass)
		{
			return false;
		}

		FWidgetBlueprintAsset Screen = LoadOrCreate(
			ScreenAssetName, UWacomRunMapScreen::StaticClass());
		if (!Screen.Blueprint || !BuildScreen(Screen)
			|| !SetNodeWidgetClass(*Screen.Blueprint, Node.Blueprint->GeneratedClass)
			|| !CompileAndSave(Screen, false))
		{
			return false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[RunMapUIAssetBuilder] Built WBP_RunMapNode and WBP_RunMapScreen"));
		return true;
	}
}
