// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/PlayerStatusUIBuilder.h"

#include "Animation/WidgetAnimation.h"
#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Materials/MaterialInterface.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/Battle/WacomBattleStatusIconWidget.h"
#include "UObject/SavePackage.h"
#include "WidgetBlueprint.h"

namespace
{
	constexpr TCHAR PlayerStatusObjectPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_PlayerStatusBar.WBP_PlayerStatusBar");
	constexpr TCHAR BattleHudObjectPath[] =
		TEXT("/Game/Wacom/UI/Battle/BP_BattleHUD.BP_BattleHUD");
	constexpr TCHAR StatusIconObjectPath[] =
		TEXT("/Game/Wacom/UI/Battle/PlayerStatusBar/WBP_BattleStatusIcon.WBP_BattleStatusIcon");
	constexpr TCHAR VitalsMaterialObjectPath[] =
		TEXT("/Game/DreamMaterials/UI/MI_WacomBattle_PlayerVitals_Default.MI_WacomBattle_PlayerVitals_Default");
	constexpr TCHAR ContractMarker[] = TEXT("WacomPlayerStatus.ContractVersion=2");

	const FName DamageAnimationName(TEXT("DamagePulseAnimation"));
	const FName ShieldAnimationName(TEXT("ShieldPulseAnimation"));

	void RegisterWidgetGuid(UWidgetBlueprint& Blueprint, const UWidget& Widget)
	{
		const FString StablePath = FString::Printf(
			TEXT("%s:%s"), *Blueprint.GetPathName(), *Widget.GetName());
		Blueprint.WidgetVariableNameToGuidMap.FindOrAdd(Widget.GetFName()) =
			FGuid::NewDeterministicGuid(StablePath);
	}

	void MarkWidgetVariable(UWidgetBlueprint& Blueprint, UWidget& Widget)
	{
		Widget.bIsVariable = true;
		RegisterWidgetGuid(Blueprint, Widget);
	}

	void MakeTreeNonHitTestable(UWidgetBlueprint& Blueprint)
	{
		TArray<UWidget*> Widgets;
		Blueprint.WidgetTree->GetAllWidgets(Widgets);
		for (UWidget* Widget : Widgets)
		{
			if (!Widget)
			{
				continue;
			}
			RegisterWidgetGuid(Blueprint, *Widget);
			Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
	}

	void LogWidgetTreeForAudit(const UWidgetBlueprint& Blueprint)
	{
		if (!Blueprint.WidgetTree)
		{
			return;
		}
		UE_LOG(LogTemp, Display, TEXT("[PlayerStatusUIBuilder] Widget tree: %s"),
			*Blueprint.GetPathName());
		Blueprint.WidgetTree->ForEachWidget([](UWidget* Widget)
		{
			if (Widget)
			{
				UE_LOG(LogTemp, Display, TEXT("[PlayerStatusUIBuilder]   %s : %s"),
					*Widget->GetName(), *Widget->GetClass()->GetName());
			}
		});
	}

	template <typename TWidget>
	TWidget* FindUniqueWidgetOfType(UWidgetBlueprint& Blueprint)
	{
		if (!Blueprint.WidgetTree)
		{
			return nullptr;
		}

		TWidget* Result = nullptr;
		bool bFoundMultiple = false;
		Blueprint.WidgetTree->ForEachWidget([&Result, &bFoundMultiple](UWidget* Widget)
		{
			TWidget* Candidate = Cast<TWidget>(Widget);
			if (!Candidate)
			{
				return;
			}
			if (Result)
			{
				bFoundMultiple = true;
				return;
			}
			Result = Candidate;
		});
		return bFoundMultiple ? nullptr : Result;
	}

	TSet<FName> BuildLiveWidgetVariableNames(UWidgetBlueprint& Blueprint)
	{
		TSet<FName> LiveNames;
		if (Blueprint.WidgetTree)
		{
			Blueprint.WidgetTree->ForEachWidget([&LiveNames](UWidget* Widget)
			{
				if (Widget)
				{
					LiveNames.Add(Widget->GetFName());
				}
			});
		}
		for (const UWidgetAnimation* Animation : Blueprint.Animations)
		{
			if (Animation)
			{
				LiveNames.Add(Animation->GetFName());
			}
		}
		return LiveNames;
	}

	bool HasStaleWidgetVariableGuids(UWidgetBlueprint& Blueprint)
	{
		const TSet<FName> LiveNames = BuildLiveWidgetVariableNames(Blueprint);
		for (const TPair<FName, FGuid>& Entry : Blueprint.WidgetVariableNameToGuidMap)
		{
			if (!LiveNames.Contains(Entry.Key))
			{
				return true;
			}
		}
		return false;
	}

	void RemoveStaleWidgetVariableGuids(UWidgetBlueprint& Blueprint)
	{
		const TSet<FName> LiveNames = BuildLiveWidgetVariableNames(Blueprint);
		for (auto It = Blueprint.WidgetVariableNameToGuidMap.CreateIterator(); It; ++It)
		{
			if (!LiveNames.Contains(It.Key()))
			{
				It.RemoveCurrent();
			}
		}
	}

	bool IsRecognizedLayout(const UWidgetBlueprint& Blueprint)
	{
		return Blueprint.ParentClass
			&& Blueprint.ParentClass->IsChildOf(UPlayerStatusBar::StaticClass())
			&& Blueprint.WidgetTree
			&& Blueprint.WidgetTree->RootWidget
			&& (Blueprint.WidgetTree->FindWidget(TEXT("HpBar"))
				|| Blueprint.WidgetTree->FindWidget(TEXT("VitalsTrackImage")));
	}

	bool HasVitalsContract(UWidgetBlueprint& Blueprint)
	{
		if (!IsRecognizedLayout(Blueprint) || !Blueprint.WidgetTree)
		{
			return false;
		}

		const UImage* Track = Cast<UImage>(
			Blueprint.WidgetTree->FindWidget(TEXT("VitalsTrackImage")));
		const UTextBlock* HpText = Cast<UTextBlock>(
			Blueprint.WidgetTree->FindWidget(TEXT("HpValueText")));
		const USizeBox* ShieldRoot = Cast<USizeBox>(
			Blueprint.WidgetTree->FindWidget(TEXT("ShieldValueRoot")));
		const UTextBlock* ShieldText = Cast<UTextBlock>(
			Blueprint.WidgetTree->FindWidget(TEXT("ShieldText")));
		UWacomBattleStatusIconListWidget* StatusList =
			FindUniqueWidgetOfType<UWacomBattleStatusIconListWidget>(
				Blueprint);
		const bool bLegacyAnimationsRemoved = !Blueprint.Animations.ContainsByPredicate(
			[](const UWidgetAnimation* Animation)
			{
				return Animation
					&& (Animation->GetFName() == DamageAnimationName
						|| Animation->GetFName() == ShieldAnimationName);
			});
		return Track
			&& Cast<UMaterialInterface>(Track->GetBrush().GetResourceObject())
			&& HpText
			&& ShieldRoot
			&& ShieldText
			&& StatusList
			&& bLegacyAnimationsRemoved;
	}

	void StyleText(UTextBlock& Text, const int32 FontSize, const FLinearColor& Color)
	{
		FSlateFontInfo Font = Text.GetFont();
		Font.Size = FontSize;
		Font.TypefaceFontName = TEXT("Bold");
		Text.SetFont(Font);
		Text.SetColorAndOpacity(FSlateColor(Color));
		Text.SetJustification(ETextJustify::Center);
		Text.SetShadowOffset(FVector2D(1.0f, 1.0f));
		Text.SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.85f));
	}

	bool BuildVitalsTree(UWidgetBlueprint& Blueprint, UMaterialInterface& VitalsMaterial)
	{
		UWacomBattleStatusIconListWidget* StatusList =
			FindUniqueWidgetOfType<UWacomBattleStatusIconListWidget>(Blueprint);
		if (!StatusList)
		{
			LogWidgetTreeForAudit(Blueprint);
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Existing StatusList has an incompatible type"));
			return false;
		}
		StatusList->RemoveFromParent();

		UTextBlock* ShieldText = Cast<UTextBlock>(
			Blueprint.WidgetTree->FindWidget(TEXT("ShieldText")));
		if (!ShieldText)
		{
			ShieldText = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
				UTextBlock::StaticClass(), TEXT("ShieldText"));
			MarkWidgetVariable(Blueprint, *ShieldText);
		}
		else
		{
			ShieldText->RemoveFromParent();
		}

		USizeBox* Root = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("PlayerStatusRootV2"));
		Root->SetWidthOverride(680.0f);
		Root->SetHeightOverride(86.0f);
		Blueprint.WidgetTree->RootWidget = Root;

		UVerticalBox* Column = Blueprint.WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(), TEXT("PlayerStatusColumnV2"));
		Root->AddChild(Column);

		USizeBox* VitalsBox = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("VitalsSizeBoxV2"));
		VitalsBox->SetWidthOverride(680.0f);
		VitalsBox->SetHeightOverride(46.0f);
		Column->AddChildToVerticalBox(VitalsBox);

		UOverlay* VitalsOverlay = Blueprint.WidgetTree->ConstructWidget<UOverlay>(
			UOverlay::StaticClass(), TEXT("VitalsOverlayV2"));
		VitalsBox->AddChild(VitalsOverlay);

		UImage* Track = Blueprint.WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(), TEXT("VitalsTrackImage"));
		Track->SetBrushFromMaterial(&VitalsMaterial);
		MarkWidgetVariable(Blueprint, *Track);
		if (UOverlaySlot* Slot = VitalsOverlay->AddChildToOverlay(Track))
		{
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Fill);
		}

		UTextBlock* HpText = Blueprint.WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(), TEXT("HpValueText"));
		HpText->SetText(FText::FromString(TEXT("300 / 300")));
		StyleText(*HpText, 22, FLinearColor::White);
		MarkWidgetVariable(Blueprint, *HpText);
		if (UOverlaySlot* Slot = VitalsOverlay->AddChildToOverlay(HpText))
		{
			Slot->SetPadding(FMargin(72.0f, 0.0f, 100.0f, 0.0f));
			Slot->SetHorizontalAlignment(HAlign_Fill);
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		USizeBox* ShieldRoot = Blueprint.WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("ShieldValueRoot"));
		ShieldRoot->SetWidthOverride(76.0f);
		ShieldRoot->SetHeightOverride(46.0f);
		ShieldRoot->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
		MarkWidgetVariable(Blueprint, *ShieldRoot);
		if (UOverlaySlot* Slot = VitalsOverlay->AddChildToOverlay(ShieldRoot))
		{
			Slot->SetHorizontalAlignment(HAlign_Right);
			Slot->SetVerticalAlignment(VAlign_Center);
		}

		ShieldText->SetText(FText::AsNumber(19));
		StyleText(*ShieldText, 20, FLinearColor(0.55f, 0.84f, 1.0f, 1.0f));
		ShieldRoot->AddChild(ShieldText);

		if (UVerticalBoxSlot* Slot = Column->AddChildToVerticalBox(StatusList))
		{
			Slot->SetPadding(FMargin(10.0f, 5.0f, 0.0f, 0.0f));
			Slot->SetHorizontalAlignment(HAlign_Left);
			Slot->SetVerticalAlignment(VAlign_Top);
		}

		Blueprint.Animations.RemoveAll([](const UWidgetAnimation* Animation)
		{
			return Animation
				&& (Animation->GetFName() == DamageAnimationName
					|| Animation->GetFName() == ShieldAnimationName);
		});
		return true;
	}

	bool ConfigureBattleHudPlacement(UWidgetBlueprint& BattleHud, const bool bInspectOnly)
	{
		UWidget* StatusBar = BattleHud.WidgetTree
			? BattleHud.WidgetTree->FindWidget(TEXT("PlayerStatusBar"))
			: nullptr;
		UWidget* CommandBar = BattleHud.WidgetTree
			? BattleHud.WidgetTree->FindWidget(TEXT("CommandBar"))
			: nullptr;
		UCanvasPanelSlot* Slot = StatusBar ? Cast<UCanvasPanelSlot>(StatusBar->Slot) : nullptr;
		if (!Slot || !CommandBar)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] BP_BattleHUD is missing the PlayerStatusBar Canvas slot or CommandBar"));
			return false;
		}

		const FMargin Offsets = Slot->GetOffsets();
		const bool bOffsetsReady = FMath::IsNearlyEqual(Offsets.Left, 28.0f, 0.01f)
			&& FMath::IsNearlyEqual(Offsets.Top, 24.0f, 0.01f)
			&& FMath::IsNearlyEqual(Offsets.Right, 680.0f, 0.01f)
			&& FMath::IsNearlyEqual(Offsets.Bottom, 86.0f, 0.01f);
		const bool bPlacementReady = Slot->GetAnchors() == FAnchors(0.0f, 0.0f)
			&& Slot->GetAlignment().Equals(FVector2D::ZeroVector)
			&& bOffsetsReady
			&& !Slot->GetAutoSize();
		const ESlateVisibility CommandBarVisibility = CommandBar->GetVisibility();
		const bool bCommandBarAllowsChildHitTesting =
			CommandBarVisibility == ESlateVisibility::Visible
			|| CommandBarVisibility == ESlateVisibility::SelfHitTestInvisible;
		const bool bReady = bPlacementReady && bCommandBarAllowsChildHitTesting;
		if (bReady || bInspectOnly)
		{
			return bReady;
		}

		BattleHud.Modify();
		Slot->Modify();
		Slot->SetAnchors(FAnchors(0.0f, 0.0f));
		Slot->SetAlignment(FVector2D::ZeroVector);
		Slot->SetOffsets(FMargin(28.0f, 24.0f, 680.0f, 86.0f));
		Slot->SetAutoSize(false);
		if (!bCommandBarAllowsChildHitTesting)
		{
			CommandBar->Modify();
			CommandBar->SetVisibility(ESlateVisibility::Visible);
		}
		return true;
	}

	bool ConfigureStatusIcon(UWidgetBlueprint& Blueprint, const bool bInspectOnly)
	{
		USizeBox* Root = FindUniqueWidgetOfType<USizeBox>(Blueprint);
		UTextBlock* StackText = Blueprint.WidgetTree
			? Cast<UTextBlock>(Blueprint.WidgetTree->FindWidget(TEXT("StackText")))
			: nullptr;
		if (!Root || !StackText)
		{
			LogWidgetTreeForAudit(Blueprint);
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Status icon layout is not recognized"));
			return false;
		}

		const bool bReady = FMath::IsNearlyEqual(Root->GetWidthOverride(), 32.0f)
			&& FMath::IsNearlyEqual(Root->GetHeightOverride(), 32.0f)
			&& StackText->GetFont().Size == 11;
		if (bReady || bInspectOnly)
		{
			return bReady;
		}

		Blueprint.Modify();
		Root->Modify();
		StackText->Modify();
		Root->SetWidthOverride(32.0f);
		Root->SetHeightOverride(32.0f);
		FSlateFontInfo Font = StackText->GetFont();
		Font.Size = 11;
		Font.TypefaceFontName = TEXT("Bold");
		StackText->SetFont(Font);
		StackText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		StackText->SetShadowColorAndOpacity(FLinearColor(0.0f, 0.0f, 0.0f, 0.90f));
		return true;
	}

	bool CompileAndSave(
		UWidgetBlueprint& Blueprint,
		const bool bMakeEntireTreeNonHitTestable)
	{
		if (bMakeEntireTreeNonHitTestable)
		{
			MakeTreeNonHitTestable(Blueprint);
		}
		RemoveStaleWidgetVariableGuids(Blueprint);
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(&Blueprint);
		FKismetEditorUtilities::CompileBlueprint(&Blueprint);
		if (Blueprint.Status == BS_Error || !Blueprint.GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Compile failed: %s"), *Blueprint.GetPathName());
			return false;
		}

		UPackage* Package = Blueprint.GetOutermost();
		Package->MarkPackageDirty();
		Blueprint.MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			Package->GetName(), FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}
}

bool Wacom::ContentBuilder::ProcessPlayerStatusVitalsUI(
	const bool bBuildVitalsV2,
	const bool bInspectOnly)
{
	if (bBuildVitalsV2 == bInspectOnly)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Choose exactly one build or inspect mode"));
		return false;
	}

	UWidgetBlueprint* Blueprint = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr, PlayerStatusObjectPath));
	UWidgetBlueprint* BattleHud = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr, BattleHudObjectPath));
	UWidgetBlueprint* StatusIcon = Cast<UWidgetBlueprint>(StaticLoadObject(
		UWidgetBlueprint::StaticClass(), nullptr, StatusIconObjectPath));
	UMaterialInterface* VitalsMaterial = Cast<UMaterialInterface>(StaticLoadObject(
		UMaterialInterface::StaticClass(), nullptr, VitalsMaterialObjectPath));
	if (!Blueprint || !BattleHud || !StatusIcon || !VitalsMaterial)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Missing PlayerStatus WBP, BattleHUD WBP, or Vitals MI"));
		return false;
	}
	if (!IsRecognizedLayout(*Blueprint))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[PlayerStatusUIBuilder] Refusing unrecognized manual layout: %s"),
			*Blueprint->GetPathName());
		return false;
	}

	const bool bStatusReady = HasVitalsContract(*Blueprint);
	const bool bStatusHasStaleGuids = HasStaleWidgetVariableGuids(*Blueprint);
	const bool bHudReady = ConfigureBattleHudPlacement(*BattleHud, true);
	const bool bStatusIconReady = ConfigureStatusIcon(*StatusIcon, true);
	if (bInspectOnly)
	{
		if (!bStatusReady || !bHudReady || !bStatusIconReady)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] V2 contract incomplete: Status=%d HUD=%d Icon=%d"),
				bStatusReady, bHudReady, bStatusIconReady);
			return false;
		}
		UE_LOG(LogTemp, Display, TEXT("[PlayerStatusUIBuilder] V2 contract ready"));
		return true;
	}

	bool bChanged = false;
	if (!bStatusReady || bStatusHasStaleGuids)
	{
		Blueprint->Modify();
		if (!bStatusReady && !BuildVitalsTree(*Blueprint, *VitalsMaterial))
		{
			return false;
		}
		if (!Blueprint->BlueprintDescription.Contains(ContractMarker))
		{
			Blueprint->BlueprintDescription = Blueprint->BlueprintDescription.IsEmpty()
				? FString(ContractMarker)
				: Blueprint->BlueprintDescription + TEXT("\n") + ContractMarker;
		}
		if (!CompileAndSave(*Blueprint, true) || !HasVitalsContract(*Blueprint))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Saved PlayerStatus WBP failed V2 validation"));
			return false;
		}
		bChanged = true;
	}

	if (!bHudReady)
	{
		if (!ConfigureBattleHudPlacement(*BattleHud, false)
			|| !CompileAndSave(*BattleHud, false)
			|| !ConfigureBattleHudPlacement(*BattleHud, true))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Failed to persist BP_BattleHUD placement"));
			return false;
		}
		bChanged = true;
	}

	if (!bStatusIconReady)
	{
		if (!ConfigureStatusIcon(*StatusIcon, false)
			|| !CompileAndSave(*StatusIcon, true)
			|| !ConfigureStatusIcon(*StatusIcon, true))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[PlayerStatusUIBuilder] Failed to persist status-icon styling"));
			return false;
		}
		bChanged = true;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[PlayerStatusUIBuilder] V2 contract %s"),
		bChanged ? TEXT("built") : TEXT("already ready"));
	return true;
}
