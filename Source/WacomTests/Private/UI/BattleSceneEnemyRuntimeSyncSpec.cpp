// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyRuntimeSyncSpec
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

	UPaperFlipbook* MakeOneFrameFlipbookForTest(UObject* Outer)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 12.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = 1;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	struct FSceneEnemyActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	AWacomBattleEnemyPartActor* SpawnPart(
		UWorld& World,
		AWacomBattleEnemyActor& Host,
		const FEnemyPartSlot& Slot,
		bool bAddVisualLayer)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyPartActor* Part = World.SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Part)
		{
			return nullptr;
		}

		Part->PartId = Slot.PartDef ? Slot.PartDef->PartId : NAME_None;
		Part->PartSlotId = Slot.PartSlotId;
		if (bAddVisualLayer)
		{
			FWacomBattleEnemyPartVisualLayer Layer;
			Layer.LayerId = TEXT("RuntimeSyncLayer");
			Layer.Sprite = NewObject<UPaperSprite>(Part);
			Part->VisualLayers = { Layer };
		}
		Part->AttachToActor(&Host, FAttachmentTransformRules::KeepWorldTransform);
		Part->RefreshAuthoringState();
		return Part;
	}

	FSceneEnemyActors SpawnSceneEnemy(
		UWorld& World,
		UEnemyDefinition& EnemyDefinition,
		int32 PartCount,
		bool bConfigureVisuals)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		FSceneEnemyActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = &EnemyDefinition;
		Result.Host->EnemySlotId = TEXT("Enemy");
		if (bConfigureVisuals)
		{
			Result.Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
			Result.Host->HostFlipbook = MakeOneFrameFlipbookForTest(Result.Host);
			Result.Host->bHostVisualVisible = true;
		}

		const int32 SafePartCount = FMath::Min(PartCount, EnemyDefinition.Parts.Num());
		for (int32 Index = 0; Index < SafePartCount; ++Index)
		{
			if (AWacomBattleEnemyPartActor* Part = SpawnPart(
				World,
				*Result.Host,
				EnemyDefinition.Parts[Index],
				bConfigureVisuals && Index == 0))
			{
				Result.Parts.Add(Part);
			}
		}
		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	void DestroySceneEnemy(FSceneEnemyActors& Actors)
	{
		for (AWacomBattleEnemyPartActor* Part : Actors.Parts)
		{
			if (IsValid(Part))
			{
				Part->Destroy();
			}
		}
		Actors.Parts.Reset();
		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}

	UPaperSpriteComponent* FindGeneratedPartSpriteComponent(
		const AWacomBattleEnemyPartActor& Part)
	{
		TInlineComponentArray<UPaperSpriteComponent*> Components;
		Part.GetComponents(Components);
		for (UPaperSpriteComponent* Component : Components)
		{
			if (Component
				&& Component->GetAttachParent() == Part.GetVisualLayersRoot()
				&& Component->GetName().StartsWith(TEXT("VisualLayer_")))
			{
				return Component;
			}
		}
		return nullptr;
	}

	UBattleSession* CreateThreePartSession(FWacomBattleFixture& Fixture, UEnemyDefinition*& OutEnemy)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{ Fixture.MakeSimpleDamageCard(0, 1) });
		OutEnemy = Fixture.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
		return Fixture.CreateSession(Character, OutEnemy, 1);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyRuntimeSnapshotKeepsTopologyAndVisualsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyRuntimeSync.SnapshotKeepsRegistryAndVisualComponentsStable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyRuntimeSnapshotKeepsTopologyAndVisualsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyRuntimeSyncSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = nullptr;
	UBattleSession* Session = CreateThreePartSession(Fixture, Enemy);
	FSceneEnemyActors SceneEnemy = SpawnSceneEnemy(*World, *Enemy, 3, true);
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Scene part count"), SceneEnemy.Parts.Num(), 3))
	{
		DestroySceneEnemy(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT { DestroySceneEnemy(SceneEnemy); };

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });

	UPaperFlipbookComponent* HostVisualBefore =
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent();
	UPaperSpriteComponent* PartVisualBefore =
		FindGeneratedPartSpriteComponent(*SceneEnemy.Parts[0]);
	if (!TestNotNull(TEXT("Generated host flipbook"), HostVisualBefore)
		|| !TestNotNull(TEXT("Generated part visual layer"), PartVisualBefore))
	{
		return false;
	}

	HostVisualBefore->Stop();
	HostVisualBefore->SetPlaybackPosition(0.04f, false);
	const float PlaybackPositionBefore = HostVisualBefore->GetPlaybackPosition();
	const int32 RegistryRevisionBefore =
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest();
	const int32 PresentationTargetCountBefore =
		HUD->GetBattlePresentationTargetCountForTest();

	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });

	TestEqual(TEXT("Ordinary snapshot keeps registry revision"),
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest(),
		RegistryRevisionBefore);
	TestEqual(TEXT("Ordinary snapshot keeps presentation registrations"),
		HUD->GetBattlePresentationTargetCountForTest(),
		PresentationTargetCountBefore);
	TestTrue(TEXT("Host flipbook component is not rebuilt"),
		SceneEnemy.Host->GetGeneratedHostFlipbookVisualComponent() == HostVisualBefore);
	TestTrue(TEXT("Part visual layer component is not rebuilt"),
		FindGeneratedPartSpriteComponent(*SceneEnemy.Parts[0]) == PartVisualBefore);
	TestTrue(TEXT("Host flipbook playback position is preserved"),
		FMath::IsNearlyEqual(HostVisualBefore->GetPlaybackPosition(), PlaybackPositionBefore));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyRuntimeTopologyRevisionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyRuntimeSync.TopologyChangesRebuildRegistryOnce",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyRuntimeTopologyRevisionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyRuntimeSyncSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = nullptr;
	UBattleSession* Session = CreateThreePartSession(Fixture, Enemy);
	FSceneEnemyActors SceneEnemy = SpawnSceneEnemy(*World, *Enemy, 2, false);
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestEqual(TEXT("Initial scene part count"), SceneEnemy.Parts.Num(), 2))
	{
		DestroySceneEnemy(SceneEnemy);
		return false;
	}
	ON_SCOPE_EXIT { DestroySceneEnemy(SceneEnemy); };

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	const int32 InitialRevision = HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest();
	TestEqual(TEXT("Initial registry contains two parts"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 2);

	AWacomBattleEnemyPartActor* AddedPart = SpawnPart(
		*World,
		*SceneEnemy.Host,
		Enemy->Parts[2],
		false);
	if (!TestNotNull(TEXT("Dynamically added part"), AddedPart))
	{
		return false;
	}
	SceneEnemy.Parts.Add(AddedPart);
	SceneEnemy.Host->InvalidateRuntimePartTopology();
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("Part addition rebuilds registry once"),
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest(),
		InitialRevision + 1);
	TestEqual(TEXT("Added part enters registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 3);
	TestTrue(TEXT("Added part binds to snapshot"),
		AddedPart->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());

	const int32 RevisionBeforeDestroy = HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest();
	AddedPart->Destroy();
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	TestEqual(TEXT("Part destruction rebuilds registry once"),
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest(),
		RevisionBeforeDestroy + 1);
	TestEqual(TEXT("Destroyed part leaves registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 2);
	TestEqual(TEXT("Destroyed part leaves presentation registry"),
		HUD->GetBattlePresentationTargetCountForTest(), 2);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyRuntimeClearAndReentrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyRuntimeSync.BattleEndClearsAndReentryRebinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyRuntimeClearAndReentrySpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyRuntimeSyncSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = nullptr;
	UBattleSession* Session = CreateThreePartSession(Fixture, Enemy);
	FSceneEnemyActors SceneEnemy = SpawnSceneEnemy(*World, *Enemy, 3, false);
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT { DestroySceneEnemy(SceneEnemy); };

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	const int32 RevisionBeforeBattleEnd =
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest();
	TestEqual(TEXT("Initial presentation target count"),
		HUD->GetBattlePresentationTargetCountForTest(), 3);

	FBattleSnapshot BattleEndSnapshot = Session->BuildSnapshot();
	BattleEndSnapshot.Phase = EBattlePhase::BattleEnd;
	HUD->RefreshFromSnapshotForTest(BattleEndSnapshot);
	TestEqual(TEXT("BattleEnd clears scene target registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 0);
	TestEqual(TEXT("BattleEnd clears presentation registry"),
		HUD->GetBattlePresentationTargetCountForTest(), 0);
	for (AWacomBattleEnemyPartActor* Part : SceneEnemy.Parts)
	{
		TestFalse(TEXT("BattleEnd clears bridge binding"),
			Part->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
	}

	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	TestEqual(TEXT("Reentry rebuilds registry once"),
		HUD->GetBattleSceneEnemyTargetRegistryRevisionForTest(),
		RevisionBeforeBattleEnd + 1);
	TestEqual(TEXT("Reentry restores scene targets"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 3);
	TestEqual(TEXT("Reentry restores presentation targets"),
		HUD->GetBattlePresentationTargetCountForTest(), 3);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyRuntimeDestroyedHostClearsRegistrySpec,
	"Wacom.UI.Battle.BattleSceneEnemyRuntimeSync.DestroyedHostClearsRegistry",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyRuntimeDestroyedHostClearsRegistrySpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyRuntimeSyncSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = nullptr;
	UBattleSession* Session = CreateThreePartSession(Fixture, Enemy);
	FSceneEnemyActors SceneEnemy = SpawnSceneEnemy(*World, *Enemy, 3, false);
	if (!TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host))
	{
		return false;
	}
	ON_SCOPE_EXIT { DestroySceneEnemy(SceneEnemy); };

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	TestEqual(TEXT("Initial scene target count"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 3);

	SceneEnemy.Host->Destroy();
	SceneEnemy.Host = nullptr;
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	TestEqual(TEXT("Destroyed host clears scene target registry"),
		HUD->GetBattleSceneEnemyPartWorldTargetBridgeCountForTest(), 0);
	TestEqual(TEXT("Destroyed host clears presentation registry"),
		HUD->GetBattlePresentationTargetCountForTest(), 0);
	for (AWacomBattleEnemyPartActor* Part : SceneEnemy.Parts)
	{
		if (IsValid(Part))
		{
			TestFalse(TEXT("Destroyed host clears surviving bridge binding"),
				Part->GetWorldTargetBridgeComponent()->IsBoundToBattlePart());
		}
	}
	return true;
}
