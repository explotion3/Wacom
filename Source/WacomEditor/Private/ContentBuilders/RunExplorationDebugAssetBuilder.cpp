// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunExplorationDebugAssetBuilder.h"

#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Actors/WacomRunKeyChestActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomRunRewardPickupActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Actors/BattleTriggerActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Characters/CharacterDefinition.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomRunPathTraversalComponent.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Engine/Blueprint.h"
#include "EngineUtils.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "FileHelpers.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "HAL/FileManager.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"
#include "UObject/SavePackage.h"
#include "Validation/WacomMapDefinitionValidation.h"
#include "Validation/WacomRunSceneBindingValidation.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr const TCHAR* JourneyAssetName = TEXT("DA_Journey_Debug");
	constexpr const TCHAR* FloorAssetName = TEXT("DA_Floor_Debug_01");

	FString MapDataRoot()
	{
		return DataRoot() / TEXT("Map");
	}

	FString PathBlueprintRoot()
	{
		return TEXT("/Game/Wacom/Run/Path/Blueprints");
	}

	template <typename T>
	T* LoadRequired(const TCHAR* ObjectPath)
	{
		T* Asset = LoadObject<T>(nullptr, ObjectPath);
		if (!Asset)
		{
			UE_LOG(LogTemp, Error, TEXT("[RunExplorationDebugAssetBuilder] Missing asset: %s"), ObjectPath);
		}
		return Asset;
	}

	FWacomMapNodeDefinition MakeNode(
		const TCHAR* NodeId,
		const EWacomMapNodeType NodeType,
		const FVector2D Position,
		const bool bAllowsCamp = false)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = NodeId;
		Node.DisplayName = FText::FromString(NodeId);
		Node.NodeType = NodeType;
		Node.MapPosition = Position;
		Node.bAllowsCamp = bAllowsCamp;
		return Node;
	}

	FWacomMapEdgeDefinition MakeEdge(const TCHAR* EdgeId, const TCHAR* From, const TCHAR* To)
	{
		FWacomMapEdgeDefinition Edge;
		Edge.EdgeId = EdgeId;
		Edge.FromNodeId = From;
		Edge.ToNodeId = To;
		return Edge;
	}

	bool SaveBlueprint(UBlueprint& Blueprint, const FString& PackagePath)
	{
		FKismetEditorUtilities::CompileBlueprint(&Blueprint);
		if (Blueprint.Status == BS_Error)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Blueprint compile failed: %s"),
				*Blueprint.GetPathName());
			return false;
		}

		UPackage* Package = Blueprint.GetOutermost();
		FAssetRegistryModule::AssetCreated(&Blueprint);
		Package->MarkPackageDirty();
		Blueprint.MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}

	bool EnsureActorBlueprint(const TCHAR* AssetName, UClass& ParentClass)
	{
		const FString PackagePath = MakePackagePath(PathBlueprintRoot(), AssetName);
		UPackage* Package = FindOrCreatePackage(PackagePath);
		if (!Package)
		{
			return false;
		}

		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *MakeObjectPath(PackagePath));
		if (Blueprint)
		{
			if (!Blueprint->GeneratedClass || !Blueprint->GeneratedClass->IsChildOf(&ParentClass))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunExplorationDebugAssetBuilder] Existing Blueprint has wrong parent: %s"),
					*PackagePath);
				return false;
			}
		}
		else
		{
			Blueprint = FKismetEditorUtilities::CreateBlueprint(
				&ParentClass,
				Package,
				AssetName,
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
		}
		return Blueprint && SaveBlueprint(*Blueprint, PackagePath);
	}

	bool BuildPathBlueprints()
	{
		return EnsureActorBlueprint(
			TEXT("BP_WacomRunPathSegmentActor"), *AWacomRunPathSegmentActor::StaticClass())
			&& EnsureActorBlueprint(
				TEXT("BP_WacomRunPathBranchTargetActor"), *AWacomRunPathBranchTargetActor::StaticClass())
			&& EnsureActorBlueprint(
				TEXT("BP_WacomRunMapNodeAnchorActor"), *AWacomRunMapNodeAnchorActor::StaticClass());
	}

	bool SaveLoadedBlueprint(UBlueprint& Blueprint)
	{
		UPackage* Package = Blueprint.GetOutermost();
		if (!Package)
		{
			return false;
		}
		Package->MarkPackageDirty();
		Blueprint.MarkPackageDirty();
		const FString PackagePath = Package->GetName();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(Package, &Blueprint, *Filename, Args);
	}

	bool ConfigureRuntimeBlueprints(UWacomJourneyDefinition& Journey)
	{
		UBlueprint* GameModeBlueprint = LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Core/GameModes/GM_Wacom.GM_Wacom"));
		AWacomGameMode* GameModeCDO = GameModeBlueprint && GameModeBlueprint->GeneratedClass
			? Cast<AWacomGameMode>(GameModeBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!GameModeBlueprint || !GameModeCDO)
		{
			UE_LOG(LogTemp, Error, TEXT("[RunExplorationDebugAssetBuilder] Missing GM_Wacom"));
			return false;
		}
		GameModeBlueprint->Modify();
		GameModeCDO->Modify();
		GameModeCDO->DefaultJourneyDefinition = &Journey;
		FBlueprintEditorUtils::MarkBlueprintAsModified(GameModeBlueprint);

		UBlueprint* PlayerBlueprint = LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Core/Player/BP_WacomPlayerCharacter.BP_WacomPlayerCharacter"));
		AWacomPlayerCharacter* PlayerCDO = PlayerBlueprint && PlayerBlueprint->GeneratedClass
			? Cast<AWacomPlayerCharacter>(PlayerBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		UWacomRunPathTraversalComponent* NewTraversal = PlayerCDO
			? PlayerCDO->GetRunPathTraversalComponent()
			: nullptr;
		if (!PlayerBlueprint || !PlayerCDO || !NewTraversal)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Player Blueprint missing Run Path traversal component"));
			return false;
		}
		PlayerBlueprint->Modify();
		PlayerCDO->Modify();
		NewTraversal->Modify();
		FBlueprintEditorUtils::MarkBlueprintAsModified(PlayerBlueprint);
		FKismetEditorUtilities::CompileBlueprint(GameModeBlueprint);
		FKismetEditorUtilities::CompileBlueprint(PlayerBlueprint);
		if (GameModeBlueprint->Status == BS_Error || PlayerBlueprint->Status == BS_Error)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Runtime Blueprint compile failed"));
			return false;
		}

		return SaveLoadedBlueprint(*GameModeBlueprint) && SaveLoadedBlueprint(*PlayerBlueprint);
	}

	FTransform MakeFallbackNodeTransform(const FWacomMapNodeDefinition& Node)
	{
		const FVector EntryLocation(-4920.f, 370.f, 172.f);
		const FVector2D EntryMapPosition(960.f, 980.f);
		const FVector WorldOffset(
			(EntryMapPosition.Y - Node.MapPosition.Y) * 5.0,
			(Node.MapPosition.X - EntryMapPosition.X) * 2.0,
			0.0);
		return FTransform(FRotator::ZeroRotator, EntryLocation + WorldOffset);
	}

	template <typename TActor, typename Predicate>
	TActor* FindActor(UWorld& World, Predicate&& Matches)
	{
		for (TActorIterator<TActor> It(&World); It; ++It)
		{
			if (Matches(**It))
			{
				return *It;
			}
		}
		return nullptr;
	}

	AActor* FindContentHost(UWorld& World, const FWacomMapNodeDefinition& Node)
	{
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			return FindActor<ABattleTriggerActor>(World,
				[&Node](const ABattleTriggerActor& Actor)
				{
					return Actor.EncounterDefinition == Node.Content.Encounter.EncounterDefinition;
				});
		case EWacomMapNodeType::RunEvent:
			return FindActor<AWacomRunEventTriggerActor>(World,
				[&Node](const AWacomRunEventTriggerActor& Actor)
				{
					return Actor.EventDefinition == Node.Content.RunEvent.RunEventDefinition;
				});
		case EWacomMapNodeType::Shop:
			return FindActor<AWacomShopTriggerActor>(World,
				[&Node](const AWacomShopTriggerActor& Actor)
				{
					return Actor.ShopDefinition == Node.Content.Shop.ShopDefinition;
				});
		case EWacomMapNodeType::Treasure:
			if (Node.Content.Treasure.PickupDefinition)
			{
				return FindActor<AWacomRunRewardPickupActor>(World,
					[&Node](const AWacomRunRewardPickupActor& Actor)
					{
						return Actor.PickupDefinition == Node.Content.Treasure.PickupDefinition;
					});
			}
			return FindActor<AWacomRunKeyChestActor>(World,
				[&Node](const AWacomRunKeyChestActor& Actor)
				{
					return Actor.CardInteractionDefinition ==
						Node.Content.Treasure.WorldCardInteractionDefinition;
				});
		default:
			return nullptr;
		}
	}

	bool BindContentHost(AActor& Host, const FWacomMapNodeDefinition& Node)
	{
		UWacomRunMapNodeBindingComponent* Binding =
			FindObject<UWacomRunMapNodeBindingComponent>(&Host, TEXT("RunMapNodeBinding"));
		if (!Binding)
		{
			Binding = NewObject<UWacomRunMapNodeBindingComponent>(
				&Host, TEXT("RunMapNodeBinding"), RF_Transactional);
			Binding->CreationMethod = EComponentCreationMethod::Instance;
			Host.AddInstanceComponent(Binding);
			Binding->OnComponentCreated();
			Binding->RegisterComponent();
		}
		Host.Modify();
		Binding->Modify();
		Binding->NodeId = Node.NodeId;
		Binding->NodeType = Node.NodeType;
		return true;
	}

	bool MigrateExplorationWorld(UWacomFloorMapDefinition& Floor)
	{
		UWorld* World = UEditorLoadingAndSavingUtils::LoadMap(TEXT("/Game/Wacom/Maps/L_Exploration"));
		if (!World)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Failed to load L_Exploration"));
			return false;
		}

		static const FName GeneratedTag = TEXT("Wacom.Generated.RunExploration");
		TMap<FName, FTransform> ExistingAnchorTransforms;
		for (TActorIterator<AWacomRunMapNodeAnchorActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(GeneratedTag) && !It->NodeId.IsNone())
			{
				ExistingAnchorTransforms.Add(It->NodeId, It->GetActorTransform());
			}
		}

		TArray<AActor*> ActorsToRemove;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			if (It->ActorHasTag(GeneratedTag))
			{
				ActorsToRemove.Add(*It);
			}
		}
		for (AActor* Actor : ActorsToRemove)
		{
			World->DestroyActor(Actor);
		}

		UBlueprint* AnchorBlueprint = LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor.BP_WacomRunMapNodeAnchorActor"));
		UBlueprint* PathBlueprint = LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor.BP_WacomRunPathSegmentActor"));
		UBlueprint* BranchBlueprint = LoadObject<UBlueprint>(nullptr,
			TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor.BP_WacomRunPathBranchTargetActor"));
		if (!AnchorBlueprint || !AnchorBlueprint->GeneratedClass
			|| !PathBlueprint || !PathBlueprint->GeneratedClass
			|| !BranchBlueprint || !BranchBlueprint->GeneratedClass)
		{
			return false;
		}

		TMap<FName, AActor*> Hosts;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			if (Node.NodeType == EWacomMapNodeType::Navigation)
			{
				continue;
			}
			AActor* Host = FindContentHost(*World, Node);
			if (!Host)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunExplorationDebugAssetBuilder] Missing content host for NodeId=%s"),
					*Node.NodeId.ToString());
				return false;
			}
			BindContentHost(*Host, Node);
			Hosts.Add(Node.NodeId, Host);
		}

		TMap<FName, AWacomRunMapNodeAnchorActor*> Anchors;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			const FTransform Transform = ExistingAnchorTransforms.Contains(Node.NodeId)
				? ExistingAnchorTransforms.FindChecked(Node.NodeId)
				: MakeFallbackNodeTransform(Node);

			AWacomRunMapNodeAnchorActor* Anchor = Cast<AWacomRunMapNodeAnchorActor>(
				World->SpawnActor<AActor>(AnchorBlueprint->GeneratedClass, Transform));
			if (!Anchor)
			{
				return false;
			}
			Anchor->NodeId = Node.NodeId;
			Anchor->Tags.AddUnique(GeneratedTag);
			Anchor->SetActorLabel(FString::Printf(TEXT("RunNode_%s"), *Node.NodeId.ToString()));
			Anchors.Add(Node.NodeId, Anchor);
		}

		TMap<FName, int32> DeclaredOutgoingEdgeCounts;
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			++DeclaredOutgoingEdgeCounts.FindOrAdd(Edge.FromNodeId);
		}
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			AWacomRunMapNodeAnchorActor* const* SourceAnchor = Anchors.Find(Edge.FromNodeId);
			AWacomRunMapNodeAnchorActor* const* TargetAnchor = Anchors.Find(Edge.ToNodeId);
			if (!SourceAnchor || !TargetAnchor)
			{
				return false;
			}
			AWacomRunPathSegmentActor* Path = Cast<AWacomRunPathSegmentActor>(
				World->SpawnActor<AActor>(PathBlueprint->GeneratedClass, FTransform::Identity));
			if (!Path || !Path->GetPathSpline())
			{
				return false;
			}
			Path->EdgeId = Edge.EdgeId;
			Path->Tags.AddUnique(GeneratedTag);
			Path->SetActorLabel(FString::Printf(TEXT("RunPath_%s"), *Edge.EdgeId.ToString()));
			USplineComponent* Spline = Path->GetPathSpline();
			Spline->ClearSplinePoints(false);
			Spline->AddSplinePoint((*SourceAnchor)->GetActorLocation(), ESplineCoordinateSpace::World, false);
			Spline->AddSplinePoint((*TargetAnchor)->GetActorLocation(), ESplineCoordinateSpace::World, false);
			Spline->SetSplinePointType(0, ESplinePointType::Curve, false);
			Spline->SetSplinePointType(1, ESplinePointType::Curve, false);
			Spline->UpdateSpline();

			if (DeclaredOutgoingEdgeCounts.FindRef(Edge.FromNodeId) < 2)
			{
				continue;
			}

			const FVector InitialDirection =
				Spline->GetDirectionAtSplinePoint(0, ESplineCoordinateSpace::World)
					.GetSafeNormal();
			FTransform BranchTransform = (*SourceAnchor)->GetActorTransform();
			BranchTransform.SetLocation(
				(*SourceAnchor)->GetActorLocation() + InitialDirection * 160.0f);
			BranchTransform.SetRotation(InitialDirection.Rotation().Quaternion());
			AWacomRunPathBranchTargetActor* Branch = Cast<AWacomRunPathBranchTargetActor>(
				World->SpawnActor<AActor>(BranchBlueprint->GeneratedClass, BranchTransform));
			if (!Branch)
			{
				return false;
			}
			Branch->EdgeId = Edge.EdgeId;
			Branch->Tags.AddUnique(GeneratedTag);
			Branch->SetActorLabel(FString::Printf(TEXT("RunBranch_%s"), *Edge.EdgeId.ToString()));
		}

		const FWacomRunSceneBindingValidationReport SceneReport =
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(World, &Floor);
		for (const FText& Error : SceneReport.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("[RunExplorationDebugAssetBuilder] Scene validation: %s"),
				*Error.ToString());
		}
		return SceneReport.IsValid()
			&& UEditorLoadingAndSavingUtils::SaveMap(World, TEXT("/Game/Wacom/Maps/L_Exploration"));
	}
}

namespace Wacom::ContentBuilder
{
	FRunExplorationDebugAssetBuildResult BuildRunExplorationDebugAssets()
	{
		FRunExplorationDebugAssetBuildResult Result;
		UCharacterDefinition* Character = LoadRequired<UCharacterDefinition>(
			TEXT("/Game/Wacom/Data/Characters/DA_Character_BugGirl.DA_Character_BugGirl"));
		UEncounterDefinition* Encounter = LoadRequired<UEncounterDefinition>(
			TEXT("/Game/Wacom/Data/Encounters/DA_Encounter_SnakeSingle.DA_Encounter_SnakeSingle"));
		UWacomRunEventDefinition* EventSnake = LoadRequired<UWacomRunEventDefinition>(
			TEXT("/Game/Wacom/Data/Events/DA_Event_DebugSnakeGift.DA_Event_DebugSnakeGift"));
		UWacomRunEventDefinition* EventFlag = LoadRequired<UWacomRunEventDefinition>(
			TEXT("/Game/Wacom/Data/Events/DA_Event_DebugFlagReward.DA_Event_DebugFlagReward"));
		UShopDefinition* Shop = LoadRequired<UShopDefinition>(
			TEXT("/Game/Wacom/Data/Shops/DA_Shop_DebugSnake.DA_Shop_DebugSnake"));
		UWacomRunPickupDefinition* Pickup = LoadRequired<UWacomRunPickupDefinition>(
			TEXT("/Game/Wacom/Data/Pickups/DA_Pickup_DebugPoisonFang.DA_Pickup_DebugPoisonFang"));
		UWacomRunWorldCardInteractionDefinition* Chest =
			LoadRequired<UWacomRunWorldCardInteractionDefinition>(
				TEXT("/Game/Wacom/Data/Interactions/DA_RunWorldCardInteraction_DebugKeyGold3.DA_RunWorldCardInteraction_DebugKeyGold3"));
		if (!Character || !Encounter || !EventSnake || !EventFlag || !Shop || !Pickup || !Chest)
		{
			return Result;
		}

		const FString FloorPackagePath = MakePackagePath(MapDataRoot(), FloorAssetName);
		UPackage* FloorPackage = FindOrCreatePackage(FloorPackagePath);
		Result.Floor = FloorPackage
			? CreateOrReplaceAsset<UWacomFloorMapDefinition>(FloorPackage, FloorAssetName)
			: nullptr;
		if (!Result.Floor)
		{
			return Result;
		}

		Result.Floor->FloorId = TEXT("Floor.Debug.01");
		Result.Floor->DisplayName = FText::FromString(TEXT("蛇巢浅层"));
		Result.Floor->EntryNodeId = TEXT("Entry");
		FWacomMapNodeDefinition Entry = MakeNode(
			TEXT("Entry"), EWacomMapNodeType::Navigation, FVector2D(960.f, 980.f), true);
		Entry.DisplayName = FText::FromString(TEXT("林地入口"));
		Entry.ShortDescription = FText::FromString(TEXT("进入蛇巢浅层的安全落脚点。"));
		FWacomMapNodeDefinition Battle = MakeNode(
			TEXT("Battle.Snake"), EWacomMapNodeType::Encounter, FVector2D(960.f, 840.f));
		Battle.DisplayName = FText::FromString(TEXT("伏蛇草径"));
		Battle.ShortDescription = FText::FromString(TEXT("草丛中传来持续的窸窣声。"));
		Battle.Content.Encounter.EncounterDefinition = Encounter;
		FWacomMapNodeDefinition ShopNode = MakeNode(
			TEXT("Shop.Snake"), EWacomMapNodeType::Shop, FVector2D(960.f, 700.f));
		ShopNode.DisplayName = FText::FromString(TEXT("行商营帐"));
		ShopNode.ShortDescription = FText::FromString(TEXT("一处临时搭起的补给点。"));
		ShopNode.Content.Shop.ShopDefinition = Shop;
		FWacomMapNodeDefinition EventSnakeNode = MakeNode(
			TEXT("Event.SnakeGift"), EWacomMapNodeType::RunEvent, FVector2D(960.f, 560.f));
		EventSnakeNode.DisplayName = FText::FromString(TEXT("蛇蜕空地"));
		EventSnakeNode.ShortDescription = FText::FromString(TEXT("残留的蛇蜕似乎掩盖着什么。"));
		EventSnakeNode.Content.RunEvent.RunEventDefinition = EventSnake;
		FWacomMapNodeDefinition Junction = MakeNode(
			TEXT("Junction"), EWacomMapNodeType::Navigation, FVector2D(960.f, 420.f), true);
		Junction.DisplayName = FText::FromString(TEXT("三岔旧路"));
		Junction.ShortDescription = FText::FromString(TEXT("道路在这里分向三处。"));
		FWacomMapNodeDefinition PickupNode = MakeNode(
			TEXT("Treasure.PoisonFang"), EWacomMapNodeType::Treasure, FVector2D(650.f, 260.f), true);
		PickupNode.DisplayName = FText::FromString(TEXT("毒牙遗落处"));
		PickupNode.ShortDescription = FText::FromString(TEXT("一枚仍带毒性的断牙落在泥里。"));
		PickupNode.Content.Treasure.PickupDefinition = Pickup;
		FWacomMapNodeDefinition ChestNode = MakeNode(
			TEXT("Treasure.KeyChest"), EWacomMapNodeType::Treasure, FVector2D(1270.f, 260.f), true);
		ChestNode.DisplayName = FText::FromString(TEXT("封锁宝箱"));
		ChestNode.ShortDescription = FText::FromString(TEXT("沉重的箱锁需要合适的钥匙。"));
		ChestNode.Content.Treasure.WorldCardInteractionDefinition = Chest;
		FWacomMapNodeDefinition EventFlagNode = MakeNode(
			TEXT("Event.FlagReward"), EWacomMapNodeType::RunEvent, FVector2D(960.f, 120.f), true);
		EventFlagNode.DisplayName = FText::FromString(TEXT("褪色路标"));
		EventFlagNode.ShortDescription = FText::FromString(TEXT("一面褪色标记指向更深的林地。"));
		EventFlagNode.Content.RunEvent.RunEventDefinition = EventFlag;
		Result.Floor->Nodes =
		{
			Entry, Battle, ShopNode, EventSnakeNode, Junction, PickupNode, ChestNode, EventFlagNode,
		};
		Result.Floor->Edges =
		{
			MakeEdge(TEXT("EntryToBattle"), TEXT("Entry"), TEXT("Battle.Snake")),
			MakeEdge(TEXT("BattleToShop"), TEXT("Battle.Snake"), TEXT("Shop.Snake")),
			MakeEdge(TEXT("ShopToEventSnake"), TEXT("Shop.Snake"), TEXT("Event.SnakeGift")),
			MakeEdge(TEXT("EventSnakeToJunction"), TEXT("Event.SnakeGift"), TEXT("Junction")),
			MakeEdge(TEXT("JunctionToPickup"), TEXT("Junction"), TEXT("Treasure.PoisonFang")),
			MakeEdge(TEXT("JunctionToChest"), TEXT("Junction"), TEXT("Treasure.KeyChest")),
			MakeEdge(TEXT("JunctionToEventFlag"), TEXT("Junction"), TEXT("Event.FlagReward")),
		};
		if (!FWacomMapDefinitionValidation::ValidateFloor(Result.Floor).IsValid()
			|| !SaveAssetPackage(FloorPackage, Result.Floor, FloorPackagePath))
		{
			Result.Floor = nullptr;
			return Result;
		}

		const FString JourneyPackagePath = MakePackagePath(MapDataRoot(), JourneyAssetName);
		UPackage* JourneyPackage = FindOrCreatePackage(JourneyPackagePath);
		Result.Journey = JourneyPackage
			? CreateOrReplaceAsset<UWacomJourneyDefinition>(JourneyPackage, JourneyAssetName)
			: nullptr;
		if (!Result.Journey)
		{
			return Result;
		}
		Result.Journey->JourneyId = TEXT("Journey.Debug");
		Result.Journey->SupportedCharacters = {Character};
		Result.Journey->Floors = {Result.Floor};
		if (!FWacomMapDefinitionValidation::ValidateJourney(Result.Journey).IsValid()
			|| !SaveAssetPackage(JourneyPackage, Result.Journey, JourneyPackagePath))
		{
			Result.Journey = nullptr;
			return Result;
		}

		Result.bPathBlueprintsBuilt = BuildPathBlueprints();
		if (!Result.bPathBlueprintsBuilt)
		{
			return Result;
		}
		Result.bRuntimeAssetsConfigured = ConfigureRuntimeBlueprints(*Result.Journey);
		if (!Result.bRuntimeAssetsConfigured)
		{
			return Result;
		}
		Result.bExplorationWorldMigrated = MigrateExplorationWorld(*Result.Floor);
		return Result;
	}
}
