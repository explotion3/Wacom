// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SettingsWidgetBlueprintBuilder.h"

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
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/Spacer.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Blueprint.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Foundation/WacomMenuButtonWidget.h"
#include "UI/Settings/WacomSettingsConfirmationDialog.h"
#include "UI/Settings/WacomSettingsOptionRow.h"
#include "UI/Settings/WacomSettingsScreen.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr const TCHAR* AssetRoot = TEXT("/Game/Wacom/UI/Settings");
	constexpr const TCHAR* ButtonAssetName = TEXT("WBP_SettingsButton");
	constexpr const TCHAR* RowAssetName = TEXT("WBP_SettingsOptionRow");
	constexpr const TCHAR* DialogAssetName = TEXT("WBP_SettingsConfirmationDialog");
	constexpr const TCHAR* ScreenAssetName = TEXT("WBP_SettingsScreen");

	const FLinearColor Ink(0.012f, 0.020f, 0.032f, 0.985f);
	const FLinearColor Panel(0.025f, 0.045f, 0.067f, 0.96f);
	const FLinearColor PanelSoft(0.035f, 0.060f, 0.082f, 0.90f);
	const FLinearColor Cyan(0.25f, 0.90f, 0.84f, 1.0f);
	const FLinearColor Amber(1.0f, 0.72f, 0.25f, 1.0f);
	const FLinearColor Paper(0.91f, 0.91f, 0.84f, 1.0f);
	const FLinearColor Muted(0.54f, 0.64f, 0.66f, 1.0f);

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
					TEXT("[SettingsWidgetBlueprintBuilder] Incompatible existing asset: %s"),
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
		return Result;
	}

	void Reset(UWidgetBlueprint& Blueprint, const TCHAR* Description)
	{
		Blueprint.Modify();
		if (Blueprint.WidgetTree)
		{
			Blueprint.WidgetTree->Rename(
				*MakeUniqueObjectName(GetTransientPackage(), UWidgetTree::StaticClass(), TEXT("PreviousSettingsTree")).ToString(),
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Blueprint.WidgetTree = NewObject<UWidgetTree>(&Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint.Bindings.Reset();
		Blueprint.Animations.Reset();
		Blueprint.WidgetVariableNameToGuidMap.Reset();
		Blueprint.BlueprintDescription = Description;
		Blueprint.bCanCallInitializedWithoutPlayerContext = true;
	}

	void RegisterGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(TEXT("%s:%s"),
			*Blueprint.GetPathName(), *Widget.GetName());
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

	void Stretch(UCanvasPanelSlot& CanvasSlot, const FAnchors& Anchors)
	{
		CanvasSlot.SetAnchors(Anchors);
		CanvasSlot.SetOffsets(FMargin(0.0f));
		CanvasSlot.SetAlignment(FVector2D::ZeroVector);
	}

	bool SaveBlueprint(const FWidgetBlueprintAsset& Asset, bool bCompile)
	{
		if (!Asset.Blueprint)
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
					TEXT("[SettingsWidgetBlueprintBuilder] Compile failed: %s"),
					*Asset.PackagePath);
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

	UWacomMenuButtonWidget* AddButton(
		UWidgetBlueprint& Blueprint,
		UClass* ButtonClass,
		UPanelWidget& Parent,
		FName Name,
		const FText& Label)
	{
		UWacomMenuButtonWidget* Button = Blueprint.WidgetTree->ConstructWidget<UWacomMenuButtonWidget>(
			ButtonClass, Name);
		if (!Button)
		{
			return nullptr;
		}
		Button->SetButtonText(Label);
		MarkVariable(Blueprint, *Button);
		Parent.AddChild(Button);
		return Button;
	}

	bool BuildButton(FWidgetBlueprintAsset& Asset)
	{
		Reset(*Asset.Blueprint, TEXT("设置页像素风 CommonUI 按钮。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;
		USizeBox* Root = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ButtonSize"));
		Root->SetMinDesiredWidth(150.0f);
		Root->SetMinDesiredHeight(42.0f);
		Tree->RootWidget = Root;
		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("ButtonOverlay"));
		Root->AddChild(Overlay);
		UBorder* Backdrop = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ButtonBackdrop"));
		Backdrop->SetBrushColor(PanelSoft);
		Overlay->AddChildToOverlay(Backdrop);
		USizeBox* AccentSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("AccentSize"));
		AccentSize->SetWidthOverride(4.0f);
		UBorder* Accent = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Accent"));
		Accent->SetBrushColor(Amber);
		AccentSize->AddChild(Accent);
		if (UOverlaySlot* AccentSlot = Overlay->AddChildToOverlay(AccentSize))
		{
			AccentSlot->SetHorizontalAlignment(HAlign_Left);
			AccentSlot->SetVerticalAlignment(VAlign_Fill);
		}
		UCommonTextBlock* Text = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ButtonText"));
		StyleText(*Text, FText::FromString(TEXT("OPTION")), 16, Paper);
		Text->SetJustification(ETextJustify::Center);
		MarkVariable(*Asset.Blueprint, *Text);
		if (UOverlaySlot* TextSlot = Overlay->AddChildToOverlay(Text))
		{
			TextSlot->SetPadding(FMargin(14.0f, 7.0f));
			TextSlot->SetHorizontalAlignment(HAlign_Fill);
			TextSlot->SetVerticalAlignment(VAlign_Center);
		}
		return SaveBlueprint(Asset, true);
	}

	bool BuildRow(FWidgetBlueprintAsset& Asset, UClass* ButtonClass)
	{
		Reset(*Asset.Blueprint, TEXT("设置字段复用行。只上报步进与 Slider 输入。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;
		UBorder* Root = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("RowBackdrop"));
		Root->SetBrushColor(PanelSoft);
		Root->SetPadding(FMargin(14.0f, 8.0f));
		Tree->RootWidget = Root;
		UHorizontalBox* Row = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Row"));
		Root->AddChild(Row);

		UCommonTextBlock* Label = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("LabelText"));
		StyleText(*Label, FText::FromString(TEXT("SETTING")), 16, Paper);
		MarkVariable(*Asset.Blueprint, *Label);
		if (UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label))
		{
			LabelSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			LabelSlot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* DecreaseSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DecreaseSize"));
		DecreaseSize->SetWidthOverride(52.0f);
		UWacomMenuButtonWidget* Decrease = AddButton(
			*Asset.Blueprint, ButtonClass, *DecreaseSize, TEXT("DecreaseButton"), FText::FromString(TEXT("<")));
		if (UHorizontalBoxSlot* DecreaseSlot = Row->AddChildToHorizontalBox(DecreaseSize))
		{
			DecreaseSlot->SetVerticalAlignment(VAlign_Center);
		}
		USlider* Slider = Tree->ConstructWidget<USlider>(USlider::StaticClass(), TEXT("ValueSlider"));
		Slider->SetMinValue(0.0f);
		Slider->SetMaxValue(1.0f);
		Slider->SetValue(0.5f);
		MarkVariable(*Asset.Blueprint, *Slider);
		if (UHorizontalBoxSlot* SliderSlot = Row->AddChildToHorizontalBox(Slider))
		{
			SliderSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			SliderSlot->SetPadding(FMargin(10.0f, 0.0f));
			SliderSlot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* ValueSize = Tree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("ValueSize"));
		ValueSize->SetWidthOverride(155.0f);
		UCommonTextBlock* Value = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("ValueText"));
		StyleText(*Value, FText::FromString(TEXT("100%")), 16, Cyan);
		Value->SetJustification(ETextJustify::Center);
		MarkVariable(*Asset.Blueprint, *Value);
		ValueSize->AddChild(Value);
		if (UHorizontalBoxSlot* ValueSlot = Row->AddChildToHorizontalBox(ValueSize))
		{
			ValueSlot->SetPadding(FMargin(8.0f, 0.0f));
			ValueSlot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* IncreaseSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("IncreaseSize"));
		IncreaseSize->SetWidthOverride(52.0f);
		UWacomMenuButtonWidget* Increase = AddButton(
			*Asset.Blueprint, ButtonClass, *IncreaseSize, TEXT("IncreaseButton"), FText::FromString(TEXT(">")));
		if (UHorizontalBoxSlot* IncreaseSlot = Row->AddChildToHorizontalBox(IncreaseSize))
		{
			IncreaseSlot->SetVerticalAlignment(VAlign_Center);
		}
		return Decrease && Increase && SaveBlueprint(Asset, true);
	}

	bool BuildDialog(FWidgetBlueprintAsset& Asset, UClass* ButtonClass)
	{
		Reset(*Asset.Blueprint, TEXT("设置页专用放弃修改 / 视频模式确认 Modal。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;
		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;
		UBorder* Shade = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("ModalShade"));
		Shade->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.70f));
		if (UCanvasPanelSlot* ShadeSlot = Root->AddChildToCanvas(Shade))
		{
			Stretch(*ShadeSlot, FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		}
		UBorder* PanelBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("DialogPanel"));
		PanelBorder->SetBrushColor(Panel);
		PanelBorder->SetPadding(FMargin(28.0f, 24.0f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBorder))
		{
			PanelSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelSlot->SetOffsets(FMargin(-300.0f, -140.0f, 600.0f, 280.0f));
		}
		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("DialogColumn"));
		PanelBorder->AddChild(Column);
		UCommonTextBlock* Title = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("TitleText"));
		StyleText(*Title, FText::FromString(TEXT("确认设置")), 28, Amber);
		MarkVariable(*Asset.Blueprint, *Title);
		Column->AddChildToVerticalBox(Title);
		UCommonTextBlock* Message = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("MessageText"));
		StyleText(*Message, FText::FromString(TEXT("是否保留本次修改？")), 16, Paper);
		Message->SetAutoWrapText(true);
		MarkVariable(*Asset.Blueprint, *Message);
		if (UVerticalBoxSlot* MessageSlot = Column->AddChildToVerticalBox(Message))
		{
			MessageSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 22.0f));
			MessageSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UHorizontalBox* Buttons = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Buttons"));
		if (UVerticalBoxSlot* ButtonRowSlot = Column->AddChildToVerticalBox(Buttons))
		{
			ButtonRowSlot->SetHorizontalAlignment(HAlign_Center);
		}
		return AddButton(*Asset.Blueprint, ButtonClass, *Buttons, TEXT("ConfirmButton"), FText::FromString(TEXT("保留设置")))
			&& AddButton(*Asset.Blueprint, ButtonClass, *Buttons, TEXT("CancelButton"), FText::FromString(TEXT("恢复")))
			&& SaveBlueprint(Asset, true);
	}

	bool BuildScreen(FWidgetBlueprintAsset& Asset, UClass* ButtonClass)
	{
		Reset(*Asset.Blueprint, TEXT("五分类本地设置页。持有 token 事务并协调确认 Modal。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;
		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;
		UBorder* Shade = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SceneShade"));
		Shade->SetBrushColor(Ink);
		if (UCanvasPanelSlot* ShadeSlot = Root->AddChildToCanvas(Shade))
		{
			Stretch(*ShadeSlot, FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
		}
		UBorder* PanelBorder = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("SettingsPanel"));
		PanelBorder->SetBrushColor(Panel);
		PanelBorder->SetPadding(FMargin(34.0f, 28.0f));
		if (UCanvasPanelSlot* PanelSlot = Root->AddChildToCanvas(PanelBorder))
		{
			Stretch(*PanelSlot, FAnchors(0.055f, 0.065f, 0.945f, 0.935f));
		}
		UVerticalBox* Page = Tree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass(), TEXT("Page"));
		PanelBorder->AddChild(Page);
		UCommonTextBlock* Title = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("SettingsTitle"));
		StyleText(*Title, FText::FromString(TEXT("设置 // SETTINGS")), 34, Paper);
		if (UVerticalBoxSlot* TitleSlot = Page->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 18.0f));
		}
		UHorizontalBox* Body = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Body"));
		if (UVerticalBoxSlot* BodySlot = Page->AddChildToVerticalBox(Body))
		{
			BodySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UBorder* CategoryPanel = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("CategoryPanel"));
		CategoryPanel->SetBrushColor(PanelSoft);
		CategoryPanel->SetPadding(FMargin(12.0f));
		if (UHorizontalBoxSlot* CategoryPanelSlot = Body->AddChildToHorizontalBox(CategoryPanel))
		{
			CategoryPanelSlot->SetPadding(FMargin(0.0f, 0.0f, 24.0f, 0.0f));
		}
		UVerticalBox* Categories = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("CategoryContainer"));
		MarkVariable(*Asset.Blueprint, *Categories);
		CategoryPanel->AddChild(Categories);

		UVerticalBox* OptionsColumn = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("OptionsColumn"));
		if (UHorizontalBoxSlot* OptionsColumnSlot = Body->AddChildToHorizontalBox(OptionsColumn))
		{
			OptionsColumnSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UCommonTextBlock* CategoryTitle = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("CategoryTitleText"));
		StyleText(*CategoryTitle, FText::FromString(TEXT("显示")), 25, Cyan);
		MarkVariable(*Asset.Blueprint, *CategoryTitle);
		if (UVerticalBoxSlot* CategoryTitleSlot = OptionsColumn->AddChildToVerticalBox(CategoryTitle))
		{
			CategoryTitleSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}
		UScrollBox* Scroll = Tree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass(), TEXT("OptionsScroll"));
		if (UVerticalBoxSlot* ScrollSlot = OptionsColumn->AddChildToVerticalBox(Scroll))
		{
			ScrollSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UVerticalBox* Options = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("OptionsContainer"));
		MarkVariable(*Asset.Blueprint, *Options);
		Scroll->AddChild(Options);

		UHorizontalBox* Footer = Tree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("Footer"));
		if (UVerticalBoxSlot* FooterSlot = Page->AddChildToVerticalBox(Footer))
		{
			FooterSlot->SetPadding(FMargin(0.0f, 18.0f, 0.0f, 0.0f));
		}
		UCommonTextBlock* Status = Tree->ConstructWidget<UCommonTextBlock>(
			UCommonTextBlock::StaticClass(), TEXT("StatusText"));
		StyleText(*Status, FText::FromString(TEXT("修改后选择应用")), 13, Muted);
		MarkVariable(*Asset.Blueprint, *Status);
		if (UHorizontalBoxSlot* StatusSlot = Footer->AddChildToHorizontalBox(Status))
		{
			StatusSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			StatusSlot->SetVerticalAlignment(VAlign_Center);
		}
		return AddButton(*Asset.Blueprint, ButtonClass, *Footer, TEXT("RestoreDefaultsButton"), FText::FromString(TEXT("恢复默认")))
			&& AddButton(*Asset.Blueprint, ButtonClass, *Footer, TEXT("ApplyButton"), FText::FromString(TEXT("应用")))
			&& AddButton(*Asset.Blueprint, ButtonClass, *Footer, TEXT("BackButton"), FText::FromString(TEXT("返回")))
			&& SaveBlueprint(Asset, true);
	}

	bool SetClassProperty(UObject& Object, FName PropertyName, UClass* Value)
	{
		FClassProperty* Property = FindFProperty<FClassProperty>(Object.GetClass(), PropertyName);
		if (!Property || !Value)
		{
			return false;
		}
		Property->SetPropertyValue_InContainer(&Object, Value);
		return true;
	}
}

namespace Wacom::ContentBuilder
{
	bool BuildSettingsWidgetBlueprintContent()
	{
		FWidgetBlueprintAsset Button = LoadOrCreate(ButtonAssetName, UWacomMenuButtonWidget::StaticClass());
		if (!Button.Blueprint || !BuildButton(Button))
		{
			return false;
		}
		UClass* ButtonClass = Button.Blueprint->GeneratedClass;

		FWidgetBlueprintAsset Row = LoadOrCreate(RowAssetName, UWacomSettingsOptionRow::StaticClass());
		if (!Row.Blueprint || !BuildRow(Row, ButtonClass))
		{
			return false;
		}
		FWidgetBlueprintAsset Dialog = LoadOrCreate(
			DialogAssetName, UWacomSettingsConfirmationDialog::StaticClass());
		if (!Dialog.Blueprint || !BuildDialog(Dialog, ButtonClass))
		{
			return false;
		}
		FWidgetBlueprintAsset Screen = LoadOrCreate(ScreenAssetName, UWacomSettingsScreen::StaticClass());
		if (!Screen.Blueprint || !BuildScreen(Screen, ButtonClass))
		{
			return false;
		}

		UWacomSettingsScreen* ScreenCDO = Screen.Blueprint->GeneratedClass
			? Cast<UWacomSettingsScreen>(Screen.Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!ScreenCDO
			|| !SetClassProperty(*ScreenCDO, TEXT("SettingsButtonClass"), ButtonClass)
			|| !SetClassProperty(*ScreenCDO, TEXT("OptionRowClass"), Row.Blueprint->GeneratedClass)
			|| !SetClassProperty(*ScreenCDO, TEXT("ConfirmationDialogClass"), Dialog.Blueprint->GeneratedClass))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[SettingsWidgetBlueprintBuilder] Failed to configure reusable WBP classes"));
			return false;
		}
		ScreenCDO->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsModified(Screen.Blueprint);
		if (!SaveBlueprint(Screen, false))
		{
			return false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[SettingsWidgetBlueprintBuilder] Built four Settings Widget Blueprints"));
		return true;
	}
}
