// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Runtime/BattleEnemyKeys.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/BattleSceneTargetClickTestAccess.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/BattleHUDTestHarness.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyTargetSpec
{
	UEnemyPartDefinition* MakeSharedPartDefinition(
		FWacomBattleFixture& Fixture,
		FName PartId,
		int32 Hp)
	{
		UEnemyDefinition* TemplateEnemy = Fixture.MakeSinglePartEnemy(Hp, /*Initiative*/50, /*IntentResist*/0);
		UEnemyPartDefinition* Part = TemplateEnemy && TemplateEnemy->Parts.Num() > 0
			? TemplateEnemy->Parts[0].PartDef
			: nullptr;
		if (Part)
		{
			Part->PartId = PartId;
		}
		return Part;
	}

	UEnemyDefinition* MakeSingleSlotEnemy(
		FWacomBattleFixture& Fixture,
		FName EnemyId,
		FName PartDefinitionId,
		FName PartSlotId,
		int32 Hp)
	{
		UEnemyPartDefinition* Part = MakeSharedPartDefinition(Fixture, PartDefinitionId, Hp);
		if (!Part)
		{
			return nullptr;
		}

		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(GetTransientPackage(), NAME_None, RF_Transient);
		Enemy->EnemyId = EnemyId;
		FEnemyPartSlot Slot;
		Slot.PartDef = Part;
		Slot.PartSlotId = PartSlotId;
		Enemy->Parts.Add(Slot);
		return Enemy;
	}

	const FEnemyPartSnapshot* FindPartBySlot(
		const FBattleSnapshot& Snapshot,
		FName EnemySlotId,
		FName PartSlotId)
	{
		for (const FEnemySnapshot& Enemy : Snapshot.Enemies)
		{
			if (Enemy.EnemySlotId != EnemySlotId)
			{
				continue;
			}

			for (const FEnemyPartSnapshot& Part : Enemy.Parts)
			{
				if (Part.PartSlotId == PartSlotId)
				{
					return &Part;
				}
			}
		}
		return nullptr;
	}

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

	struct FSceneHostForKeyRouting
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	FSceneHostForKeyRouting SpawnSceneHostForSlot(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		FName EnemySlotId,
		const TArray<FName>& PartDefinitionIds,
		const TArray<FName>& PartSlotIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneHostForKeyRouting Result;
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
					FTransform(FVector(100.0f * static_cast<float>(Index + 1), 0.0f, 0.0f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			PartActor->PartId = PartDefinitionIds[Index];
			PartActor->PartSlotId = PartSlotIds[Index];
			PartActor->AttachToActor(Result.Host, FAttachmentTransformRules::KeepWorldTransform);
			Result.Parts.Add(PartActor);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	void DestroySceneHost(FSceneHostForKeyRouting& SceneHost)
	{
		for (AWacomBattleEnemyPartActor* PartActor : SceneHost.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		SceneHost.Parts.Reset();

		if (IsValid(SceneHost.Host))
		{
			SceneHost.Host->Destroy();
		}
		SceneHost.Host = nullptr;
	}

	void ClearRuntimeWorldTargetId(AWacomBattleEnemyPartActor* PartActor)
	{
		if (PartActor && PartActor->GetInteractionTargetComponent())
		{
			PartActor->GetInteractionTargetComponent()->SetTargetId(FGuid());
		}
	}

	FWacomFirstPersonCardDragView MakeAimingDragView(const FGuid& CardId)
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
		DragView.bHasPointerViewportPosition = true;
		DragView.PointerViewportPosition = FVector2D(540.0f, 590.0f);
		return DragView;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetClickRoutesByEnemyPartKeySpec,
	"Wacom.UI.Battle.SceneEnemyTarget.ClickRoutesByEnemyPartKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetClickRoutesByEnemyPartKeySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/7);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(/*Cost*/0),
		DamageCard,
		{ Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0) });
	UEnemyDefinition* LeftEnemy = WacomBattleSceneEnemyTargetSpec::MakeSingleSlotEnemy(
		Fixture,
		TEXT("Test.Enemy.Left"),
		TEXT("Test.Part.SharedCore"),
		TEXT("Core"),
		/*Hp*/30);
	UEnemyDefinition* RightEnemy = WacomBattleSceneEnemyTargetSpec::MakeSingleSlotEnemy(
		Fixture,
		TEXT("Test.Enemy.Right"),
		TEXT("Test.Part.SharedCore"),
		TEXT("Core"),
		/*Hp*/30);

	FBattleInitParams Params;
	Params.Character = Character;
	Params.EncounterId = TEXT("Encounter.UI.SceneTarget");
	Params.RandomSeed = 1;
	FBattleEnemySlotInit LeftSlot;
	LeftSlot.EnemySlotId = TEXT("LeftEnemy");
	LeftSlot.Enemy = LeftEnemy;
	Params.EnemySlots.Add(LeftSlot);
	FBattleEnemySlotInit RightSlot;
	RightSlot.EnemySlotId = TEXT("RightEnemy");
	RightSlot.Enemy = RightEnemy;
	Params.EnemySlots.Add(RightSlot);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient));
	const FWacomStatus InitStatus = Session->Initialize(Params);
	if (!TestTrue(TEXT("Session initializes"), InitStatus.IsOk()))
	{
		return false;
	}

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(nullptr);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}

	UBattleHUD* HUD = Harness->HUD();
	Harness->SetSession(Session.Get());
	const FBattleSnapshot Before = Session->BuildSnapshot();
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Before, DamageCard->CardId);
	const FEnemyPartSnapshot* LeftPart =
		WacomBattleSceneEnemyTargetSpec::FindPartBySlot(Before, TEXT("LeftEnemy"), TEXT("Core"));
	const FEnemyPartSnapshot* RightPart =
		WacomBattleSceneEnemyTargetSpec::FindPartBySlot(Before, TEXT("RightEnemy"), TEXT("Core"));
	if (!TestTrue(TEXT("Damage card is in hand"), CardId.IsValid())
		|| !TestNotNull(TEXT("Left part exists"), LeftPart)
		|| !TestNotNull(TEXT("Right part exists"), RightPart))
	{
		return false;
	}
	TestEqual(TEXT("Both parts share the same authored part definition id"),
		LeftPart->Definition ? LeftPart->Definition->PartId : NAME_None,
		RightPart->Definition ? RightPart->Definition->PartId : NAME_None);

	HUD->OnCardClickedByUser(CardId);
	TestEqual(TEXT("Card click enters target select"), HUD->GetUIState(), EBattleUIState::TargetSelect);

	const FWacomInteractionTargetHandle RightHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		FGuid(),
		nullptr,
		FVector::ZeroVector,
		FVector2D::ZeroVector,
		WacomTags::Interaction_Target_Battle_EnemyPart,
		TEXT("Test.Part.SharedCore"),
		RightPart->EncounterId,
		RightPart->EnemySlotId,
		RightPart->PartSlotId);
	TestFalse(TEXT("Test handle intentionally does not rely on runtime part instance id"),
		RightHandle.WorldTargetId.IsValid());
	TestTrue(TEXT("Test handle carries stable battle part slot identity"),
		RightHandle.HasBattlePartSlotIdentity());

	HUD->OnEnemyPartClickedByUser(RightHandle);

	const FBattleSnapshot After = Session->BuildSnapshot();
	const FEnemyPartSnapshot* LeftAfter =
		WacomBattleSceneEnemyTargetSpec::FindPartBySlot(After, TEXT("LeftEnemy"), TEXT("Core"));
	const FEnemyPartSnapshot* RightAfter =
		WacomBattleSceneEnemyTargetSpec::FindPartBySlot(After, TEXT("RightEnemy"), TEXT("Core"));
	if (!TestNotNull(TEXT("Left part still exists"), LeftAfter)
		|| !TestNotNull(TEXT("Right part still exists"), RightAfter))
	{
		return false;
	}

	TestEqual(TEXT("Left duplicate part is not damaged by right-slot handle"),
		LeftAfter->CurrentHp,
		LeftPart->CurrentHp);
	TestEqual(TEXT("Right duplicate part is damaged by key-routed handle"),
		RightAfter->CurrentHp,
		RightPart->CurrentHp - 7);
	TestEqual(TEXT("Target submission returns HUD to idle"), HUD->GetUIState(), EBattleUIState::Idle);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetClickProbeBuildsKeyOnlyHandleSpec,
	"Wacom.UI.Battle.SceneEnemyTarget.ClickProbeBuildsKeyOnlyEnemyPartHandle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetClickProbeBuildsKeyOnlyHandleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/5);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(/*Cost*/0),
		DamageCard,
		{ Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0) });
	UEnemyDefinition* Enemy = WacomBattleSceneEnemyTargetSpec::MakeSingleSlotEnemy(
		Fixture,
		TEXT("Test.Enemy.KeyOnly"),
		TEXT("Test.Part.SharedCore"),
		TEXT("Core"),
		/*Hp*/30);

	FBattleInitParams Params;
	Params.Character = Character;
	Params.EncounterId = TEXT("Encounter.UI.KeyOnlyClick");
	Params.RandomSeed = 1;
	FBattleEnemySlotInit Slot;
	Slot.EnemySlotId = TEXT("EnemyA");
	Slot.Enemy = Enemy;
	Params.EnemySlots.Add(Slot);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestTrue(TEXT("Session initializes"), Session->Initialize(Params).IsOk()))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyTargetSpec::FSceneHostForKeyRouting SceneHost =
		WacomBattleSceneEnemyTargetSpec::SpawnSceneHostForSlot(
			*World,
			Enemy,
			TEXT("EnemyA"),
			{ TEXT("Test.Part.SharedCore") },
			{ TEXT("Core") });
	AWacomBattleEnemyPartActor* PartActor = SceneHost.Parts.Num() > 0 ? SceneHost.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene host"), SceneHost.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session.Get());
	HUD->SetBattleSceneEnemyHostsForTest({ SceneHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	HUD->OnCardClickedByUser(FWacomBattleFixture::FindHandInstanceByCardId(Session->BuildSnapshot(), DamageCard->CardId));

	WacomBattleSceneEnemyTargetSpec::ClearRuntimeWorldTargetId(PartActor);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	FWacomInteractionTargetHandle ProbedHandle;
	TestTrue(TEXT("Probe builds key-only battle enemy part handle"),
		FWacomBattleSceneTargetClickTestAccess::ProbeTarget(PC, ProbedHandle));
	TestTrue(TEXT("Probed handle is valid"), ProbedHandle.IsValid());
	TestFalse(TEXT("Probed handle does not rely on runtime world target id"),
		ProbedHandle.WorldTargetId.IsValid());
	TestEqual(TEXT("Probed encounter id"), ProbedHandle.EncounterId, FName(TEXT("Encounter.UI.KeyOnlyClick")));
	TestEqual(TEXT("Probed enemy slot id"), ProbedHandle.EnemySlotId, FName(TEXT("EnemyA")));
	TestEqual(TEXT("Probed part slot id"), ProbedHandle.PartSlotId, FName(TEXT("Core")));
	TestTrue(TEXT("Probed handle is current scene registry target"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(ProbedHandle));

	const int32 HpBefore =
		WacomBattleSceneEnemyTargetSpec::FindPartBySlot(Session->BuildSnapshot(), TEXT("EnemyA"), TEXT("Core"))->CurrentHp;
	TestTrue(TEXT("Click routes through key-only handle"),
		FWacomBattleSceneTargetClickTestAccess::RouteClick(PC));
	const int32 HpAfter =
		WacomBattleSceneEnemyTargetSpec::FindPartBySlot(Session->BuildSnapshot(), TEXT("EnemyA"), TEXT("Core"))->CurrentHp;
	TestEqual(TEXT("Click damages keyed target"), HpAfter, HpBefore - 5);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetRegistryRejectsSourceObjectFallbackSpec,
	"Wacom.UI.Battle.SceneEnemyTarget.RegistryRejectsSourceObjectFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetRegistryRejectsSourceObjectFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(/*Cost*/0),
		Fixture.MakeNoopCard(/*Cost*/0),
		{ Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0) });
	UEnemyDefinition* Enemy = WacomBattleSceneEnemyTargetSpec::MakeSingleSlotEnemy(
		Fixture,
		TEXT("Test.Enemy.RegistryKeyOnly"),
		TEXT("Test.Part.SharedCore"),
		TEXT("Core"),
		/*Hp*/30);

	FBattleInitParams Params;
	Params.Character = Character;
	Params.EncounterId = TEXT("Encounter.UI.RegistryKeyOnly");
	Params.RandomSeed = 1;
	FBattleEnemySlotInit Slot;
	Slot.EnemySlotId = TEXT("EnemyA");
	Slot.Enemy = Enemy;
	Params.EnemySlots.Add(Slot);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestTrue(TEXT("Session initializes"), Session->Initialize(Params).IsOk()))
	{
		return false;
	}

	WacomBattleSceneEnemyTargetSpec::FSceneHostForKeyRouting SceneHost =
		WacomBattleSceneEnemyTargetSpec::SpawnSceneHostForSlot(
			*World,
			Enemy,
			TEXT("EnemyA"),
			{ TEXT("Test.Part.SharedCore") },
			{ TEXT("Core") });
	AWacomBattleEnemyPartActor* PartActor = SceneHost.Parts.Num() > 0 ? SceneHost.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Scene host"), SceneHost.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor)
		|| !TestNotNull(TEXT("Interaction target"), PartActor ? PartActor->GetInteractionTargetComponent() : nullptr))
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(GetTransientPackage()));
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session.Get());
	HUD->SetBattleSceneEnemyHostsForTest({ SceneHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());

	const FWacomInteractionTargetHandle ValidHandle = FWacomInteractionTargetHandle::ForWorldTarget(
		FGuid(),
		PartActor->GetInteractionTargetComponent(),
		FVector::ZeroVector,
		FVector2D::ZeroVector,
		WacomTags::Interaction_Target_Battle_EnemyPart,
		TEXT("Test.Part.SharedCore"),
		TEXT("Encounter.UI.RegistryKeyOnly"),
		TEXT("EnemyA"),
		TEXT("Core"));
	TestTrue(TEXT("Correct key is in the current scene registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(ValidHandle));

	const FWacomInteractionTargetHandle WrongKeySameSourceObjectHandle =
		FWacomInteractionTargetHandle::ForWorldTarget(
			FGuid(),
			PartActor->GetInteractionTargetComponent(),
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			WacomTags::Interaction_Target_Battle_EnemyPart,
			TEXT("Test.Part.SharedCore"),
			TEXT("Encounter.UI.RegistryKeyOnly"),
			TEXT("EnemyA"),
			TEXT("WrongCore"));
	TestFalse(TEXT("Wrong key is rejected even when SourceObject points at a registered part"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(WrongKeySameSourceObjectHandle));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetHoverRoutesByEnemyPartKeySpec,
	"Wacom.UI.Battle.SceneEnemyTarget.HoverRoutesByEnemyPartKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetHoverRoutesByEnemyPartKeySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(/*Cost*/0),
		Fixture.MakeNoopCard(/*Cost*/0),
		{ Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/1), Fixture.MakeNoopCard(/*Cost*/0) });
	UEnemyDefinition* Enemy = WacomBattleSceneEnemyTargetSpec::MakeSingleSlotEnemy(
		Fixture,
		TEXT("Test.Enemy.HoverKey"),
		TEXT("Test.Part.SharedCore"),
		TEXT("Core"),
		/*Hp*/30);

	FBattleInitParams Params;
	Params.Character = Character;
	Params.EncounterId = TEXT("Encounter.UI.KeyOnlyHover");
	Params.RandomSeed = 1;
	FBattleEnemySlotInit Slot;
	Slot.EnemySlotId = TEXT("EnemyA");
	Slot.Enemy = Enemy;
	Params.EnemySlots.Add(Slot);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestTrue(TEXT("Session initializes"), Session->Initialize(Params).IsOk()))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyTargetSpec::FSceneHostForKeyRouting SceneHost =
		WacomBattleSceneEnemyTargetSpec::SpawnSceneHostForSlot(
			*World,
			Enemy,
			TEXT("EnemyA"),
			{ TEXT("Test.Part.SharedCore") },
			{ TEXT("Core") });
	AWacomBattleEnemyPartActor* PartActor = SceneHost.Parts.Num() > 0 ? SceneHost.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene host"), SceneHost.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session.Get());
	HUD->SetBattleSceneEnemyHostsForTest({ SceneHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	WacomBattleSceneEnemyTargetSpec::ClearRuntimeWorldTargetId(PartActor);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest(1.0f);
	const FWacomBattleEnemyPartPresentationDebugView View =
		PartActor->GetPresentationComponent()->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("Hover probe activates through key-only handle"), View.bHoverActive);
	TestFalse(TEXT("Hover probe world id remains empty for key-only route"),
		View.HoverWorldTargetId.IsValid());
	TestEqual(TEXT("Hover stable id"), View.HoverStableId, FName(TEXT("Test.Part.SharedCore")));
	TestEqual(TEXT("Hover reason"), View.HoverReason, FName(TEXT("Hovered")));
	TestFalse(TEXT("Hover prediction is handled by enemy panel, not prediction badge"), View.PredictionView.bVisible);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetDragDropRoutesByEnemyPartKeySpec,
	"Wacom.UI.Battle.SceneEnemyTarget.DragDropRoutesByEnemyPartKey",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetDragDropRoutesByEnemyPartKeySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyTargetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/6);
	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(/*Cost*/0),
		DamageCard,
		{ Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0), Fixture.MakeNoopCard(/*Cost*/0) });
	UEnemyDefinition* Enemy = WacomBattleSceneEnemyTargetSpec::MakeSingleSlotEnemy(
		Fixture,
		TEXT("Test.Enemy.DragKey"),
		TEXT("Test.Part.SharedCore"),
		TEXT("Core"),
		/*Hp*/30);

	FBattleInitParams Params;
	Params.Character = Character;
	Params.EncounterId = TEXT("Encounter.UI.KeyOnlyDrag");
	Params.RandomSeed = 1;
	FBattleEnemySlotInit Slot;
	Slot.EnemySlotId = TEXT("EnemyA");
	Slot.Enemy = Enemy;
	Params.EnemySlots.Add(Slot);

	TStrongObjectPtr<UBattleSession> Session(NewObject<UBattleSession>(GetTransientPackage(), NAME_None, RF_Transient));
	if (!TestTrue(TEXT("Session initializes"), Session->Initialize(Params).IsOk()))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	WacomBattleSceneEnemyTargetSpec::FSceneHostForKeyRouting SceneHost =
		WacomBattleSceneEnemyTargetSpec::SpawnSceneHostForSlot(
			*World,
			Enemy,
			TEXT("EnemyA"),
			{ TEXT("Test.Part.SharedCore") },
			{ TEXT("Core") });
	AWacomBattleEnemyPartActor* PartActor = SceneHost.Parts.Num() > 0 ? SceneHost.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Scene host"), SceneHost.Host)
		|| !TestNotNull(TEXT("Part actor"), PartActor))
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyTargetSpec::DestroySceneHost(SceneHost);
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>(PC));
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->SetSession(Session.Get());
	HUD->SetBattleSceneEnemyHostsForTest({ SceneHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	const FGuid CardId = FWacomBattleFixture::FindHandInstanceByCardId(Session->BuildSnapshot(), DamageCard->CardId);
	if (!TestTrue(TEXT("Damage card in hand"), CardId.IsValid()))
	{
		return false;
	}

	WacomBattleSceneEnemyTargetSpec::ClearRuntimeWorldTargetId(PartActor);
	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD.Get());
	FWacomBattleSceneTargetClickTestAccess::SetHit(PC, PartActor, PartActor->GetHitBounds());

	FWacomFirstPersonCardDragView DragView =
		WacomBattleSceneEnemyTargetSpec::MakeAimingDragView(CardId);
	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(CardId, DragView);
	TestEqual(TEXT("Drop intent routes to world target"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	TestTrue(TEXT("Drop intent can submit"), DropResult.bCanSubmit);
	TestFalse(TEXT("Drop intent does not rely on runtime world target id"),
		DropResult.TargetHandle.WorldTargetId.IsValid());
	TestTrue(TEXT("Drop intent carries battle key"), DropResult.TargetHandle.HasBattlePartSlotIdentity());
	TestEqual(TEXT("Drop target enemy slot"), DropResult.TargetHandle.EnemySlotId, FName(TEXT("EnemyA")));
	TestEqual(TEXT("Drop target part slot"), DropResult.TargetHandle.PartSlotId, FName(TEXT("Core")));
	return true;
}
