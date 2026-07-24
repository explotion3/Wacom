// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "NiagaraEmitterHandle.h"
#include "NiagaraSystem.h"
#include "NiagaraTypes.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleFloatingCombatTextEntryWidget.h"
#include "UI/Battle/WacomBattleFloatingCombatTextLayerWidget.h"
#include "UI/Battle/WacomBattleFloatingCombatTextStyle.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "WidgetBlueprint.h"

namespace WacomBattleFloatingCombatTextAssetSpec
{
	constexpr TCHAR EntryObject[] =
		TEXT("/Game/Wacom/UI/Battle/FloatingText/WBP_BattleFloatingCombatTextEntry.WBP_BattleFloatingCombatTextEntry");
	constexpr TCHAR LayerObject[] =
		TEXT("/Game/Wacom/UI/Battle/FloatingText/WBP_BattleFloatingCombatTextLayer.WBP_BattleFloatingCombatTextLayer");
	constexpr TCHAR StyleObject[] =
		TEXT("/Game/Wacom/UI/Battle/FloatingText/DA_BattleFloatingCombatTextStyle_Default.DA_BattleFloatingCombatTextStyle_Default");
	constexpr TCHAR BattleHudObject[] =
		TEXT("/Game/Wacom/UI/Battle/BP_BattleHUD.BP_BattleHUD");
	constexpr TCHAR ShieldSystemObject[] =
		TEXT("/Game/Wacom/VFX/Battle/FloatingText/NS_WacomBattleFloatingShield_Pixel.NS_WacomBattleFloatingShield_Pixel");
	constexpr TCHAR PeriodicSystemObject[] =
		TEXT("/Game/Wacom/VFX/Battle/FloatingText/NS_WacomBattleFloatingPeriodic_Pixel.NS_WacomBattleFloatingPeriodic_Pixel");
	constexpr TCHAR CriticalSystemObject[] =
		TEXT("/Game/Wacom/VFX/Battle/FloatingText/NS_WacomBattleFloatingCritical_Pixel.NS_WacomBattleFloatingCritical_Pixel");

	bool HasFloatingTextContract(const UObject& Asset)
	{
		UPackage* Package = Asset.GetPackage();
		return Package
			&& Package->GetMetaData().GetValue(
				&Asset,
				TEXT("WacomFloatingCombatTextContractVersion")) == TEXT("1");
	}

	bool HasValidAccentEmitter(
		const UNiagaraSystem& System,
		const FString& ExpectedKind)
	{
		if (!HasFloatingTextContract(System)
			|| System.GetPackage()->GetMetaData().GetValue(
				&System,
				TEXT("WacomFloatingCombatTextNiagaraKind")) != ExpectedKind)
		{
			return false;
		}

		const TArray<FNiagaraEmitterHandle>& Emitters = System.GetEmitterHandles();
		if (Emitters.Num() != 1
			|| Emitters[0].GetName() != TEXT("ConfirmStamp"))
		{
			return false;
		}

		const FNiagaraVariable ShieldBroken(
			FNiagaraTypeDefinition::GetBoolDef(),
			TEXT("User.ShieldBroken"));
		return System.GetExposedParameters().IndexOf(ShieldBroken) != INDEX_NONE;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFloatingCombatTextAssetContractSpec,
	"Wacom.UI.Battle.FloatingCombatText.Assets.FormalContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFloatingCombatTextAssetContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleFloatingCombatTextAssetSpec;

	UWidgetBlueprint* EntryBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		EntryObject);
	UWidgetBlueprint* LayerBlueprint = LoadObject<UWidgetBlueprint>(
		nullptr,
		LayerObject);
	UWacomBattleFloatingCombatTextStyle* Style =
		LoadObject<UWacomBattleFloatingCombatTextStyle>(
			nullptr,
			StyleObject);
	UWidgetBlueprint* BattleHud = LoadObject<UWidgetBlueprint>(
		nullptr,
		BattleHudObject);
	UNiagaraSystem* Shield = LoadObject<UNiagaraSystem>(
		nullptr,
		ShieldSystemObject);
	UNiagaraSystem* Periodic = LoadObject<UNiagaraSystem>(
		nullptr,
		PeriodicSystemObject);
	UNiagaraSystem* Critical = LoadObject<UNiagaraSystem>(
		nullptr,
		CriticalSystemObject);

	TestNotNull(TEXT("Floating Text Entry WBP exists"), EntryBlueprint);
	TestNotNull(TEXT("Floating Text Layer WBP exists"), LayerBlueprint);
	TestNotNull(TEXT("Floating Text Style exists"), Style);
	TestNotNull(TEXT("BattleHUD exists"), BattleHud);
	TestNotNull(TEXT("Shield decoration Niagara exists"), Shield);
	TestNotNull(TEXT("Periodic decoration Niagara exists"), Periodic);
	TestNotNull(TEXT("Critical decoration Niagara exists"), Critical);
	if (!EntryBlueprint || !LayerBlueprint || !Style || !BattleHud
		|| !Shield || !Periodic || !Critical)
	{
		return false;
	}

	TestTrue(
		TEXT("Entry WBP derives from the passive pooled Entry class"),
		EntryBlueprint->ParentClass
			&& EntryBlueprint->ParentClass->IsChildOf(
				UWacomBattleFloatingCombatTextEntryWidget::StaticClass()));
	TestTrue(
		TEXT("Entry WBP compiled"),
		EntryBlueprint->GeneratedClass && EntryBlueprint->Status != BS_Error);
	TestNotNull(
		TEXT("Entry owns SemanticIcon binding"),
		EntryBlueprint->WidgetTree
			? Cast<UImage>(
				EntryBlueprint->WidgetTree->FindWidget(TEXT("SemanticIcon")))
			: nullptr);
	TestNotNull(
		TEXT("Entry owns CriticalText binding"),
		EntryBlueprint->WidgetTree
			? Cast<UTextBlock>(
				EntryBlueprint->WidgetTree->FindWidget(TEXT("CriticalText")))
			: nullptr);
	TestNotNull(
		TEXT("Entry owns ValueText binding"),
		EntryBlueprint->WidgetTree
			? Cast<UTextBlock>(
				EntryBlueprint->WidgetTree->FindWidget(TEXT("ValueText")))
			: nullptr);
	TestTrue(
		TEXT("Entry is passive and cannot intercept input"),
		EntryBlueprint->WidgetTree
			&& EntryBlueprint->WidgetTree->RootWidget
			&& EntryBlueprint->WidgetTree->RootWidget->GetVisibility()
				== ESlateVisibility::HitTestInvisible);

	TestTrue(
		TEXT("Layer WBP derives from the HUD-owned playback layer"),
		LayerBlueprint->ParentClass
			&& LayerBlueprint->ParentClass->IsChildOf(
				UWacomBattleFloatingCombatTextLayerWidget::StaticClass()));
	const UCanvasPanel* EntryCanvas = LayerBlueprint->WidgetTree
		? Cast<UCanvasPanel>(
			LayerBlueprint->WidgetTree->FindWidget(TEXT("EntryCanvas")))
		: nullptr;
	TestNotNull(TEXT("Layer owns EntryCanvas binding"), EntryCanvas);
	TestTrue(
		TEXT("Layer canvas cannot intercept input"),
		EntryCanvas
			&& EntryCanvas->GetVisibility()
				== ESlateVisibility::HitTestInvisible);

	const UObject* LayerDefaults = LayerBlueprint->GeneratedClass
		? LayerBlueprint->GeneratedClass->GetDefaultObject()
		: nullptr;
	const FClassProperty* EntryClassProperty =
		LayerBlueprint->GeneratedClass
			? FindFProperty<FClassProperty>(
				LayerBlueprint->GeneratedClass,
				TEXT("EntryWidgetClass"))
			: nullptr;
	TestTrue(
		TEXT("Layer uses the formal pooled Entry WBP class"),
		LayerDefaults
			&& EntryClassProperty
			&& EntryClassProperty->GetPropertyValue_InContainer(LayerDefaults)
				== EntryBlueprint->GeneratedClass);

	TArray<FText> StyleErrors;
	TestTrue(TEXT("Floating Text Style validates"), Style->ValidateStyle(StyleErrors));
	TestTrue(TEXT("Style has the shield icon"), Style->ShieldIconBrush.GetResourceObject() != nullptr);
	TestTrue(TEXT("Style references Shield Niagara"), Style->ShieldNiagara == Shield);
	TestTrue(TEXT("Style references Periodic Niagara"), Style->PeriodicNiagara == Periodic);
	TestTrue(TEXT("Style references Critical Niagara"), Style->CriticalNiagara == Critical);
	TestTrue(TEXT("Style carries the formal asset contract"), HasFloatingTextContract(*Style));

	TestTrue(
		TEXT("Shield Niagara owns one compiled accent-emitter contract"),
		HasValidAccentEmitter(*Shield, TEXT("Shield")));
	TestTrue(
		TEXT("Periodic Niagara owns one compiled accent-emitter contract"),
		HasValidAccentEmitter(*Periodic, TEXT("Periodic")));
	TestTrue(
		TEXT("Critical Niagara owns one compiled accent-emitter contract"),
		HasValidAccentEmitter(*Critical, TEXT("Critical")));

	UWidget* FloatingLayer = BattleHud->WidgetTree
		? BattleHud->WidgetTree->FindWidget(TEXT("FloatingCombatTextLayer"))
		: nullptr;
	const UCanvasPanelSlot* FloatingLayerSlot =
		FloatingLayer ? Cast<UCanvasPanelSlot>(FloatingLayer->Slot) : nullptr;
	TestTrue(
		TEXT("BattleHUD embeds the formal Floating Text Layer class"),
		FloatingLayer
			&& FloatingLayer->GetClass() == LayerBlueprint->GeneratedClass);
	TestTrue(
		TEXT("BattleHUD Floating Text Layer cannot intercept input"),
		FloatingLayer
			&& FloatingLayer->GetVisibility()
				== ESlateVisibility::HitTestInvisible);
	if (FloatingLayerSlot)
	{
		const FAnchors Anchors = FloatingLayerSlot->GetAnchors();
		const FMargin Offsets = FloatingLayerSlot->GetOffsets();
		TestTrue(
			TEXT("BattleHUD Floating Text Layer fills the viewport"),
			Anchors.Minimum.Equals(FVector2D::ZeroVector)
				&& Anchors.Maximum.Equals(FVector2D(1.0f, 1.0f))
				&& Offsets.Left == 0.0f
				&& Offsets.Top == 0.0f
				&& Offsets.Right == 0.0f
				&& Offsets.Bottom == 0.0f);
		TestEqual(
			TEXT("Floating Text Layer stays under modal and details screens"),
			FloatingLayerSlot->GetZOrder(),
			9);
	}
	else
	{
		AddError(TEXT("BattleHUD Floating Text Layer does not use a Canvas slot."));
	}

	return true;
}
