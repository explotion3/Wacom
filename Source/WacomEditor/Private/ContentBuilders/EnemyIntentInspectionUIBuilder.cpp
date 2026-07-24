// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemyIntentInspectionUIBuilder.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "ContentBuilders/EnemyUIHitTestPolicy.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
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
#include "Misc/DataValidation.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleIntentEffectRowWidget.h"
#include "UI/Battle/WacomBattleIntentTooltipWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR IntentAssetRoot[] = TEXT("/Game/Wacom/UI/Enemy/Intent");
	constexpr TCHAR TooltipAssetName[] = TEXT("WBP_BattleIntentTooltip");
	constexpr TCHAR EffectRowAssetName[] = TEXT("WBP_BattleIntentEffectRow");
	constexpr TCHAR EntryObjectPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget");
	constexpr TCHAR InspectionObjectPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget.WBP_WacomBattleEnemyInspectionWidget");
	constexpr TCHAR StyleObjectPath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default.DA_EnemyIntentPresentation_Default");
	constexpr TCHAR AttackTexturePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Attack.T_UI_EnemyIntent_Attack");
	constexpr TCHAR GuardTexturePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Guard.T_UI_EnemyIntent_Guard");
	constexpr TCHAR FallbackTexturePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Fallback.T_UI_EnemyIntent_Fallback");
	constexpr TCHAR ContractMarker[] =
		TEXT("WacomEnemyIntentInspection.ContractVersion=1");

	struct FWidgetBlueprintAsset
	{
		UWidgetBlueprint* Blueprint = nullptr;
		FString PackagePath;
		bool bCreated = false;
	};

	FString MakePackagePath(const TCHAR* AssetName)
	{
		return FString::Printf(TEXT("%s/%s"), IntentAssetRoot, AssetName);
	}

	FString MakeObjectPath(const FString& PackagePath)
	{
		return FString::Printf(
			TEXT("%s.%s"),
			*PackagePath,
			*FPackageName::GetLongPackageAssetName(PackagePath));
	}

	UPackage* FindOrCreatePackage(const FString& PackagePath)
	{
		return CreatePackage(*PackagePath);
	}

	void RegisterWidgetGuid(
		UWidgetBlueprint& Blueprint,
		const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"),
			*Blueprint.GetPathName(),
			*Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(
		UWidgetBlueprint& Blueprint,
		UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void RegisterAllWidgetGuids(UWidgetBlueprint& Blueprint)
	{
		TSet<FName> LiveNames;
		if (Blueprint.WidgetTree)
		{
			Blueprint.WidgetTree->ForEachWidget(
				[&](UWidget* Widget)
				{
					if (Widget)
					{
						LiveNames.Add(Widget->GetFName());
						RegisterWidgetGuid(Blueprint, *Widget);
					}
				});
		}
		for (const UWidgetAnimation* Animation : Blueprint.Animations)
		{
			if (Animation)
			{
				LiveNames.Add(Animation->GetFName());
				const FString StablePath = FString::Printf(
					TEXT("%s:%s"),
					*Blueprint.GetPathName(),
					*Animation->GetName());
				Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(
					Animation->GetFName()) =
						FGuid::NewDeterministicGuid(StablePath);
			}
		}
		for (auto It = Blueprint.WidgetVariableNameToGuidMap.CreateIterator();
			It; ++It)
		{
			if (!LiveNames.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}

	bool HasMissingVariableGuids(const UWidgetBlueprint& Blueprint)
	{
		bool bMissing = false;
		if (Blueprint.WidgetTree)
		{
			Blueprint.WidgetTree->ForEachWidget(
				[&](UWidget* Widget)
				{
					bMissing |= Widget
						&& !Blueprint.WidgetVariableNameToGuidMap.Contains(
							Widget->GetFName());
				});
		}
		for (const UWidgetAnimation* Animation : Blueprint.Animations)
		{
			bMissing |= Animation
				&& !Blueprint.WidgetVariableNameToGuidMap.Contains(
					Animation->GetFName());
		}
		return bMissing;
	}

	FWidgetBlueprintAsset LoadOrCreateWidgetBlueprint(
		const TCHAR* AssetName,
		UClass* ParentClass,
		const bool bAllowCreate)
	{
		FWidgetBlueprintAsset Result;
		Result.PackagePath = MakePackagePath(AssetName);
		const FString ObjectPath = MakeObjectPath(Result.PackagePath);
		if (UObject* Existing = StaticLoadObject(
			UObject::StaticClass(), nullptr, *ObjectPath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(Existing);
			if (!Result.Blueprint
				|| !Result.Blueprint->ParentClass
				|| !Result.Blueprint->ParentClass->IsChildOf(ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemyIntentUIBuilder] Incompatible WBP: %s"),
					*ObjectPath);
				Result.Blueprint = nullptr;
			}
			return Result;
		}
		if (!bAllowCreate)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] Missing WBP: %s"),
				*ObjectPath);
			return Result;
		}
		if (UPackage* Package = FindOrCreatePackage(Result.PackagePath))
		{
			Result.Blueprint = Cast<UWidgetBlueprint>(
				FKismetEditorUtilities::CreateBlueprint(
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

	void ResetWidgetBlueprint(
		UWidgetBlueprint& Blueprint,
		const TCHAR* Description)
	{
		Blueprint.Modify();
		if (UWidgetTree* PreviousTree = Blueprint.WidgetTree)
		{
			PreviousTree->Rename(
				nullptr,
				GetTransientPackage(),
				REN_DontCreateRedirectors | REN_NonTransactional);
		}
		Blueprint.WidgetTree = NewObject<UWidgetTree>(
			&Blueprint, TEXT("WidgetTree"), RF_Transactional);
		Blueprint.Bindings.Reset();
		Blueprint.Animations.Reset();
		Blueprint.WidgetVariableNameToGuidMap.Reset();
		Blueprint.BlueprintDescription =
			FString(Description) + TEXT("\n") + ContractMarker;
		Blueprint.bCanCallInitializedWithoutPlayerContext = true;
	}

	bool SaveAssetPackage(UPackage& Package, UObject& Asset)
	{
		Package.MarkPackageDirty();
		Asset.MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package.GetName(),
			FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(&Package, &Asset, *Filename, Args);
	}

	bool CompileAndSave(UWidgetBlueprint& Blueprint)
	{
		RegisterAllWidgetGuids(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
		FKismetEditorUtilities::CompileBlueprint(&Blueprint);
		if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] Compile failed: %s"),
				*Blueprint.GetPathName());
			return false;
		}
		FAssetRegistryModule::AssetCreated(&Blueprint);
		return SaveAssetPackage(*Blueprint.GetOutermost(), Blueprint);
	}

	void ConfigureText(
		UTextBlock& Text,
		const int32 Size,
		const FLinearColor Color,
		const bool bWrap = true)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = Size;
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetAutoWrapText(bWrap);
	}

	FButtonStyle MakeTransparentButtonStyle()
	{
		FButtonStyle Style;
		Style.Normal.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Hovered.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Pressed.DrawAs = ESlateBrushDrawType::NoDrawType;
		Style.Disabled.DrawAs = ESlateBrushDrawType::NoDrawType;
		return Style;
	}

	void BuildEffectRowTree(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(
			Blueprint,
			TEXT("Enemy Intent effect row. Deterministically generated."));
		UHorizontalBox* Root =
			Blueprint.WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(), TEXT("Root"));
		Root->SetVisibility(ESlateVisibility::HitTestInvisible);
		Blueprint.WidgetTree->RootWidget = Root;

		USizeBox* IconBox =
			Blueprint.WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(), TEXT("EffectIconBox"));
		IconBox->SetWidthOverride(24.0f);
		IconBox->SetHeightOverride(24.0f);
		IconBox->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* WidgetSlot =
			Root->AddChildToHorizontalBox(IconBox))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 2.0f, 8.0f, 2.0f));
			WidgetSlot->SetVerticalAlignment(VAlign_Top);
		}
		UImage* EffectIcon =
			Blueprint.WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(), TEXT("EffectIcon"));
		EffectIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconBox->AddChild(EffectIcon);
		MarkWidgetVariable(Blueprint, *EffectIcon);

		UVerticalBox* Copy =
			Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(), TEXT("Copy"));
		Copy->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* WidgetSlot =
			Root->AddChildToHorizontalBox(Copy))
		{
			WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}

		UHorizontalBox* Summary =
			Blueprint.WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(), TEXT("Summary"));
		Summary->SetVisibility(ESlateVisibility::HitTestInvisible);
		Copy->AddChildToVerticalBox(Summary);
		UTextBlock* TargetText =
			Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("TargetText"));
		ConfigureText(*TargetText, 14, FLinearColor(0.78f, 0.84f, 0.90f, 1.0f));
		TargetText->SetVisibility(ESlateVisibility::HitTestInvisible);
		Summary->AddChildToHorizontalBox(TargetText);
		MarkWidgetVariable(Blueprint, *TargetText);
		UTextBlock* EffectText =
			Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("EffectText"));
		ConfigureText(*EffectText, 14, FLinearColor::White);
		EffectText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* WidgetSlot =
			Summary->AddChildToHorizontalBox(EffectText))
		{
			WidgetSlot->SetPadding(FMargin(6.0f, 0.0f, 0.0f, 0.0f));
		}
		MarkWidgetVariable(Blueprint, *EffectText);
		UTextBlock* CoreRuleText =
			Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("CoreRuleText"));
		ConfigureText(
			*CoreRuleText,
			11,
			FLinearColor(0.64f, 0.72f, 0.80f, 1.0f));
		CoreRuleText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UVerticalBoxSlot* WidgetSlot =
			Copy->AddChildToVerticalBox(CoreRuleText))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 2.0f, 0.0f, 4.0f));
		}
		MarkWidgetVariable(Blueprint, *CoreRuleText);
	}

	void BuildTooltipTree(UWidgetBlueprint& Blueprint)
	{
		ResetWidgetBlueprint(
			Blueprint,
			TEXT("Enemy Intent mouse tooltip. Deterministically generated."));
		USizeBox* TooltipRoot =
			Blueprint.WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(), TEXT("TooltipRoot"));
		TooltipRoot->SetWidthOverride(320.0f);
		TooltipRoot->SetVisibility(ESlateVisibility::HitTestInvisible);
		Blueprint.WidgetTree->RootWidget = TooltipRoot;

		UBorder* Surface =
			Blueprint.WidgetTree->ConstructWidget<UBorder>(
				UBorder::StaticClass(), TEXT("Surface"));
		Surface->SetBrushColor(FLinearColor(0.025f, 0.055f, 0.085f, 0.97f));
		Surface->SetPadding(FMargin(14.0f, 12.0f));
		Surface->SetVisibility(ESlateVisibility::HitTestInvisible);
		TooltipRoot->AddChild(Surface);
		UVerticalBox* Content =
			Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(), TEXT("Content"));
		Content->SetVisibility(ESlateVisibility::HitTestInvisible);
		Surface->SetContent(Content);

		UHorizontalBox* Header =
			Blueprint.WidgetTree->ConstructWidget<UHorizontalBox>(
				UHorizontalBox::StaticClass(), TEXT("Header"));
		Header->SetVisibility(ESlateVisibility::HitTestInvisible);
		Content->AddChildToVerticalBox(Header);
		USizeBox* IconBox =
			Blueprint.WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(), TEXT("IntentIconBox"));
		IconBox->SetWidthOverride(32.0f);
		IconBox->SetHeightOverride(32.0f);
		Header->AddChildToHorizontalBox(IconBox);
		UImage* IntentIcon =
			Blueprint.WidgetTree->ConstructWidget<UImage>(
				UImage::StaticClass(), TEXT("IntentIcon"));
		IntentIcon->SetVisibility(ESlateVisibility::HitTestInvisible);
		IconBox->AddChild(IntentIcon);
		MarkWidgetVariable(Blueprint, *IntentIcon);
		UVerticalBox* HeaderCopy =
			Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(), TEXT("HeaderCopy"));
		HeaderCopy->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UHorizontalBoxSlot* WidgetSlot =
			Header->AddChildToHorizontalBox(HeaderCopy))
		{
			WidgetSlot->SetPadding(FMargin(10.0f, 0.0f, 0.0f, 0.0f));
			WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UTextBlock* IntentNameText =
			Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("IntentNameText"));
		ConfigureText(
			*IntentNameText,
			17,
			FLinearColor(0.94f, 0.96f, 1.0f, 1.0f),
			false);
		IntentNameText->SetVisibility(ESlateVisibility::HitTestInvisible);
		HeaderCopy->AddChildToVerticalBox(IntentNameText);
		MarkWidgetVariable(Blueprint, *IntentNameText);
		UTextBlock* IntentMetaText =
			Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("IntentMetaText"));
		ConfigureText(
			*IntentMetaText,
			12,
			FLinearColor(0.72f, 0.80f, 0.88f, 1.0f),
			false);
		IntentMetaText->SetVisibility(ESlateVisibility::HitTestInvisible);
		HeaderCopy->AddChildToVerticalBox(IntentMetaText);
		MarkWidgetVariable(Blueprint, *IntentMetaText);

		UVerticalBox* EffectsList =
			Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(), TEXT("EffectsList"));
		EffectsList->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UVerticalBoxSlot* WidgetSlot =
			Content->AddChildToVerticalBox(EffectsList))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 10.0f, 0.0f, 0.0f));
		}
		MarkWidgetVariable(Blueprint, *EffectsList);
		UTextBlock* OverflowText =
			Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("OverflowText"));
		ConfigureText(
			*OverflowText,
			11,
			FLinearColor(0.70f, 0.76f, 0.82f, 1.0f));
		OverflowText->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (UVerticalBoxSlot* WidgetSlot =
			Content->AddChildToVerticalBox(OverflowText))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 8.0f, 0.0f, 0.0f));
		}
		MarkWidgetVariable(Blueprint, *OverflowText);
	}

	UVerticalBox* FindVerticalBoxAncestor(UWidget* Widget)
	{
		for (UWidget* Current = Widget ? Widget->GetParent() : nullptr;
			Current;
			Current = Current->GetParent())
		{
			if (UVerticalBox* Vertical = Cast<UVerticalBox>(Current))
			{
				return Vertical;
			}
		}
		return nullptr;
	}

	UWidget* FindDirectChildUnder(
		UWidget* Descendant,
		UPanelWidget* Ancestor)
	{
		UWidget* Current = Descendant;
		while (Current && Current->GetParent() != Ancestor)
		{
			Current = Current->GetParent();
		}
		return Current;
	}

	bool EnsurePaintsAbove(
		UWidgetBlueprint& Blueprint,
		UWidget& Foreground,
		UWidget& Background,
		const bool bBuild,
		bool& bOutChanged)
	{
		for (UPanelWidget* Parent = Foreground.GetParent();
			Parent;
			Parent = Parent->GetParent())
		{
			UWidget* ForegroundBranch =
				FindDirectChildUnder(&Foreground, Parent);
			UWidget* BackgroundBranch =
				FindDirectChildUnder(&Background, Parent);
			if (!ForegroundBranch || !BackgroundBranch
				|| ForegroundBranch == BackgroundBranch)
			{
				continue;
			}

			const int32 ForegroundIndex =
				Parent->GetChildIndex(ForegroundBranch);
			const int32 BackgroundIndex =
				Parent->GetChildIndex(BackgroundBranch);
			if (ForegroundIndex > BackgroundIndex)
			{
				return true;
			}
			if (!bBuild)
			{
				return false;
			}

			Blueprint.Modify();
			Parent->Modify();
			Parent->ShiftChild(
				Parent->GetChildrenCount() - 1,
				ForegroundBranch);
			bOutChanged = true;
			return Parent->GetChildIndex(ForegroundBranch)
				> Parent->GetChildIndex(BackgroundBranch);
		}

		return true;
	}

	bool HasIconOwnedIntentTooltipTarget(const UWidgetBlueprint* Blueprint)
	{
		const UButton* Target = Blueprint && Blueprint->WidgetTree
			? Cast<UButton>(
				Blueprint->WidgetTree->FindWidget(TEXT("IntentTooltipTarget")))
			: nullptr;
		const UImage* IntentIcon = Blueprint && Blueprint->WidgetTree
			? Cast<UImage>(Blueprint->WidgetTree->FindWidget(TEXT("IntentIcon")))
			: nullptr;
		return Target && IntentIcon
			&& Target->GetContent() == IntentIcon
			&& IntentIcon->GetParent() == Target;
	}

	UButton* EnsureIntentTooltipTarget(
		UWidgetBlueprint& Blueprint,
		bool& bOutChanged)
	{
		UImage* IntentIcon = Cast<UImage>(
			Blueprint.WidgetTree->FindWidget(TEXT("IntentIcon")));
		UButton* Target = Cast<UButton>(
			Blueprint.WidgetTree->FindWidget(TEXT("IntentTooltipTarget")));
		if (!IntentIcon)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] Missing IntentIcon: %s"),
				*Blueprint.GetPathName());
			return nullptr;
		}
		if (Target
			&& Target->GetContent() == IntentIcon
			&& IntentIcon->GetParent() == Target)
		{
			return Target;
		}

		UPanelWidget* IntentParent = IntentIcon->GetParent();
		if (!IntentParent)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] IntentIcon has no parent: %s"),
				*Blueprint.GetPathName());
			return nullptr;
		}

		Blueprint.Modify();
		IntentParent->Modify();
		IntentIcon->Modify();
		if (!Target)
		{
			Target = Blueprint.WidgetTree->ConstructWidget<UButton>(
				UButton::StaticClass(), TEXT("IntentTooltipTarget"));
			MarkWidgetVariable(Blueprint, *Target);
		}
		else
		{
			Target->Modify();
			Target->RemoveFromParent();
			Target->ClearChildren();
		}

PRAGMA_DISABLE_DEPRECATION_WARNINGS
		Target->IsFocusable = false;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
		Target->SetStyle(MakeTransparentButtonStyle());
		Target->SetVisibility(ESlateVisibility::HitTestInvisible);
		if (!IntentParent->ReplaceChild(IntentIcon, Target))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] Failed to replace IntentIcon slot: %s"),
				*Blueprint.GetPathName());
			return nullptr;
		}
		IntentIcon->Slot = nullptr;
		UButtonSlot* IconSlot = Cast<UButtonSlot>(Target->AddChild(IntentIcon));
		if (!IconSlot)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] Failed to wrap IntentIcon: %s"),
				*Blueprint.GetPathName());
			return nullptr;
		}
		IconSlot->SetPadding(FMargin(0.0f));
		IconSlot->SetHorizontalAlignment(HAlign_Fill);
		IconSlot->SetVerticalAlignment(VAlign_Fill);
		bOutChanged = true;
		return Target;
	}

	UPanelWidget* EnsureInspectionEffectList(UWidgetBlueprint& Blueprint)
	{
		if (UPanelWidget* Existing = Cast<UPanelWidget>(
			Blueprint.WidgetTree->FindWidget(TEXT("IntentEffectsList"))))
		{
			return Existing;
		}
		UWidget* StatusList =
			Blueprint.WidgetTree->FindWidget(TEXT("StatusList"));
		UVerticalBox* Details = FindVerticalBoxAncestor(StatusList);
		if (!Details)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyIntentUIBuilder] StatusList has no vertical details ancestor"));
			return nullptr;
		}
		USizeBox* ScrollBounds =
			Blueprint.WidgetTree->ConstructWidget<USizeBox>(
				USizeBox::StaticClass(), TEXT("IntentEffectsBounds"));
		ScrollBounds->SetMaxDesiredHeight(190.0f);
		ScrollBounds->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		if (UVerticalBoxSlot* WidgetSlot =
			Details->AddChildToVerticalBox(ScrollBounds))
		{
			WidgetSlot->SetPadding(FMargin(0.0f, 12.0f, 0.0f, 0.0f));
			WidgetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		}
		UScrollBox* Scroll =
			Blueprint.WidgetTree->ConstructWidget<UScrollBox>(
				UScrollBox::StaticClass(), TEXT("IntentEffectsScroll"));
		Scroll->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		ScrollBounds->AddChild(Scroll);
		UVerticalBox* List =
			Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
				UVerticalBox::StaticClass(), TEXT("IntentEffectsList"));
		List->SetVisibility(ESlateVisibility::HitTestInvisible);
		Scroll->AddChild(List);
		MarkWidgetVariable(Blueprint, *List);
		return List;
	}

	bool SetBrushTexture(
		FSlateBrush& Brush,
		UObject* Resource,
		const FVector2f ImageSize)
	{
		const bool bChanged = Brush.GetResourceObject() != Resource
			|| Brush.GetImageSize() != ImageSize
			|| Brush.DrawAs != ESlateBrushDrawType::Image;
		if (bChanged)
		{
			Brush.SetResourceObject(Resource);
			Brush.SetImageSize(ImageSize);
			Brush.DrawAs = ESlateBrushDrawType::Image;
		}
		return bChanged;
	}

	bool ConfigureStyle(
		UWacomBattleEnemyIntentPresentationStyle& Style,
		const bool bSave)
	{
		UObject* Attack = StaticLoadObject(
			UTexture2D::StaticClass(), nullptr, AttackTexturePath);
		UObject* Guard = StaticLoadObject(
			UTexture2D::StaticClass(), nullptr, GuardTexturePath);
		UObject* Fallback = StaticLoadObject(
			UTexture2D::StaticClass(), nullptr, FallbackTexturePath);
		if (!Attack || !Guard || !Fallback)
		{
			return false;
		}
		bool bChanged = false;
		bChanged |= SetBrushTexture(
			Style.DamageEffectIconBrush, Attack, FVector2f(24.0f));
		bChanged |= SetBrushTexture(
			Style.ShieldEffectIconBrush, Guard, FVector2f(24.0f));
		bChanged |= SetBrushTexture(
			Style.FallbackEffectIconBrush, Fallback, FVector2f(24.0f));
		if (bChanged && bSave)
		{
			Style.Modify();
			return SaveAssetPackage(*Style.GetOutermost(), Style);
		}
		return !bChanged || bSave;
	}

	bool HasWidget(
		const UWidgetBlueprint* Blueprint,
		const FName Name,
		const UClass* Class)
	{
		const UWidget* Widget = Blueprint && Blueprint->WidgetTree
			? Blueprint->WidgetTree->FindWidget(Name)
			: nullptr;
		return Widget && Widget->IsA(Class);
	}

	bool ValidateTooltipBlueprint(
		const UWidgetBlueprint* Blueprint,
		const UClass* EffectRowClass)
	{
		if (!Blueprint || !Blueprint->GeneratedClass
			|| !Blueprint->ParentClass
			|| !Blueprint->ParentClass->IsChildOf(
				UWacomBattleIntentTooltipWidget::StaticClass()))
		{
			return false;
		}
		const UWacomBattleIntentTooltipWidget* Defaults =
			Cast<UWacomBattleIntentTooltipWidget>(
				Blueprint->GeneratedClass->GetDefaultObject());
		return Blueprint->BlueprintDescription.Contains(ContractMarker)
			&& HasWidget(Blueprint, TEXT("IntentIcon"), UImage::StaticClass())
			&& HasWidget(Blueprint, TEXT("IntentNameText"), UTextBlock::StaticClass())
			&& HasWidget(Blueprint, TEXT("IntentMetaText"), UTextBlock::StaticClass())
			&& HasWidget(Blueprint, TEXT("EffectsList"), UPanelWidget::StaticClass())
			&& HasWidget(Blueprint, TEXT("OverflowText"), UTextBlock::StaticClass())
			&& Defaults
			&& Defaults->GetEffectRowWidgetClass().Get() == EffectRowClass;
	}

	bool ValidateEffectRowBlueprint(const UWidgetBlueprint* Blueprint)
	{
		return Blueprint
			&& Blueprint->GeneratedClass
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(
				UWacomBattleIntentEffectRowWidget::StaticClass())
			&& Blueprint->BlueprintDescription.Contains(ContractMarker)
			&& HasWidget(Blueprint, TEXT("EffectIcon"), UImage::StaticClass())
			&& HasWidget(Blueprint, TEXT("TargetText"), UTextBlock::StaticClass())
			&& HasWidget(Blueprint, TEXT("EffectText"), UTextBlock::StaticClass())
			&& HasWidget(Blueprint, TEXT("CoreRuleText"), UTextBlock::StaticClass());
	}

	bool ValidateExistingBindings(
		const UWidgetBlueprint* Entry,
		const UWidgetBlueprint* Inspection,
		const UClass* TooltipClass,
		const UClass* EffectRowClass)
	{
		const UWacomBattleEnemyPartEntryWidget* EntryDefaults =
			Entry && Entry->GeneratedClass
				? Cast<UWacomBattleEnemyPartEntryWidget>(
					Entry->GeneratedClass->GetDefaultObject())
				: nullptr;
		const UWacomBattleEnemyInspectionWidget* InspectionDefaults =
			Inspection && Inspection->GeneratedClass
				? Cast<UWacomBattleEnemyInspectionWidget>(
					Inspection->GeneratedClass->GetDefaultObject())
				: nullptr;
		return HasWidget(
				Entry, TEXT("IntentTooltipTarget"), UButton::StaticClass())
			&& HasIconOwnedIntentTooltipTarget(Entry)
			&& HasWidget(
				Inspection, TEXT("IntentTooltipTarget"), UButton::StaticClass())
			&& HasIconOwnedIntentTooltipTarget(Inspection)
			&& HasWidget(
				Inspection, TEXT("IntentEffectsList"), UPanelWidget::StaticClass())
			&& EntryDefaults
			&& EntryDefaults->GetIntentTooltipWidgetClass().Get() == TooltipClass
			&& InspectionDefaults
			&& InspectionDefaults->GetIntentPresentationStyle() != nullptr
			&& InspectionDefaults->GetIntentTooltipWidgetClass().Get()
				== TooltipClass
			&& InspectionDefaults->GetIntentEffectRowWidgetClass().Get()
				== EffectRowClass;
	}

	bool Process(const bool bBuild)
	{
		FWidgetBlueprintAsset EffectRow = LoadOrCreateWidgetBlueprint(
			EffectRowAssetName,
			UWacomBattleIntentEffectRowWidget::StaticClass(),
			bBuild);
		FWidgetBlueprintAsset Tooltip = LoadOrCreateWidgetBlueprint(
			TooltipAssetName,
			UWacomBattleIntentTooltipWidget::StaticClass(),
			bBuild);
		UWidgetBlueprint* Entry = Cast<UWidgetBlueprint>(StaticLoadObject(
			UWidgetBlueprint::StaticClass(), nullptr, EntryObjectPath));
		UWidgetBlueprint* Inspection = Cast<UWidgetBlueprint>(StaticLoadObject(
			UWidgetBlueprint::StaticClass(), nullptr, InspectionObjectPath));
		UWacomBattleEnemyIntentPresentationStyle* Style =
			Cast<UWacomBattleEnemyIntentPresentationStyle>(StaticLoadObject(
				UWacomBattleEnemyIntentPresentationStyle::StaticClass(),
				nullptr,
				StyleObjectPath));
		if (!EffectRow.Blueprint || !Tooltip.Blueprint
			|| !Entry || !Inspection || !Style)
		{
			return false;
		}

		if (!ValidateEffectRowBlueprint(EffectRow.Blueprint))
		{
			if (!bBuild)
			{
				return false;
			}
			BuildEffectRowTree(*EffectRow.Blueprint);
			if (!CompileAndSave(*EffectRow.Blueprint))
			{
				return false;
			}
		}
		if (!ValidateTooltipBlueprint(
			Tooltip.Blueprint,
			EffectRow.Blueprint->GeneratedClass))
		{
			if (!bBuild)
			{
				return false;
			}
			BuildTooltipTree(*Tooltip.Blueprint);
			if (!CompileAndSave(*Tooltip.Blueprint))
			{
				return false;
			}
			UWacomBattleIntentTooltipWidget* TooltipDefaults =
				Cast<UWacomBattleIntentTooltipWidget>(
					Tooltip.Blueprint->GeneratedClass->GetDefaultObject());
			if (!TooltipDefaults)
			{
				return false;
			}
			TooltipDefaults->SetEffectRowWidgetClass(
				TSubclassOf<UWacomBattleIntentEffectRowWidget>(
					EffectRow.Blueprint->GeneratedClass.Get()));
			if (!SaveAssetPackage(
				*Tooltip.Blueprint->GetOutermost(),
				*Tooltip.Blueprint))
			{
				return false;
			}
		}

		bool bEntryChanged = HasMissingVariableGuids(*Entry);
		if (!HasIconOwnedIntentTooltipTarget(Entry))
		{
			if (!bBuild
				|| !EnsureIntentTooltipTarget(*Entry, bEntryChanged))
			{
				return false;
			}
		}
		UWidget* EntryIntentTarget =
			Entry->WidgetTree->FindWidget(TEXT("IntentTooltipTarget"));
		UWidget* EntryInspectTarget =
			Entry->WidgetTree->FindWidget(TEXT("InspectHitTarget"));
		if (!EntryIntentTarget || !EntryInspectTarget
			|| !EnsurePaintsAbove(
				*Entry,
				*EntryIntentTarget,
				*EntryInspectTarget,
				bBuild,
				bEntryChanged))
		{
			return false;
		}
		if (!Wacom::ContentBuilder::EnemyUIHitTestPolicy::
			ValidateInteractiveRoutes(*Entry))
		{
			int32 RepairedWidgetCount = 0;
			if (!bBuild
				|| !Wacom::ContentBuilder::EnemyUIHitTestPolicy::
					NormalizeInteractiveRoutes(
						*Entry,
						RepairedWidgetCount))
			{
				return false;
			}
			bEntryChanged |= RepairedWidgetCount > 0;
		}
		if (bEntryChanged && !CompileAndSave(*Entry))
		{
			return false;
		}
		UWacomBattleEnemyPartEntryWidget* EntryDefaults =
			Cast<UWacomBattleEnemyPartEntryWidget>(
				Entry->GeneratedClass->GetDefaultObject());
		if (!EntryDefaults)
		{
			return false;
		}
		if (EntryDefaults->GetIntentTooltipWidgetClass().Get()
			!= Tooltip.Blueprint->GeneratedClass)
		{
			if (!bBuild)
			{
				return false;
			}
			EntryDefaults->SetIntentTooltipWidgetClass(
				TSubclassOf<UWacomBattleIntentTooltipWidget>(
					Tooltip.Blueprint->GeneratedClass.Get()));
			if (!SaveAssetPackage(*Entry->GetOutermost(), *Entry))
			{
				return false;
			}
		}

		bool bInspectionChanged = HasMissingVariableGuids(*Inspection);
		if (!HasIconOwnedIntentTooltipTarget(Inspection))
		{
			if (!bBuild
				|| !EnsureIntentTooltipTarget(
					*Inspection,
					bInspectionChanged))
			{
				return false;
			}
		}
		if (!Wacom::ContentBuilder::EnemyUIHitTestPolicy::
			ValidateInteractiveRoutes(*Inspection))
		{
			int32 RepairedWidgetCount = 0;
			if (!bBuild
				|| !Wacom::ContentBuilder::EnemyUIHitTestPolicy::
					NormalizeInteractiveRoutes(
						*Inspection,
						RepairedWidgetCount))
			{
				return false;
			}
			bInspectionChanged |= RepairedWidgetCount > 0;
		}
		if (!HasWidget(
			Inspection, TEXT("IntentEffectsList"), UPanelWidget::StaticClass()))
		{
			if (!bBuild || !EnsureInspectionEffectList(*Inspection))
			{
				return false;
			}
			bInspectionChanged = true;
		}
		if (bInspectionChanged && !CompileAndSave(*Inspection))
		{
			return false;
		}
		UWacomBattleEnemyInspectionWidget* InspectionDefaults =
			Cast<UWacomBattleEnemyInspectionWidget>(
				Inspection->GeneratedClass->GetDefaultObject());
		if (!InspectionDefaults)
		{
			return false;
		}
		if (InspectionDefaults->GetIntentPresentationStyle() != Style
			|| !ValidateExistingBindings(
				Entry,
				Inspection,
				Tooltip.Blueprint->GeneratedClass,
				EffectRow.Blueprint->GeneratedClass))
		{
			if (!bBuild)
			{
				return false;
			}
			InspectionDefaults->SetIntentPresentationStyle(Style);
			InspectionDefaults->SetIntentEffectRowWidgetClass(
				TSubclassOf<UWacomBattleIntentEffectRowWidget>(
					EffectRow.Blueprint->GeneratedClass.Get()));
			InspectionDefaults->SetIntentTooltipWidgetClass(
				TSubclassOf<UWacomBattleIntentTooltipWidget>(
					Tooltip.Blueprint->GeneratedClass.Get()));
			if (!SaveAssetPackage(
				*Inspection->GetOutermost(),
				*Inspection))
			{
				return false;
			}
		}

		if (!ConfigureStyle(*Style, bBuild))
		{
			return false;
		}
		FDataValidationContext ValidationContext;
		const bool bStyleValid =
			Style->IsDataValid(ValidationContext)
				== EDataValidationResult::Valid;
		return bStyleValid
			&& ValidateEffectRowBlueprint(EffectRow.Blueprint)
			&& ValidateTooltipBlueprint(
				Tooltip.Blueprint,
				EffectRow.Blueprint->GeneratedClass)
			&& ValidateExistingBindings(
				Entry,
				Inspection,
				Tooltip.Blueprint->GeneratedClass,
				EffectRow.Blueprint->GeneratedClass);
	}
}

bool Wacom::ContentBuilder::BuildEnemyIntentInspectionUI()
{
	const bool bResult = Process(true);
	if (bResult)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[EnemyIntentUIBuilder] BuildIntentInspection ready"));
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemyIntentUIBuilder] BuildIntentInspection failed"));
	}
	return bResult;
}

bool Wacom::ContentBuilder::InspectEnemyIntentInspectionUI()
{
	const bool bResult = Process(false);
	if (bResult)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[EnemyIntentUIBuilder] Intent inspection contract ready"));
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemyIntentUIBuilder] Intent inspection contract failed"));
	}
	return bResult;
}
