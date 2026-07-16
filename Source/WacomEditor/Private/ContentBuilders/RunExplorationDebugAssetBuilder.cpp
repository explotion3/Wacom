// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunExplorationDebugAssetBuilder.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunKeyChestActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomRunRewardPickupActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Characters/CharacterDefinition.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Engine/Blueprint.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Encounters/EncounterDefinition.h"
#include "Events/RunEventDefinition.h"
#include "FileHelpers.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WorldSettings.h"
#include "HAL/FileManager.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
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
	constexpr const TCHAR* DebugGameModeAssetName = TEXT("GM_WacomRunDebug");
	constexpr const TCHAR* DebugGameModePackagePath =
		TEXT("/Game/Wacom/Debug/GameModes/GM_WacomRunDebug");
	constexpr const TCHAR* DebugMapPackagePath =
		TEXT("/Game/Wacom/Maps/Debug/L_RunExploration_Debug");
	const FName DebugGeneratedTag(TEXT("Wacom.Generated.RunExploration"));

	struct FSharedBlueprintClasses
	{
		UClass* AnchorClass = nullptr;
		UClass* PathClass = nullptr;
		UClass* BranchClass = nullptr;
	};

	struct FExistingPathPresentation
	{
		FTransform ActorTransform = FTransform::Identity;
		TArray<FVector> SplinePoints;
	};

	FString MapDataRoot()
	{
		return DataRoot() / TEXT("Map");
	}

	template <typename T>
	T* LoadRequired(const TCHAR* ObjectPath)
	{
		T* Asset = LoadObject<T>(nullptr, ObjectPath);
		if (!Asset)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Missing asset: %s"),
				ObjectPath);
		}
		return Asset;
	}

	bool ValidateSharedBlueprint(
		const FString& ObjectPath,
		const UClass& ExpectedParent,
		UClass*& OutGeneratedClass)
	{
		OutGeneratedClass = nullptr;
		UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, *ObjectPath);
		if (!Blueprint || !Blueprint->GeneratedClass)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Missing shared Blueprint: %s"),
				*ObjectPath);
			return false;
		}
		if (!Blueprint->GeneratedClass->IsChildOf(&ExpectedParent))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Shared Blueprint has wrong parent: %s (expected %s)"),
				*ObjectPath, *ExpectedParent.GetPathName());
			return false;
		}
		OutGeneratedClass = Blueprint->GeneratedClass;
		return true;
	}

	bool ValidateSharedDependencies(
		const FRunExplorationDebugSharedDependencies& Dependencies,
		FSharedBlueprintClasses& OutClasses)
	{
		UClass* PlayerClass = nullptr;
		return ValidateSharedBlueprint(
			Dependencies.PlayerBlueprintObjectPath,
			*AWacomPlayerCharacter::StaticClass(), PlayerClass)
			&& ValidateSharedBlueprint(
				Dependencies.AnchorBlueprintObjectPath,
				*AWacomRunMapNodeAnchorActor::StaticClass(), OutClasses.AnchorClass)
			&& ValidateSharedBlueprint(
				Dependencies.PathBlueprintObjectPath,
				*AWacomRunPathSegmentActor::StaticClass(), OutClasses.PathClass)
			&& ValidateSharedBlueprint(
				Dependencies.BranchBlueprintObjectPath,
				*AWacomRunPathBranchTargetActor::StaticClass(), OutClasses.BranchClass);
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

	FWacomMapEdgeDefinition MakeEdge(
		const TCHAR* EdgeId,
		const TCHAR* From,
		const TCHAR* To)
	{
		FWacomMapEdgeDefinition Edge;
		Edge.EdgeId = EdgeId;
		Edge.FromNodeId = From;
		Edge.ToNodeId = To;
		return Edge;
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

	AActor* FindContentHost(
		UWorld& World,
		const FWacomMapNodeDefinition& Node)
	{
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			return FindActor<ABattleTriggerActor>(World,
				[&Node](const ABattleTriggerActor& Actor)
				{
					return Actor.EncounterDefinition ==
						Node.Content.Encounter.EncounterDefinition;
				});
		case EWacomMapNodeType::RunEvent:
			return FindActor<AWacomRunEventTriggerActor>(World,
				[&Node](const AWacomRunEventTriggerActor& Actor)
				{
					return Actor.EventDefinition ==
						Node.Content.RunEvent.RunEventDefinition;
				});
		case EWacomMapNodeType::Shop:
			return FindActor<AWacomShopTriggerActor>(World,
				[&Node](const AWacomShopTriggerActor& Actor)
				{
					return Actor.ShopDefinition ==
						Node.Content.Shop.ShopDefinition;
				});
		case EWacomMapNodeType::Treasure:
			if (Node.Content.Treasure.PickupDefinition)
			{
				return FindActor<AWacomRunRewardPickupActor>(World,
					[&Node](const AWacomRunRewardPickupActor& Actor)
					{
						return Actor.PickupDefinition ==
							Node.Content.Treasure.PickupDefinition;
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

	AActor* SpawnContentHost(
		UWorld& World,
		const FWacomMapNodeDefinition& Node,
		const FTransform& Transform)
	{
		AActor* Host = nullptr;
		switch (Node.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			if (ABattleTriggerActor* Actor = Cast<ABattleTriggerActor>(
				World.SpawnActor<AActor>(ABattleTriggerActor::StaticClass(), Transform)))
			{
				Actor->EncounterDefinition = Node.Content.Encounter.EncounterDefinition;
				Host = Actor;
			}
			break;
		case EWacomMapNodeType::RunEvent:
			if (AWacomRunEventTriggerActor* Actor = Cast<AWacomRunEventTriggerActor>(
				World.SpawnActor<AActor>(AWacomRunEventTriggerActor::StaticClass(), Transform)))
			{
				Actor->EventDefinition = Node.Content.RunEvent.RunEventDefinition;
				Host = Actor;
			}
			break;
		case EWacomMapNodeType::Shop:
			if (AWacomShopTriggerActor* Actor = Cast<AWacomShopTriggerActor>(
				World.SpawnActor<AActor>(AWacomShopTriggerActor::StaticClass(), Transform)))
			{
				Actor->ShopDefinition = Node.Content.Shop.ShopDefinition;
				Host = Actor;
			}
			break;
		case EWacomMapNodeType::Treasure:
			if (Node.Content.Treasure.PickupDefinition)
			{
				if (AWacomRunRewardPickupActor* Actor =
					Cast<AWacomRunRewardPickupActor>(World.SpawnActor<AActor>(
						AWacomRunRewardPickupActor::StaticClass(), Transform)))
				{
					Actor->PickupDefinition = Node.Content.Treasure.PickupDefinition;
					Host = Actor;
				}
			}
			else if (AWacomRunKeyChestActor* Actor = Cast<AWacomRunKeyChestActor>(
				World.SpawnActor<AActor>(AWacomRunKeyChestActor::StaticClass(), Transform)))
			{
				Actor->CardInteractionDefinition =
					Node.Content.Treasure.WorldCardInteractionDefinition;
				Host = Actor;
			}
			break;
		default:
			break;
		}
		if (Host)
		{
			Host->Tags.AddUnique(DebugGeneratedTag);
			Host->SetActorLabel(FString::Printf(
				TEXT("RunHost_%s"), *Node.NodeId.ToString()));
		}
		return Host;
	}

	bool BindContentHost(
		AActor& Host,
		const FWacomMapNodeDefinition& Node)
	{
		UWacomRunMapNodeBindingComponent* Binding =
			Host.FindComponentByClass<UWacomRunMapNodeBindingComponent>();
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

	AWacomRunFloorSceneDescriptorActor* EnsureUniqueDescriptor(
		UWorld& World,
		UWacomFloorMapDefinition& Floor)
	{
		TArray<AWacomRunFloorSceneDescriptorActor*> Descriptors;
		for (TActorIterator<AWacomRunFloorSceneDescriptorActor> It(&World); It; ++It)
		{
			Descriptors.Add(*It);
		}
		AWacomRunFloorSceneDescriptorActor* Descriptor =
			Descriptors.IsEmpty() ? nullptr : Descriptors[0];
		for (int32 Index = 1; Index < Descriptors.Num(); ++Index)
		{
			World.DestroyActor(Descriptors[Index]);
		}
		if (!Descriptor)
		{
			Descriptor = World.SpawnActor<AWacomRunFloorSceneDescriptorActor>();
		}
		if (Descriptor)
		{
			Descriptor->Modify();
			Descriptor->FloorDefinition = &Floor;
			Descriptor->SetActorLabel(TEXT("RunFloorSceneDescriptor"));
		}
		return Descriptor;
	}

	bool ConfigureDebugWorld(
		UWorld& World,
		UWacomFloorMapDefinition& Floor,
		UClass& DebugGameModeClass,
		const FSharedBlueprintClasses& SharedClasses)
	{
		TMap<FName, FTransform> ExistingAnchorTransforms;
		TMap<FName, FExistingPathPresentation> ExistingPaths;
		TMap<FName, FTransform> ExistingBranchTransforms;
		TArray<AActor*> GeneratedActors;
		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			AActor* Actor = *It;
			if (!Actor->ActorHasTag(DebugGeneratedTag))
			{
				continue;
			}
			GeneratedActors.Add(Actor);
			if (const AWacomRunMapNodeAnchorActor* Anchor =
				Cast<AWacomRunMapNodeAnchorActor>(Actor))
			{
				ExistingAnchorTransforms.Add(Anchor->NodeId, Anchor->GetActorTransform());
			}
			else if (const AWacomRunPathSegmentActor* Path =
				Cast<AWacomRunPathSegmentActor>(Actor))
			{
				FExistingPathPresentation& Existing = ExistingPaths.Add(Path->EdgeId);
				Existing.ActorTransform = Path->GetActorTransform();
				if (const USplineComponent* Spline = Path->GetPathSpline())
				{
					for (int32 Point = 0; Point < Spline->GetNumberOfSplinePoints(); ++Point)
					{
						Existing.SplinePoints.Add(Spline->GetLocationAtSplinePoint(
							Point, ESplineCoordinateSpace::World));
					}
				}
			}
			else if (const AWacomRunPathBranchTargetActor* Branch =
				Cast<AWacomRunPathBranchTargetActor>(Actor))
			{
				ExistingBranchTransforms.Add(Branch->EdgeId, Branch->GetActorTransform());
			}
		}
		for (AActor* Actor : GeneratedActors)
		{
			World.DestroyActor(Actor);
		}

		if (!EnsureUniqueDescriptor(World, Floor))
		{
			return false;
		}
		AWorldSettings* WorldSettings = World.GetWorldSettings();
		if (!WorldSettings)
		{
			return false;
		}
		WorldSettings->Modify();
		WorldSettings->DefaultGameMode = &DebugGameModeClass;

		TMap<FName, AWacomRunMapNodeAnchorActor*> Anchors;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			const FTransform Transform = ExistingAnchorTransforms.Contains(Node.NodeId)
				? ExistingAnchorTransforms.FindChecked(Node.NodeId)
				: MakeFallbackNodeTransform(Node);
			AWacomRunMapNodeAnchorActor* Anchor = Cast<AWacomRunMapNodeAnchorActor>(
				World.SpawnActor<AActor>(SharedClasses.AnchorClass, Transform));
			if (!Anchor)
			{
				return false;
			}
			Anchor->NodeId = Node.NodeId;
			Anchor->Tags.AddUnique(DebugGeneratedTag);
			Anchor->SetActorLabel(FString::Printf(
				TEXT("RunNode_%s"), *Node.NodeId.ToString()));
			Anchors.Add(Node.NodeId, Anchor);
		}

		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			if (Node.NodeType == EWacomMapNodeType::Navigation)
			{
				continue;
			}
			AActor* Host = FindContentHost(World, Node);
			if (!Host)
			{
				const AWacomRunMapNodeAnchorActor* Anchor = Anchors.FindRef(Node.NodeId);
				Host = SpawnContentHost(World, Node,
					Anchor ? Anchor->GetActorTransform() : MakeFallbackNodeTransform(Node));
			}
			if (!Host || !BindContentHost(*Host, Node))
			{
				UE_LOG(LogTemp, Error,
					TEXT("[RunExplorationDebugAssetBuilder] Failed to bind Debug host for NodeId=%s"),
					*Node.NodeId.ToString());
				return false;
			}
		}

		TMap<FName, int32> OutgoingEdgeCounts;
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			++OutgoingEdgeCounts.FindOrAdd(Edge.FromNodeId);
		}
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			AWacomRunMapNodeAnchorActor* SourceAnchor = Anchors.FindRef(Edge.FromNodeId);
			AWacomRunMapNodeAnchorActor* TargetAnchor = Anchors.FindRef(Edge.ToNodeId);
			if (!SourceAnchor || !TargetAnchor)
			{
				return false;
			}
			const FExistingPathPresentation* Existing = ExistingPaths.Find(Edge.EdgeId);
			const FTransform PathTransform = Existing
				? Existing->ActorTransform
				: FTransform::Identity;
			AWacomRunPathSegmentActor* Path = Cast<AWacomRunPathSegmentActor>(
				World.SpawnActor<AActor>(SharedClasses.PathClass, PathTransform));
			if (!Path || !Path->GetPathSpline())
			{
				return false;
			}
			Path->EdgeId = Edge.EdgeId;
			Path->Tags.AddUnique(DebugGeneratedTag);
			Path->SetActorLabel(FString::Printf(
				TEXT("RunPath_%s"), *Edge.EdgeId.ToString()));
			USplineComponent* Spline = Path->GetPathSpline();
			Spline->ClearSplinePoints(false);
			const bool bReuseSpline = Existing && Existing->SplinePoints.Num() >= 2;
			const TArray<FVector> FallbackPoints =
			{
				SourceAnchor->GetActorLocation(), TargetAnchor->GetActorLocation(),
			};
			const TArray<FVector>& Points = bReuseSpline
				? Existing->SplinePoints
				: FallbackPoints;
			for (const FVector& Point : Points)
			{
				Spline->AddSplinePoint(Point, ESplineCoordinateSpace::World, false);
			}
			for (int32 PointIndex = 0; PointIndex < Points.Num(); ++PointIndex)
			{
				Spline->SetSplinePointType(
					PointIndex, ESplinePointType::Curve, false);
			}
			Spline->UpdateSpline();

			if (OutgoingEdgeCounts.FindRef(Edge.FromNodeId) < 2)
			{
				continue;
			}
			FTransform BranchTransform;
			if (const FTransform* ExistingTransform =
				ExistingBranchTransforms.Find(Edge.EdgeId))
			{
				BranchTransform = *ExistingTransform;
			}
			else
			{
				const FVector InitialDirection = Spline->GetDirectionAtSplinePoint(
					0, ESplineCoordinateSpace::World).GetSafeNormal();
				BranchTransform = SourceAnchor->GetActorTransform();
				BranchTransform.SetLocation(
					SourceAnchor->GetActorLocation() + InitialDirection * 160.0f);
				BranchTransform.SetRotation(InitialDirection.Rotation().Quaternion());
			}
			AWacomRunPathBranchTargetActor* Branch =
				Cast<AWacomRunPathBranchTargetActor>(World.SpawnActor<AActor>(
					SharedClasses.BranchClass, BranchTransform));
			if (!Branch)
			{
				return false;
			}
			Branch->EdgeId = Edge.EdgeId;
			Branch->Tags.AddUnique(DebugGeneratedTag);
			Branch->SetActorLabel(FString::Printf(
				TEXT("RunBranch_%s"), *Edge.EdgeId.ToString()));
		}
		return true;
	}

	UBlueprint* ConfigureDebugGameMode(UWacomJourneyDefinition& Journey)
	{
		UPackage* Package = FindOrCreatePackage(DebugGameModePackagePath);
		if (!Package)
		{
			return nullptr;
		}
		UBlueprint* Blueprint = LoadObject<UBlueprint>(
			nullptr, *MakeObjectPath(DebugGameModePackagePath));
		if (Blueprint && (!Blueprint->GeneratedClass
			|| !Blueprint->GeneratedClass->IsChildOf(AWacomGameMode::StaticClass())))
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Debug GameMode has wrong parent: %s"),
				DebugGameModePackagePath);
			return nullptr;
		}
		if (!Blueprint)
		{
			Blueprint = FKismetEditorUtilities::CreateBlueprint(
				AWacomGameMode::StaticClass(), Package, DebugGameModeAssetName,
				BPTYPE_Normal, UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
		}
		AWacomGameMode* GameModeCDO = Blueprint && Blueprint->GeneratedClass
			? Cast<AWacomGameMode>(Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Blueprint || !GameModeCDO)
		{
			return nullptr;
		}
		Blueprint->Modify();
		GameModeCDO->Modify();
		GameModeCDO->DefaultJourneyDefinition = &Journey;
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder] Debug GameMode compile failed"));
			return nullptr;
		}
		return Blueprint;
	}

	bool SaveBlueprintAsset(UBlueprint& Blueprint, const FString& PackagePath)
	{
		UPackage* Package = Blueprint.GetOutermost();
		if (!Package)
		{
			return false;
		}
		FAssetRegistryModule::AssetCreated(&Blueprint);
		Package->MarkPackageDirty();
		Blueprint.MarkPackageDirty();
		const FString Filename = FPackageName::LongPackageNameToFilename(
			PackagePath, FPackageName::GetAssetPackageExtension());
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Filename), true);
		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.SaveFlags = SAVE_NoError;
		return UPackage::SavePackage(
			Package, &Blueprint, *Filename, Args);
	}

	void LogSceneDiagnostic(
		const FWacomRunSceneBindingDiagnostic& Diagnostic)
	{
		if (Diagnostic.Severity ==
			EWacomRunSceneBindingDiagnosticSeverity::Error)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[RunExplorationDebugAssetBuilder][%s][%s] %s: %s"),
				LexToString(Diagnostic.Severity), LexToString(Diagnostic.Code),
				*Diagnostic.ObjectPath, *Diagnostic.Message.ToString());
		}
		else if (Diagnostic.Severity ==
			EWacomRunSceneBindingDiagnosticSeverity::Warning)
		{
			UE_LOG(LogTemp, Warning,
				TEXT("[RunExplorationDebugAssetBuilder][%s][%s] %s: %s"),
				LexToString(Diagnostic.Severity), LexToString(Diagnostic.Code),
				*Diagnostic.ObjectPath, *Diagnostic.Message.ToString());
		}
	}
}

namespace Wacom::ContentBuilder
{
	FRunExplorationDebugAssetBuildResult BuildRunExplorationDebugAssets()
	{
		return BuildRunExplorationDebugAssets(
			FRunExplorationDebugSharedDependencies{});
	}

	FRunExplorationDebugAssetBuildResult BuildRunExplorationDebugAssets(
		const FRunExplorationDebugSharedDependencies& Dependencies)
	{
		FRunExplorationDebugAssetBuildResult Result;
		FSharedBlueprintClasses SharedClasses;
		Result.bSharedDependenciesValid =
			ValidateSharedDependencies(Dependencies, SharedClasses);
		if (!Result.bSharedDependenciesValid)
		{
			return Result;
		}

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
		if (!Character || !Encounter || !EventSnake || !EventFlag
			|| !Shop || !Pickup || !Chest)
		{
			return Result;
		}

		const FString FloorPackagePath =
			MakePackagePath(MapDataRoot(), FloorAssetName);
		UPackage* FloorPackage = FindOrCreatePackage(FloorPackagePath);
		Result.DebugFloor = FloorPackage
			? CreateOrReplaceAsset<UWacomFloorMapDefinition>(
				FloorPackage, FloorAssetName)
			: nullptr;
		if (!Result.DebugFloor)
		{
			return Result;
		}
		Result.DebugFloor->FloorId = TEXT("Floor.Debug.01");
		Result.DebugFloor->DisplayName = FText::FromString(TEXT("蛇巢浅层"));
		Result.DebugFloor->EntryNodeId = TEXT("Entry");

		FWacomMapNodeDefinition Entry = MakeNode(
			TEXT("Entry"), EWacomMapNodeType::Navigation,
			FVector2D(960.f, 980.f), true);
		Entry.DisplayName = FText::FromString(TEXT("林地入口"));
		Entry.ShortDescription = FText::FromString(
			TEXT("进入蛇巢浅层的安全落脚点。"));
		FWacomMapNodeDefinition Battle = MakeNode(
			TEXT("Battle.Snake"), EWacomMapNodeType::Encounter,
			FVector2D(960.f, 840.f));
		Battle.DisplayName = FText::FromString(TEXT("伏蛇草径"));
		Battle.ShortDescription = FText::FromString(
			TEXT("草丛中传来持续的窸窣声。"));
		Battle.Content.Encounter.EncounterDefinition = Encounter;
		FWacomMapNodeDefinition ShopNode = MakeNode(
			TEXT("Shop.Snake"), EWacomMapNodeType::Shop,
			FVector2D(960.f, 700.f));
		ShopNode.DisplayName = FText::FromString(TEXT("行商营帐"));
		ShopNode.ShortDescription = FText::FromString(
			TEXT("一处临时搭起的补给点。"));
		ShopNode.Content.Shop.ShopDefinition = Shop;
		FWacomMapNodeDefinition EventSnakeNode = MakeNode(
			TEXT("Event.SnakeGift"), EWacomMapNodeType::RunEvent,
			FVector2D(960.f, 560.f));
		EventSnakeNode.DisplayName = FText::FromString(TEXT("蛇蜕空地"));
		EventSnakeNode.ShortDescription = FText::FromString(
			TEXT("残留的蛇蜕似乎掩盖着什么。"));
		EventSnakeNode.Content.RunEvent.RunEventDefinition = EventSnake;
		FWacomMapNodeDefinition Junction = MakeNode(
			TEXT("Junction"), EWacomMapNodeType::Navigation,
			FVector2D(960.f, 420.f), true);
		Junction.DisplayName = FText::FromString(TEXT("三岔旧路"));
		Junction.ShortDescription = FText::FromString(
			TEXT("道路在这里分向三处。"));
		FWacomMapNodeDefinition PickupNode = MakeNode(
			TEXT("Treasure.PoisonFang"), EWacomMapNodeType::Treasure,
			FVector2D(650.f, 260.f), true);
		PickupNode.DisplayName = FText::FromString(TEXT("毒牙遗落处"));
		PickupNode.ShortDescription = FText::FromString(
			TEXT("一枚仍带毒性的断牙落在泥里。"));
		PickupNode.Content.Treasure.PickupDefinition = Pickup;
		FWacomMapNodeDefinition ChestNode = MakeNode(
			TEXT("Treasure.KeyChest"), EWacomMapNodeType::Treasure,
			FVector2D(1270.f, 260.f), true);
		ChestNode.DisplayName = FText::FromString(TEXT("封锁宝箱"));
		ChestNode.ShortDescription = FText::FromString(
			TEXT("沉重的箱锁需要合适的钥匙。"));
		ChestNode.Content.Treasure.WorldCardInteractionDefinition = Chest;
		FWacomMapNodeDefinition EventFlagNode = MakeNode(
			TEXT("Event.FlagReward"), EWacomMapNodeType::RunEvent,
			FVector2D(960.f, 120.f), true);
		EventFlagNode.DisplayName = FText::FromString(TEXT("褪色路标"));
		EventFlagNode.ShortDescription = FText::FromString(
			TEXT("一面褪色标记指向更深的林地。"));
		EventFlagNode.Content.RunEvent.RunEventDefinition = EventFlag;
		Result.DebugFloor->Nodes =
		{
			Entry, Battle, ShopNode, EventSnakeNode, Junction,
			PickupNode, ChestNode, EventFlagNode,
		};
		Result.DebugFloor->Edges =
		{
			MakeEdge(TEXT("EntryToBattle"), TEXT("Entry"), TEXT("Battle.Snake")),
			MakeEdge(TEXT("BattleToShop"), TEXT("Battle.Snake"), TEXT("Shop.Snake")),
			MakeEdge(TEXT("ShopToEventSnake"), TEXT("Shop.Snake"), TEXT("Event.SnakeGift")),
			MakeEdge(TEXT("EventSnakeToJunction"), TEXT("Event.SnakeGift"), TEXT("Junction")),
			MakeEdge(TEXT("JunctionToPickup"), TEXT("Junction"), TEXT("Treasure.PoisonFang")),
			MakeEdge(TEXT("JunctionToChest"), TEXT("Junction"), TEXT("Treasure.KeyChest")),
			MakeEdge(TEXT("JunctionToEventFlag"), TEXT("Junction"), TEXT("Event.FlagReward")),
		};

		const FString JourneyPackagePath =
			MakePackagePath(MapDataRoot(), JourneyAssetName);
		UPackage* JourneyPackage = FindOrCreatePackage(JourneyPackagePath);
		Result.DebugJourney = JourneyPackage
			? CreateOrReplaceAsset<UWacomJourneyDefinition>(
				JourneyPackage, JourneyAssetName)
			: nullptr;
		if (!Result.DebugJourney)
		{
			return Result;
		}
		Result.DebugJourney->JourneyId = TEXT("Journey.Debug");
		Result.DebugJourney->SupportedCharacters = {Character};
		Result.DebugJourney->Floors = {Result.DebugFloor};
		Result.bDataValidationPassed =
			FWacomMapDefinitionValidation::ValidateFloor(Result.DebugFloor).IsValid()
			&& FWacomMapDefinitionValidation::ValidateJourney(
				Result.DebugJourney).IsValid();
		if (!Result.bDataValidationPassed)
		{
			return Result;
		}

		Result.DebugGameMode = ConfigureDebugGameMode(*Result.DebugJourney);
		if (!Result.DebugGameMode || !Result.DebugGameMode->GeneratedClass)
		{
			return Result;
		}

		Result.DebugWorld = FPackageName::DoesPackageExist(DebugMapPackagePath)
			? UEditorLoadingAndSavingUtils::LoadMap(DebugMapPackagePath)
			: UEditorLoadingAndSavingUtils::NewBlankMap(false);
		if (!Result.DebugWorld
			|| !ConfigureDebugWorld(*Result.DebugWorld, *Result.DebugFloor,
				*Result.DebugGameMode->GeneratedClass.Get(), SharedClasses))
		{
			return Result;
		}

		const FWacomRunSceneBindingValidationReport SceneReport =
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(Result.DebugWorld);
		for (const FWacomRunSceneBindingDiagnostic& Diagnostic :
			SceneReport.Diagnostics)
		{
			LogSceneDiagnostic(Diagnostic);
		}
		Result.bSceneValidationPassed = SceneReport.IsValid();
		if (!Result.bSceneValidationPassed)
		{
			return Result;
		}

		Result.bOwnedAssetsSaved =
			SaveAssetPackage(FloorPackage, Result.DebugFloor, FloorPackagePath)
			&& SaveAssetPackage(
				JourneyPackage, Result.DebugJourney, JourneyPackagePath)
			&& SaveBlueprintAsset(
				*Result.DebugGameMode, DebugGameModePackagePath)
			&& UEditorLoadingAndSavingUtils::SaveMap(
				Result.DebugWorld, DebugMapPackagePath);
		return Result;
	}
}
