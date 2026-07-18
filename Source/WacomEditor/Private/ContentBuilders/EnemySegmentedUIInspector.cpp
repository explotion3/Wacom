// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemySegmentedUIInspector.h"
#include "ContentBuilders/EnemyUIHitTestPolicy.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/ProgressBar.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "UI/Battle/WacomBattleEnemyInspectionPartRowWidget.h"
#include "UI/Battle/WacomBattleEnemyInspectionWidget.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR MultiPanelPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget");
	constexpr TCHAR MultiEntryPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPartEntryWidget.BP_WacomBattleEnemyPartEntryWidget");
	constexpr TCHAR SinglePanelPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget.WBP_WacomBattleEnemySinglePartPanelWidget");
	constexpr TCHAR SingleEntryPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartEntryWidget.WBP_WacomBattleEnemySinglePartEntryWidget");
	constexpr TCHAR InspectionPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionWidget.WBP_WacomBattleEnemyInspectionWidget");
	constexpr TCHAR InspectionRowPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemyInspectionPartRowWidget.WBP_WacomBattleEnemyInspectionPartRowWidget");
	constexpr TCHAR ShieldBadgePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyShieldBadge.T_UI_EnemyShieldBadge");
	constexpr TCHAR ShieldFramePath[] =
		TEXT("/Game/Wacom/UI/Enemy/Vitals/Textures/T_UI_EnemyShieldFrame_9Slice.T_UI_EnemyShieldFrame_9Slice");
	constexpr TCHAR SegmentedVitalsContractMarker[] =
		TEXT("WacomEnemyPanelWBP.ContractVersion=2");
	constexpr TCHAR InspectionContractMarker[] =
		TEXT("WacomEnemyInspectionWBP.ContractVersion=1");
	constexpr TCHAR InspectionRowContractMarker[] =
		TEXT("WacomEnemyInspectionPartRowWBP.ContractVersion=1");

	UWidgetBlueprint* LoadWidgetBlueprint(const TCHAR* Path)
	{
		return Cast<UWidgetBlueprint>(StaticLoadObject(
			UWidgetBlueprint::StaticClass(), nullptr, Path));
	}

	bool HasWidget(
		const UWidgetBlueprint* Blueprint,
		const FName Name,
		const UClass* RequiredClass)
	{
		const UWidget* Widget = Blueprint && Blueprint->WidgetTree
			? Blueprint->WidgetTree->FindWidget(Name)
			: nullptr;
		return Widget && Widget->IsA(RequiredClass);
	}

	bool HasAnimation(const UWidgetBlueprint* Blueprint, const FName Name)
	{
		if (!Blueprint)
		{
			return false;
		}
		for (const UWidgetAnimation* Animation : Blueprint->Animations)
		{
			if (Animation
				&& (Animation->GetFName() == Name
					|| Animation->GetDisplayLabel() == Name.ToString())
				&& !Animation->GetBindings().IsEmpty())
			{
				return true;
			}
		}
		return false;
	}

	bool IsExpectedInteractiveWidget(const UWidget* Widget)
	{
		if (!Widget || !Widget->IsA<UButton>())
		{
			return false;
		}
		const FName Name = Widget->GetFName();
		return Name == TEXT("InspectHitTarget")
			|| Name == TEXT("CloseButton")
			|| Name == TEXT("PartSelectButton");
	}

	bool ValidateHitTestPolicy(const UWidgetBlueprint* Blueprint)
	{
		if (!Blueprint || !Blueprint->WidgetTree || !Blueprint->WidgetTree->RootWidget)
		{
			return false;
		}

		bool bValid = Blueprint->WidgetTree->RootWidget->GetVisibility()
			== ESlateVisibility::SelfHitTestInvisible;
		TArray<UWidget*> Widgets;
		Blueprint->WidgetTree->GetAllWidgets(Widgets);
		for (const UWidget* Widget : Widgets)
		{
			if (!Widget || Widget == Blueprint->WidgetTree->RootWidget)
			{
				continue;
			}
			const ESlateVisibility Visibility = Widget->GetVisibility();
			const bool bValidContainer = Visibility == ESlateVisibility::SelfHitTestInvisible
				&& Widget->IsA<UPanelWidget>();
			bValid &= Visibility != ESlateVisibility::Visible
				? Visibility != ESlateVisibility::SelfHitTestInvisible || bValidContainer
				: IsExpectedInteractiveWidget(Widget);
		}
		return bValid
			&& Wacom::ContentBuilder::EnemyUIHitTestPolicy::ValidateInteractiveRoutes(
				*Blueprint);
	}

	bool ValidatePanel(const UWidgetBlueprint* Blueprint, const TCHAR* Label)
	{
		const UWacomBattleEnemyPanelWidget* PanelDefaults = Blueprint && Blueprint->GeneratedClass
			? Cast<UWacomBattleEnemyPanelWidget>(Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(UWacomBattleEnemyPanelWidget::StaticClass())
			&& HasWidget(Blueprint, TEXT("EnemyNameText"), UTextBlock::StaticClass())
			&& HasWidget(Blueprint, TEXT("EnemyInitiativeText"), UTextBlock::StaticClass())
			&& HasWidget(Blueprint, TEXT("PartList"), UHorizontalBox::StaticClass())
			&& HasWidget(Blueprint, TEXT("PanelContextHighlight"), UWidget::StaticClass())
			&& PanelDefaults
			&& PanelDefaults->GetPartEntryWidgetClass()
			&& PanelDefaults->GetPartEntryWidgetClass()->IsChildOf(
				UWacomBattleEnemyPartEntryWidget::StaticClass())
			&& Blueprint->BlueprintDescription.Contains(
				SegmentedVitalsContractMarker)
			&& ValidateHitTestPolicy(Blueprint);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemySegmentedUIInspector] Invalid panel: %s"), Label);
		}
		return bValid;
	}

	bool ValidateEntry(const UWidgetBlueprint* Blueprint, const TCHAR* Label)
	{
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(UWacomBattleEnemyPartEntryWidget::StaticClass());
		const TPair<FName, UClass*> Required[] = {
			{ TEXT("InitiativeDiamond"), UWidget::StaticClass() },
			{ TEXT("IntentDiamond"), UWidget::StaticClass() },
			{ TEXT("IntentIcon"), UImage::StaticClass() },
			{ TEXT("HpBar"), UProgressBar::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldContainer"), UWidget::StaticClass() },
			{ TEXT("ShieldFrame"), UWidget::StaticClass() },
			{ TEXT("ShieldBadge"), UWidget::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("IntentText"), UTextBlock::StaticClass() },
			{ TEXT("StatusList"), UWacomBattleStatusIconListWidget::StaticClass() },
			{ TEXT("StatusOverflowText"), UTextBlock::StaticClass() },
			{ TEXT("DestroyedOverlay"), UWidget::StaticClass() },
			{ TEXT("DestroyedMark"), UWidget::StaticClass() },
			{ TEXT("InspectHitTarget"), UButton::StaticClass() },
		};
		for (const TPair<FName, UClass*>& Binding : Required)
		{
			bValid &= HasWidget(Blueprint, Binding.Key, Binding.Value);
		}
		bValid &= Blueprint
			&& Blueprint->BlueprintDescription.Contains(
				SegmentedVitalsContractMarker);
		bValid &= ValidateHitTestPolicy(Blueprint);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemySegmentedUIInspector] Invalid part entry: %s"), Label);
		}
		return bValid;
	}

	bool ValidateSinglePartGeometry(
		const UWidgetBlueprint* PanelBlueprint,
		const UWidgetBlueprint* EntryBlueprint)
	{
		const USizeBox* PanelRoot = PanelBlueprint && PanelBlueprint->WidgetTree
			? Cast<USizeBox>(PanelBlueprint->WidgetTree->FindWidget(TEXT("SinglePartPanelRoot")))
			: nullptr;
		const USizeBox* EntryRoot = EntryBlueprint && EntryBlueprint->WidgetTree
			? Cast<USizeBox>(EntryBlueprint->WidgetTree->FindWidget(TEXT("SinglePartEntryRoot")))
			: nullptr;
		const USizeBox* CompactSize = EntryBlueprint && EntryBlueprint->WidgetTree
			? Cast<USizeBox>(EntryBlueprint->WidgetTree->FindWidget(TEXT("CompactSize")))
			: nullptr;
		const bool bValid = PanelRoot
			&& EntryRoot
			&& CompactSize
			&& PanelRoot->IsWidthOverride()
			&& FMath::IsNearlyEqual(PanelRoot->GetWidthOverride(), 250.0f)
			&& !PanelRoot->IsMinDesiredWidthOverride()
			&& !EntryRoot->IsWidthOverride()
			&& !EntryRoot->IsMinDesiredWidthOverride()
			&& !CompactSize->IsWidthOverride()
			&& CompactSize->IsHeightOverride()
			&& FMath::IsNearlyEqual(CompactSize->GetHeightOverride(), 84.0f);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EnemySegmentedUIInspector] Invalid single-part geometry ownership"));
		}
		return bValid;
	}

	bool ValidateInspection(const UWidgetBlueprint* Blueprint)
	{
		const UWacomBattleEnemyInspectionWidget* InspectionDefaults =
			Blueprint && Blueprint->GeneratedClass
				? Cast<UWacomBattleEnemyInspectionWidget>(
					Blueprint->GeneratedClass->GetDefaultObject())
				: nullptr;
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(UWacomBattleEnemyInspectionWidget::StaticClass());
		const TPair<FName, UClass*> Required[] = {
			{ TEXT("LeftPanel"), UWidget::StaticClass() },
			{ TEXT("RightPanel"), UWidget::StaticClass() },
			{ TEXT("EnemyNameText"), UTextBlock::StaticClass() },
			{ TEXT("EnemyStateText"), UTextBlock::StaticClass() },
			{ TEXT("PartNavigator"), UPanelWidget::StaticClass() },
			{ TEXT("SelectedPartNameText"), UTextBlock::StaticClass() },
			{ TEXT("HpBar"), UProgressBar::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldContainer"), UWidget::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("IntentText"), UTextBlock::StaticClass() },
			{ TEXT("ResistanceText"), UTextBlock::StaticClass() },
			{ TEXT("StatusList"), UWacomBattleStatusIconListWidget::StaticClass() },
			{ TEXT("DestroyedOverlay"), UWidget::StaticClass() },
			{ TEXT("CloseButton"), UButton::StaticClass() },
		};
		for (const TPair<FName, UClass*>& Binding : Required)
		{
			bValid &= HasWidget(Blueprint, Binding.Key, Binding.Value);
		}
		bValid &= HasAnimation(Blueprint, TEXT("OpenLeftAnimation"));
		bValid &= HasAnimation(Blueprint, TEXT("OpenRightAnimation"));
		bValid &= HasAnimation(Blueprint, TEXT("CloseAnimation"));
		bValid &= InspectionDefaults
			&& InspectionDefaults->GetPartRowWidgetClass()
			&& InspectionDefaults->GetPartRowWidgetClass()->IsChildOf(
				UWacomBattleEnemyInspectionPartRowWidget::StaticClass());
		bValid &= Blueprint
			&& Blueprint->BlueprintDescription.Contains(
				InspectionContractMarker);
		bValid &= ValidateHitTestPolicy(Blueprint);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemySegmentedUIInspector] Invalid inspection WBP"));
		}
		return bValid;
	}

	bool ValidateInspectionRow(const UWidgetBlueprint* Blueprint)
	{
		bool bValid = Blueprint
			&& Blueprint->ParentClass
			&& Blueprint->ParentClass->IsChildOf(
				UWacomBattleEnemyInspectionPartRowWidget::StaticClass());
		const TPair<FName, UClass*> Required[] = {
			{ TEXT("PartSelectButton"), UButton::StaticClass() },
			{ TEXT("PartNameText"), UTextBlock::StaticClass() },
			{ TEXT("HpText"), UTextBlock::StaticClass() },
			{ TEXT("ShieldContainer"), UWidget::StaticClass() },
			{ TEXT("ShieldText"), UTextBlock::StaticClass() },
			{ TEXT("InitiativeText"), UTextBlock::StaticClass() },
			{ TEXT("SelectionHighlight"), UWidget::StaticClass() },
			{ TEXT("DestroyedOverlay"), UWidget::StaticClass() },
		};
		for (const TPair<FName, UClass*>& Binding : Required)
		{
			bValid &= HasWidget(Blueprint, Binding.Key, Binding.Value);
		}
		bValid &= Blueprint
			&& Blueprint->BlueprintDescription.Contains(
				InspectionRowContractMarker);
		bValid &= ValidateHitTestPolicy(Blueprint);
		if (!bValid)
		{
			UE_LOG(LogTemp, Error, TEXT("[EnemySegmentedUIInspector] Invalid inspection row WBP"));
		}
		return bValid;
	}
}

bool Wacom::ContentBuilder::InspectEnemySegmentedUI()
{
	const UWidgetBlueprint* MultiPanel = LoadWidgetBlueprint(MultiPanelPath);
	const UWidgetBlueprint* MultiEntry = LoadWidgetBlueprint(MultiEntryPath);
	const UWidgetBlueprint* SinglePanel = LoadWidgetBlueprint(SinglePanelPath);
	const UWidgetBlueprint* SingleEntry = LoadWidgetBlueprint(SingleEntryPath);
	const UWidgetBlueprint* Inspection = LoadWidgetBlueprint(InspectionPath);
	const UWidgetBlueprint* InspectionRow = LoadWidgetBlueprint(InspectionRowPath);
	const UTexture2D* ShieldBadge = Cast<UTexture2D>(StaticLoadObject(
		UTexture2D::StaticClass(), nullptr, ShieldBadgePath));
	const UTexture2D* ShieldFrame = Cast<UTexture2D>(StaticLoadObject(
		UTexture2D::StaticClass(), nullptr, ShieldFramePath));

	bool bValid = ValidatePanel(MultiPanel, TEXT("multi-part"));
	bValid &= ValidatePanel(SinglePanel, TEXT("single-part"));
	bValid &= ValidateEntry(MultiEntry, TEXT("multi-part"));
	bValid &= ValidateEntry(SingleEntry, TEXT("single-part"));
	bValid &= ValidateSinglePartGeometry(SinglePanel, SingleEntry);
	bValid &= ValidateInspection(Inspection);
	bValid &= ValidateInspectionRow(InspectionRow);
	bValid &= ShieldBadge && ShieldFrame
		&& ShieldBadge->LODGroup == TEXTUREGROUP_UI
		&& ShieldFrame->LODGroup == TEXTUREGROUP_UI
		&& ShieldBadge->Filter == TF_Nearest
		&& ShieldFrame->Filter == TF_Nearest
		&& ShieldBadge->MipGenSettings == TMGS_NoMipmaps
		&& ShieldFrame->MipGenSettings == TMGS_NoMipmaps;
	if (!ShieldBadge || !ShieldFrame)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[EnemySegmentedUIInspector] Missing Shield badge/frame texture"));
	}

	if (bValid)
	{
		UE_LOG(LogTemp, Display,
			TEXT("[EnemySegmentedUIInspector] Segmented vitals and inspection contracts are valid"));
	}
	return bValid;
}
