// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunKeyChestActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomRunRewardPickupActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "Engine/World.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Interaction/WacomRunWorldCardDropReceiver.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "RunSession.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"

namespace WacomRunTreasureReturnToRunPathSpec
{
	AWacomRunMapNodeAnchorActor* SpawnTreasureReturnAnchor(
		UWorld& World,
		const FName NodeId,
		const FVector& Location)
	{
		AWacomRunMapNodeAnchorActor* Anchor =
			World.SpawnActor<AWacomRunMapNodeAnchorActor>(
				AWacomRunMapNodeAnchorActor::StaticClass(),
				FTransform(Location));
		if (Anchor)
		{
			Anchor->NodeId = NodeId;
		}
		return Anchor;
	}

	AWacomRunPathSegmentActor* SpawnTreasureReturnPath(
		UWorld& World,
		const FName EdgeId,
		const FVector& Start,
		const FVector& End)
	{
		AWacomRunPathSegmentActor* Path =
			World.SpawnActor<AWacomRunPathSegmentActor>();
		if (!Path || !Path->GetPathSpline())
		{
			return Path;
		}

		Path->EdgeId = EdgeId;
		USplineComponent* Spline = Path->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Start, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(End, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Path;
	}

	struct FTreasureReturnFixture
	{
		FWacomRunExplorationFixture Exploration;
		UWorld* World = nullptr;
		URunSession* Session = nullptr;
		AWacomPlayerControllerProbe* PlayerController = nullptr;
		AWacomPlayerCharacter* PlayerCharacter = nullptr;
		AWacomRunMapNodeAnchorActor* SourceAnchor = nullptr;
		AWacomRunMapNodeAnchorActor* TargetAnchor = nullptr;
		AWacomRunPathSegmentActor* Path = nullptr;
		AWacomRunFloorSceneDescriptorActor* Descriptor = nullptr;
		AActor* ContentHost = nullptr;
		UWacomRunPathTraversalComponent* Traversal = nullptr;

		bool Initialize(
			UClass* ContentHostClass,
			UCharacterDefinition* CharacterDefinition = nullptr)
		{
			World = UWorld::CreateWorld(EWorldType::Game, false);
			if (!World || !ContentHostClass)
			{
				return false;
			}

			const FName FloorId(TEXT("Treasure.Return.Floor"));
			const FName SourceNodeId(TEXT("Treasure.Return.Source"));
			const FName TargetNodeId(TEXT("Treasure.Return.Target"));
			const FName EdgeId(TEXT("Treasure.Return.Edge"));

			FWacomMapNodeDefinition SourceNode;
			SourceNode.NodeId = SourceNodeId;
			SourceNode.NodeType = EWacomMapNodeType::Treasure;
			SourceNode.DisplayName = FText::FromString(TEXT("Treasure"));
			FWacomMapNodeDefinition TargetNode;
			TargetNode.NodeId = TargetNodeId;
			TargetNode.NodeType = EWacomMapNodeType::Navigation;
			TargetNode.DisplayName = FText::FromString(TEXT("Path"));
			FWacomMapEdgeDefinition Edge;
			Edge.EdgeId = EdgeId;
			Edge.FromNodeId = SourceNodeId;
			Edge.ToNodeId = TargetNodeId;

			UWacomFloorMapDefinition* Floor = Exploration.MakeFloor(
				FloorId,
				FText::FromString(TEXT("Treasure return")),
				{SourceNode, TargetNode},
				{Edge},
				SourceNodeId);
			UWacomJourneyDefinition* Journey = Exploration.MakeJourney({Floor});
			Journey->PhaseBudgets.Morning = 8;
			const FWacomInitializedRunExplorationSession Initialized =
				Exploration.CreateInitializedSession(CharacterDefinition, Journey);
			if (!Initialized.Initialization.IsOk())
			{
				return false;
			}
			Session = Initialized.Session;

			PlayerController = World->SpawnActor<AWacomPlayerControllerProbe>();
			PlayerCharacter = World->SpawnActor<AWacomPlayerCharacter>();
			SourceAnchor = SpawnTreasureReturnAnchor(
				*World,
				SourceNodeId,
				FVector::ZeroVector);
			TargetAnchor = SpawnTreasureReturnAnchor(
				*World,
				TargetNodeId,
				FVector(1000.0f, 0.0f, 0.0f));
			Path = SpawnTreasureReturnPath(
				*World,
				EdgeId,
				FVector::ZeroVector,
				FVector(1000.0f, 0.0f, 0.0f));
			Descriptor = World->SpawnActor<AWacomRunFloorSceneDescriptorActor>();
			ContentHost = World->SpawnActor<AActor>(
				ContentHostClass,
				FTransform::Identity);
			if (!PlayerController
				|| !PlayerCharacter
				|| !SourceAnchor
				|| !TargetAnchor
				|| !Path
				|| !Descriptor
				|| !ContentHost)
			{
				return false;
			}

			Descriptor->FloorDefinition = Floor;
			UWacomRunMapNodeBindingComponent* Binding =
				NewObject<UWacomRunMapNodeBindingComponent>(
					ContentHost,
					TEXT("TreasureBinding"));
			ContentHost->AddInstanceComponent(Binding);
			Binding->NodeId = SourceNodeId;
			Binding->NodeType = EWacomMapNodeType::Treasure;
			Binding->RegisterComponent();

			PlayerController->Possess(PlayerCharacter);
			PlayerController->SetPawn(PlayerCharacter);
			FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(
				PlayerController,
				Session);
			if (!FWacomPlayerControllerRunInteractionTestAccess::
				RefreshRunExplorationPresentationBinding(PlayerController))
			{
				return false;
			}
			Traversal = PlayerCharacter->GetRunPathTraversalComponent();
			return Traversal != nullptr;
		}

		void Cleanup()
		{
			if (!World)
			{
				return;
			}
			if (IsValid(ContentHost)) ContentHost->Destroy();
			if (IsValid(Path)) Path->Destroy();
			if (IsValid(SourceAnchor)) SourceAnchor->Destroy();
			if (IsValid(TargetAnchor)) TargetAnchor->Destroy();
			if (IsValid(Descriptor)) Descriptor->Destroy();
			if (IsValid(PlayerCharacter)) PlayerCharacter->Destroy();
			if (IsValid(PlayerController)) PlayerController->Destroy();
			World->DestroyWorld(false);
			World = nullptr;
		}
	};

	FGuid FindTreasureReturnCardInstance(
		const URunSession& Session,
		const UCardDefinition* Definition)
	{
		const auto FindInPile = [Definition](const TArray<FCardInstance>& Pile)
		{
			const FCardInstance* Found = Pile.FindByPredicate(
				[Definition](const FCardInstance& Instance)
				{
					return Instance.Definition == Definition;
				});
			return Found ? Found->InstanceId : FGuid();
		};

		const FRunState& State = Session.GetRunState();
		if (const FGuid Found = FindInPile(State.BattleDeck); Found.IsValid())
		{
			return Found;
		}
		if (const FGuid Found = FindInPile(State.Backpack); Found.IsValid())
		{
			return Found;
		}
		if (const FGuid Found = FindInPile(State.BurdenZone); Found.IsValid())
		{
			return Found;
		}
		for (const FSpecialZone& Zone : State.SpecialZones)
		{
			if (const FGuid Found = FindInPile(Zone.Cards); Found.IsValid())
			{
				return Found;
			}
		}
		return FGuid();
	}

	bool VerifyTreasureReturnState(
		FAutomationTestBase& Test,
		FTreasureReturnFixture& Fixture)
	{
		FName RejectDetail = NAME_None;
		const bool bMapCanOpen =
			FWacomPlayerControllerRunInteractionTestAccess::CanPresentRunMap(
				Fixture.PlayerController,
				RejectDetail);
		Test.TestTrue(TEXT("Map can open immediately after Treasure settlement"), bMapCanOpen);
		Test.TestTrue(TEXT("Treasure settlement leaves no map rejection detail"),
			RejectDetail.IsNone());
		Test.TestEqual(TEXT("Treasure settlement remains anchored"),
			Fixture.Traversal->GetTraversalState(),
			EWacomRunPathTraversalState::Anchored);
		Test.TestTrue(TEXT("First W after Treasure is consumed by Run Path"),
			Fixture.Traversal->HandleMoveInput(FVector2D(0.0f, 1.0f)));
		Test.TestTrue(TEXT("First W after Treasure does not deactivate traversal"),
			Fixture.Traversal->GetTraversalState()
				!= EWacomRunPathTraversalState::Inactive);
		return bMapCanOpen;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRewardPickupReturnKeepsRunPathSynchronizedTest,
	"Wacom.UI.WorldInteraction.TreasureReturnToRunPath.RewardPickupKeepsMapAndTraversalSynchronized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRewardPickupReturnKeepsRunPathSynchronizedTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomRunTreasureReturnToRunPathSpec;
	FTreasureReturnFixture Fixture;
	if (!TestTrue(TEXT("Reward fixture initializes"),
		Fixture.Initialize(AWacomRunRewardPickupActor::StaticClass())))
	{
		Fixture.Cleanup();
		return false;
	}

	AWacomRunRewardPickupActor* Pickup =
		Cast<AWacomRunRewardPickupActor>(Fixture.ContentHost);
	UCardDefinition* RewardCard = NewObject<UCardDefinition>(Fixture.Session);
	RewardCard->CardId = TEXT("Treasure.Return.RewardCard");
	UWacomRunPickupDefinition* Definition =
		NewObject<UWacomRunPickupDefinition>(Pickup);
	Definition->PickupId = TEXT("Treasure.Return.RewardDefinition");
	Definition->RewardType = EWacomRunPickupRewardType::Card;
	Definition->CardDefinition = RewardCard;
	Pickup->PersistentId = TEXT("Treasure.Return.RewardActor");
	Pickup->PickupDefinition = Definition;
	Pickup->bDestroyWhenCollected = false;

	TestTrue(TEXT("Reward Pickup settles through the production Actor"),
		Pickup->TryInteract_Implementation(Fixture.PlayerController));
	TestTrue(TEXT("Reward Pickup is recorded as collected"),
		Fixture.Session->IsPickupCollected(Pickup->PersistentId));
	VerifyTreasureReturnState(*this, Fixture);
	Fixture.Cleanup();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIKeyChestReturnKeepsRunPathSynchronizedTest,
	"Wacom.UI.WorldInteraction.TreasureReturnToRunPath.KeyChestKeepsMapAndTraversalSynchronized",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIKeyChestReturnKeepsRunPathSynchronizedTest::RunTest(
	const FString& Parameters)
{
	using namespace WacomRunTreasureReturnToRunPathSpec;
	FTreasureReturnFixture Fixture;
	UCardDefinition* KeyCard = NewObject<UCardDefinition>(GetTransientPackage());
	KeyCard->CardId = TEXT("Treasure.Return.KeyCard");
	UCharacterDefinition* Character =
		Fixture.Exploration.MakeCharacter(TEXT("Treasure.Return.Character"));
	Character->StarterDeck = {KeyCard};
	if (!TestTrue(TEXT("KeyChest fixture initializes"),
		Fixture.Initialize(AWacomRunKeyChestActor::StaticClass(), Character)))
	{
		Fixture.Cleanup();
		return false;
	}

	AWacomRunKeyChestActor* Chest = Cast<AWacomRunKeyChestActor>(Fixture.ContentHost);
	Chest->PersistentId = TEXT("Treasure.Return.KeyChest");
	UWacomRunWorldCardDropReceiverComponent* Receiver =
		Chest->GetCardDropReceiverComponent();
	if (!TestNotNull(TEXT("KeyChest card-drop receiver"), Receiver))
	{
		Fixture.Cleanup();
		return false;
	}
	Receiver->AllowedCardDefinitions = {KeyCard};
	Receiver->bConsumeCardOnSuccess = false;
	FWacomRunWorldCardInteractionReward Reward;
	Reward.Type = EWacomRunWorldCardInteractionRewardType::Gold;
	Reward.GoldAmount = 3;
	Receiver->Rewards = {Reward};

	const FGuid KeyInstanceId =
		FindTreasureReturnCardInstance(*Fixture.Session, KeyCard);
	TestTrue(TEXT("Key card instance exists"), KeyInstanceId.IsValid());
	FRunWorldCardInteractionValidation Validation;
	TestTrue(TEXT("KeyChest settles through the production card-drop receiver"),
		Receiver->SubmitRunWorldCardDrop_Implementation(
			Fixture.PlayerController,
			Chest->PersistentId,
			KeyInstanceId,
			Validation));
	TestTrue(TEXT("KeyChest interaction is recorded as completed"),
		Fixture.Session->IsRunWorldInteractionCompleted(Chest->PersistentId));
	VerifyTreasureReturnState(*this, Fixture);
	Fixture.Cleanup();
	return true;
}

#endif
