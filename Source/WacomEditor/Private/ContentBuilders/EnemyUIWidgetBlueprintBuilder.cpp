// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemyUIWidgetBlueprintBuilder.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetBlueprintGeneratedClass.h"
#include "Blueprint/WidgetTree.h"
#include "Channels/MovieSceneFloatChannel.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "Sections/MovieSceneFloatSection.h"
#include "Tracks/MovieSceneFloatTrack.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR PanelObjectPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget");
	constexpr TCHAR PartEntryObjectPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget");
	constexpr TCHAR StatusListClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_BattleStatusIconList.WBP_BattleStatusIconList_C");
	constexpr TCHAR PixelPanelTexturePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Textures/T_UI_PixelPanel_EnemyInfo_9Slice_512x160.T_UI_PixelPanel_EnemyInfo_9Slice_512x160");
	constexpr TCHAR ContractMarker[] = TEXT("WacomEnemyPanelWBP.ContractVersion=1");

	const FLinearColor EntryInk(0.030f, 0.044f, 0.062f, 0.92f);
	const FLinearColor Paper(0.92f, 0.90f, 0.81f, 1.0f);
	const FLinearColor Muted(0.58f, 0.66f, 0.70f, 1.0f);
	const FLinearColor Cyan(0.30f, 0.88f, 0.82f, 1.0f);
	const FLinearColor Amber(0.98f, 0.72f, 0.27f, 1.0f);
	const FLinearColor Damage(0.92f, 0.20f, 0.20f, 1.0f);

	enum class EWidgetBlueprintState : uint8
	{
		LegacyEmpty,
		Formal,
		Unknown
	};

	struct FEnemyUIAssets
	{
		UWidgetBlueprint* Panel = nullptr;
		UWidgetBlueprint* PartEntry = nullptr;
	};

	void StyleText(
		UTextBlock* Text,
		const FText& Value,
		const int32 FontSize,
		const FLinearColor& Color,
		const FName Typeface = TEXT("Regular"))
	{
		check(Text);
		Text->SetText(Value);
		FSlateFontInfo Font = Text->GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = Typeface;
		Text->SetFont(Font);
		Text->SetColorAndOpacity(FSlateColor(Color));
		Text->SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.86f));
	}

	void RegisterWidgetGuid(UWidgetBlueprint* Blueprint, const UWidget* Widget)
	{
		check(Blueprint && Widget);
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"), *Blueprint->GetPathName(), *Widget->GetName());
		Blueprint->WidgetVariableNameToGuidMap.FindOrAdd(Widget->GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(UWidgetBlueprint* Blueprint, UWidget* Widget)
	{
		check(Blueprint && Widget);
		Widget->bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void ResetWidgetBlueprint(UWidgetBlueprint* Blueprint, const FString& Description)
	{
		check(Blueprint);
		Blueprint->Modify();
		if (UWidgetTree* PreviousTree = Blueprint->WidgetTree)
		{
			PreviousTree->Rename(
				nullptr,
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		for (UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation)
			{
				Animation->Rename(
					nullptr,
					GetTransientPackage(),
					REN_DontCreateRedirectors | REN_NonTransactional);
			}
		}

		Blueprint->WidgetTree = NewObject<UWidgetTree>(
			Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint->Bindings.Reset();
		Blueprint->Animations.Reset();
		Blueprint->WidgetVariableNameToGuidMap.Reset();
		Blueprint->BlueprintDescription = Description + TEXT("\n") + ContractMarker;
		Blueprint->bCanCallInitializedWithoutPlayerContext = true;
	}

	void MakeTreeNonHitTestable(UWidgetBlueprint* Blueprint)
	{
		TArray<UWidget*> Widgets;
		Blueprint->WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}
			RegisterWidgetGuid(Blueprint, Widget);
			if (Widget->GetVisibility() == ESlateVisibility::Visible
				|| Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
		}
	}

	UWidgetAnimation* AddOpacityAnimation(
		UWidgetBlueprint* Blueprint,
		const FName AnimationName,
		UWidget* Target,
		const TArray<TPair<float, float>>& Keys)
	{
		check(Blueprint && Target && !Keys.IsEmpty());
		UWidgetAnimation* Animation = NewObject<UWidgetAnimation>(
			Blueprint, AnimationName, RF_Transactional);
		Animation->SetDisplayLabel(AnimationName.ToString());
		const FString StableAnimationPath = FString::Printf(
			TEXT("%s:Animation:%s"), *Blueprint->GetPathName(), *AnimationName.ToString());
		Blueprint->WidgetVariableNameToGuidMap.FindOrAdd(AnimationName) =
			FGuid::NewDeterministicGuid(StableAnimationPath);
		Animation->MovieScene = NewObject<UMovieScene>(
			Animation, AnimationName, RF_Transactional);
		Animation->MovieScene->SetDisplayRate(FFrameRate(30, 1));
		Animation->MovieScene->SetTickResolutionDirectly(FFrameRate(30, 1));

		const FFrameNumber EndFrame(FMath::Max(1, FMath::RoundToInt(Keys.Last().Key * 30.0f)));
		Animation->MovieScene->SetPlaybackRange(
			TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		Animation->MovieScene->GetEditorData().WorkStart = 0.0;
		Animation->MovieScene->GetEditorData().WorkEnd = Keys.Last().Key;

		const FGuid BindingGuid = Animation->MovieScene->AddPossessable(
			Target->GetName(), Target->GetClass());
		Animation->MovieScene->SetObjectDisplayName(
			BindingGuid, FText::FromName(Target->GetFName()));
		FWidgetAnimationBinding Binding;
		Binding.AnimationGuid = BindingGuid;
		Binding.WidgetName = Target->GetFName();
		Animation->AnimationBindings.Add(Binding);

		UMovieSceneFloatTrack* Track =
			Animation->MovieScene->AddTrack<UMovieSceneFloatTrack>(BindingGuid);
		if (!Track)
		{
			return nullptr;
		}
		Track->SetPropertyNameAndPath(TEXT("RenderOpacity"), TEXT("RenderOpacity"));
		UMovieSceneFloatSection* Section =
			Cast<UMovieSceneFloatSection>(Track->CreateNewSection());
		if (!Section)
		{
			return nullptr;
		}
		Section->SetRange(TRange<FFrameNumber>(FFrameNumber(0), EndFrame + 1));
		for (const TPair<float, float>& Key : Keys)
		{
			Section->GetChannel().AddLinearKey(
				FFrameNumber(FMath::RoundToInt(Key.Key * 30.0f)), Key.Value);
		}
		Track->AddSection(*Section);
		Blueprint->Animations.Add(Animation);
		return Animation;
	}

	bool CompileAndSave(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint && Blueprint->WidgetTree);
		MakeTreeNonHitTestable(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] Compile failed: %s"),
				*Blueprint->GetPathName());
			return false;
		}

		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		Blueprint->MarkPackageDirty();
		const FString PackagePath = Package->GetName();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *Filename, Args);
	}

	bool SaveCompiledDefaults(UWidgetBlueprint* Blueprint)
	{
		check(Blueprint && Blueprint->GeneratedClass);
		UPackage* Package = Blueprint->GetOutermost();
		Package->MarkPackageDirty();
		Blueprint->GeneratedClass->GetDefaultObject()->MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, Blueprint, *Filename, Args);
	}

	FEnemyUIAssets LoadAssets()
	{
		FEnemyUIAssets Assets;
		Assets.Panel = Cast<UWidgetBlueprint>(StaticLoadObject(
			UWidgetBlueprint::StaticClass(), nullptr, PanelObjectPath));
		Assets.PartEntry = Cast<UWidgetBlueprint>(StaticLoadObject(
			UWidgetBlueprint::StaticClass(), nullptr, PartEntryObjectPath));
		return Assets;
	}

	bool IsLegacyEmptyShell(const UWidgetBlueprint* Blueprint)
	{
		return Blueprint
			&& (!Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
			&& Blueprint->Animations.IsEmpty();
	}

	bool HasWidgetOfClass(
		const UWidgetBlueprint* Blueprint,
		const FName WidgetName,
		const UClass* RequiredClass)
	{
		const UWidget* Widget = Blueprint && Blueprint->WidgetTree
			? Blueprint->WidgetTree->FindWidget(WidgetName)
			: nullptr;
		return Widget && Widget->IsA(RequiredClass);
	}

	bool HasAnimation(const UWidgetBlueprint* Blueprint, const FName AnimationName)
	{
		for (const UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation && Animation->GetFName() == AnimationName
				&& Animation->MovieScene
				&& !Animation->GetBindings().IsEmpty())
			{
				return true;
			}
		}
		return false;
	}

	bool IsTreeNonHitTestable(const UWidgetBlueprint* Blueprint)
	{
		if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
		{
			return false;
		}
		TArray<UWidget*> Widgets;
		Blueprint->WidgetTree->GetAllWidgets(Widgets);
		for (const UWidget* Widget : Widgets)
		{
			if (Widget && (Widget->GetVisibility() == ESlateVisibility::Visible
				|| Widget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible))
			{
				return false;
			}
		}
		return true;
	}

	bool ValidatePartEntryBlueprint(const UWidgetBlueprint* Blueprint, const bool bLogErrors)
	{
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(UWacomBattleEnemyPartEntryWidget::StaticClass())
			&& Blueprint->BlueprintDescription.Contains(ContractMarker);
		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] PartEntry parent/marker invalid"));
		}

		const TPair<FName, UClass*> RequiredWidgets[] = {
			{ TEXT("PartNameText"), UTextBlock::StaticClass() },
			{ TEXT("HpBar"), UProgressBar::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldContainer"), UWidget::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("IntentText"), UTextBlock::StaticClass() },
			{ TEXT("ResistanceText"), UTextBlock::StaticClass() },
			{ TEXT("DetailsContainer"), UWidget::StaticClass() },
			{ TEXT("StatusList"), UWacomBattleStatusIconListWidget::StaticClass() },
			{ TEXT("ContextHighlight"), UWidget::StaticClass() },
			{ TEXT("ActionPreviewOverlay"), UWidget::StaticClass() },
			{ TEXT("DestroyedOverlay"), UWidget::StaticClass() },
		};
		for (const TPair<FName, UClass*>& Required : RequiredWidgets)
		{
			const bool bBindingValid = HasWidgetOfClass(Blueprint, Required.Key, Required.Value);
			bValid &= bBindingValid;
			if (!bBindingValid && bLogErrors)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemyUIWidgetBlueprintBuilder] Missing PartEntry binding %s (%s)"),
					*Required.Key.ToString(), *Required.Value->GetName());
			}
		}
		const FName RequiredAnimations[] = {
			TEXT("IntroAnimation"), TEXT("DamagePulseAnimation"),
			TEXT("ShieldPulseAnimation"), TEXT("DestroyedPulseAnimation"),
			TEXT("ContextHighlightAnimation")
		};
		for (const FName AnimationName : RequiredAnimations)
		{
			const bool bAnimationValid = HasAnimation(Blueprint, AnimationName);
			bValid &= bAnimationValid;
			if (!bAnimationValid && bLogErrors)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemyUIWidgetBlueprintBuilder] Missing or empty animation %s"),
					*AnimationName.ToString());
			}
		}
		const bool bNonHitTestable = IsTreeNonHitTestable(Blueprint);
		bValid &= bNonHitTestable;
		if (!bNonHitTestable && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] PartEntry tree contains a hit-testable widget"));
		}

		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] PartEntry WBP contract mismatch: %s"),
				Blueprint ? *Blueprint->GetPathName() : PartEntryObjectPath);
		}
		return bValid;
	}

	bool ValidatePanelBlueprint(
		const UWidgetBlueprint* Blueprint,
		const UClass* ExpectedPartEntryClass,
		const bool bLogErrors)
	{
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass())
			&& Blueprint->BlueprintDescription.Contains(ContractMarker)
			&& HasWidgetOfClass(Blueprint, TEXT("EnemyNameText"), UTextBlock::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("EnemyInitiativeText"), UTextBlock::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("PartList"), UPanelWidget::StaticClass())
			&& HasWidgetOfClass(Blueprint, TEXT("PanelContextHighlight"), UWidget::StaticClass())
			&& IsTreeNonHitTestable(Blueprint);

		const UWacomBattleEnemyPanelWidget* PanelCDO = Blueprint && Blueprint->GeneratedClass
			? Cast<UWacomBattleEnemyPanelWidget>(Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		bValid &= PanelCDO
			&& PanelCDO->GetPartEntryWidgetClass().Get() == ExpectedPartEntryClass;

		const UBorder* Background = Blueprint && Blueprint->WidgetTree
			? Cast<UBorder>(Blueprint->WidgetTree->FindWidget(TEXT("PanelBackground")))
			: nullptr;
		const UObject* BackgroundResource = Background
			? Background->Background.GetResourceObject()
			: nullptr;
		bValid &= BackgroundResource
			&& BackgroundResource->GetPathName() == PixelPanelTexturePath;

		if (!bValid && bLogErrors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] Panel WBP contract mismatch: %s"),
				Blueprint ? *Blueprint->GetPathName() : PanelObjectPath);
		}
		return bValid;
	}

	EWidgetBlueprintState ClassifyPartEntry(UWidgetBlueprint* Blueprint)
	{
		if (IsLegacyEmptyShell(Blueprint)
			|| (Blueprint && Blueprint->BlueprintDescription.Contains(ContractMarker)
				&& !ValidatePartEntryBlueprint(Blueprint, false)))
		{
			return EWidgetBlueprintState::LegacyEmpty;
		}
		return ValidatePartEntryBlueprint(Blueprint, false)
			? EWidgetBlueprintState::Formal
			: EWidgetBlueprintState::Unknown;
	}

	EWidgetBlueprintState ClassifyPanel(
		UWidgetBlueprint* Blueprint,
		const UClass* ExpectedPartEntryClass)
	{
		if (IsLegacyEmptyShell(Blueprint)
			|| (Blueprint && Blueprint->BlueprintDescription.Contains(ContractMarker)
				&& !ValidatePanelBlueprint(Blueprint, ExpectedPartEntryClass, false)))
		{
			return EWidgetBlueprintState::LegacyEmpty;
		}
		return ValidatePanelBlueprint(Blueprint, ExpectedPartEntryClass, false)
			? EWidgetBlueprintState::Formal
			: EWidgetBlueprintState::Unknown;
	}

	bool BuildPartEntryBlueprint(UWidgetBlueprint* Blueprint)
	{
		UClass* StatusListClass = LoadClass<UWacomBattleStatusIconListWidget>(
			nullptr, StatusListClassPath);
		if (!StatusListClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] Missing formal status-list class: %s"),
				StatusListClassPath);
			return false;
		}

		ResetWidgetBlueprint(
			Blueprint,
			TEXT("Scene Enemy Panel 的被动部位条目。布局和语义动画由 WBP 拥有。"));
		UWidgetTree* Tree = Blueprint->WidgetTree;

		USizeBox* Root = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("PartEntryRoot"));
		Root->SetMinDesiredWidth(416.0f);
		Root->SetMinDesiredHeight(58.0f);
		Tree->RootWidget = Root;

		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("PartEntryOverlay"));
		Root->AddChild(Overlay);

		UBorder* Background = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("EntryBackground"));
		Background->SetBrushColor(EntryInk);
		Background->SetPadding(FMargin(0.0f));
		Overlay->AddChildToOverlay(Background);

		auto AddOverlaySurface = [Tree, Overlay](
			const FName Name,
			const FLinearColor& Color,
			const ESlateVisibility Visibility,
			const float RenderOpacity = 1.0f)
		{
			UBorder* Surface = Tree->ConstructWidget<UBorder>(UBorder::StaticClass(), Name);
			Surface->SetBrushColor(Color);
			Surface->SetVisibility(Visibility);
			Surface->SetRenderOpacity(RenderOpacity);
			if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(Surface))
			{
				Slot->SetHorizontalAlignment(HAlign_Fill);
				Slot->SetVerticalAlignment(VAlign_Fill);
			}
			return Surface;
		};

		UBorder* ContextHighlight = AddOverlaySurface(
			TEXT("ContextHighlight"), FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.18f),
			ESlateVisibility::Collapsed, 0.0f);
		UBorder* ActionPreviewOverlay = AddOverlaySurface(
			TEXT("ActionPreviewOverlay"), FLinearColor(Amber.R, Amber.G, Amber.B, 0.12f),
			ESlateVisibility::Collapsed);
		UBorder* DamagePulseSurface = AddOverlaySurface(
			TEXT("DamagePulseSurface"), FLinearColor(Damage.R, Damage.G, Damage.B, 0.42f),
			ESlateVisibility::HitTestInvisible, 0.0f);
		UBorder* ShieldPulseSurface = AddOverlaySurface(
			TEXT("ShieldPulseSurface"), FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.36f),
			ESlateVisibility::HitTestInvisible, 0.0f);
		UBorder* DestroyedOverlay = AddOverlaySurface(
			TEXT("DestroyedOverlay"), FLinearColor(0.20f, 0.025f, 0.025f, 0.72f),
			ESlateVisibility::Collapsed, 0.65f);
		MarkWidgetVariable(Blueprint, ContextHighlight);
		MarkWidgetVariable(Blueprint, ActionPreviewOverlay);
		MarkWidgetVariable(Blueprint, DestroyedOverlay);

		UBorder* EntryContent = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("EntryContent"));
		EntryContent->SetBrushColor(FLinearColor::Transparent);
		EntryContent->SetPadding(FMargin(10.0f, 7.0f, 10.0f, 7.0f));
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(EntryContent))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Rows = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PartRows"));
		EntryContent->SetContent(Rows);

		UHorizontalBox* CompactRow = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("CompactRow"));
		Rows->AddChildToVerticalBox(CompactRow);

		UTextBlock* PartName = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("PartNameText"));
		StyleText(PartName, FText::FromString(TEXT("部位")), 15, Paper, TEXT("Bold"));
		MarkWidgetVariable(Blueprint, PartName);
		if (UHorizontalBoxSlot* Slot = CompactRow->AddChildToHorizontalBox(PartName))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* HpBarSize = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("HpBarSize"));
		HpBarSize->SetWidthOverride(82.0f);
		HpBarSize->SetHeightOverride(10.0f);
		if (UHorizontalBoxSlot* Slot = CompactRow->AddChildToHorizontalBox(HpBarSize))
		{
			Slot->SetPadding(FMargin(7.0f, 0.0f, 5.0f, 0.0f));
			Slot->SetVerticalAlignment(VAlign_Center);
		}
		UProgressBar* HpBar = Tree->ConstructWidget<UProgressBar>(
			UProgressBar::StaticClass(), TEXT("HpBar"));
		HpBar->SetPercent(1.0f);
		HpBar->SetFillColorAndOpacity(FLinearColor(0.82f, 0.18f, 0.16f, 1.0f));
		HpBarSize->AddChild(HpBar);
		MarkWidgetVariable(Blueprint, HpBar);

		UTextBlock* HpText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("HpText"));
		StyleText(HpText, FText::FromString(TEXT("24/24")), 13, Paper, TEXT("Bold"));
		MarkWidgetVariable(Blueprint, HpText);
		CompactRow->AddChildToHorizontalBox(HpText)->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));

		UHorizontalBox* ShieldContainer = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("ShieldContainer"));
		ShieldContainer->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, ShieldContainer);
		CompactRow->AddChildToHorizontalBox(ShieldContainer)->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
		UTextBlock* ShieldLabel = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ShieldLabel"));
		StyleText(ShieldLabel, FText::FromString(TEXT("SH ")), 12, Cyan, TEXT("Bold"));
		ShieldContainer->AddChildToHorizontalBox(ShieldLabel);
		UTextBlock* ShieldText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ShieldText"));
		StyleText(ShieldText, FText::AsNumber(0), 12, Cyan, TEXT("Bold"));
		ShieldContainer->AddChildToHorizontalBox(ShieldText);
		MarkWidgetVariable(Blueprint, ShieldText);

		UTextBlock* InitiativeLabel = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("InitiativeLabel"));
		StyleText(InitiativeLabel, FText::FromString(TEXT("I")), 11, Muted, TEXT("Bold"));
		CompactRow->AddChildToHorizontalBox(InitiativeLabel)->SetPadding(FMargin(0.0f, 0.0f, 3.0f, 0.0f));
		UTextBlock* InitiativeText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("InitiativeText"));
		StyleText(InitiativeText, FText::AsNumber(0), 13, Amber, TEXT("Bold"));
		CompactRow->AddChildToHorizontalBox(InitiativeText)->SetPadding(FMargin(0.0f, 0.0f, 7.0f, 0.0f));
		MarkWidgetVariable(Blueprint, InitiativeText);

		UTextBlock* IntentText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("IntentText"));
		StyleText(IntentText, FText::FromString(TEXT("意图")), 12, Amber);
		IntentText->SetJustification(ETextJustify::Right);
		MarkWidgetVariable(Blueprint, IntentText);
		if (UHorizontalBoxSlot* Slot = CompactRow->AddChildToHorizontalBox(IntentText))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		UHorizontalBox* Details = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("DetailsContainer"));
		Details->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, Details);
		Rows->AddChildToVerticalBox(Details)->SetPadding(FMargin(0.0f, 5.0f, 0.0f, 0.0f));

		UTextBlock* ResistanceText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("ResistanceText"));
		StyleText(ResistanceText, FText::FromString(TEXT("RES 0")), 12, Muted, TEXT("Bold"));
		MarkWidgetVariable(Blueprint, ResistanceText);
		Details->AddChildToHorizontalBox(ResistanceText)->SetPadding(FMargin(0.0f, 0.0f, 8.0f, 0.0f));

		UWacomBattleStatusIconListWidget* StatusList =
			Tree->ConstructWidget<UWacomBattleStatusIconListWidget>(
				StatusListClass, TEXT("StatusList"));
		StatusList->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, StatusList);
		if (UHorizontalBoxSlot* Slot = Details->AddChildToHorizontalBox(StatusList))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		if (!AddOpacityAnimation(Blueprint, TEXT("IntroAnimation"), EntryContent,
			{{ 0.0f, 0.0f }, { 0.18f, 1.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("DamagePulseAnimation"), DamagePulseSurface,
				{{ 0.0f, 0.0f }, { 0.06f, 1.0f }, { 0.22f, 0.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("ShieldPulseAnimation"), ShieldPulseSurface,
				{{ 0.0f, 0.0f }, { 0.07f, 1.0f }, { 0.24f, 0.0f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("DestroyedPulseAnimation"), DestroyedOverlay,
				{{ 0.0f, 0.25f }, { 0.08f, 1.0f }, { 0.32f, 0.65f }})
			|| !AddOpacityAnimation(Blueprint, TEXT("ContextHighlightAnimation"), ContextHighlight,
				{{ 0.0f, 0.0f }, { 0.12f, 1.0f }}))
		{
			return false;
		}

		return CompileAndSave(Blueprint);
	}

	bool BuildPanelBlueprint(UWidgetBlueprint* Blueprint, UClass* PartEntryClass)
	{
		UTexture2D* PixelPanelTexture = LoadObject<UTexture2D>(nullptr, PixelPanelTexturePath);
		if (!PixelPanelTexture || !PartEntryClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] Missing panel texture or PartEntry class"));
			return false;
		}

		ResetWidgetBlueprint(
			Blueprint,
			TEXT("每个 Scene Enemy Host 的被动 Screen-space 面板。布局和皮肤由 WBP 拥有。"));
		UWidgetTree* Tree = Blueprint->WidgetTree;

		USizeBox* Root = Tree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("EnemyPanelRoot"));
		Root->SetMinDesiredWidth(440.0f);
		Tree->RootWidget = Root;

		UOverlay* Overlay = Tree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("EnemyPanelOverlay"));
		Root->AddChild(Overlay);

		UBorder* PanelBackground = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("PanelBackground"));
		FSlateBrush PanelBrush;
		PanelBrush.SetResourceObject(PixelPanelTexture);
		PanelBrush.DrawAs = ESlateBrushDrawType::Box;
		PanelBrush.Margin = FMargin(0.055f, 0.18f, 0.055f, 0.18f);
		PanelBrush.ImageSize = FVector2D(512.0f, 160.0f);
		PanelBackground->SetBrush(PanelBrush);
		PanelBackground->SetBrushColor(FLinearColor::White);
		Overlay->AddChildToOverlay(PanelBackground);

		UBorder* ContentPadding = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("PanelContentPadding"));
		ContentPadding->SetBrushColor(FLinearColor::Transparent);
		ContentPadding->SetPadding(FMargin(18.0f, 14.0f, 18.0f, 16.0f));
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(ContentPadding))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UVerticalBox* Content = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PanelContent"));
		ContentPadding->SetContent(Content);

		UHorizontalBox* Header = Tree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("EnemyHeader"));
		Content->AddChildToVerticalBox(Header)->SetPadding(FMargin(2.0f, 0.0f, 2.0f, 8.0f));

		UTextBlock* EnemyNameText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EnemyNameText"));
		StyleText(EnemyNameText, FText::FromString(TEXT("敌人")), 18, Paper, TEXT("Bold"));
		MarkWidgetVariable(Blueprint, EnemyNameText);
		if (UHorizontalBoxSlot* Slot = Header->AddChildToHorizontalBox(EnemyNameText))
		{
			Slot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		UTextBlock* EnemyInitiativeText = Tree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("EnemyInitiativeText"));
		StyleText(EnemyInitiativeText, FText::FromString(TEXT("INIT 0")), 13, Amber, TEXT("Bold"));
		EnemyInitiativeText->SetJustification(ETextJustify::Right);
		MarkWidgetVariable(Blueprint, EnemyInitiativeText);
		Header->AddChildToHorizontalBox(EnemyInitiativeText);

		UVerticalBox* PartList = Tree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PartList"));
		MarkWidgetVariable(Blueprint, PartList);
		Content->AddChildToVerticalBox(PartList);

		UBorder* PanelContextHighlight = Tree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("PanelContextHighlight"));
		PanelContextHighlight->SetBrushColor(
			FLinearColor(Cyan.R, Cyan.G, Cyan.B, 0.08f));
		PanelContextHighlight->SetVisibility(ESlateVisibility::Collapsed);
		MarkWidgetVariable(Blueprint, PanelContextHighlight);
		if (UOverlaySlot* Slot = Overlay->AddChildToOverlay(PanelContextHighlight))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		if (!CompileAndSave(Blueprint))
		{
			return false;
		}

		UWacomBattleEnemyPanelWidget* PanelCDO =
			Cast<UWacomBattleEnemyPanelWidget>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!PanelCDO)
		{
			return false;
		}
		PanelCDO->SetPartEntryWidgetClass(PartEntryClass);
		return SaveCompiledDefaults(Blueprint);
	}
}

bool Wacom::ContentBuilder::ProcessEnemyUIWidgetBlueprints(
	const bool bMigrateLegacy,
	const bool bInspectOnly)
{
	if (bMigrateLegacy == bInspectOnly)
	{
		return false;
	}

	FEnemyUIAssets Assets = LoadAssets();
	if (!Assets.Panel || !Assets.PartEntry)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemyUIWidgetBlueprintBuilder] Both recognized legacy assets must already exist"));
		return false;
	}

	const EWidgetBlueprintState PartState = ClassifyPartEntry(Assets.PartEntry);
	UClass* ExistingPartClass = Assets.PartEntry->GeneratedClass;
	const EWidgetBlueprintState PanelState = ClassifyPanel(Assets.Panel, ExistingPartClass);
	if (PartState == EWidgetBlueprintState::Unknown
		|| PanelState == EWidgetBlueprintState::Unknown)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemyUIWidgetBlueprintBuilder] Unknown non-empty manual layout detected; no asset was modified"));
		return false;
	}

	if (bInspectOnly)
	{
		if (PartState != EWidgetBlueprintState::Formal
			|| PanelState != EWidgetBlueprintState::Formal)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyUIWidgetBlueprintBuilder] Legacy empty shell requires -MigrateLegacy"));
			return false;
		}
		return ValidatePartEntryBlueprint(Assets.PartEntry, true)
			&& ValidatePanelBlueprint(Assets.Panel, Assets.PartEntry->GeneratedClass, true);
	}

	if (PartState == EWidgetBlueprintState::LegacyEmpty)
	{
		if (!BuildPartEntryBlueprint(Assets.PartEntry))
		{
			return false;
		}
	}
	if (!Assets.PartEntry->GeneratedClass)
	{
		return false;
	}

	if (PanelState == EWidgetBlueprintState::LegacyEmpty)
	{
		if (!BuildPanelBlueprint(Assets.Panel, Assets.PartEntry->GeneratedClass))
		{
			return false;
		}
	}

	const bool bValid = ValidatePartEntryBlueprint(Assets.PartEntry, true)
		&& ValidatePanelBlueprint(Assets.Panel, Assets.PartEntry->GeneratedClass, true);
	if (bValid && PartState == EWidgetBlueprintState::Formal
		&& PanelState == EWidgetBlueprintState::Formal)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[EnemyUIWidgetBlueprintBuilder] Assets already satisfy the formal contract; no packages dirtied"));
	}
	return bValid;
}
