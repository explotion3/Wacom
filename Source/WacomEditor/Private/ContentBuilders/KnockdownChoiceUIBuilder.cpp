// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/KnockdownChoiceUIBuilder.h"

#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Animation/WidgetAnimation.h"
#include "Animation/WidgetAnimationBinding.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "CommonButtonBase.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Components/Spacer.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UI/Battle/WacomKnockdownChoiceDialog.h"
#include "UI/Battle/WacomKnockdownChoiceOptionWidget.h"
#include "UI/Card/WacomCardView.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr const TCHAR* AssetRoot =
		TEXT("/Game/Wacom/UI/Battle/Knockdown");
	constexpr const TCHAR* OptionAssetName =
		TEXT("WBP_BattleKnockdownChoiceOption");
	constexpr const TCHAR* DialogAssetName =
		TEXT("WBP_BattleKnockdownChoiceDialog");
	constexpr const TCHAR* CardViewClassPath =
		TEXT("/Game/Wacom/UI/Card/WBP_CardView.WBP_CardView_C");
	constexpr const TCHAR* ButtonStyleClassPath =
		TEXT("/Game/Wacom/UI/Style/Button/tiny_menu_Button.tiny_menu_Button_C");

	const FLinearColor Ink(0.015f, 0.022f, 0.038f, 0.985f);
	const FLinearColor Panel(0.030f, 0.045f, 0.070f, 0.94f);
	const FLinearColor PanelSoft(0.050f, 0.070f, 0.100f, 0.78f);
	const FLinearColor Cyan(0.30f, 0.88f, 0.82f, 1.0f);
	const FLinearColor Amber(0.98f, 0.78f, 0.32f, 1.0f);
	const FLinearColor Paper(0.91f, 0.90f, 0.82f, 1.0f);
	const FLinearColor Muted(0.57f, 0.63f, 0.65f, 1.0f);
	const FLinearColor Danger(0.95f, 0.40f, 0.34f, 1.0f);

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	FWidgetBlueprintAsset LoadOrCreate(
		const TCHAR* AssetName,
		UClass* ParentClass)
	{
		FWidgetBlueprintAsset Result;
		Result.PackagePath = MakePackagePath(AssetRoot, AssetName);
		const FString ObjectPath = MakeObjectPath(Result.PackagePath);

		if (UObject* ExistingObject = StaticLoadObject(
			UObject::StaticClass(), nullptr, *ObjectPath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(ExistingObject);
			if (!Result.Blueprint)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[KnockdownChoiceUIBuilder] Existing asset is not a Widget Blueprint: %s"),
					*ObjectPath);
				return Result;
			}
			if (!Result.Blueprint->ParentClass
				|| !Result.Blueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[KnockdownChoiceUIBuilder] Incompatible parent for %s Expected=%s Actual=%s"),
					*ObjectPath,
					*GetNameSafe(ParentClass),
					*GetNameSafe(Result.Blueprint->ParentClass));
				Result.Blueprint = nullptr;
			}
			return Result;
		}

		UPackage* Package = FindOrCreatePackage(Result.PackagePath);
		if (!Package)
		{
			return Result;
		}

		Result.Blueprint = Cast<UWidgetBlueprint>(
			FKismetEditorUtilities::CreateBlueprint(
				ParentClass,
				Package,
				AssetName,
				BPTYPE_Normal,
				UWidgetBlueprint::StaticClass(),
				UWidgetBlueprintGeneratedClass::StaticClass()));
		Result.bCreated = Result.Blueprint != nullptr;
		return Result;
	}

	void ResetWidgetBlueprint(
		UWidgetBlueprint& Blueprint,
		const FString& Description)
	{
		Blueprint.Modify();
		if (UWidgetTree* PreviousTree = Blueprint.WidgetTree)
		{
			const FName TransientTreeName = MakeUniqueObjectName(
				GetTransientPackage(),
				UWidgetTree::StaticClass(),
				*FString::Printf(
					TEXT("%s_PreviousWidgetTree"),
					*Blueprint.GetName()));
			PreviousTree->Rename(
				*TransientTreeName.ToString(),
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		for (UWidgetAnimation* Animation : Blueprint.Animations)
		{
			if (Animation)
			{
				Animation->Rename(
				nullptr,
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}

		Blueprint.WidgetTree = NewObject<UWidgetTree>(
			&Blueprint,
			TEXT("WidgetTree"),
			RF_Transactional);
		Blueprint.Bindings.Reset();
		Blueprint.Animations.Reset();
		Blueprint.WidgetVariableNameToGuidMap.Reset();
		Blueprint.BlueprintDescription = Description;
		Blueprint.bCanCallInitializedWithoutPlayerContext = true;
	}

	bool AddOpacityPulseAnimation(
		UWidgetBlueprint& Blueprint,
		UWidget& TargetWidget)
	{
		const FName AnimationName(TEXT("SubmissionRejectedAnimation"));
		UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(
			&Blueprint,
			AnimationName,
			RF_Transactional);
		if (!Animation)
		{
			return false;
		}

		Animation->SetDisplayLabel(AnimationName.ToString());
		Animation->MovieScene = NewObject<UMovieScene>(
			Animation,
			AnimationName,
			RF_Transactional);
		if (!Animation->MovieScene)
		{
			return false;
		}

		Animation->MovieScene->SetDisplayRate(FFrameRate(60, 1));
		Animation->MovieScene->SetTickResolutionDirectly(FFrameRate(60, 1));
		Animation->MovieScene->SetPlaybackRange(FFrameNumber(0), 13);

		const FGuid BindingGuid = Animation->MovieScene->AddPossessable(
			TargetWidget.GetName(),
			TargetWidget.GetClass());
		FWidgetAnimationBinding& Binding =
			Animation->AnimationBindings.AddDefaulted_GetRef();
		Binding.AnimationGuid = BindingGuid;
		Binding.WidgetName = TargetWidget.GetFName();
		Binding.bIsRootWidget = false;

		UMovieSceneFloatTrack* OpacityTrack =
			Animation->MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!OpacityTrack)
		{
			return false;
		}
		OpacityTrack->SetPropertyNameAndPath(
			TEXT("RenderOpacity"),
			TEXT("RenderOpacity"));
		UMovieSceneFloatSection* Section = Cast<UMovieSceneFloatSection>(
			OpacityTrack->CreateNewSection());
		if (!Section)
		{
			return false;
		}
		Section->SetRange(TRange<FFrameNumber>(
			FFrameNumber(0),
			FFrameNumber(13)));
		FMovieSceneFloatChannel& Channel = Section->GetChannel();
		Channel.AddLinearKey(FFrameNumber(0), 1.0f);
		Channel.AddLinearKey(FFrameNumber(3), 0.45f);
		Channel.AddLinearKey(FFrameNumber(6), 1.0f);
		Channel.AddLinearKey(FFrameNumber(9), 0.45f);
		Channel.AddLinearKey(FFrameNumber(12), 1.0f);
		OpacityTrack->AddSection(*Section);

		Blueprint.Animations.Add(Animation);
		const FString StableAnimationPath = FString::Printf(
			TEXT("%s:%s"),
			*Blueprint.GetPathName(),
			*AnimationName.ToString());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(AnimationName) =
			FGuid::NewDeterministicGuid(StableAnimationPath);
		return true;
	}

	void RegisterWidgetGuid(
		UWidgetBlueprint& Blueprint,
		const UWidget& Widget)
	{
		const FString StableWidgetPath = FString::Printf(
			TEXT("%s:%s"),
			*Blueprint.GetPathName(),
			*Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StableWidgetPath);
	}

	void MarkWidgetVariable(
		UWidgetBlueprint& Blueprint,
		UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void StyleText(
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
		Text.SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text.SetShadowColorAndOpacity(
			FLinearColor(0.0f, 0.0f, 0.0f, 0.82f));
	}

	bool SetClassDefault(
		UObject& Object,
		FName PropertyName,
		UClass* Value)
	{
		FClassProperty* Property =
			FindFProperty<FClassProperty>(Object.GetClass(), PropertyName);
		if (!Property || (Value && !Value->IsChildOf(Property->MetaClass)))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] Invalid class default %s on %s Value=%s"),
				*PropertyName.ToString(),
				*Object.GetClass()->GetName(),
				*GetNameSafe(Value));
			return false;
		}
		Object.Modify();
		Property->SetPropertyValue_InContainer(&Object, Value);
		return true;
	}

	bool CompileAndSave(
		const FWidgetBlueprintAsset& Asset,
		TFunctionRef<bool(UClass*)> ConfigureDefaults)
	{
		if (!Asset.Blueprint || !Asset.Blueprint->WidgetTree)
		{
			return false;
		}

		TArray<UWidget*> SourceWidgets;
		Asset.Blueprint->WidgetTree->GetAllWidgets(SourceWidgets);
		for (UWidget* Widget : SourceWidgets)
		{
			if (Widget)
			{
				RegisterWidgetGuid(*Asset.Blueprint, *Widget);
			}
		}

		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(
			Asset.Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Asset.Blueprint);
		if (Asset.Blueprint->Status == BS_Error
			|| !Asset.Blueprint->GeneratedClass
			|| !ConfigureDefaults(Asset.Blueprint->GeneratedClass))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] Compile/default configuration failed: %s"),
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
		Asset.Blueprint->GeneratedClass->GetDefaultObject()->MarkPackageDirty();

		const FString Filename = FPackageName::LongPackageNameToFilename(
			Asset.PackagePath,
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(
			*FPaths::GetPath(Filename),
			/*Tree*/true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(
			Package,
			Asset.Blueprint,
			*Filename,
			Args);
	}

	bool BuildOptionBlueprint(
		FWidgetBlueprintAsset& Asset,
		UClass* CardViewClass,
		UClass* ButtonStyleClass)
	{
		ResetWidgetBlueprint(
			*Asset.Blueprint,
			TEXT("击倒选择三联卡的被动 CommonButton。只显示 UI ViewData 并广播 typed intent。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;

		USizeBox* Root = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("OptionContentRoot"));
		Root->SetMinDesiredWidth(220.0f);
		Root->SetMinDesiredHeight(462.0f);
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		Tree->RootWidget = Root;

		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("OptionOverlay"));
		Root->SetContent(Overlay);

		UBorder* Backdrop = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("OptionBackdrop"));
		Backdrop->SetBrushColor(PanelSoft);
		Backdrop->SetPadding(FMargin(0.0f));
		if (UOverlaySlot* BackdropSlot = Overlay->AddChildToOverlay(Backdrop))
		{
			BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
			BackdropSlot->SetVerticalAlignment(VAlign_Fill);
		}

		USizeBox* AccentSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("OptionAccentSize"));
		AccentSize->SetHeightOverride(4.0f);
		UBorder* Accent = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("OptionAccent"));
		Accent->SetBrushColor(Cyan);
		AccentSize->SetContent(Accent);
		if (UOverlaySlot* AccentSlot = Overlay->AddChildToOverlay(AccentSize))
		{
			AccentSlot->SetHorizontalAlignment(HAlign_Fill);
			AccentSlot->SetVerticalAlignment(VAlign_Top);
		}

		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("OptionContentColumn"));
		if (UOverlaySlot* ContentSlot = Overlay->AddChildToOverlay(Column))
		{
			ContentSlot->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 14.0f));
			ContentSlot->SetHorizontalAlignment(HAlign_Fill);
			ContentSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UTextBlock* BranchLabel = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("BranchLabelText"));
		StyleText(*BranchLabel, FText::FromString(TEXT("左手")), 13, Cyan, TEXT("Bold"));
		BranchLabel->SetJustification(ETextJustify::Center);
		MarkWidgetVariable(*Asset.Blueprint, *BranchLabel);
		Column->AddChildToVerticalBox(BranchLabel);

		UTextBlock* ChoiceLabel = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ChoiceLabelText"));
		StyleText(*ChoiceLabel, FText::FromString(TEXT("援助")), 25, Paper, TEXT("Bold"));
		ChoiceLabel->SetJustification(ETextJustify::Center);
		MarkWidgetVariable(*Asset.Blueprint, *ChoiceLabel);
		if (UVerticalBoxSlot* ChoiceSlot =
			Column->AddChildToVerticalBox(ChoiceLabel))
		{
			ChoiceSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 5.0f));
		}

		UTextBlock* Description = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("DescriptionText"));
		StyleText(
			*Description,
			FText::FromString(TEXT("接纳此部位的奖励\n不消耗手牌")),
			13,
			Muted);
		Description->SetJustification(ETextJustify::Center);
		Description->SetAutoWrapText(true);
		MarkWidgetVariable(*Asset.Blueprint, *Description);
		if (UVerticalBoxSlot* DescriptionSlot =
			Column->AddChildToVerticalBox(Description))
		{
			DescriptionSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 8.0f));
		}

		USizeBox* RewardCardSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("RewardCardSize"));
		RewardCardSize->SetWidthOverride(178.0f);
		RewardCardSize->SetHeightOverride(252.0f);
		if (UVerticalBoxSlot* CardSlot =
			Column->AddChildToVerticalBox(RewardCardSize))
		{
			CardSlot->SetHorizontalAlignment(HAlign_Center);
		}

		UScaleBox* RewardCardHost = Tree->ConstructWidget<UScaleBox>(
			UScaleBox::StaticClass(), TEXT("RewardCardHost"));
		RewardCardHost->SetStretch(EStretch::ScaleToFit);
		RewardCardHost->SetStretchDirection(EStretchDirection::DownOnly);
		RewardCardHost->SetVisibility(ESlateVisibility::HitTestInvisible);
		MarkWidgetVariable(*Asset.Blueprint, *RewardCardHost);
		RewardCardSize->SetContent(RewardCardHost);

		UTextBlock* RewardFallback = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("RewardFallbackText"));
		StyleText(
			*RewardFallback,
			FText::FromString(TEXT("无卡牌奖励")),
			13,
			Amber,
			TEXT("Bold"));
		RewardFallback->SetJustification(ETextJustify::Center);
		RewardFallback->SetAutoWrapText(true);
		RewardFallback->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(*Asset.Blueprint, *RewardFallback);
		if (UVerticalBoxSlot* RewardFallbackSlot =
			Column->AddChildToVerticalBox(RewardFallback))
		{
			RewardFallbackSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}

		UTextBlock* DisabledReason = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("DisabledReasonText"));
		StyleText(
			*DisabledReason,
			FText::FromString(TEXT("当前无法选择")),
			12,
			Danger,
			TEXT("Bold"));
		DisabledReason->SetJustification(ETextJustify::Center);
		DisabledReason->SetAutoWrapText(true);
		DisabledReason->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(*Asset.Blueprint, *DisabledReason);
		if (UVerticalBoxSlot* DisabledSlot =
			Column->AddChildToVerticalBox(DisabledReason))
		{
			DisabledSlot->SetPadding(FMargin(0.0f, 7.0f, 0.0f, 0.0f));
		}

		return CompileAndSave(
			Asset,
			[CardViewClass, ButtonStyleClass](UClass* GeneratedClass)
			{
				UWacomKnockdownChoiceOptionWidget* Defaults = GeneratedClass
					? Cast<UWacomKnockdownChoiceOptionWidget>(
						GeneratedClass->GetDefaultObject())
					: nullptr;
				if (!Defaults
					|| !SetClassDefault(
						*Defaults,
						TEXT("RewardCardViewClass"),
						CardViewClass))
				{
					return false;
				}
				if (ButtonStyleClass)
				{
					Defaults->SetStyle(ButtonStyleClass);
				}
				return true;
			});
	}

	UWacomKnockdownChoiceOptionWidget* AddOption(
		UWidgetBlueprint& Blueprint,
		UHorizontalBox& Row,
		UClass* OptionClass,
		FName Name,
		float Width)
	{
		USizeBox* Sizer = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			*FString::Printf(TEXT("%sSize"), *Name.ToString()));
		Sizer->SetWidthOverride(Width);
		Sizer->SetHeightOverride(470.0f);

		UWacomKnockdownChoiceOptionWidget* Option =
			Blueprint.WidgetTree->ConstructWidget<UWacomKnockdownChoiceOptionWidget>(
				OptionClass,
				Name);
		if (!Option)
		{
			return nullptr;
		}
		MarkWidgetVariable(Blueprint, *Option);
		Sizer->SetContent(Option);
		if (UHorizontalBoxSlot* OptionSlot = Row.AddChildToHorizontalBox(Sizer))
		{
			OptionSlot->SetPadding(FMargin(8.0f, 0.0f));
			OptionSlot->SetVerticalAlignment(VAlign_Fill);
		}
		return Option;
	}

	bool BuildDialogBlueprint(
		FWidgetBlueprintAsset& Asset,
		UClass* OptionClass)
	{
		if (!OptionClass
			|| !OptionClass->IsChildOf(
				UWacomKnockdownChoiceOptionWidget::StaticClass()))
		{
			return false;
		}

		ResetWidgetBlueprint(
			*Asset.Blueprint,
			TEXT("击倒事件正式 CommonUI Modal。WBP 只提供三联安全布局与表现。"));
		UWidgetTree* Tree = Asset.Blueprint->WidgetTree;

		UCanvasPanel* Root = Tree->ConstructWidget<UCanvasPanel>(
			UCanvasPanel::StaticClass(), TEXT("Root"));
		Tree->RootWidget = Root;

		UBorder* Backdrop = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("DimBackdrop"));
		Backdrop->SetBrushColor(FLinearColor(0.0f, 0.0f, 0.0f, 0.72f));
		if (UCanvasPanelSlot* BackdropSlot = Root->AddChildToCanvas(Backdrop))
		{
			BackdropSlot->SetAnchors(FAnchors(0.0f, 0.0f, 1.0f, 1.0f));
			BackdropSlot->SetOffsets(FMargin(0.0f));
		}

		USizeBox* PanelSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("PanelSize"));
		PanelSize->SetWidthOverride(1040.0f);
		PanelSize->SetHeightOverride(620.0f);
		if (UCanvasPanelSlot* PanelCanvasSlot = Root->AddChildToCanvas(PanelSize))
		{
			PanelCanvasSlot->SetAnchors(FAnchors(0.5f, 0.5f));
			PanelCanvasSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			PanelCanvasSlot->SetOffsets(FMargin(0.0f, 0.0f, 1040.0f, 620.0f));
		}

		UBorder* PanelBorder = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("PanelBorder"));
		PanelBorder->SetBrushColor(Ink);
		// Keep at least 964 px for the 340 / 236 / 340 option row plus gutters.
		PanelBorder->SetPadding(FMargin(16.0f, 12.0f));
		PanelSize->SetContent(PanelBorder);

		UOverlay* PanelOverlay = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("PanelOverlay"));
		PanelBorder->SetContent(PanelOverlay);

		UBorder* InnerPanel = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("InnerPanel"));
		InnerPanel->SetBrushColor(Panel);
		InnerPanel->SetPadding(FMargin(10.0f, 8.0f));
		if (UOverlaySlot* InnerPanelSlot = PanelOverlay->AddChildToOverlay(InnerPanel))
		{
			InnerPanelSlot->SetHorizontalAlignment(HAlign_Fill);
			InnerPanelSlot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Column = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("DialogContentColumn"));
		InnerPanel->SetContent(Column);

		UTextBlock* Eyebrow = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("DialogEyebrow"));
		StyleText(
			*Eyebrow,
			FText::FromString(TEXT("KNOCKDOWN // CHOOSE AN OUTCOME")),
			12,
			Cyan,
			TEXT("Bold"));
		Eyebrow->SetJustification(ETextJustify::Center);
		Column->AddChildToVerticalBox(Eyebrow);

		UTextBlock* Title = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("TitleText"));
		StyleText(
			*Title,
			FText::FromString(TEXT("选择击倒结果")),
			29,
			Paper,
			TEXT("Bold"));
		Title->SetJustification(ETextJustify::Center);
		MarkWidgetVariable(*Asset.Blueprint, *Title);
		if (UVerticalBoxSlot* TitleSlot = Column->AddChildToVerticalBox(Title))
		{
			TitleSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 1.0f));
		}

		UTextBlock* PartName = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("PartNameText"));
		StyleText(
			*PartName,
			FText::FromString(TEXT("已击倒：部位")),
			14,
			Amber,
			TEXT("Bold"));
		PartName->SetJustification(ETextJustify::Center);
		MarkWidgetVariable(*Asset.Blueprint, *PartName);
		if (UVerticalBoxSlot* PartNameSlot = Column->AddChildToVerticalBox(PartName))
		{
			PartNameSlot->SetPadding(FMargin(0.0f, 0.0f, 0.0f, 12.0f));
		}

		UHorizontalBox* Options = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("OptionsRow"));
		if (UVerticalBoxSlot* OptionsSlot = Column->AddChildToVerticalBox(Options))
		{
			OptionsSlot->SetHorizontalAlignment(HAlign_Center);
			OptionsSlot->SetVerticalAlignment(VAlign_Fill);
		}

		if (!AddOption(
			*Asset.Blueprint, *Options, OptionClass, TEXT("AidOption"), 340.0f)
			|| !AddOption(
				*Asset.Blueprint, *Options, OptionClass, TEXT("WithdrawOption"), 236.0f)
			|| !AddOption(
				*Asset.Blueprint, *Options, OptionClass, TEXT("DestroyOption"), 340.0f))
		{
			return false;
		}
		if (!AddOpacityPulseAnimation(*Asset.Blueprint, *PanelBorder))
		{
			return false;
		}

		UTextBlock* InputHint = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("InputHintText"));
		StyleText(
			*InputHint,
			FText::FromString(TEXT("[← →] 选择    [确认] 执行    返回键不可关闭")),
			11,
			Muted,
			TEXT("Bold"));
		InputHint->SetJustification(ETextJustify::Center);
		if (UVerticalBoxSlot* InputHintSlot =
			Column->AddChildToVerticalBox(InputHint))
		{
			InputHintSlot->SetPadding(FMargin(0.0f, 9.0f, 0.0f, 0.0f));
		}

		return CompileAndSave(
			Asset,
			[](UClass* GeneratedClass)
			{
				return GeneratedClass
					&& GeneratedClass->IsChildOf(
						UWacomKnockdownChoiceDialog::StaticClass());
			});
	}

	UWidgetTree* GetWidgetTree(UClass* WidgetClass)
	{
		const UWidgetBlueprintGeneratedClass* GeneratedClass =
			Cast<UWidgetBlueprintGeneratedClass>(WidgetClass);
		return GeneratedClass
			? GeneratedClass->GetWidgetTreeArchetype()
			: nullptr;
	}

	bool RequireWidget(
		UWidgetTree& Tree,
		FName Name,
		UClass* ExpectedClass)
	{
		UWidget* Widget = Tree.FindWidget(Name);
		if (!Widget || !Widget->IsA(ExpectedClass))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] Missing/wrong binding %s Expected=%s Actual=%s"),
				*Name.ToString(),
				*GetNameSafe(ExpectedClass),
				*GetNameSafe(Widget ? Widget->GetClass() : nullptr));
			return false;
		}
		return true;
	}
}

namespace Wacom::ContentBuilder
{
	bool BuildKnockdownChoiceUI()
	{
		UClass* CardViewClass = LoadClass<UWacomCardView>(
			nullptr,
			CardViewClassPath);
		if (!CardViewClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] Missing generic CardView: %s"),
				CardViewClassPath);
			return false;
		}

		UClass* ButtonStyleClass = LoadClass<UCommonButtonStyle>(
			nullptr,
			ButtonStyleClassPath);
		if (!ButtonStyleClass)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[KnockdownChoiceUIBuilder] CommonButtonStyle missing; native CommonButton state remains functional: %s"),
				ButtonStyleClassPath);
		}

		FWidgetBlueprintAsset Option = LoadOrCreate(
			OptionAssetName,
			UWacomKnockdownChoiceOptionWidget::StaticClass());
		if (!Option.Blueprint
			|| !BuildOptionBlueprint(
				Option,
				CardViewClass,
				ButtonStyleClass))
		{
			return false;
		}

		FWidgetBlueprintAsset Dialog = LoadOrCreate(
			DialogAssetName,
			UWacomKnockdownChoiceDialog::StaticClass());
		if (!Dialog.Blueprint
			|| !BuildDialogBlueprint(
				Dialog,
				Option.Blueprint->GeneratedClass))
		{
			return false;
		}

		return InspectKnockdownChoiceUI();
	}

	bool InspectKnockdownChoiceUI()
	{
		UClass* OptionClass = LoadClass<UWacomKnockdownChoiceOptionWidget>(
			nullptr,
			*FString::Printf(
				TEXT("%s/%s.%s_C"),
				AssetRoot,
				OptionAssetName,
				OptionAssetName));
		UClass* DialogClass = LoadClass<UWacomKnockdownChoiceDialog>(
			nullptr,
			*FString::Printf(
				TEXT("%s/%s.%s_C"),
				AssetRoot,
				DialogAssetName,
				DialogAssetName));
		UClass* CardViewClass = LoadClass<UWacomCardView>(
			nullptr,
			CardViewClassPath);
		if (!OptionClass || !DialogClass || !CardViewClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] Inspect failed to load formal classes"));
			return false;
		}

		UWidgetTree* OptionTree = GetWidgetTree(OptionClass);
		UWidgetTree* DialogTree = GetWidgetTree(DialogClass);
		if (!OptionTree || !DialogTree)
		{
			return false;
		}

		bool bValid = true;
		bValid &= RequireWidget(
			*OptionTree, TEXT("BranchLabelText"), UTextBlock::StaticClass());
		bValid &= RequireWidget(
			*OptionTree, TEXT("ChoiceLabelText"), UTextBlock::StaticClass());
		bValid &= RequireWidget(
			*OptionTree, TEXT("DescriptionText"), UTextBlock::StaticClass());
		bValid &= RequireWidget(
			*OptionTree, TEXT("RewardCardHost"), UScaleBox::StaticClass());
		bValid &= RequireWidget(
			*OptionTree, TEXT("RewardFallbackText"), UTextBlock::StaticClass());
		bValid &= RequireWidget(
			*OptionTree, TEXT("DisabledReasonText"), UTextBlock::StaticClass());

		bValid &= RequireWidget(
			*DialogTree, TEXT("TitleText"), UTextBlock::StaticClass());
		bValid &= RequireWidget(
			*DialogTree, TEXT("PartNameText"), UTextBlock::StaticClass());
		bValid &= RequireWidget(
			*DialogTree, TEXT("AidOption"), OptionClass);
		bValid &= RequireWidget(
			*DialogTree, TEXT("WithdrawOption"), OptionClass);
		bValid &= RequireWidget(
			*DialogTree, TEXT("DestroyOption"), OptionClass);
		const UWidgetBlueprintGeneratedClass* DialogGeneratedClass =
			Cast<UWidgetBlueprintGeneratedClass>(DialogClass);
		if (!DialogGeneratedClass
			|| !DialogGeneratedClass->Animations.ContainsByPredicate(
				[](const UWidgetAnimation* Animation)
				{
					return Animation
						&& Animation->GetDisplayLabel() ==
							TEXT("SubmissionRejectedAnimation");
				}))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] Missing SubmissionRejectedAnimation"));
			bValid = false;
		}

		const UWacomKnockdownChoiceOptionWidget* Defaults =
			OptionClass->GetDefaultObject<UWacomKnockdownChoiceOptionWidget>();
		if (!Defaults
			|| Defaults->GetRewardCardViewClass().Get() != CardViewClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[KnockdownChoiceUIBuilder] RewardCardViewClass must be exact WBP_CardView"));
			bValid = false;
		}

		UE_LOG(LogTemp, Display,
			TEXT("[KnockdownChoiceUIBuilder] Inspect %s Option=%s Dialog=%s RewardCard=%s"),
			bValid ? TEXT("passed") : TEXT("failed"),
			*OptionClass->GetPathName(),
			*DialogClass->GetPathName(),
			*CardViewClass->GetPathName());
		return bValid;
	}
}
