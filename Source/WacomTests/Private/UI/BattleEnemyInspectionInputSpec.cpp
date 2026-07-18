// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/HorizontalBox.h"
#include "Components/PanelWidget.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UI/Battle/WacomBattleEnemyPartEntryWidget.h"
#include "UObject/UnrealType.h"
#include "../../../WacomApp/Private/UI/Battle/WacomBattleEnemyUILayerPolicy.h"

namespace WacomBattleEnemyInspectionInputSpec
{
	constexpr TCHAR MultiPartPanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/BP_WacomBattleEnemyPanelWidget.BP_WacomBattleEnemyPanelWidget_C");
	constexpr TCHAR SinglePartPanelClassPath[] =
		TEXT("/Game/Wacom/UI/Enemy/WBP_WacomBattleEnemySinglePartPanelWidget.WBP_WacomBattleEnemySinglePartPanelWidget_C");

	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	FName ReadSharedLayerName(const UWidgetComponent& Component)
	{
		const FNameProperty* Property = FindFProperty<FNameProperty>(
			UWidgetComponent::StaticClass(), TEXT("SharedLayerName"));
		return Property ? Property->GetPropertyValue_InContainer(&Component) : NAME_None;
	}

	int32 ReadLayerZOrder(const UWidgetComponent& Component)
	{
		const FIntProperty* Property = FindFProperty<FIntProperty>(
			UWidgetComponent::StaticClass(), TEXT("LayerZOrder"));
		return Property ? Property->GetPropertyValue_InContainer(&Component) : MIN_int32;
	}

	FWacomBattleEnemyPanelViewData MakeEnemyView()
	{
		FWacomBattleEnemyPanelViewData Enemy;
		Enemy.EncounterId = TEXT("Encounter");
		Enemy.EnemySlotId = TEXT("Enemy");
		Enemy.UnitKey = FBattleEnemyUnitKey::Make(Enemy.EncounterId, Enemy.EnemySlotId);

		FWacomBattleEnemyPartEntryViewData Part;
		Part.EnemySlotId = Enemy.EnemySlotId;
		Part.PartSlotId = TEXT("Body");
		Part.Identity = FBattlePartSlotIdentity::Make(
			Enemy.EncounterId, Enemy.EnemySlotId, Part.PartSlotId);
		Part.CurrentHp = 18;
		Part.MaxHp = 24;
		Enemy.Parts.Add(Part);
		return Enemy;
	}

	bool AllowsDescendantHitTesting(const ESlateVisibility Visibility)
	{
		return Visibility == ESlateVisibility::Visible
			|| Visibility == ESlateVisibility::SelfHitTestInvisible;
	}

	bool TestAncestorPath(
		FAutomationTestBase& Test,
		const FString& Label,
		const UWidget* Descendant)
	{
		bool bValid = true;
		for (const UWidget* Ancestor = Descendant ? Descendant->GetParent() : nullptr;
			Ancestor;
			Ancestor = Ancestor->GetParent())
		{
			const ESlateVisibility Visibility = Ancestor->GetVisibility();
			bValid &= Test.TestTrue(
				*FString::Printf(
					TEXT("%s ancestor %s allows descendant hit testing (Visibility=%d)"),
					*Label,
					*Ancestor->GetName(),
					static_cast<int32>(Visibility)),
				AllowsDescendantHitTesting(Visibility));
		}
		return bValid;
	}

	bool TestPanelHotspotPath(
		FAutomationTestBase& Test,
		UWorld& World,
		const TCHAR* PanelClassPath,
		const FString& Label)
	{
		UClass* PanelClass = LoadClass<UWacomBattleEnemyPanelWidget>(nullptr, PanelClassPath);
		UWacomBattleEnemyPanelWidget* Panel = PanelClass
			? CreateWidget<UWacomBattleEnemyPanelWidget>(&World, PanelClass)
			: nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s panel"), *Label), Panel))
		{
			return false;
		}

		Panel->TakeWidget();
		Panel->SetEnemyPanelViewData(MakeEnemyView());
		Panel->SetInspectionInteractionEnabled(true);
		UHorizontalBox* PartList = Panel->WidgetTree
			? Cast<UHorizontalBox>(Panel->WidgetTree->FindWidget(TEXT("PartList")))
			: nullptr;
		UWacomBattleEnemyPartEntryWidget* Entry = PartList && PartList->GetChildrenCount() == 1
			? Cast<UWacomBattleEnemyPartEntryWidget>(PartList->GetChildAt(0))
			: nullptr;
		UButton* InspectHitTarget = Entry && Entry->WidgetTree
			? Cast<UButton>(Entry->WidgetTree->FindWidget(TEXT("InspectHitTarget")))
			: nullptr;
		if (!Test.TestNotNull(*FString::Printf(TEXT("%s PartList"), *Label), PartList)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s entry"), *Label), Entry)
			|| !Test.TestNotNull(*FString::Printf(TEXT("%s InspectHitTarget"), *Label), InspectHitTarget))
		{
			return false;
		}

		bool bValid = true;
		bValid &= Test.TestEqual(
			*FString::Printf(TEXT("%s entry exposes child hit testing"), *Label),
			Entry->GetVisibility(),
			ESlateVisibility::SelfHitTestInvisible);
		bValid &= Test.TestEqual(
			*FString::Printf(TEXT("%s hotspot is visible"), *Label),
			InspectHitTarget->GetVisibility(),
			ESlateVisibility::Visible);
		bValid &= Test.TestTrue(
			*FString::Printf(TEXT("%s hotspot is enabled"), *Label),
			InspectHitTarget->GetIsEnabled());
		bValid &= TestAncestorPath(Test, Label + TEXT(" entry"), InspectHitTarget);
		bValid &= TestAncestorPath(Test, Label + TEXT(" panel"), PartList);
		Panel->ClearEnemyPanelViewData();
		return bValid;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyInspectionScreenLayerSpec,
	"Wacom.UI.Battle.EnemyInspection.ScreenLayerInputRouting",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyInspectionScreenLayerSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInspectionInputSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
	if (!TestNotNull(TEXT("Scene enemy host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	UWidgetComponent* EnemyPanelComponent = Host->FindComponentByClass<UWidgetComponent>();
	if (!TestNotNull(TEXT("Enemy panel WidgetComponent"), EnemyPanelComponent))
	{
		return false;
	}

	TestEqual(TEXT("Enemy panel remains a screen-space projection"),
		EnemyPanelComponent->GetWidgetSpace(), EWidgetSpace::Screen);
	TestEqual(TEXT("Enemy panel uses its dedicated screen layer"),
		ReadSharedLayerName(*EnemyPanelComponent),
		WacomBattleEnemyUILayerPolicy::CompactPanelSharedLayerName);
	TestEqual(TEXT("Enemy panel screen layer renders above the base BattleHUD"),
		ReadLayerZOrder(*EnemyPanelComponent),
		WacomBattleEnemyUILayerPolicy::CompactPanelZOrder);
	TestTrue(TEXT("Enemy panel screen layer is above default viewport/CommonUI content"),
		ReadLayerZOrder(*EnemyPanelComponent) > 0);
	TestTrue(TEXT("Enemy panel screen layer remains below the inspection overlay"),
		ReadLayerZOrder(*EnemyPanelComponent)
			< WacomBattleEnemyUILayerPolicy::InspectionPanelZOrder);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEnemyInspectionHotspotAncestrySpec,
	"Wacom.UI.Battle.EnemyInspection.HotspotHitTestAncestry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEnemyInspectionHotspotAncestrySpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInspectionInputSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	TestPanelHotspotPath(*this, *World, MultiPartPanelClassPath, TEXT("Multi-part"));
	TestPanelHotspotPath(*this, *World, SinglePartPanelClassPath, TEXT("Single-part"));
	return !HasAnyErrors();
}
