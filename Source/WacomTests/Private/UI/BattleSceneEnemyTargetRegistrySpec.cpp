// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyTargetRegistrySpec
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

	FGuid FindFirstHandCardByTargetMode(const FBattleSnapshot& Snapshot, ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	void SettleBattlePresentationQueue(UWacomBattleHUDDetailTest& HUD, int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}

	EDataValidationResult ValidateObjectForTest(
		const UObject* Object,
		TArray<FText>& OutWarnings,
		TArray<FText>& OutErrors)
	{
		OutWarnings.Reset();
		OutErrors.Reset();
		if (!Object)
		{
			OutErrors.Add(FText::FromString(TEXT("Missing object")));
			return EDataValidationResult::Invalid;
		}

		FDataValidationContext Context;
		const EDataValidationResult Result = Object->IsDataValid(Context);
		Context.SplitIssues(OutWarnings, OutErrors);
		return Result;
	}

	bool ValidationIssuesContain(const TArray<FText>& Issues, const TCHAR* ExpectedText)
	{
		for (const FText& Issue : Issues)
		{
			if (Issue.ToString().Contains(ExpectedText))
			{
				return true;
			}
		}
		return false;
	}

	FName ResolvePartSlotIdForDefinitionPart(
		const UEnemyDefinition* EnemyDefinition,
		FName PartId)
	{
		if (!EnemyDefinition || PartId.IsNone())
		{
			return NAME_None;
		}

		for (const FEnemyPartSlot& Slot : EnemyDefinition->Parts)
		{
			if (Slot.PartDef && Slot.PartDef->PartId == PartId)
			{
				return Slot.PartSlotId;
			}
		}
		return NAME_None;
	}

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	void AttachPartActorToHost(
		AWacomBattleEnemyActor* Host,
		FName PartId,
		AWacomBattleEnemyPartActor* PartActor)
	{
		if (!Host || !PartActor)
		{
			return;
		}

		PartActor->PartId = PartId;
		PartActor->PartSlotId = ResolvePartSlotIdForDefinitionPart(Host->EnemyDefinition, PartId);
		PartActor->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	}

	void AttachPartActorToHost(
		AWacomBattleEnemyActor* Host,
		FName PartId,
		FName PartSlotId,
		AWacomBattleEnemyPartActor* PartActor)
	{
		if (!Host || !PartActor)
		{
			return;
		}

		PartActor->PartId = PartId;
		PartActor->PartSlotId = PartSlotId;
		PartActor->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
	}

	FSceneEnemyHostActors SpawnSceneEnemyHost(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		const TArray<FName>& PartIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyHostActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = EnemyDefinition;
		for (int32 Index = 0; Index < PartIds.Num(); ++Index)
		{
			AWacomBattleEnemyPartActor* PartActor =
				World.SpawnActor<AWacomBattleEnemyPartActor>(
					AWacomBattleEnemyPartActor::StaticClass(),
					FTransform(FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			Result.Parts.Add(PartActor);
			AttachPartActorToHost(Result.Host, PartIds[Index], PartActor);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	FSceneEnemyHostActors SpawnSceneEnemyHostForSlot(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		FName EnemySlotId,
		const TArray<FName>& PartDefinitionIds,
		const TArray<FName>& PartSlotIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyHostActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = EnemyDefinition;
		Result.Host->EnemySlotId = EnemySlotId;
		const int32 PartCount = FMath::Min(PartDefinitionIds.Num(), PartSlotIds.Num());
		for (int32 Index = 0; Index < PartCount; ++Index)
		{
			AWacomBattleEnemyPartActor* PartActor =
				World.SpawnActor<AWacomBattleEnemyPartActor>(
					AWacomBattleEnemyPartActor::StaticClass(),
					FTransform(FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			Result.Parts.Add(PartActor);
			AttachPartActorToHost(Result.Host, PartDefinitionIds[Index], PartSlotIds[Index], PartActor);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	UEncounterDefinition* MakeEncounterDefinitionForTest(
		UObject* Outer,
		const TArray<TPair<FName, UEnemyDefinition*>>& EnemySlots)
	{
		UEncounterDefinition* Encounter =
			NewObject<UEncounterDefinition>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
		Encounter->EncounterDefinitionId = TEXT("Test.Encounter.SceneEnemyHost");
		for (const TPair<FName, UEnemyDefinition*>& EnemySlot : EnemySlots)
		{
			FEncounterEnemySlot Slot;
			Slot.EnemySlotId = EnemySlot.Key;
			Slot.EnemyDefinition = EnemySlot.Value;
			Encounter->EnemySlots.Add(Slot);
		}
		return Encounter;
	}

	void ConfigureTriggerSceneEnemyHostSlotForTest(
		ABattleTriggerActor* Trigger,
		FName EnemySlotId,
		UEnemyDefinition* EnemyDefinition,
		AWacomBattleEnemyActor* Host)
	{
		if (!Trigger)
		{
			return;
		}

		Trigger->EncounterDefinition = MakeEncounterDefinitionForTest(
			Trigger,
			{ TPair<FName, UEnemyDefinition*>(EnemySlotId, EnemyDefinition) });

		FWacomBattleSceneEnemyHostSlot Slot;
		Slot.EnemySlotId = EnemySlotId;
		Slot.SceneEnemyHost = Host;
		Trigger->SceneEnemyHostSlots = { Slot };
	}

	void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
	{
		for (AWacomBattleEnemyPartActor* PartActor : Actors.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		Actors.Parts.Reset();

		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostRegistrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.BattleTriggerSceneEnemyHostDrivesHUDTargetRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostRegistrySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetRegistrySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	WacomBattleSceneEnemyTargetRegistrySpec::ConfigureTriggerSceneEnemyHostSlotForTest(
		Trigger.Get(),
		TEXT("Enemy"),
		Enemy,
		SceneEnemy.Host);
	const FWacomBattleTriggerDebugView TriggerView = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Trigger debug reports host part count"), TriggerView.SceneEnemyHostPartCount, 2);
	TestTrue(TEXT("Trigger debug reports matching definition"), TriggerView.bSceneEnemyHostDefinitionMatches);

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneHosts);
	HUD->SetBattleSceneEnemyHostsForTest(SceneHosts);
	TestEqual(TEXT("HUD registry uses trigger host parts"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestEqual(TEXT("Host debug reports active HUD usage"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	TestTrue(TEXT("Host HUD summary reports usage"),
		SceneEnemy.Host->GetBattleSceneEnemyDebugSummaryForHUD(HUD.Get()).Contains(TEXT("UsedByBattleHUD=true")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostSlotsRegistrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.BattleTriggerSceneEnemyHostSlotsDriveHUDTargetRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostSlotsRegistrySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetRegistrySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = nullptr;
	UBattleSession* Session = nullptr;
	UEnemyDefinition* LeftEnemy = nullptr;
	UEnemyDefinition* RightEnemy = nullptr;
	UCharacterDefinition* Character = nullptr;
	{
		DamageCard = Fx.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/3);
		Character = Fx.MakeCharacter(
			Fx.MakeNoopCard(0),
			DamageCard,
			{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
		LeftEnemy = Fx.MakeSinglePartEnemy(/*Hp*/20, /*Initiative*/5, /*IntentResist*/0);
		RightEnemy = Fx.MakeSinglePartEnemy(/*Hp*/20, /*Initiative*/5, /*IntentResist*/0);

		FBattleInitParams Params;
		Params.Character = Character;
		Params.EncounterId = TEXT("Encounter.SceneHostSlots");
		Params.RandomSeed = 1;
		FBattleEnemySlotInit LeftSlot;
		LeftSlot.EnemySlotId = TEXT("LeftEnemy");
		LeftSlot.Enemy = LeftEnemy;
		Params.EnemySlots.Add(LeftSlot);
		FBattleEnemySlotInit RightSlot;
		RightSlot.EnemySlotId = TEXT("RightEnemy");
		RightSlot.Enemy = RightEnemy;
		Params.EnemySlots.Add(RightSlot);

		Session = NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient);
		const FWacomStatus Status = Session->Initialize(Params);
		if (!TestTrue(TEXT("Session initializes"), Status.IsOk()))
		{
			return false;
		}
	}

	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors LeftHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHostForSlot(
			*World,
			LeftEnemy,
			TEXT("LeftEnemy"),
			{ TEXT("Test.Part.Solo") },
			{ TEXT("Test.Part.Solo") });
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors RightHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHostForSlot(
			*World,
			RightEnemy,
			TEXT("RightEnemy"),
			{ TEXT("Test.Part.Solo") },
			{ TEXT("Test.Part.Solo") });
	if (!TestNotNull(TEXT("Left host"), LeftHost.Host)
		|| !TestNotNull(TEXT("Right host"), RightHost.Host)
		|| !TestEqual(TEXT("Left host part count"), LeftHost.Parts.Num(), 1)
		|| !TestEqual(TEXT("Right host part count"), RightHost.Parts.Num(), 1))
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(RightHost);
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(LeftHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(RightHost);
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(LeftHost);
	};

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.SceneHostSlots");
	Trigger->EncounterDefinition = WacomBattleSceneEnemyTargetRegistrySpec::MakeEncounterDefinitionForTest(
		Trigger.Get(),
		{
			TPair<FName, UEnemyDefinition*>(TEXT("LeftEnemy"), LeftEnemy),
			TPair<FName, UEnemyDefinition*>(TEXT("RightEnemy"), RightEnemy),
		});
	FWacomBattleSceneEnemyHostSlot LeftSceneSlot;
	LeftSceneSlot.EnemySlotId = TEXT("LeftEnemy");
	LeftSceneSlot.SceneEnemyHost = LeftHost.Host;
	FWacomBattleSceneEnemyHostSlot RightSceneSlot;
	RightSceneSlot.EnemySlotId = TEXT("RightEnemy");
	RightSceneSlot.SceneEnemyHost = RightHost.Host;
	Trigger->SceneEnemyHostSlots = { LeftSceneSlot, RightSceneSlot };

	TArray<AWacomBattleEnemyActor*> SceneHosts;
	Trigger->BuildBattleSceneEnemyHosts(SceneHosts);
	TestEqual(TEXT("Trigger exports two scene hosts"), SceneHosts.Num(), 2);
	TestEqual(TEXT("Left host slot id synced"), LeftHost.Host->GetEffectiveEnemySlotId(), FName(TEXT("LeftEnemy")));
	TestEqual(TEXT("Right host slot id synced"), RightHost.Host->GetEffectiveEnemySlotId(), FName(TEXT("RightEnemy")));

	const FWacomBattleTriggerDebugView TriggerView = Trigger->GetBattleTriggerDebugView(nullptr);
	TestEqual(TEXT("Trigger debug reports host slot count"), TriggerView.SceneEnemyHostSlotCount, 2);
	TestEqual(TEXT("Trigger debug reports host count"), TriggerView.SceneEnemyHostCount, 2);
	TestEqual(TEXT("Trigger debug reports total part count"), TriggerView.SceneEnemyHostPartCount, 2);

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest(SceneHosts);
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("HUD registry uses both scene host slots"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestTrue(TEXT("Left host is in current registry"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(LeftHost.Host));
	TestTrue(TEXT("Right host is in current registry"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(RightHost.Host));
	TestTrue(TEXT("Left part binds to left enemy slot"),
		LeftHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	TestTrue(TEXT("Right part binds to right enemy slot"),
		RightHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().bBoundToSnapshot);
	TestEqual(TEXT("Left bridge enemy slot"),
		LeftHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().EnemySlotId,
		FName(TEXT("LeftEnemy")));
	TestEqual(TEXT("Right bridge enemy slot"),
		RightHost.Parts[0]->GetWorldTargetBridgeComponent()->GetBattleWorldTargetDebugView().EnemySlotId,
		FName(TEXT("RightEnemy")));
	TestEqual(TEXT("Left host debug reports active HUD usage"),
		LeftHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	TestEqual(TEXT("Right host debug reports active HUD usage"),
		RightHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		true);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSyncsOnlyCurrentHostSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.HUDSyncsOnlyCurrentHostChildPartActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSyncsOnlyCurrentHostSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetRegistrySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body") });
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors OtherHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(CurrentHost);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("Only current host bridges are registered"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(),
		2);
	TestTrue(TEXT("Current head binds"),
		CurrentHost.Parts[0]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestTrue(TEXT("Current body binds"),
		CurrentHost.Parts[1]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestFalse(TEXT("Unrelated host does not bind"),
		OtherHost.Parts[0]->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	TestEqual(TEXT("Other host debug stays unused"),
		OtherHost.Host->GetBattleSceneEnemyDebugViewForHUD(HUD.Get()).bUsedByBattleHUD,
		false);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDIgnoresUnrelatedSceneEnemyPartsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.HUDDoesNotBindOrPreviewUnrelatedSceneEnemyParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDIgnoresUnrelatedSceneEnemyPartsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetRegistrySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeSimpleDamageCard(1, 1) });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleSceneEnemyTargetRegistrySpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors OtherHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleSceneEnemyTargetRegistrySpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	OtherHost.Parts[0]->GetInteractionTargetComponent()->SetTargetId(FWacomBattleFixture::FindPartInstanceId(Snapshot, 1));
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, OtherHost.Parts[0], OtherHost.Parts[0]->GetHitBounds());

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);

	TestFalse(TEXT("Unrelated part does not preview"),
		OtherHost.Parts[0]->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bDragPreviewActive);
	TestFalse(TEXT("Unrelated part does not hover"),
		OtherHost.Parts[0]->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bHoverActive);
	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Unrelated world target rejected"),
		DropResult.RejectReason,
		EWacomBattleCardDropRejectReason::InvalidWorldTarget);

	HUD->SetTargetSelectionStateForTest(CardId);
	const int32 VersionBeforeClick = Session->BuildSnapshot().Version;
	TestFalse(TEXT("Unrelated part click is not routed"),
		FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	TestEqual(TEXT("Unrelated part click does not submit"),
		Session->BuildSnapshot().Version,
		VersionBeforeClick);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerEncounterMissingSceneEnemyHostInvalidSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.EncounterMissingSceneEnemyHostInvalidatesTrigger",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerEncounterMissingSceneEnemyHostInvalidSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeSinglePartEnemy(20, 5, 0);

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.EncounterMissingHost");
	Trigger->EncounterDefinition = WacomBattleSceneEnemyTargetRegistrySpec::MakeEncounterDefinitionForTest(
		Trigger.Get(),
		{ TPair<FName, UEnemyDefinition*>(TEXT("Enemy"), Enemy) });
	Trigger->SceneEnemyHostSlots.Reset();

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyTargetRegistrySpec::ValidateObjectForTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Encounter missing scene host invalidates trigger"),
		Result,
		EDataValidationResult::Invalid);
	TestTrue(TEXT("Missing encounter host error mentions SceneEnemyHost"),
		WacomBattleSceneEnemyTargetRegistrySpec::ValidationIssuesContain(Errors, TEXT("SceneEnemyHost")));
	TestFalse(TEXT("Debug reports host missing"),
		Trigger->GetBattleTriggerDebugView(nullptr).bSceneEnemyHostConfigured);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerSceneEnemyHostDefinitionMismatchWarningSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.SceneEnemyHostDefinitionMismatchReportsWarning",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerSceneEnemyHostDefinitionMismatchWarningSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetRegistrySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* TriggerEnemy = Fx.MakeSinglePartEnemy(20, 5, 0);
	UEnemyDefinition* HostEnemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors SceneEnemy =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			HostEnemy,
			{ TEXT("Test.Part.Head") });
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(SceneEnemy);
	};

	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());
	Trigger->PersistentId = TEXT("Test.Battle.MismatchedHost");
	WacomBattleSceneEnemyTargetRegistrySpec::ConfigureTriggerSceneEnemyHostSlotForTest(
		Trigger.Get(),
		TEXT("Enemy"),
		TriggerEnemy,
		SceneEnemy.Host);

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult Result =
		WacomBattleSceneEnemyTargetRegistrySpec::ValidateObjectForTest(Trigger.Get(), Warnings, Errors);
	TestEqual(TEXT("Definition mismatch keeps trigger valid"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Definition mismatch has no errors"), Errors.Num(), 0);
	TestTrue(TEXT("Mismatch warning mentions SceneEnemyHost"),
		WacomBattleSceneEnemyTargetRegistrySpec::ValidationIssuesContain(Warnings, TEXT("SceneEnemyHost")));
	TestFalse(TEXT("Debug reports definition mismatch"),
		Trigger->GetBattleTriggerDebugView(nullptr).bSceneEnemyHostDefinitionMatches);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCurrentHostRegistryRoutesFeedbackSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetRegistry.TargetCueHoverPredictionAndDragPreviewUseCurrentHostRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCurrentHostRegistryRoutesFeedbackSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetRegistrySpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UCardDefinition* TargetCard = Fx.MakeSimpleDamageCard(2, 1);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ TargetCard });
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid CardId = WacomBattleSceneEnemyTargetRegistrySpec::FindFirstHandCardByTargetMode(
		Snapshot,
		ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors CurrentHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Head") });
	WacomBattleSceneEnemyTargetRegistrySpec::FSceneEnemyHostActors OtherHost =
		WacomBattleSceneEnemyTargetRegistrySpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Test.Part.Body") });
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Current host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other host"), OtherHost.Host)
		|| !TestTrue(TEXT("Target card exists"), CardId.IsValid()))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(OtherHost);
		WacomBattleSceneEnemyTargetRegistrySpec::DestroySceneEnemyHost(CurrentHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	AWacomBattleEnemyPartActor* CurrentPart = CurrentHost.Parts[0];
	AWacomBattleEnemyPartActor* OtherPart = OtherHost.Parts[0];
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Snapshot);
	WacomBattleSceneEnemyTargetRegistrySpec::SettleBattlePresentationQueue(*HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());

	HUD->PlayTargetConfirmedCueForTest(Snapshot.Enemies[0].Parts[0].Identity);
	TestEqual(TEXT("Current host cue routed"),
		CurrentPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().CuePlayCount,
		1);
	TestEqual(TEXT("Other host cue ignored"),
		OtherPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().CuePlayCount,
		0);

	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, CurrentPart, CurrentPart->GetHitBounds());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest();
	TestTrue(TEXT("Current host hover activates"),
		CurrentPart->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView().bHoverActive);
	TestFalse(TEXT("Current host hover prediction handled by enemy panel"),
		CurrentPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.PredictionView.bVisible);

	FWacomFirstPersonCardDragView DragView;
	DragView.CardInstanceId = CardId;
	DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
	DragView.bHasPointerViewportPosition = true;
	DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
	HUD->HandleFirstPersonCardDragStartedForTest(CardId, DragView);
	HUD->HandleFirstPersonCardDragUpdatedForTest(CardId, DragView);
	TestTrue(TEXT("Current host drag preview activates"),
		CurrentPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bDragPreviewActive);
	TestFalse(TEXT("Other host drag preview stays inactive"),
		OtherPart->GetPresentationComponent()
			->GetBattleEnemyPartPresentationDebugView()
			.bDragPreviewActive);

	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Current host drop target resolves"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	return true;
}
