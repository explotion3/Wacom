// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Components/WacomRunEncounterSceneBindingComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Misc/ScopeExit.h"

namespace WacomEncounterSceneBindingSpec
{
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

	struct FHarness
	{
		UWorld* World = nullptr;
		AWacomRunMapNodeAnchorActor* Anchor = nullptr;
		UWacomRunEncounterSceneBindingComponent* Binding = nullptr;
		UEncounterDefinition* Encounter = nullptr;
		TArray<AWacomBattleEnemyActor*> Hosts;

		FHarness()
		{
			World = FindAutomationWorld();
			if (!World)
			{
				return;
			}
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			Anchor = World->SpawnActor<AWacomRunMapNodeAnchorActor>(
				AWacomRunMapNodeAnchorActor::StaticClass(),
				FTransform::Identity,
				Params);
			if (!Anchor)
			{
				return;
			}
			Anchor->NodeId = TEXT("Encounter.Test");
			Binding = NewObject<UWacomRunEncounterSceneBindingComponent>(
				Anchor, TEXT("EncounterSceneBinding"), RF_Transient);
			Anchor->AddInstanceComponent(Binding);
			Binding->RegisterComponent();
			Encounter = NewObject<UEncounterDefinition>(Anchor);
			Encounter->EncounterDefinitionId = TEXT("Encounter.Test");
		}

		~FHarness()
		{
			for (AWacomBattleEnemyActor* Host : Hosts)
			{
				if (::IsValid(Host))
				{
					Host->Destroy();
				}
			}
			if (::IsValid(Anchor))
			{
				Anchor->Destroy();
			}
		}

		UEnemyDefinition* AddRuleSlot(const FName SlotId)
		{
			UEnemyDefinition* Enemy =
				NewObject<UEnemyDefinition>(Encounter);
			Enemy->EnemyId = FName(
				*FString::Printf(TEXT("Enemy.%s"), *SlotId.ToString()));
			FEncounterEnemySlot& Slot =
				Encounter->EnemySlots.AddDefaulted_GetRef();
			Slot.EnemySlotId = SlotId;
			Slot.EnemyDefinition = Enemy;
			return Enemy;
		}

		AWacomBattleEnemyActor* SpawnHost(
			const FName AuthoredSlotId,
			UEnemyDefinition* Enemy)
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			AWacomBattleEnemyActor* Host =
				World->SpawnActor<AWacomBattleEnemyActor>(
					AWacomBattleEnemyActor::StaticClass(),
					FTransform::Identity,
					Params);
			if (Host)
			{
				Host->EnemySlotId = AuthoredSlotId;
				Host->EnemyDefinition = Enemy;
				Hosts.Add(Host);
			}
			return Host;
		}

		void AddSceneSlot(
			const FName SlotId,
			AWacomBattleEnemyActor* Host)
		{
			FWacomBattleSceneEnemyHostSlot& Slot =
				Binding->SceneEnemyHostSlots.AddDefaulted_GetRef();
			Slot.EnemySlotId = SlotId;
			Slot.SceneEnemyHost = Host;
		}

		bool IsValid() const
		{
			return World && Anchor && Binding && Encounter;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneBindingExportsRuleAndSceneSlotsInRuleOrderSpec,
	"Wacom.UI.RunSceneBinding.Encounter.ExportsRuleAndSceneSlotsInRuleOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneBindingExportsRuleAndSceneSlotsInRuleOrderSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomEncounterSceneBindingSpec;
	FHarness Harness;
	if (!TestTrue(TEXT("Harness is valid"), Harness.IsValid()))
	{
		return false;
	}
	UEnemyDefinition* LeftEnemy = Harness.AddRuleSlot(TEXT("Left"));
	UEnemyDefinition* RightEnemy = Harness.AddRuleSlot(TEXT("Right"));
	AWacomBattleEnemyActor* LeftHost =
		Harness.SpawnHost(TEXT("Authored.Left"), LeftEnemy);
	AWacomBattleEnemyActor* RightHost =
		Harness.SpawnHost(TEXT("Authored.Right"), RightEnemy);
	Harness.AddSceneSlot(TEXT("Right"), RightHost);
	Harness.AddSceneSlot(TEXT("Left"), LeftHost);

	TestTrue(TEXT("Complete one-to-one binding validates"),
		Harness.Binding->ValidateForEncounter(*Harness.Encounter).IsOk());

	TArray<FBattleEnemySlotInit> RuleSlots;
	Harness.Binding->BuildBattleEnemySlots(*Harness.Encounter, RuleSlots);
	TestEqual(TEXT("Two rule slots exported"), RuleSlots.Num(), 2);
	if (RuleSlots.Num() == 2)
	{
		TestEqual(TEXT("Rule order keeps Left"), RuleSlots[0].EnemySlotId,
			FName(TEXT("Left")));
		TestEqual(TEXT("Rule order keeps Right"), RuleSlots[1].EnemySlotId,
			FName(TEXT("Right")));
	}

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Harness.Binding->BuildBattleSceneEnemyHosts(
		*Harness.Encounter, SceneHosts);
	TestEqual(TEXT("Two scene hosts exported"), SceneHosts.Num(), 2);
	if (SceneHosts.Num() == 2)
	{
		TestTrue(TEXT("Left host follows rule order"),
			SceneHosts[0] == LeftHost);
		TestTrue(TEXT("Right host follows rule order"),
			SceneHosts[1] == RightHost);
	}
	TestEqual(TEXT("Binding owns runtime Left identity"),
		LeftHost->GetEffectiveEnemySlotId(), FName(TEXT("Left")));
	TestEqual(TEXT("Binding owns runtime Right identity"),
		RightHost->GetEffectiveEnemySlotId(), FName(TEXT("Right")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneBindingRejectsIncompleteDuplicateAndExtraMappingsSpec,
	"Wacom.UI.RunSceneBinding.Encounter.RejectsIncompleteDuplicateAndExtraMappings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneBindingRejectsIncompleteDuplicateAndExtraMappingsSpec::
	RunTest(const FString& /*Parameters*/)
{
	using namespace WacomEncounterSceneBindingSpec;
	FHarness Harness;
	if (!TestTrue(TEXT("Harness is valid"), Harness.IsValid()))
	{
		return false;
	}
	UEnemyDefinition* Enemy = Harness.AddRuleSlot(TEXT("Enemy"));
	UEnemyDefinition* Support = Harness.AddRuleSlot(TEXT("Support"));
	AWacomBattleEnemyActor* EnemyHost =
		Harness.SpawnHost(TEXT("Enemy"), Enemy);
	AWacomBattleEnemyActor* SupportHost =
		Harness.SpawnHost(TEXT("Support"), Support);

	Harness.AddSceneSlot(TEXT("Enemy"), EnemyHost);
	TestFalse(TEXT("Missing rule slot is rejected"),
		Harness.Binding->ValidateForEncounter(*Harness.Encounter).IsOk());

	Harness.Binding->SceneEnemyHostSlots.Reset();
	Harness.AddSceneSlot(TEXT("Enemy"), EnemyHost);
	Harness.AddSceneSlot(TEXT("Enemy"), SupportHost);
	TestFalse(TEXT("Duplicate slot id is rejected"),
		Harness.Binding->ValidateForEncounter(*Harness.Encounter).IsOk());

	Harness.Binding->SceneEnemyHostSlots.Reset();
	Harness.AddSceneSlot(TEXT("Enemy"), EnemyHost);
	Harness.AddSceneSlot(TEXT("Support"), SupportHost);
	Harness.AddSceneSlot(TEXT("Extra"), SupportHost);
	TestFalse(TEXT("Extra slot is rejected"),
		Harness.Binding->ValidateForEncounter(*Harness.Encounter).IsOk());

	Harness.Binding->SceneEnemyHostSlots.Reset();
	Harness.AddSceneSlot(TEXT("Enemy"), EnemyHost);
	Harness.AddSceneSlot(TEXT("Support"), EnemyHost);
	TestFalse(TEXT("Shared Host is rejected"),
		Harness.Binding->ValidateForEncounter(*Harness.Encounter).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneBindingRequiresEncounterAnchorOwnerSpec,
	"Wacom.UI.RunSceneBinding.Encounter.RequiresEncounterAnchorOwner",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneBindingRequiresEncounterAnchorOwnerSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UEncounterDefinition> Encounter(
		NewObject<UEncounterDefinition>());
	TStrongObjectPtr<UWacomRunEncounterSceneBindingComponent> Binding(
		NewObject<UWacomRunEncounterSceneBindingComponent>());
	TestFalse(TEXT("Ownerless binding is rejected"),
		Binding->ValidateForEncounter(*Encounter).IsOk());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEncounterSceneBindingLegacyBlueprintPackagesRemovedSpec,
	"Wacom.UI.RunSceneBinding.Encounter.LegacyBattleTriggerBlueprintPackagesRemoved",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEncounterSceneBindingLegacyBlueprintPackagesRemovedSpec::RunTest(
	const FString& /*Parameters*/)
{
	TestFalse(TEXT("Legacy root BattleTrigger Blueprint package is removed"),
		FPackageName::DoesPackageExist(
			TEXT("/Game/Wacom/Maps/BP_BattleTriggerActor")));
	TestFalse(TEXT("Legacy SceneActor BattleTrigger Blueprint package is removed"),
		FPackageName::DoesPackageExist(
			TEXT("/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor")));
	return true;
}
