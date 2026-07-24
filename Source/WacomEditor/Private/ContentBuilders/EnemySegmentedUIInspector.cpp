// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemySegmentedUIInspector.h"

#include "ContentBuilders/EnemyUIHitTestPolicy.h"

#include "Animation/WidgetAnimation.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Font.h"
#include "Engine/FontFace.h"
#include "Engine/Texture2D.h"
#include "Materials/Material.h"
#include "Materials/MaterialInterface.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "MovieScene.h"
#include "UI/Battle/WacomBattleEnemyInspectionPartRowWidget.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleEnemyIntentPresentationStyle.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleIntentEffectRowWidget.h"
#include "UI/Battle/WacomBattleIntentTooltipWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR PanelPath[] = TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget");
	constexpr TCHAR EntryPath[] = TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget");
	constexpr TCHAR InspectionPath[] = TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget.WBP_WacomBattleEnemyInspectionWidget");
	constexpr TCHAR InspectionRowPath[] = TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionPartRowWidget.WBP_WacomBattleEnemyInspectionPartRowWidget");
	constexpr TCHAR IntentStylePath[] = TEXT("/Game/Wacom/UI/Enemy/Intent/DA_EnemyIntentPresentation_Default.DA_EnemyIntentPresentation_Default");
	constexpr TCHAR MaterialPath[] = TEXT("/Game/Wacom/UI/Enemy/Vitals/Materials/M_UI_EnemyVitalsTrack.M_UI_EnemyVitalsTrack");
	constexpr TCHAR FontPath[] = TEXT("/Game/Wacom/UI/Foundation/Fonts/Silkscreen/F_Silkscreen.F_Silkscreen");
	constexpr TCHAR BoldFacePath[] = TEXT("/Game/Wacom/UI/Foundation/Fonts/Silkscreen/FF_Silkscreen_Bold.FF_Silkscreen_Bold");
	constexpr TCHAR RegularFacePath[] = TEXT("/Game/Wacom/UI/Foundation/Fonts/Silkscreen/FF_Silkscreen_Regular.FF_Silkscreen_Regular");
	constexpr TCHAR InspectionMarker[] = TEXT("WacomEnemyInspection.ContractVersion=2");
	constexpr TCHAR LegacyPanelPackage[] = TEXT("/Game/Wacom/UI/Enemy/Textures/T_UI_PixelPanel_EnemyInfo_9Slice_512x160");
	constexpr TCHAR LegacySinglePanelPackage[] = TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget");
	constexpr TCHAR LegacySingleEntryPackage[] = TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartEntryWidget");

	struct FWidgetRequirement
	{
		FName Name;
		UClass* Class;
	};

	struct FAnimationRequirement
	{
		FName Name;
		float DurationSeconds;
	};

	const TCHAR* TexturePaths[] = {
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyPanelFrame_9Slice.T_UI_EnemyPanelFrame_9Slice"),
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyDossierFrame_9Slice.T_UI_EnemyDossierFrame_9Slice"),
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyShieldBadge.T_UI_EnemyShieldBadge"),
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyShieldFrame_9Slice.T_UI_EnemyShieldFrame_9Slice"),
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyInitiativeSocket.T_UI_EnemyInitiativeSocket"),
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyIntentSocket.T_UI_EnemyIntentSocket"),
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyDestroyedCrack.T_UI_EnemyDestroyedCrack"),
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Fallback.T_UI_EnemyIntent_Fallback"),
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Attack.T_UI_EnemyIntent_Attack"),
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Guard.T_UI_EnemyIntent_Guard"),
		TEXT("/Game/Wacom/UI/Enemy/Intent/Textures/T_UI_EnemyIntent_Cleave.T_UI_EnemyIntent_Cleave"),
	};

	UWidgetBlueprint* LoadWBP(const TCHAR* Path)
	{
		return Cast<UWidgetBlueprint>(StaticLoadObject(UWidgetBlueprint::StaticClass(), nullptr, Path));
	}

	bool HasWidget(const UWidgetBlueprint* Blueprint, FName Name, const UClass* Class)
	{
		const UWidget* Widget = Blueprint && Blueprint->WidgetTree
			? Blueprint->WidgetTree->FindWidget(Name)
			: nullptr;
		return Widget && Widget->IsA(Class);
	}

	bool UsesBrushResource(const UWidgetBlueprint* Blueprint, const TCHAR* ResourcePath)
	{
		UObject* Expected = StaticLoadObject(UObject::StaticClass(), nullptr, ResourcePath);
		if (!Blueprint || !Blueprint->WidgetTree || !Expected)
		{
			return false;
		}

		bool bFound = false;
		Blueprint->WidgetTree->ForEachWidget([Expected, &bFound](UWidget* Widget)
		{
			if (bFound || !Widget)
			{
				return;
			}
			if (const UImage* Image = Cast<UImage>(Widget))
			{
				bFound = Image->GetBrush().GetResourceObject() == Expected;
			}
			else if (const UBorder* Border = Cast<UBorder>(Widget))
			{
				bFound = Border->Background.GetResourceObject() == Expected;
			}
			else if (const UButton* Button = Cast<UButton>(Widget))
			{
				const FButtonStyle& Style = Button->GetStyle();
				bFound = Style.Normal.GetResourceObject() == Expected
					|| Style.Hovered.GetResourceObject() == Expected
					|| Style.Pressed.GetResourceObject() == Expected
					|| Style.Disabled.GetResourceObject() == Expected;
			}
		});
		if (!bFound)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyHUDInspector] Widget tree does not use required art resource: %s"),
				ResourcePath);
		}
		return bFound;
	}

	UWidgetAnimation* FindAnimation(const UWidgetBlueprint* Blueprint, FName Name)
	{
		if (!Blueprint)
		{
			return nullptr;
		}
		for (UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation && (Animation->GetFName() == Name || Animation->GetDisplayLabel() == Name.ToString()))
			{
				return Animation;
			}
		}
		return nullptr;
	}

	bool HasAnimation(const UWidgetBlueprint* Blueprint, FName Name, float Duration)
	{
		const UWidgetAnimation* Animation = FindAnimation(Blueprint, Name);
		const double DisplayRate = Animation && Animation->MovieScene
			? Animation->MovieScene->GetDisplayRate().AsDecimal()
			: 0.0;
		const float AuthoredEndTime = Animation && DisplayRate > 0.0
			? FMath::Max(0.0f,
				Animation->GetEndTime() - static_cast<float>(1.0 / DisplayRate))
			: 0.0f;
		const bool bValid = Animation
			&& Animation->MovieScene
			&& !Animation->GetBindings().IsEmpty()
			&& FMath::IsNearlyEqual(AuthoredEndTime, Duration, 0.02f);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyHUDInspector] Animation diagnostic %s: found=%s movieScene=%s bindings=%d authoredEnd=%.3f playbackEnd=%.3f expected=%.3f"),
				*Name.ToString(),
				Animation ? TEXT("true") : TEXT("false"),
				Animation && Animation->MovieScene ? TEXT("true") : TEXT("false"),
				Animation ? Animation->GetBindings().Num() : 0,
				AuthoredEndTime,
				Animation ? Animation->GetEndTime() : 0.0f,
				Duration);
		}
		return bValid;
	}

	bool ValidateHitTesting(const UWidgetBlueprint& Blueprint)
	{
		return Blueprint.WidgetTree
			&& Blueprint.WidgetTree->RootWidget
			&& Blueprint.WidgetTree->RootWidget->GetVisibility() == ESlateVisibility::SelfHitTestInvisible
			&& Wacom::ContentBuilder::EnemyUIHitTestPolicy::ValidateInteractiveRoutes(Blueprint);
	}

	bool IntentTooltipTargetOwnsIcon(const UWidgetBlueprint* Blueprint)
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

	const UWidget* FindDirectChildUnder(
		const UWidget* Descendant,
		const UPanelWidget* Ancestor)
	{
		const UWidget* Current = Descendant;
		while (Current && Current->GetParent() != Ancestor)
		{
			Current = Current->GetParent();
		}
		return Current;
	}

	bool PaintsAboveAtFirstSharedParent(
		const UWidget* Foreground,
		const UWidget* Background)
	{
		if (!Foreground || !Background)
		{
			return false;
		}

		for (const UPanelWidget* Parent = Foreground->GetParent();
			Parent;
			Parent = Parent->GetParent())
		{
			const UWidget* ForegroundBranch =
				FindDirectChildUnder(Foreground, Parent);
			const UWidget* BackgroundBranch =
				FindDirectChildUnder(Background, Parent);
			if (!ForegroundBranch || !BackgroundBranch
				|| ForegroundBranch == BackgroundBranch)
			{
				continue;
			}

			return Parent->GetChildIndex(ForegroundBranch)
				> Parent->GetChildIndex(BackgroundBranch);
		}

		return true;
	}

	bool ValidatePanel(const UWidgetBlueprint* Blueprint, const UWidgetBlueprint* Entry)
	{
		if (!Blueprint || !Entry || !Blueprint->GeneratedClass || !Entry->GeneratedClass)
		{
			return false;
		}
		const UWacomBattleEnemyPanelWidget* Defaults = Cast<UWacomBattleEnemyPanelWidget>(
			Blueprint->GeneratedClass->GetDefaultObject());
		return Blueprint->ParentClass == UWacomBattleEnemyPanelWidget::StaticClass()
			&& HasWidget(Blueprint, TEXT("PanelRoot"), USizeBox::StaticClass())
			&& HasWidget(Blueprint, TEXT("PartList"), UHorizontalBox::StaticClass())
			&& !HasWidget(Blueprint, TEXT("EnemyNameText"), UTextBlock::StaticClass())
			&& !HasWidget(Blueprint, TEXT("PanelContextHighlight"), UWidget::StaticClass())
			&& Defaults
			&& Defaults->GetPartEntryWidgetClass().Get() == Entry->GeneratedClass
			&& ValidateHitTesting(*Blueprint);
	}

	bool ValidateEntry(const UWidgetBlueprint* Blueprint, UFont* Font, UMaterial* Material)
	{
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			return false;
		}
		const FWidgetRequirement Required[] = {
			{ TEXT("PartEntryRoot"), USizeBox::StaticClass() },
			{ TEXT("VitalsTrackImage"), UImage::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldValueRoot"), UWidget::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeSocket"), UWidget::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("IntentSocket"), UWidget::StaticClass() },
			{ TEXT("IntentIcon"), UImage::StaticClass() },
			{ TEXT("OutgoingIntentIcon"), UImage::StaticClass() },
			{ TEXT("StatusList"), UWacomBattleStatusIconListWidget::StaticClass() },
			{ TEXT("ContextSurface"), UWidget::StaticClass() },
			{ TEXT("DestroyedSurface"), UWidget::StaticClass() },
			{ TEXT("DestroyedMark"), UWidget::StaticClass() },
			{ TEXT("InspectHitTarget"), UButton::StaticClass() },
			{ TEXT("IntentTooltipTarget"), UButton::StaticClass() },
		};
		bool bValid = true;
		const auto CheckEntry = [&bValid](const FString& Contract, const bool bCondition)
		{
			bValid &= bCondition;
			if (!bCondition)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemyHUDInspector] Base entry failed: %s"), *Contract);
			}
		};
		CheckEntry(TEXT("parent class"),
			Blueprint->ParentClass == UWacomBattleEnemyPartEntryWidget::StaticClass());
		CheckEntry(TEXT("hit-test policy"), ValidateHitTesting(*Blueprint));
		const UWidget* StatusList = Blueprint->WidgetTree->FindWidget(TEXT("StatusList"));
		const UWidget* InspectHitTarget =
			Blueprint->WidgetTree->FindWidget(TEXT("InspectHitTarget"));
		const UWidget* IntentTooltipTarget =
			Blueprint->WidgetTree->FindWidget(TEXT("IntentTooltipTarget"));
		const UPanelWidget* SharedInputParent =
			StatusList && InspectHitTarget
			&& StatusList->GetParent() == InspectHitTarget->GetParent()
				? StatusList->GetParent()
				: nullptr;
		CheckEntry(TEXT("status list owns overflow"),
			!Blueprint->WidgetTree->FindWidget(TEXT("StatusOverflowText")));
		CheckEntry(TEXT("status list authored interaction root"),
			StatusList
			&& (StatusList->GetVisibility() == ESlateVisibility::Visible
				|| StatusList->GetVisibility()
					== ESlateVisibility::SelfHitTestInvisible));
		CheckEntry(TEXT("status list paints above shared inspect hit target"),
			!SharedInputParent
			|| SharedInputParent->GetChildIndex(StatusList)
				> SharedInputParent->GetChildIndex(InspectHitTarget));
		CheckEntry(TEXT("intent tooltip target paints above inspect hit target"),
			PaintsAboveAtFirstSharedParent(
				IntentTooltipTarget,
				InspectHitTarget));
		CheckEntry(TEXT("intent tooltip target owns exact IntentIcon bounds"),
			IntentTooltipTargetOwnsIcon(Blueprint));
		for (const FWidgetRequirement& Binding : Required)
		{
			CheckEntry(FString::Printf(TEXT("binding %s"), *Binding.Name.ToString()),
				HasWidget(Blueprint, Binding.Name, Binding.Class));
		}
		const FAnimationRequirement Animations[] = {
			{ TEXT("IntroAnimation"), 0.22f }, { TEXT("DamageImpactAnimation"), 0.22f },
			{ TEXT("ShieldImpactAnimation"), 0.18f }, { TEXT("ShieldBreakAnimation"), 0.24f },
			{ TEXT("InitiativeStepAnimation"), 0.12f }, { TEXT("IntentChangeAnimation"), 0.18f },
			{ TEXT("ContextAnimation"), 0.12f }, { TEXT("DestroyedAnimation"), 0.30f },
		};
		for (const FAnimationRequirement& Animation : Animations)
		{
			CheckEntry(FString::Printf(TEXT("animation %s"), *Animation.Name.ToString()),
				HasAnimation(Blueprint, Animation.Name, Animation.DurationSeconds));
		}
		const UImage* Vitals = Cast<UImage>(Blueprint->WidgetTree->FindWidget(TEXT("VitalsTrackImage")));
		CheckEntry(TEXT("vitals material"),
			Vitals && Vitals->GetBrush().GetResourceObject() == Material);
		const UTextBlock* HpText = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("HpText")));
		const UTextBlock* Initiative = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("InitiativeText")));
		const UTextBlock* Shield = Cast<UTextBlock>(Blueprint->WidgetTree->FindWidget(TEXT("ShieldText")));
		CheckEntry(TEXT("HP font"), HpText && HpText->GetFont().FontObject == Font
			&& HpText->GetFont().TypefaceFontName == FName(TEXT("Bold")) && HpText->GetFont().Size == 18);
		CheckEntry(TEXT("initiative font"), Initiative && Initiative->GetFont().FontObject == Font
			&& Initiative->GetFont().TypefaceFontName == FName(TEXT("Bold"))
			&& Initiative->GetFont().Size == 16);
		CheckEntry(TEXT("shield font"), Shield && Shield->GetFont().FontObject == Font
			&& Shield->GetFont().TypefaceFontName == FName(TEXT("Bold")) && Shield->GetFont().Size == 14);
		for (FName Legacy : { FName(TEXT("HpBar")), FName(TEXT("PartNameText")), FName(TEXT("IntentText")),
			FName(TEXT("ResistanceText")), FName(TEXT("DetailsContainer")), FName(TEXT("ActionPreviewOverlay")) })
		{
			CheckEntry(FString::Printf(TEXT("legacy binding removed: %s"), *Legacy.ToString()),
				!Blueprint->WidgetTree->FindWidget(Legacy));
		}
		return bValid;
	}

	bool ValidateInspectionRow(const UWidgetBlueprint* Blueprint)
	{
		const FWidgetRequirement Required[] = {
			{ TEXT("PartSelectButton"), UButton::StaticClass() }, { TEXT("PartNameText"), UTextBlock::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() }, { TEXT("ShieldContainer"), UWidget::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() }, { TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("SelectionHighlight"), UWidget::StaticClass() }, { TEXT("DestroyedOverlay"), UWidget::StaticClass() },
		};
		bool bValid = Blueprint
			&& Blueprint->ParentClass == UWacomBattleEnemyInspectionPartRowWidget::StaticClass()
			&& Blueprint->BlueprintDescription.Contains(InspectionMarker)
			&& ValidateHitTesting(*Blueprint);
		for (const FWidgetRequirement& Binding : Required)
		{
			bValid &= HasWidget(Blueprint, Binding.Name, Binding.Class);
		}
		return bValid;
	}

	bool ValidateInspection(const UWidgetBlueprint* Blueprint, const UWidgetBlueprint* Row, UWacomBattleEnemyIntentPresentationStyle* Style)
	{
		const FWidgetRequirement Required[] = {
			{ TEXT("LeftPanel"), USizeBox::StaticClass() }, { TEXT("RightPanel"), USizeBox::StaticClass() },
			{ TEXT("EnemyNameText"), UTextBlock::StaticClass() }, { TEXT("EnemyStateText"), UTextBlock::StaticClass() },
			{ TEXT("PartNavigator"), UPanelWidget::StaticClass() }, { TEXT("SelectedPartNameText"), UTextBlock::StaticClass() },
			{ TEXT("HpBar"), UProgressBar::StaticClass() }, { TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldContainer"), UWidget::StaticClass() }, { TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() }, { TEXT("IntentIcon"), UImage::StaticClass() },
			{ TEXT("IntentText"), UTextBlock::StaticClass() }, { TEXT("ResistanceText"), UTextBlock::StaticClass() },
			{ TEXT("StatusList"), UWacomBattleStatusIconListWidget::StaticClass() },
			{ TEXT("IntentTooltipTarget"), UButton::StaticClass() },
			{ TEXT("DestroyedOverlay"), UWidget::StaticClass() }, { TEXT("CloseButton"), UButton::StaticClass() },
		};
		const UWacomBattleEnemyInspectionWidget* Defaults = Blueprint && Blueprint->GeneratedClass
			? Cast<UWacomBattleEnemyInspectionWidget>(Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		bool bValid = true;
		const auto CheckInspection = [&bValid](const FString& Contract, const bool bCondition)
		{
			bValid &= bCondition;
			if (!bCondition)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[EnemyHUDInspector] Inspection failed: %s"), *Contract);
			}
		};
		CheckInspection(TEXT("assets load"), Blueprint && Row);
		if (!Blueprint || !Row)
		{
			return false;
		}
		CheckInspection(TEXT("parent class"),
			Blueprint->ParentClass == UWacomBattleEnemyInspectionWidget::StaticClass());
		CheckInspection(TEXT("contract marker"),
			Blueprint->BlueprintDescription.Contains(InspectionMarker));
		CheckInspection(TEXT("hit-test policy"), ValidateHitTesting(*Blueprint));
		CheckInspection(TEXT("generated defaults"), Defaults != nullptr);
		CheckInspection(TEXT("part row class"),
			Defaults && Defaults->GetPartRowWidgetClass().Get() == Row->GeneratedClass);
		CheckInspection(TEXT("intent presentation style"),
			Defaults && Defaults->GetIntentPresentationStyle() == Style);
		CheckInspection(TEXT("intent tooltip class"),
			Defaults && Defaults->GetIntentTooltipWidgetClass() != nullptr);
		CheckInspection(TEXT("intent tooltip target owns exact IntentIcon bounds"),
			IntentTooltipTargetOwnsIcon(Blueprint));
		CheckInspection(TEXT("legacy dossier Intent effect widgets are absent"),
			!HasWidget(Blueprint, TEXT("IntentEffectsBounds"), UWidget::StaticClass())
			&& !HasWidget(Blueprint, TEXT("IntentEffectsScroll"), UWidget::StaticClass())
			&& !HasWidget(Blueprint, TEXT("IntentEffectsList"), UWidget::StaticClass()));
		for (const FWidgetRequirement& Binding : Required)
		{
			CheckInspection(FString::Printf(TEXT("binding %s"), *Binding.Name.ToString()),
				HasWidget(Blueprint, Binding.Name, Binding.Class));
		}
		CheckInspection(TEXT("OpenLeftAnimation"),
			HasAnimation(Blueprint, TEXT("OpenLeftAnimation"), 0.18f));
		CheckInspection(TEXT("OpenRightAnimation"),
			HasAnimation(Blueprint, TEXT("OpenRightAnimation"), 0.24f));
		CheckInspection(TEXT("CloseAnimation"),
			HasAnimation(Blueprint, TEXT("CloseAnimation"), 0.16f));
		const USizeBox* Left = Blueprint && Blueprint->WidgetTree
			? Cast<USizeBox>(Blueprint->WidgetTree->FindWidget(TEXT("LeftPanel"))) : nullptr;
		const USizeBox* Right = Blueprint && Blueprint->WidgetTree
			? Cast<USizeBox>(Blueprint->WidgetTree->FindWidget(TEXT("RightPanel"))) : nullptr;
		CheckInspection(TEXT("left dossier dimensions"), Left && Left->IsWidthOverride() && Left->IsHeightOverride()
			&& FMath::IsNearlyEqual(Left->GetWidthOverride(), 220.0f)
			&& FMath::IsNearlyEqual(Left->GetHeightOverride(), 520.0f));
		CheckInspection(TEXT("right dossier dimensions"), Right && Right->IsWidthOverride() && Right->IsHeightOverride()
			&& FMath::IsNearlyEqual(Right->GetWidthOverride(), 420.0f)
			&& FMath::IsNearlyEqual(Right->GetHeightOverride(), 560.0f));
		return bValid;
	}

	bool ValidateTextures()
	{
		bool bValid = true;
		for (const TCHAR* Path : TexturePaths)
		{
			const UTexture2D* Texture = Cast<UTexture2D>(StaticLoadObject(UTexture2D::StaticClass(), nullptr, Path));
			const bool bTextureValid = Texture
				&& Texture->LODGroup == TEXTUREGROUP_UI
				&& Texture->Filter == TF_Nearest
				&& Texture->MipGenSettings == TMGS_NoMipmaps
				&& Texture->NeverStream;
			bValid &= bTextureValid;
			if (!bTextureValid)
			{
				UE_LOG(LogTemp, Error, TEXT("[EnemyHUDInspector] Invalid pixel texture: %s"), Path);
			}
		}
		return bValid;
	}

	bool ValidateLegacyPanelRemoved()
	{
		if (!FPackageName::DoesPackageExist(LegacyPanelPackage))
		{
			return true;
		}
		TArray<FName> Referencers;
		FAssetRegistryModule::GetRegistry().GetReferencers(
			FName(LegacyPanelPackage), Referencers, UE::AssetRegistry::EDependencyCategory::Package);
		if (!Referencers.IsEmpty())
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemyHUDInspector] Legacy panel texture still has %d referencers"), Referencers.Num());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemyHUDInspector] Zero-reference legacy panel texture was not deleted"));
		}
		return false;
	}

	bool ValidateLegacyPackageRemoved(const TCHAR* PackageName, const TCHAR* Label)
	{
		if (!FPackageName::DoesPackageExist(PackageName))
		{
			return true;
		}

		TArray<FName> Referencers;
		FAssetRegistryModule::GetRegistry().GetReferencers(
			FName(PackageName), Referencers, UE::AssetRegistry::EDependencyCategory::Package);
		Referencers.Sort(FNameLexicalLess());
		UE_LOG(LogTemp, Error,
			TEXT("[EnemyHUDInspector] Legacy %s package still exists with %d referencer(s): %s"),
			Label,
			Referencers.Num(),
			*FString::JoinBy(Referencers, TEXT(", "), [](const FName Referencer)
			{
				return Referencer.ToString();
			}));
		return false;
	}

	bool ValidateLegacySinglePartConfigRemoved()
	{
		FString DefaultGameContents;
		const FString DefaultGamePath = FPaths::Combine(
			FPaths::ProjectConfigDir(),
			TEXT("DefaultGame.ini"));
		return FFileHelper::LoadFileToString(DefaultGameContents, *DefaultGamePath)
			&& !DefaultGameContents.Contains(
				TEXT("DefaultBattleEnemySinglePartPanelWidgetClass"),
				ESearchCase::CaseSensitive);
	}
}

bool Wacom::ContentBuilder::InspectEnemyHUD()
{
	UWidgetBlueprint* Panel = LoadWBP(PanelPath);
	UWidgetBlueprint* Entry = LoadWBP(EntryPath);
	UWidgetBlueprint* Inspection = LoadWBP(InspectionPath);
	UWidgetBlueprint* InspectionRow = LoadWBP(InspectionRowPath);
	UFont* Font = Cast<UFont>(StaticLoadObject(UFont::StaticClass(), nullptr, FontPath));
	UFontFace* Bold = Cast<UFontFace>(StaticLoadObject(UFontFace::StaticClass(), nullptr, BoldFacePath));
	UFontFace* Regular = Cast<UFontFace>(StaticLoadObject(UFontFace::StaticClass(), nullptr, RegularFacePath));
	UMaterial* Material = Cast<UMaterial>(StaticLoadObject(UMaterial::StaticClass(), nullptr, MaterialPath));
	UWacomBattleEnemyIntentPresentationStyle* Style = Cast<UWacomBattleEnemyIntentPresentationStyle>(
		StaticLoadObject(UWacomBattleEnemyIntentPresentationStyle::StaticClass(), nullptr, IntentStylePath));
	const UWacomUIDeveloperSettings* UISettings = GetDefault<UWacomUIDeveloperSettings>();

	bool bValid = true;
	const auto Check = [&bValid](const TCHAR* Contract, const bool bCondition)
	{
		bValid &= bCondition;
		if (!bCondition)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemyHUDInspector] Contract failed: %s"), Contract);
		}
	};

	Check(TEXT("formal assets load"), Font && Bold && Regular && Material && Style);
	Check(TEXT("vitals material domain and blend mode"),
		Material && Material->MaterialDomain == MD_UI && Material->BlendMode == BLEND_Translucent);
	Check(TEXT("intent style has three explicit mappings"),
		Style && Style->IntentIcons.Num() == 3);
	Check(TEXT("pixel texture import settings"), ValidateTextures());
	Check(TEXT("unified part entry WBP"), ValidateEntry(Entry, Font, Material));
	Check(TEXT("unified panel WBP"), ValidatePanel(Panel, Entry));
	Check(TEXT("unique project default uses unified panel WBP"),
		UISettings && Panel
		&& UISettings->DefaultBattleEnemyPanelWidgetClass.LoadSynchronous()
			== Panel->GeneratedClass);
	Check(TEXT("inspection row WBP"), ValidateInspectionRow(InspectionRow));
	Check(TEXT("inspection dossier WBP"), ValidateInspection(Inspection, InspectionRow, Style));
	Check(TEXT("part entry uses formal shield badge"), UsesBrushResource(Entry,
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyShieldBadge.T_UI_EnemyShieldBadge")));
	Check(TEXT("part entry uses formal initiative socket"), UsesBrushResource(Entry,
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyInitiativeSocket.T_UI_EnemyInitiativeSocket")));
	Check(TEXT("part entry uses formal intent socket"), UsesBrushResource(Entry,
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyIntentSocket.T_UI_EnemyIntentSocket")));
	Check(TEXT("part entry uses formal destroyed crack"), UsesBrushResource(Entry,
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyDestroyedCrack.T_UI_EnemyDestroyedCrack")));
	Check(TEXT("inspection dossier uses formal frame"), UsesBrushResource(Inspection,
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyDossierFrame_9Slice.T_UI_EnemyDossierFrame_9Slice")));
	Check(TEXT("zero-reference legacy panel texture removed"), ValidateLegacyPanelRemoved());
	Check(TEXT("legacy single-part panel removed"), ValidateLegacyPackageRemoved(
		LegacySinglePanelPackage, TEXT("single-part panel")));
	Check(TEXT("legacy single-part entry removed"), ValidateLegacyPackageRemoved(
		LegacySingleEntryPackage, TEXT("single-part entry")));
	Check(TEXT("legacy single-part DeveloperSettings key removed"),
		ValidateLegacySinglePartConfigRemoved());

	if (bValid)
	{
		UE_LOG(LogTemp, Display, TEXT("[EnemyHUDInspector] Enemy HUD V4 contract is valid"));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[EnemyHUDInspector] Enemy HUD V4 contract failed"));
	}
	return bValid;
}
