// Copyright Wacom. All Rights Reserved.

#include "BattleHUDTestHarness.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomBattleCameraLookComponent.h"
#include "Components/WacomFirstPersonCardAnchorComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "UI/Battle/BattleCombatLogFeedWidget.h"
#include "UI/Battle/BattlePresentationStackWidget.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/BattleWidgetSpecReceiver.h"

namespace
{
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
}

FWacomBattleHUDTestHarness::FWacomBattleHUDTestHarness(UWorld* InWorld)
	: World(InWorld)
{
	HUDPtr.Reset(NewObject<UWacomBattleHUDDetailTest>());
	if (HUDPtr)
	{
		HUDPtr->SetWorldForTest(InWorld);
	}
}

FWacomBattleHUDTestHarness::~FWacomBattleHUDTestHarness()
{
	DestroySpawnedActors();
}

TUniquePtr<FWacomBattleHUDTestHarness> FWacomBattleHUDTestHarness::CreateHUDOnly(UWorld* InWorld)
{
	return TUniquePtr<FWacomBattleHUDTestHarness>(new FWacomBattleHUDTestHarness(InWorld));
}

TUniquePtr<FWacomBattleHUDTestHarness> FWacomBattleHUDTestHarness::CreateHUDWithPlayer(UWorld* InWorld)
{
	TUniquePtr<FWacomBattleHUDTestHarness> Harness(new FWacomBattleHUDTestHarness(InWorld));
	if (!InWorld || !Harness->HUDPtr)
	{
		return Harness;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleHUDLocalPlayerControllerTest* SpawnedPC =
		InWorld->SpawnActor<AWacomBattleHUDLocalPlayerControllerTest>(
			AWacomBattleHUDLocalPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	Harness->PC = SpawnedPC;
	if (SpawnedPC)
	{
		Harness->HUDPtr.Reset(NewObject<UWacomBattleHUDDetailTest>(SpawnedPC));
		Harness->HUDPtr->SetOwningPlayerForTest(SpawnedPC);
		Harness->HUDPtr->SetWorldForTest(InWorld);
	}

	return Harness;
}

UBattleCombatLogFeedWidget* FWacomBattleHUDTestHarness::AttachCombatLogFeed()
{
	if (!HUDPtr)
	{
		return nullptr;
	}

	CombatLogFeedPtr.Reset(NewObject<UBattleCombatLogFeedWidget>(HUDPtr.Get()));
	if (CombatLogFeedPtr)
	{
		CombatLogFeedPtr->TakeWidget();
		HUDPtr->SetCombatLogFeedForTest(CombatLogFeedPtr.Get());
	}
	return CombatLogFeedPtr.Get();
}

UBattlePresentationStackWidget* FWacomBattleHUDTestHarness::AttachPresentationStack()
{
	if (!HUDPtr)
	{
		return nullptr;
	}

	PresentationStackPtr.Reset(NewObject<UBattlePresentationStackWidget>(HUDPtr.Get()));
	if (PresentationStackPtr)
	{
		PresentationStackPtr->TakeWidget();
		HUDPtr->SetPresentationStackForTest(PresentationStackPtr.Get());
	}
	return PresentationStackPtr.Get();
}

UWacomBattleCommandBarTestProbe* FWacomBattleHUDTestHarness::AttachCommandBar()
{
	if (!HUDPtr)
	{
		return nullptr;
	}

	CommandBarPtr.Reset(NewObject<UWacomBattleCommandBarTestProbe>(HUDPtr.Get()));
	if (CommandBarPtr)
	{
		CommandBarPtr->TakeWidget();
		HUDPtr->SetCommandBarForTest(CommandBarPtr.Get());
	}
	return CommandBarPtr.Get();
}

UPlayerStatusBar* FWacomBattleHUDTestHarness::AttachPlayerStatusBar()
{
	if (!HUDPtr)
	{
		return nullptr;
	}

	PlayerStatusBarPtr.Reset(NewObject<UPlayerStatusBar>(HUDPtr.Get()));
	if (PlayerStatusBarPtr)
	{
		PlayerStatusBarPtr->TakeWidget();
		HUDPtr->SetPlayerStatusBarForTest(PlayerStatusBarPtr.Get());
	}
	return PlayerStatusBarPtr.Get();
}

AWacomPlayerCharacter* FWacomBattleHUDTestHarness::AttachFirstPersonCharacter()
{
	UWorld* StrongWorld = World.Get();
	if (!StrongWorld)
	{
		return nullptr;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomPlayerCharacter* SpawnedCharacter =
		StrongWorld->SpawnActor<AWacomPlayerCharacter>(
			AWacomPlayerCharacter::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	FirstPersonCharacterActor = SpawnedCharacter;
	if (PC.IsValid() && SpawnedCharacter)
	{
		PC->Possess(SpawnedCharacter);
	}
	if (SpawnedCharacter)
	{
		FirstPersonAnchorPtr = SpawnedCharacter->GetFirstPersonCardAnchorComponent();
		BattleCameraLookPtr = SpawnedCharacter->GetBattleCameraLookComponent();
	}
	return SpawnedCharacter;
}

FWacomBattleHUDTestSceneEnemyHost& FWacomBattleHUDTestHarness::AttachSceneEnemyHost(
	UEnemyDefinition* EnemyDefinition,
	const TArray<FName>& PartIds)
{
	DestroySceneEnemyHost(CurrentSceneEnemyHost);

	UWorld* StrongWorld = World.Get();
	if (!StrongWorld)
	{
		return CurrentSceneEnemyHost;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	CurrentSceneEnemyHost.Host = StrongWorld->SpawnActor<AWacomBattleEnemyActor>(
		AWacomBattleEnemyActor::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!CurrentSceneEnemyHost.Host)
	{
		return CurrentSceneEnemyHost;
	}

	CurrentSceneEnemyHost.Host->EnemyDefinition = EnemyDefinition;
	for (int32 Index = 0; Index < PartIds.Num(); ++Index)
	{
		AWacomBattleEnemyPartActor* PartActor =
			StrongWorld->SpawnActor<AWacomBattleEnemyPartActor>(
				AWacomBattleEnemyPartActor::StaticClass(),
				FTransform(FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f)),
				SpawnParams);
		if (!PartActor)
		{
			continue;
		}

		CurrentSceneEnemyHost.Parts.Add(PartActor);
		PartActor->PartId = PartIds[Index];
		PartActor->PartSlotId =
			ResolvePartSlotIdForDefinitionPart(EnemyDefinition, PartIds[Index]);
		PartActor->AttachToActor(CurrentSceneEnemyHost.Host, FAttachmentTransformRules::KeepWorldTransform);
	}

	CurrentSceneEnemyHost.Host->RefreshBattleEnemyPartAuthoringState();
	return CurrentSceneEnemyHost;
}

void FWacomBattleHUDTestHarness::SetSession(
	UBattleSession* Session,
	bool bSettleInitialPresentation)
{
	if (!HUDPtr)
	{
		return;
	}

	HUDPtr->SetInjectedBattleSession(Session);
	if (bSettleInitialPresentation)
	{
		SettlePresentationQueue();
	}
}

void FWacomBattleHUDTestHarness::SetInitializedSession(
	const FWacomInitializedBattleSession& Initialized,
	bool bSettleInitialPresentation)
{
	if (!HUDPtr)
	{
		return;
	}

	HUDPtr->BeginBattleEntryPresentation();
	HUDPtr->AttachInitializedBattleSession(Initialized.Session, Initialized.Initialization);
	HUDPtr->ReleaseBattleEntryPresentation();
	if (bSettleInitialPresentation)
	{
		SettlePresentationQueue();
	}
}

void FWacomBattleHUDTestHarness::SettlePresentationQueue(int32 MaxSteps)
{
	if (!HUDPtr)
	{
		return;
	}

	for (int32 Iteration = 0; HUDPtr->IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
	{
		HUDPtr->AdvanceBattlePresentationQueueForTest();
	}
}

void FWacomBattleHUDTestHarness::SettlePresentationQueueAndExitStack(int32 MaxSteps)
{
	if (!HUDPtr)
	{
		return;
	}

	for (int32 Iteration = 0; HUDPtr->IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
	{
		bool bFinishedExit = false;
		const TArray<FWacomBattlePresentationStackEntryView> Entries =
			HUDPtr->GetPresentationStackEntriesForTest();
		for (const FWacomBattlePresentationStackEntryView& Entry : Entries)
		{
			if (Entry.bIsExiting)
			{
				HUDPtr->FinishPresentationStackEntryExitForTest(Entry.EntryId);
				bFinishedExit = true;
				break;
			}
		}

		if (!bFinishedExit)
		{
			HUDPtr->AdvanceBattlePresentationQueueForTest();
		}
	}
}

void FWacomBattleHUDTestHarness::DestroySceneEnemyHost(FWacomBattleHUDTestSceneEnemyHost& Actors)
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

void FWacomBattleHUDTestHarness::DestroySpawnedActors()
{
	DestroySceneEnemyHost(CurrentSceneEnemyHost);

	if (IsValid(FirstPersonCharacterActor.Get()))
	{
		FirstPersonCharacterActor->Destroy();
	}
	FirstPersonCharacterActor = nullptr;
	FirstPersonAnchorPtr = nullptr;
	BattleCameraLookPtr = nullptr;

	if (IsValid(PC.Get()))
	{
		PC->Destroy();
	}
	PC = nullptr;
}
