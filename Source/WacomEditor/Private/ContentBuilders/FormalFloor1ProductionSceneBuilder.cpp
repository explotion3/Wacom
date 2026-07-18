// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/FormalFloor1ProductionSceneBuilder.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Actors/WacomRunRewardPickupActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SplineComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Dom/JsonObject.h"
#include "Encounters/EncounterDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "FileHelpers.h"
#include "HAL/FileManager.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "EdGraph/EdGraph.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Misc/DateTime.h"
#include "Misc/FileHelper.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PaperFlipbook.h"
#include "Pickups/RunPickupDefinition.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Shops/ShopDefinition.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"
#include "Validation/WacomMapDefinitionValidation.h"
#include "Validation/WacomRunSceneBindingValidation.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	constexpr int32 ReportSchemaVersion = 1;
	const FString FloorPackage =
		TEXT("/Game/Wacom/Data/Map/Production/DA_Floor_Main_01");
	const FString BrushSnakeHostPackage =
		TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox");
	const FString MoltGuardHostPackage =
		TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox");
	const FString RootStalkerHostPackage =
		TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox");
	const FString GuardianHostPackage =
		TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox");
	const FString FloorEntranceMarkerPackage =
		TEXT("/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox");
	const FString WorldPackage =
		TEXT("/Game/Wacom/Maps/Run/L_Run_Floor_Main_01");
	const FString PlaceholderIdlePackage =
		TEXT("/Game/Wacom/Art/Placeholders/Enemies/Snake/Flipbooks/PF_Enemy_SnakePlaceholder_Idle");
	const FString ImpactStylePackage =
		TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartImpactStyle_Pixel");
	const FString TargetPreviewStylePackage =
		TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartTargetPreviewStyle_PixelLock");
	const FString AnchorBlueprintPackage =
		TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunMapNodeAnchorActor");
	const FString PathBlueprintPackage =
		TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathSegmentActor");
	const FString BranchBlueprintPackage =
		TEXT("/Game/Wacom/Run/Path/Blueprints/BP_WacomRunPathBranchTargetActor");
	const FString BattleTriggerBlueprintPackage =
		TEXT("/Game/Wacom/Maps/SceneActor/BP_BattleTriggerActor");
	const FString EventTriggerBlueprintPackage =
		TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomRunEventTriggerActor");
	const FString ShopTriggerBlueprintPackage =
		TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomShopTriggerActor");
	const FString RewardPickupBlueprintPackage =
		TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomRunRewardPickupActor");
	const FName ProductionFloorId(TEXT("Floor.Main.01"));

	struct FEnemyHostSeed
	{
		const TCHAR* PackagePath;
		const TCHAR* EnemyPackagePath;
		const TCHAR* Archetype;
		int32 ExpectedParts;
		float VisualScale;
		FLinearColor Tint;
	};

	const TArray<FEnemyHostSeed>& EnemyHostSeeds()
	{
		static const TArray<FEnemyHostSeed> Seeds =
		{
			{TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox"),
				TEXT("/Game/Wacom/Data/Enemies/SerpentWood/BrushSnake/DA_Enemy_BrushSnake"),
				TEXT("BrushSnake"), 2, 1.00f, FLinearColor(0.72f, 0.95f, 0.68f, 1.0f)},
			{TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox"),
				TEXT("/Game/Wacom/Data/Enemies/SerpentWood/MoltGuard/DA_Enemy_MoltGuard"),
				TEXT("MoltGuard"), 3, 1.20f, FLinearColor(0.66f, 0.82f, 1.0f, 1.0f)},
			{TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox"),
				TEXT("/Game/Wacom/Data/Enemies/SerpentWood/RootStalker/DA_Enemy_RootStalker"),
				TEXT("RootStalker"), 2, 1.12f, FLinearColor(0.72f, 0.63f, 0.43f, 1.0f)},
			{TEXT("/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox"),
				TEXT("/Game/Wacom/Data/Enemies/SerpentWood/ShallowGuardian/DA_Enemy_ShallowGuardian"),
				TEXT("ShallowGuardian"), 4, 1.60f, FLinearColor(0.95f, 0.72f, 0.30f, 1.0f)},
		};
		return Seeds;
	}

	struct FWorldNodeSeed
	{
		const TCHAR* NodeId;
		FVector Location;
	};

	const TArray<FWorldNodeSeed>& WorldNodeSeeds()
	{
		static const TArray<FWorldNodeSeed> Seeds =
		{
			{TEXT("Node.Entry"), {0.0, 0.0, 100.0}},
			{TEXT("Node.Main.01"), {1200.0, 0.0, 100.0}},
			{TEXT("Node.Junction.01"), {2400.0, 0.0, 100.0}},
			{TEXT("Node.Route.A.01"), {3600.0, -1800.0, 100.0}},
			{TEXT("Node.Route.A.02"), {4800.0, -2600.0, 100.0}},
			{TEXT("Node.Route.A.03"), {6000.0, -1800.0, 100.0}},
			{TEXT("Node.Route.B.01"), {3600.0, 1800.0, 100.0}},
			{TEXT("Node.Route.B.02"), {4800.0, 2600.0, 100.0}},
			{TEXT("Node.Route.B.03"), {6000.0, 1800.0, 100.0}},
			{TEXT("Node.Junction.02"), {7200.0, 0.0, 100.0}},
			{TEXT("Node.Route.C.01"), {8400.0, -1800.0, 100.0}},
			{TEXT("Node.Route.C.02"), {9600.0, -1200.0, 100.0}},
			{TEXT("Node.Route.D.01"), {8400.0, 1800.0, 100.0}},
			{TEXT("Node.Route.D.02"), {9600.0, 2600.0, 100.0}},
			{TEXT("Node.Route.D.03"), {10800.0, 1800.0, 100.0}},
			{TEXT("Node.Key.01"), {12000.0, 0.0, 100.0}},
			{TEXT("Node.Junction.03"), {13200.0, 0.0, 100.0}},
			{TEXT("Node.Main.02"), {14400.0, 0.0, 100.0}},
			{TEXT("Node.Guardian.01"), {15600.0, 0.0, 100.0}},
			{TEXT("Node.Exit.01"), {16800.0, 0.0, 100.0}},
		};
		return Seeds;
	}

	struct FNodeSeed
	{
		const TCHAR* NodeId;
		const TCHAR* DisplayName;
		EWacomMapNodeType NodeType;
		FVector2D MapPosition;
		bool bAllowsCamp = false;
		const TCHAR* PayloadPackage = nullptr;
		bool bBoss = false;
	};

	struct FEdgeSeed
	{
		const TCHAR* EdgeId;
		const TCHAR* FromNodeId;
		const TCHAR* ToNodeId;
	};

	const TArray<FNodeSeed>& NodeSeeds()
	{
		static const TArray<FNodeSeed> Seeds =
		{
			{TEXT("Node.Entry"), TEXT("林地入口"), EWacomMapNodeType::Navigation, {960.0, 1050.0}, true},
			{TEXT("Node.Main.01"), TEXT("教学伏击"), EWacomMapNodeType::Encounter, {960.0, 990.0}, false, TEXT("/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_Scout")},
			{TEXT("Node.Junction.01"), TEXT("第一处分岔"), EWacomMapNodeType::Navigation, {960.0, 930.0}, true},
			{TEXT("Node.Route.A.01"), TEXT("蛇蜕事件"), EWacomMapNodeType::RunEvent, {650.0, 860.0}, false, TEXT("/Game/Wacom/Data/Events/SerpentWood/DA_Event_CastSkin")},
			{TEXT("Node.Route.A.02"), TEXT("蛇蜕守卫"), EWacomMapNodeType::Encounter, {520.0, 790.0}, false, TEXT("/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_MoltGuard")},
			{TEXT("Node.Route.A.03"), TEXT("草药补给"), EWacomMapNodeType::Treasure, {650.0, 720.0}, false, TEXT("/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_HerbCache")},
			{TEXT("Node.Route.B.01"), TEXT("毒雾伏击"), EWacomMapNodeType::Encounter, {1270.0, 860.0}, false, TEXT("/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_Ambush")},
			{TEXT("Node.Route.B.02"), TEXT("猎人遗物"), EWacomMapNodeType::Treasure, {1400.0, 790.0}, false, TEXT("/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_HunterCache")},
			{TEXT("Node.Route.B.03"), TEXT("猎人痕迹"), EWacomMapNodeType::RunEvent, {1270.0, 720.0}, false, TEXT("/Game/Wacom/Data/Events/SerpentWood/DA_Event_HunterTrace")},
			{TEXT("Node.Junction.02"), TEXT("第一轮汇合"), EWacomMapNodeType::Navigation, {960.0, 650.0}, true},
			{TEXT("Node.Route.C.01"), TEXT("林下行商"), EWacomMapNodeType::Shop, {650.0, 570.0}, false, TEXT("/Game/Wacom/Data/Shops/SerpentWood/DA_Shop_Wayfarer")},
			{TEXT("Node.Route.C.02"), TEXT("行商情报"), EWacomMapNodeType::RunEvent, {720.0, 450.0}, false, TEXT("/Game/Wacom/Data/Events/SerpentWood/DA_Event_MerchantRumor")},
			{TEXT("Node.Route.D.01"), TEXT("盘根伏蛇"), EWacomMapNodeType::Encounter, {1270.0, 590.0}, false, TEXT("/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_RootStalker")},
			{TEXT("Node.Route.D.02"), TEXT("毒沼抉择"), EWacomMapNodeType::RunEvent, {1400.0, 510.0}, false, TEXT("/Game/Wacom/Data/Events/SerpentWood/DA_Event_PoisonMarsh")},
			{TEXT("Node.Route.D.03"), TEXT("蜕壳密藏"), EWacomMapNodeType::Treasure, {1270.0, 430.0}, false, TEXT("/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_MoltCache")},
			{TEXT("Node.Key.01"), TEXT("必经蛇印"), EWacomMapNodeType::Treasure, {960.0, 350.0}, false, TEXT("/Game/Wacom/Data/Pickups/SerpentWood/DA_Pickup_SerpentSigil")},
			{TEXT("Node.Junction.03"), TEXT("巢门前哨"), EWacomMapNodeType::Navigation, {960.0, 270.0}, true},
			{TEXT("Node.Main.02"), TEXT("精英巡猎者"), EWacomMapNodeType::Encounter, {960.0, 200.0}, false, TEXT("/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_EliteSentinel")},
			{TEXT("Node.Guardian.01"), TEXT("浅巢守卫"), EWacomMapNodeType::Encounter, {960.0, 130.0}, false, TEXT("/Game/Wacom/Data/Encounters/SerpentWood/DA_Encounter_ShallowGuardian"), true},
			{TEXT("Node.Exit.01"), TEXT("洞窟入口"), EWacomMapNodeType::FloorEntrance, {960.0, 60.0}},
		};
		return Seeds;
	}

	const TArray<FEdgeSeed>& EdgeSeeds()
	{
		static const TArray<FEdgeSeed> Seeds =
		{
			{TEXT("Edge.Main.01"), TEXT("Node.Entry"), TEXT("Node.Main.01")},
			{TEXT("Edge.Main.02"), TEXT("Node.Main.01"), TEXT("Node.Junction.01")},
			{TEXT("Edge.Route.A.01"), TEXT("Node.Junction.01"), TEXT("Node.Route.A.01")},
			{TEXT("Edge.Route.A.02"), TEXT("Node.Route.A.01"), TEXT("Node.Route.A.02")},
			{TEXT("Edge.Route.A.03"), TEXT("Node.Route.A.02"), TEXT("Node.Route.A.03")},
			{TEXT("Edge.Route.A.04"), TEXT("Node.Route.A.03"), TEXT("Node.Junction.02")},
			{TEXT("Edge.Route.B.01"), TEXT("Node.Junction.01"), TEXT("Node.Route.B.01")},
			{TEXT("Edge.Route.B.02"), TEXT("Node.Route.B.01"), TEXT("Node.Route.B.02")},
			{TEXT("Edge.Route.B.03"), TEXT("Node.Route.B.02"), TEXT("Node.Route.B.03")},
			{TEXT("Edge.Route.B.04"), TEXT("Node.Route.B.03"), TEXT("Node.Junction.02")},
			{TEXT("Edge.Route.C.01"), TEXT("Node.Junction.02"), TEXT("Node.Route.C.01")},
			{TEXT("Edge.Route.C.02"), TEXT("Node.Route.C.01"), TEXT("Node.Route.C.02")},
			{TEXT("Edge.Route.C.03"), TEXT("Node.Route.C.02"), TEXT("Node.Key.01")},
			{TEXT("Edge.Route.D.01"), TEXT("Node.Junction.02"), TEXT("Node.Route.D.01")},
			{TEXT("Edge.Route.D.02"), TEXT("Node.Route.D.01"), TEXT("Node.Route.D.02")},
			{TEXT("Edge.Route.D.03"), TEXT("Node.Route.D.02"), TEXT("Node.Route.D.03")},
			{TEXT("Edge.Route.D.04"), TEXT("Node.Route.D.03"), TEXT("Node.Key.01")},
			{TEXT("Edge.Main.03"), TEXT("Node.Key.01"), TEXT("Node.Junction.03")},
			{TEXT("Edge.Main.04"), TEXT("Node.Junction.03"), TEXT("Node.Main.02")},
			{TEXT("Edge.Main.05"), TEXT("Node.Main.02"), TEXT("Node.Guardian.01")},
			{TEXT("Edge.Main.06"), TEXT("Node.Guardian.01"), TEXT("Node.Exit.01")},
		};
		return Seeds;
	}

	FString ObjectPathForPackage(const FString& PackagePath)
	{
		return PackagePath + TEXT(".")
			+ FPackageName::GetLongPackageAssetName(PackagePath);
	}

	template <typename T>
	T* ResolveRequiredAsset(const TCHAR* PackagePath, TArray<FString>& OutErrors)
	{
		const FString ObjectPath = ObjectPathForPackage(PackagePath);
		T* Asset = LoadObject<T>(nullptr, *ObjectPath);
		if (!Asset)
		{
			OutErrors.Add(FString::Printf(TEXT("Missing required asset: %s"), PackagePath));
		}
		return Asset;
	}

	FWacomMapNodeDefinition MakeNode(
		const FNodeSeed& Seed,
		TArray<FString>& OutErrors)
	{
		FWacomMapNodeDefinition Node;
		Node.NodeId = Seed.NodeId;
		Node.DisplayName = FText::FromString(Seed.DisplayName);
		Node.NodeType = Seed.NodeType;
		Node.MapPosition = Seed.MapPosition;
		Node.bAllowsCamp = Seed.bAllowsCamp;
		switch (Seed.NodeType)
		{
		case EWacomMapNodeType::Encounter:
			Node.Content.Encounter.EncounterDefinition =
				ResolveRequiredAsset<UEncounterDefinition>(Seed.PayloadPackage, OutErrors);
			Node.Content.Encounter.bBoss = Seed.bBoss;
			if (Seed.bBoss)
			{
				Node.LandmarkVisibility = EWacomMapLandmarkVisibility::BossOutline;
			}
			break;
		case EWacomMapNodeType::RunEvent:
			Node.Content.RunEvent.RunEventDefinition =
				ResolveRequiredAsset<UWacomRunEventDefinition>(Seed.PayloadPackage, OutErrors);
			break;
		case EWacomMapNodeType::Shop:
			Node.Content.Shop.ShopDefinition =
				ResolveRequiredAsset<UShopDefinition>(Seed.PayloadPackage, OutErrors);
			break;
		case EWacomMapNodeType::Treasure:
			Node.Content.Treasure.PickupDefinition =
				ResolveRequiredAsset<UWacomRunPickupDefinition>(Seed.PayloadPackage, OutErrors);
			break;
		case EWacomMapNodeType::FloorEntrance:
			Node.Content.FloorEntrance.TargetFloorId = TEXT("Floor.Main.02");
			Node.Content.FloorEntrance.RequiredCredentialIds =
				{TEXT("Credential.Run.SerpentSigil")};
			Node.LandmarkVisibility =
				EWacomMapLandmarkVisibility::FloorEntranceOutline;
			break;
		case EWacomMapNodeType::Navigation:
		default:
			break;
		}
		return Node;
	}

	bool ConfigureExpectedFloor(
		UWacomFloorMapDefinition& Floor,
		TArray<FString>& OutErrors)
	{
		Floor.FloorId = TEXT("Floor.Main.01");
		Floor.DisplayName = FText::FromString(TEXT("蛇巢浅林"));
		Floor.EntryNodeId = TEXT("Node.Entry");
		Floor.Nodes.Reset(NodeSeeds().Num());
		for (const FNodeSeed& Seed : NodeSeeds())
		{
			Floor.Nodes.Add(MakeNode(Seed, OutErrors));
		}
		Floor.Edges.Reset(EdgeSeeds().Num());
		for (const FEdgeSeed& Seed : EdgeSeeds())
		{
			FWacomMapEdgeDefinition& Edge = Floor.Edges.AddDefaulted_GetRef();
			Edge.EdgeId = Seed.EdgeId;
			Edge.FromNodeId = Seed.FromNodeId;
			Edge.ToNodeId = Seed.ToNodeId;
		}
		return OutErrors.IsEmpty();
	}

	TSet<FName> BuildReachable(
		const UWacomFloorMapDefinition& Floor,
		const FName Start,
		const FName Excluded = NAME_None)
	{
		TSet<FName> Visited;
		TArray<FName> Pending;
		if (Start != Excluded)
		{
			Pending.Add(Start);
		}
		while (!Pending.IsEmpty())
		{
			const FName Current = Pending.Pop(EAllowShrinking::No);
			if (Current == Excluded || Visited.Contains(Current))
			{
				continue;
			}
			Visited.Add(Current);
			for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
			{
				if (Edge.FromNodeId == Current
					&& Edge.ToNodeId != Excluded
					&& !Visited.Contains(Edge.ToNodeId))
				{
					Pending.Add(Edge.ToNodeId);
				}
			}
		}
		return Visited;
	}

	void AppendSharedFloorErrors(
		const UWacomFloorMapDefinition& Floor,
		TArray<FString>& OutErrors)
	{
		const FWacomMapDefinitionValidationReport Report =
			FWacomMapDefinitionValidation::ValidateFloor(&Floor);
		for (const FText& Error : Report.Errors)
		{
			OutErrors.Add(Error.ToString());
		}
	}

	bool CompareStrictEditableProperties(
		const UObject& Actual,
		const UObject& Expected,
		TArray<FString>& OutErrors)
	{
		for (TFieldIterator<FProperty> It(Actual.GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Edit)
				|| Property->HasAnyPropertyFlags(
					CPF_Transient | CPF_DuplicateTransient | CPF_NonPIEDuplicateTransient))
			{
				continue;
			}
			if (!Property->Identical_InContainer(&Actual, &Expected, PPF_None))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Seed default mismatch: %s"), *Property->GetName()));
			}
		}
		return OutErrors.IsEmpty();
	}

	const FEnemyHostSeed* FindEnemyHostSeed(const FString& PackagePath)
	{
		return EnemyHostSeeds().FindByPredicate(
			[&PackagePath](const FEnemyHostSeed& Seed)
			{
				return PackagePath == Seed.PackagePath;
			});
	}

	UChildActorComponent* FindPartComponent(
		const AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& Part)
	{
		const UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Host.GetClass());
		if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript)
		{
			return nullptr;
		}
		for (USCS_Node* Node :
			BlueprintClass->SimpleConstructionScript->GetAllNodes())
		{
			UChildActorComponent* Component = Node
				? Cast<UChildActorComponent>(
					Node->GetActualComponentTemplate(
						const_cast<UBlueprintGeneratedClass*>(BlueprintClass)))
				: nullptr;
			if (Component && Component->GetChildActorTemplate() == &Part)
			{
				return Component;
			}
		}
		return nullptr;
	}

	FVector PartLayoutLocation(const int32 PartCount, const int32 Index)
	{
		static const TArray<TArray<FVector>> Layouts =
		{
			{},
			{{0.0, 0.0, 0.0}},
			{{82.0, 0.0, 26.0}, {-62.0, 0.0, -10.0}},
			{{92.0, 0.0, 30.0}, {0.0, 0.0, 0.0}, {-94.0, 0.0, -18.0}},
			{{104.0, 0.0, 34.0}, {0.0, 0.0, 0.0},
				{-108.0, 0.0, -22.0}, {28.0, 0.0, 116.0}},
		};
		return Layouts.IsValidIndex(PartCount)
			&& Layouts[PartCount].IsValidIndex(Index)
			? Layouts[PartCount][Index]
			: FVector(Index * 80.0, 0.0, 0.0);
	}

	bool ValidateEnemyHostBlueprint(
		const UBlueprint& Blueprint,
		const FEnemyHostSeed& Seed,
		TArray<FString>& OutErrors)
	{
		if (!Blueprint.GeneratedClass
			|| !Blueprint.GeneratedClass->IsChildOf(
				AWacomBattleEnemyActor::StaticClass()))
		{
			OutErrors.Add(TEXT("Enemy Host Blueprint parent/GeneratedClass mismatch"));
			return false;
		}
		const AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
			Blueprint.GeneratedClass->GetDefaultObject());
		const UEnemyDefinition* ExpectedEnemy = ResolveRequiredAsset<UEnemyDefinition>(
			Seed.EnemyPackagePath, OutErrors);
		if (!Host || !ExpectedEnemy)
		{
			OutErrors.Add(TEXT("Enemy Host CDO or definition is missing"));
			return false;
		}
		if (Host->EnemyDefinition != ExpectedEnemy)
		{
			OutErrors.Add(TEXT("Enemy Host definition mismatch"));
		}
		if (!Host->EnemySlotId.IsNone())
		{
			OutErrors.Add(TEXT("Enemy Host prefab must not hardcode an Encounter slot"));
		}
		if (Host->HostAuthoringMode
			!= EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual
			|| Host->HostVisualMode != EWacomBattleEnemyHostVisualMode::Flipbook
			|| !Host->HostFlipbook
			|| !Host->HostFlipbook->GetPathName().StartsWith(
				TEXT("/Game/Wacom/Art/Placeholders/")))
		{
			OutErrors.Add(TEXT("Enemy Host must use the controlled placeholder flipbook"));
		}
		if (Host->FindComponentByClass<UWacomRunMapNodeBindingComponent>())
		{
			OutErrors.Add(TEXT("Enemy Host prefab must not carry RunMapNodeBinding"));
		}

		const TArray<AWacomBattleEnemyPartActor*> Parts =
			Host->GetBattleEnemyPartActors();
		if (Parts.Num() != Seed.ExpectedParts
			|| ExpectedEnemy->Parts.Num() != Seed.ExpectedParts)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Enemy Host expected %d parts, got %d (definition %d)"),
				Seed.ExpectedParts, Parts.Num(), ExpectedEnemy->Parts.Num()));
		}
		TSet<FName> SeenSlots;
		for (const AWacomBattleEnemyPartActor* Part : Parts)
		{
			if (!Part || Part->PartSlotId.IsNone()
				|| SeenSlots.Contains(Part->PartSlotId))
			{
				OutErrors.Add(TEXT("Enemy Host contains a null/duplicate part slot"));
				continue;
			}
			SeenSlots.Add(Part->PartSlotId);
			const FEnemyPartSlot* ExpectedPart =
				ExpectedEnemy->Parts.FindByPredicate(
					[Part](const FEnemyPartSlot& Candidate)
					{
						return Candidate.PartSlotId == Part->PartSlotId;
					});
			if (!ExpectedPart || !ExpectedPart->PartDef
				|| Part->PartId != ExpectedPart->PartDef->PartId)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Enemy Host part identity mismatch: %s"),
					*Part->PartSlotId.ToString()));
			}
			if (Part->HitBoundsExtent.GetMin() <= 0.0)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Enemy Host part has invalid hit bounds: %s"),
					*Part->PartSlotId.ToString()));
			}
		}
		const FWacomBattleSceneEnemyHostAuthoringReport AuthoringReport =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		if (!AuthoringReport.bAuthoringReady
			|| AuthoringReport.PartActorCount != Seed.ExpectedParts)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Enemy Host authoring report is %s with %d parts"),
				*AuthoringReport.AuthoringState.ToString(),
				AuthoringReport.PartActorCount));
		}
		return OutErrors.IsEmpty();
	}

	UBlueprint* CreateEnemyHostBlueprint(
		const FEnemyHostSeed& Seed,
		TArray<FString>& OutErrors)
	{
		UPackage* Package = CreatePackage(Seed.PackagePath);
		const FName AssetName(
			*FPackageName::GetLongPackageAssetName(Seed.PackagePath));
		UEnemyDefinition* Enemy = ResolveRequiredAsset<UEnemyDefinition>(
			Seed.EnemyPackagePath, OutErrors);
		UPaperFlipbook* Placeholder = ResolveRequiredAsset<UPaperFlipbook>(
			*PlaceholderIdlePackage, OutErrors);
		if (!Package || !Enemy || !Placeholder)
		{
			return nullptr;
		}
		UBlueprint* Blueprint = FKismetEditorUtilities::CreateBlueprint(
			AWacomBattleEnemyActor::StaticClass(), Package, AssetName,
			BPTYPE_Normal, UBlueprint::StaticClass(),
			UBlueprintGeneratedClass::StaticClass());
		AWacomBattleEnemyActor* Host = Blueprint && Blueprint->GeneratedClass
			? Cast<AWacomBattleEnemyActor>(
				Blueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (!Blueprint || !Host)
		{
			OutErrors.Add(TEXT("Could not create Enemy Host Blueprint/CDO"));
			return nullptr;
		}
		Host->Modify();
		Host->EnemyDefinition = Enemy;
		Host->EnemySlotId = NAME_None;
		Host->HostAuthoringMode =
			EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;
		Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
		Host->HostSprite = nullptr;
		Host->HostFlipbook = Placeholder;
		Host->HostVisualRelativeScale3D = FVector(Seed.VisualScale);
		Host->HostVisualTint = Seed.Tint;
		Host->bHostVisualVisible = true;
		Host->bAutoPlayHostFlipbook = true;
		Host->bLoopHostFlipbook = true;
		Host->DefaultImpactStyle = LoadObject<UWacomBattleEnemyPartImpactStyle>(
			nullptr, *ObjectPathForPackage(ImpactStylePackage));
		Host->DefaultTargetPreviewStyle =
			LoadObject<UWacomBattleEnemyPartTargetPreviewStyle>(
				nullptr, *ObjectPathForPackage(TargetPreviewStylePackage));

		const TArray<AWacomBattleEnemyActor*> HostsToSync = {Host};
		const TArray<FWacomBattleSceneEnemyHostSyncResult> Results =
			FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(
				HostsToSync);
		if (Results.Num() != 1
			|| Results[0].ResultCode == TEXT("ApplyFailed")
			|| Results[0].ResultCode == TEXT("PartiallyApplied"))
		{
			OutErrors.Add(TEXT("Enemy Host initial part synchronization failed"));
			return nullptr;
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("Enemy Host Blueprint compile after sync failed"));
			return nullptr;
		}
		Host = Cast<AWacomBattleEnemyActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
		if (!Host)
		{
			OutErrors.Add(TEXT("Enemy Host CDO did not regenerate"));
			return nullptr;
		}
		const TArray<AWacomBattleEnemyPartActor*> Parts =
			Host->GetBattleEnemyPartActors();
		for (int32 Index = 0; Index < Parts.Num(); ++Index)
		{
			AWacomBattleEnemyPartActor* Part = Parts[Index];
			UChildActorComponent* Component = Part
				? FindPartComponent(*Host, *Part) : nullptr;
			if (!Part || !Component)
			{
				OutErrors.Add(TEXT("Enemy Host part presentation template is missing"));
				return nullptr;
			}
			Part->Modify();
			Component->Modify();
			Part->HitBoundsExtent = FVector(62.0f, 48.0f, 62.0f)
				* Seed.VisualScale;
			Part->VisualLayers.Reset();
			Component->SetRelativeLocation(
				PartLayoutLocation(Parts.Num(), Index) * Seed.VisualScale);
			Part->RefreshAuthoringState();
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error
			|| !ValidateEnemyHostBlueprint(*Blueprint, Seed, OutErrors))
		{
			return nullptr;
		}
		return Blueprint;
	}

	bool SetNameProperty(UObject& Object, const FName PropertyName, const FName Value)
	{
		FNameProperty* Property = FindFProperty<FNameProperty>(
			Object.GetClass(), PropertyName);
		if (!Property)
		{
			return false;
		}
		Object.Modify();
		Property->SetPropertyValue_InContainer(&Object, Value);
		return true;
	}

	bool ValidateFloorEntranceMarkerBlueprint(
		const UBlueprint& Blueprint,
		TArray<FString>& OutErrors)
	{
		if (!Blueprint.GeneratedClass
			|| !Blueprint.GeneratedClass->IsChildOf(AActor::StaticClass()))
		{
			OutErrors.Add(TEXT("FloorEntrance marker Blueprint is invalid"));
			return false;
		}
		const AActor* Marker = Cast<AActor>(
			Blueprint.GeneratedClass->GetDefaultObject());
		const UWacomRunMapNodeBindingComponent* Binding = Marker
			? Marker->FindComponentByClass<UWacomRunMapNodeBindingComponent>()
			: nullptr;
		const UStaticMeshComponent* MarkerMesh = Marker
			? Marker->FindComponentByClass<UStaticMeshComponent>() : nullptr;
		const FNameProperty* PersistentIdProperty = FindFProperty<FNameProperty>(
			Blueprint.GeneratedClass, TEXT("PersistentId"));
		if (!Marker || !Binding || !MarkerMesh || !MarkerMesh->GetStaticMesh()
			|| MarkerMesh->GetCollisionEnabled() != ECollisionEnabled::NoCollision
			|| !PersistentIdProperty)
		{
			OutErrors.Add(TEXT("FloorEntrance marker requires non-colliding mesh, binding, and PersistentId"));
		}
		if (Marker && Marker->FindComponentByClass<UWacomInteractionTargetComponent>())
		{
			OutErrors.Add(TEXT("FloorEntrance marker must remain non-interactive"));
		}
		return OutErrors.IsEmpty();
	}

	UBlueprint* CreateFloorEntranceMarkerBlueprint(TArray<FString>& OutErrors)
	{
		UPackage* Package = CreatePackage(*FloorEntranceMarkerPackage);
		const FName AssetName(
			*FPackageName::GetLongPackageAssetName(FloorEntranceMarkerPackage));
		UBlueprint* Blueprint = Package
			? FKismetEditorUtilities::CreateBlueprint(
				AActor::StaticClass(), Package, AssetName, BPTYPE_Normal,
				UBlueprint::StaticClass(), UBlueprintGeneratedClass::StaticClass())
			: nullptr;
		if (!Blueprint || !Blueprint->SimpleConstructionScript)
		{
			OutErrors.Add(TEXT("Could not create FloorEntrance marker Blueprint"));
			return nullptr;
		}
		USimpleConstructionScript* SCS = Blueprint->SimpleConstructionScript;
		USCS_Node* RootNode = SCS->GetDefaultSceneRootNode();
		if (!RootNode)
		{
			RootNode = SCS->CreateNode(
				USceneComponent::StaticClass(), TEXT("MarkerRoot"));
			if (RootNode)
			{
				SCS->AddNode(RootNode);
			}
		}
		USCS_Node* MeshNode = SCS->CreateNode(
			UStaticMeshComponent::StaticClass(), TEXT("GrayboxMarkerMesh"));
		USCS_Node* BindingNode = SCS->CreateNode(
			UWacomRunMapNodeBindingComponent::StaticClass(),
			TEXT("RunMapNodeBinding"));
		if (!RootNode || !MeshNode || !BindingNode)
		{
			OutErrors.Add(TEXT("Could not create FloorEntrance marker components"));
			return nullptr;
		}
		RootNode->AddChildNode(MeshNode);
		SCS->AddNode(BindingNode);
		UStaticMeshComponent* Mesh = Cast<UStaticMeshComponent>(
			MeshNode->ComponentTemplate);
		if (Mesh)
		{
			Mesh->SetStaticMesh(LoadObject<UStaticMesh>(
				nullptr, TEXT("/Engine/BasicShapes/Cube.Cube")));
			Mesh->SetRelativeLocation(FVector(0.0, 0.0, 160.0));
			Mesh->SetRelativeScale3D(FVector(0.45, 1.4, 3.2));
			Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
		FEdGraphPinType NameType;
		NameType.PinCategory = TEXT("name");
		if (!FBlueprintEditorUtils::AddMemberVariable(
			Blueprint, TEXT("PersistentId"), NameType))
		{
			OutErrors.Add(TEXT("Could not add FloorEntrance marker PersistentId"));
			return nullptr;
		}
		FBlueprintEditorUtils::MarkBlueprintAsStructurallyModified(Blueprint);
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("FloorEntrance marker Blueprint compile failed"));
			return nullptr;
		}
		AActor* Marker = Cast<AActor>(Blueprint->GeneratedClass->GetDefaultObject());
		if (!Marker || !SetNameProperty(*Marker, TEXT("PersistentId"), NAME_None))
		{
			OutErrors.Add(TEXT("FloorEntrance marker PersistentId default is unavailable"));
			return nullptr;
		}
		FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		if (!ValidateFloorEntranceMarkerBlueprint(*Blueprint, OutErrors))
		{
			return nullptr;
		}
		return Blueprint;
	}

	UClass* LoadBlueprintClass(
		const FString& PackagePath,
		const UClass& ExpectedParent,
		TArray<FString>& OutErrors)
	{
		UBlueprint* Blueprint = LoadObject<UBlueprint>(
			nullptr, *ObjectPathForPackage(PackagePath));
		if (!Blueprint || !Blueprint->GeneratedClass
			|| !Blueprint->GeneratedClass->IsChildOf(&ExpectedParent))
		{
			OutErrors.Add(FString::Printf(
				TEXT("Blueprint dependency has wrong/missing parent %s: %s"),
				*ExpectedParent.GetName(), *PackagePath));
			return nullptr;
		}
		return Blueprint->GeneratedClass;
	}

	FVector WorldLocationForNode(const FName NodeId)
	{
		const FWorldNodeSeed* Seed = WorldNodeSeeds().FindByPredicate(
			[NodeId](const FWorldNodeSeed& Candidate)
			{
				return Candidate.NodeId == NodeId;
			});
		return Seed ? Seed->Location : FVector::ZeroVector;
	}

	FVector ArrivalDirectionForNode(
		const UWacomFloorMapDefinition& Floor,
		const FName NodeId)
	{
		const FWacomMapEdgeDefinition* Incoming = Floor.Edges.FindByPredicate(
			[NodeId](const FWacomMapEdgeDefinition& Edge)
			{
				return Edge.ToNodeId == NodeId;
			});
		if (!Incoming)
		{
			return FVector::ForwardVector;
		}
		return (WorldLocationForNode(NodeId)
			- WorldLocationForNode(Incoming->FromNodeId)).GetSafeNormal(
				SMALL_NUMBER, FVector::ForwardVector);
	}

	FName PersistentIdForNode(const FName NodeId)
	{
		return FName(*(ProductionFloorId.ToString()
			+ TEXT(".") + NodeId.ToString()));
	}

	UWacomRunMapNodeBindingComponent* BindContentHost(
		AActor& Host,
		const FWacomMapNodeDefinition& Node)
	{
		UWacomRunMapNodeBindingComponent* Binding =
			Host.FindComponentByClass<UWacomRunMapNodeBindingComponent>();
		if (!Binding)
		{
			Binding = NewObject<UWacomRunMapNodeBindingComponent>(
				&Host, TEXT("RunMapNodeBinding"), RF_Transactional);
			if (!Binding)
			{
				return nullptr;
			}
			Binding->CreationMethod = EComponentCreationMethod::Instance;
			Host.AddInstanceComponent(Binding);
			Binding->OnComponentCreated();
			Binding->RegisterComponent();
		}
		Binding->Modify();
		Binding->NodeId = Node.NodeId;
		Binding->NodeType = Node.NodeType;
		return Binding;
	}

	AWacomFirstPersonViewpointActor* SpawnViewpoint(
		UWorld& World,
		const FString& Label,
		const FVector& Location,
		const FVector& Target)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWacomFirstPersonViewpointActor* Viewpoint =
			World.SpawnActor<AWacomFirstPersonViewpointActor>(
				AWacomFirstPersonViewpointActor::StaticClass(),
				FTransform((Target - Location).Rotation(), Location), Params);
		if (Viewpoint)
		{
			Viewpoint->SetActorLabel(Label);
			Viewpoint->StageBlendTimeSeconds = 0.45f;
		}
		return Viewpoint;
	}

	bool SpawnGrayboxPad(
		UWorld& World,
		const FName NodeId,
		UStaticMesh& Cube)
	{
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AStaticMeshActor* Pad = World.SpawnActor<AStaticMeshActor>(
			AStaticMeshActor::StaticClass(),
			FTransform(FRotator::ZeroRotator,
				WorldLocationForNode(NodeId) - FVector(0.0, 0.0, 100.0)),
			Params);
		if (!Pad || !Pad->GetStaticMeshComponent())
		{
			return false;
		}
		Pad->SetActorLabel(TEXT("GrayboxPad_")
			+ NodeId.ToString().Replace(TEXT("."), TEXT("_")));
		UStaticMeshComponent* Mesh = Pad->GetStaticMeshComponent();
		Mesh->SetStaticMesh(&Cube);
		Mesh->SetRelativeScale3D(FVector(6.0, 5.0, 0.2));
		Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		return true;
	}

	bool ConfigureBattleHost(
		UWorld& World,
		ABattleTriggerActor& Trigger,
		const FWacomMapNodeDefinition& Node,
		const UWacomFloorMapDefinition& Floor,
		const TMap<const UEnemyDefinition*, UClass*>& EnemyHostClasses,
		TArray<FString>& OutErrors)
	{
		Trigger.PersistentId = PersistentIdForNode(Node.NodeId);
		Trigger.EncounterDefinition = Node.Content.Encounter.EncounterDefinition;
		Trigger.TriggerRadius = 220.0f;
		Trigger.SceneEnemyHostSlots.Reset();
		if (!Trigger.EncounterDefinition)
		{
			OutErrors.Add(TEXT("Battle content Host has no EncounterDefinition"));
			return false;
		}
		const FVector Anchor = WorldLocationForNode(Node.NodeId);
		const FVector Arrival = ArrivalDirectionForNode(Floor, Node.NodeId);
		const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Arrival)
			.GetSafeNormal(SMALL_NUMBER, FVector::RightVector);
		const FVector EnemyCenter = Anchor + Arrival * 720.0f;
		const FVector ViewLocation = Anchor - Arrival * 360.0f
			+ FVector(0.0, 0.0, 120.0f);
		Trigger.BattleEntryViewpoint = SpawnViewpoint(
			World, TEXT("View_Battle_")
				+ Node.NodeId.ToString().Replace(TEXT("."), TEXT("_")),
			ViewLocation, EnemyCenter + FVector(0.0, 0.0, 70.0));
		if (!Trigger.BattleEntryViewpoint)
		{
			OutErrors.Add(TEXT("Could not spawn Battle viewpoint"));
			return false;
		}

		const int32 SlotCount = Trigger.EncounterDefinition->EnemySlots.Num();
		for (int32 Index = 0; Index < SlotCount; ++Index)
		{
			const FEncounterEnemySlot& Slot =
				Trigger.EncounterDefinition->EnemySlots[Index];
			UClass* const* HostClass = EnemyHostClasses.Find(Slot.EnemyDefinition);
			if (!HostClass || !*HostClass)
			{
				OutErrors.Add(FString::Printf(
					TEXT("No graybox prefab for Encounter slot %s"),
					*Slot.EnemySlotId.ToString()));
				return false;
			}
			const double LateralOffset = (Index - (SlotCount - 1) * 0.5) * 360.0;
			const FVector HostLocation = EnemyCenter + Lateral * LateralOffset;
			FActorSpawnParameters Params;
			Params.SpawnCollisionHandlingOverride =
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			AWacomBattleEnemyActor* EnemyHost = Cast<AWacomBattleEnemyActor>(
				World.SpawnActor<AActor>(*HostClass,
					FTransform((ViewLocation - HostLocation).Rotation(), HostLocation),
					Params));
			if (!EnemyHost)
			{
				OutErrors.Add(TEXT("Could not spawn graybox Enemy Host instance"));
				return false;
			}
			EnemyHost->EnemySlotId = Slot.EnemySlotId;
			EnemyHost->SetActorLabel(FString::Printf(
				TEXT("Enemy_%s_%s"), *Node.NodeId.ToString().Replace(
					TEXT("."), TEXT("_")), *Slot.EnemySlotId.ToString()));
			FWacomBattleSceneEnemyHostSlot& HostSlot =
				Trigger.SceneEnemyHostSlots.AddDefaulted_GetRef();
			HostSlot.EnemySlotId = Slot.EnemySlotId;
			HostSlot.SceneEnemyHost = EnemyHost;
		}
		return true;
	}

	AActor* SpawnContentHost(
		UWorld& World,
		const FWacomMapNodeDefinition& Node,
		const UWacomFloorMapDefinition& Floor,
		const TMap<EWacomMapNodeType, UClass*>& ContentClasses,
		const TMap<const UEnemyDefinition*, UClass*>& EnemyHostClasses,
		TArray<FString>& OutErrors)
	{
		UClass* const* ContentClass = ContentClasses.Find(Node.NodeType);
		if (!ContentClass || !*ContentClass)
		{
			OutErrors.Add(FString::Printf(TEXT("Missing Host class for NodeType %d"),
				static_cast<int32>(Node.NodeType)));
			return nullptr;
		}
		const FVector Anchor = WorldLocationForNode(Node.NodeId);
		const FVector Arrival = ArrivalDirectionForNode(Floor, Node.NodeId);
		const FVector Lateral = FVector::CrossProduct(FVector::UpVector, Arrival)
			.GetSafeNormal(SMALL_NUMBER, FVector::RightVector);
		const FVector HostLocation = Anchor + Lateral * 180.0f;
		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AActor* Host = World.SpawnActor<AActor>(*ContentClass,
			FTransform((-Arrival).Rotation(), HostLocation), Params);
		if (!Host || !BindContentHost(*Host, Node))
		{
			OutErrors.Add(TEXT("Could not spawn/bind content Host"));
			return nullptr;
		}
		Host->SetActorLabel(TEXT("Host_")
			+ Node.NodeId.ToString().Replace(TEXT("."), TEXT("_")));
		const FName PersistentId = PersistentIdForNode(Node.NodeId);
		if (ABattleTriggerActor* Battle = Cast<ABattleTriggerActor>(Host))
		{
			if (!ConfigureBattleHost(
				World, *Battle, Node, Floor, EnemyHostClasses, OutErrors))
			{
				return nullptr;
			}
		}
		else if (AWacomRunEventTriggerActor* Event =
			Cast<AWacomRunEventTriggerActor>(Host))
		{
			Event->PersistentId = PersistentId;
			Event->EventDefinition = Node.Content.RunEvent.RunEventDefinition;
			Event->RunEventEntryViewpoint = SpawnViewpoint(
				World, TEXT("View_Event_")
					+ Node.NodeId.ToString().Replace(TEXT("."), TEXT("_")),
				Anchor - Arrival * 320.0f + FVector(0.0, 0.0, 100.0),
				HostLocation + FVector(0.0, 0.0, 70.0));
		}
		else if (AWacomShopTriggerActor* Shop = Cast<AWacomShopTriggerActor>(Host))
		{
			Shop->PersistentId = PersistentId;
			Shop->ShopDefinition = Node.Content.Shop.ShopDefinition;
			Shop->ShopEntryViewpoint = SpawnViewpoint(
				World, TEXT("View_Shop_")
					+ Node.NodeId.ToString().Replace(TEXT("."), TEXT("_")),
				Anchor - Arrival * 320.0f + FVector(0.0, 0.0, 100.0),
				HostLocation + FVector(0.0, 0.0, 70.0));
		}
		else if (AWacomRunRewardPickupActor* Pickup =
			Cast<AWacomRunRewardPickupActor>(Host))
		{
			Pickup->PersistentId = PersistentId;
			Pickup->PickupDefinition = Node.Content.Treasure.PickupDefinition;
		}
		else if (Node.NodeType == EWacomMapNodeType::FloorEntrance)
		{
			if (!SetNameProperty(*Host, TEXT("PersistentId"), PersistentId))
			{
				OutErrors.Add(TEXT("FloorEntrance marker PersistentId is unavailable"));
				return nullptr;
			}
			Host->SetActorLabel(TEXT("Host_Node_Exit_01_GRAYBOX_NO_TRAVEL"));
		}
		return Host;
	}

	bool ConfigureProductionWorld(
		UWorld& World,
		UWacomFloorMapDefinition& Floor,
		TArray<FString>& OutErrors)
	{
		UClass* AnchorClass = LoadBlueprintClass(
			AnchorBlueprintPackage, *AWacomRunMapNodeAnchorActor::StaticClass(), OutErrors);
		UClass* PathClass = LoadBlueprintClass(
			PathBlueprintPackage, *AWacomRunPathSegmentActor::StaticClass(), OutErrors);
		UClass* BranchClass = LoadBlueprintClass(
			BranchBlueprintPackage, *AWacomRunPathBranchTargetActor::StaticClass(), OutErrors);
		TMap<EWacomMapNodeType, UClass*> ContentClasses;
		ContentClasses.Add(EWacomMapNodeType::Encounter, LoadBlueprintClass(
			BattleTriggerBlueprintPackage, *ABattleTriggerActor::StaticClass(), OutErrors));
		ContentClasses.Add(EWacomMapNodeType::RunEvent, LoadBlueprintClass(
			EventTriggerBlueprintPackage, *AWacomRunEventTriggerActor::StaticClass(), OutErrors));
		ContentClasses.Add(EWacomMapNodeType::Shop, LoadBlueprintClass(
			ShopTriggerBlueprintPackage, *AWacomShopTriggerActor::StaticClass(), OutErrors));
		ContentClasses.Add(EWacomMapNodeType::Treasure, LoadBlueprintClass(
			RewardPickupBlueprintPackage, *AWacomRunRewardPickupActor::StaticClass(), OutErrors));
		ContentClasses.Add(EWacomMapNodeType::FloorEntrance, LoadBlueprintClass(
			FloorEntranceMarkerPackage, *AActor::StaticClass(), OutErrors));
		TMap<const UEnemyDefinition*, UClass*> EnemyHostClasses;
		for (const FEnemyHostSeed& Seed : EnemyHostSeeds())
		{
			UEnemyDefinition* Enemy = ResolveRequiredAsset<UEnemyDefinition>(
				Seed.EnemyPackagePath, OutErrors);
			UClass* HostClass = LoadBlueprintClass(
				Seed.PackagePath, *AWacomBattleEnemyActor::StaticClass(), OutErrors);
			if (Enemy && HostClass)
			{
				EnemyHostClasses.Add(Enemy, HostClass);
			}
		}
		UStaticMesh* Cube = LoadObject<UStaticMesh>(
			nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (!AnchorClass || !PathClass || !BranchClass || !Cube
			|| OutErrors.Num() > 0)
		{
			return false;
		}

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		AWacomRunFloorSceneDescriptorActor* Descriptor =
			World.SpawnActor<AWacomRunFloorSceneDescriptorActor>(
				AWacomRunFloorSceneDescriptorActor::StaticClass(),
				FTransform::Identity, Params);
		if (!Descriptor)
		{
			OutErrors.Add(TEXT("Could not spawn Floor scene descriptor"));
			return false;
		}
		Descriptor->FloorDefinition = &Floor;
		Descriptor->SetActorLabel(TEXT("RunFloorSceneDescriptor_Floor_Main_01"));

		TMap<FName, AWacomRunMapNodeAnchorActor*> Anchors;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			const FVector Location = WorldLocationForNode(Node.NodeId);
			const FVector Arrival = ArrivalDirectionForNode(Floor, Node.NodeId);
			AWacomRunMapNodeAnchorActor* Anchor =
				Cast<AWacomRunMapNodeAnchorActor>(World.SpawnActor<AActor>(
					AnchorClass, FTransform(Arrival.Rotation(), Location), Params));
			if (!Anchor || !SpawnGrayboxPad(World, Node.NodeId, *Cube))
			{
				OutErrors.Add(TEXT("Could not spawn Anchor/graybox pad"));
				return false;
			}
			Anchor->NodeId = Node.NodeId;
			Anchor->SetActorLabel(TEXT("Anchor_")
				+ Node.NodeId.ToString().Replace(TEXT("."), TEXT("_")));
			Anchors.Add(Node.NodeId, Anchor);
		}

		TMap<FName, int32> OutgoingCounts;
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			++OutgoingCounts.FindOrAdd(Edge.FromNodeId);
		}
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			AWacomRunMapNodeAnchorActor* Source = Anchors.FindRef(Edge.FromNodeId);
			AWacomRunMapNodeAnchorActor* Target = Anchors.FindRef(Edge.ToNodeId);
			AWacomRunPathSegmentActor* Path =
				Cast<AWacomRunPathSegmentActor>(World.SpawnActor<AActor>(
					PathClass, FTransform::Identity, Params));
			if (!Source || !Target || !Path || !Path->GetPathSpline())
			{
				OutErrors.Add(TEXT("Could not spawn/configure Path"));
				return false;
			}
			Path->EdgeId = Edge.EdgeId;
			Path->SetActorLabel(TEXT("Path_")
				+ Edge.EdgeId.ToString().Replace(TEXT("."), TEXT("_")));
			USplineComponent* Spline = Path->GetPathSpline();
			Spline->ClearSplinePoints(false);
			Spline->AddSplinePoint(Source->GetActorLocation(),
				ESplineCoordinateSpace::World, false);
			Spline->AddSplinePoint(Target->GetActorLocation(),
				ESplineCoordinateSpace::World, false);
			Spline->SetSplinePointType(0, ESplinePointType::Curve, false);
			Spline->SetSplinePointType(1, ESplinePointType::Curve, false);
			Spline->UpdateSpline();

			if (OutgoingCounts.FindRef(Edge.FromNodeId) == 2)
			{
				const FVector Direction = (Target->GetActorLocation()
					- Source->GetActorLocation()).GetSafeNormal();
				AWacomRunPathBranchTargetActor* Branch =
					Cast<AWacomRunPathBranchTargetActor>(World.SpawnActor<AActor>(
						BranchClass,
						FTransform(Direction.Rotation(),
							Source->GetActorLocation() + Direction * 250.0f),
						Params));
				if (!Branch)
				{
					OutErrors.Add(TEXT("Could not spawn BranchTarget"));
					return false;
				}
				Branch->EdgeId = Edge.EdgeId;
				Branch->SetActorLabel(TEXT("Branch_")
					+ Edge.EdgeId.ToString().Replace(TEXT("."), TEXT("_")));
			}
		}

		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			if (Node.NodeType == EWacomMapNodeType::Navigation)
			{
				continue;
			}
			if (!SpawnContentHost(
				World, Node, Floor, ContentClasses, EnemyHostClasses, OutErrors))
			{
				return false;
			}
		}
		return OutErrors.IsEmpty();
	}

	FName ReadPersistentId(const AActor& Actor)
	{
		if (const ABattleTriggerActor* Battle = Cast<ABattleTriggerActor>(&Actor))
		{
			return Battle->PersistentId;
		}
		if (const AWacomRunEventTriggerActor* Event =
			Cast<AWacomRunEventTriggerActor>(&Actor))
		{
			return Event->PersistentId;
		}
		if (const AWacomShopTriggerActor* Shop =
			Cast<AWacomShopTriggerActor>(&Actor))
		{
			return Shop->PersistentId;
		}
		if (const AWacomRunRewardPickupActor* Pickup =
			Cast<AWacomRunRewardPickupActor>(&Actor))
		{
			return Pickup->PersistentId;
		}
		const FNameProperty* Property = FindFProperty<FNameProperty>(
			Actor.GetClass(), TEXT("PersistentId"));
		return Property
			? Property->GetPropertyValue_InContainer(&Actor)
			: NAME_None;
	}

	bool ValidateProductionWorld(
		UWorld& World,
		UWacomFloorMapDefinition& Floor,
		TArray<FString>& OutErrors)
	{
		const FWacomRunSceneBindingValidationReport SharedReport =
			FWacomRunSceneBindingValidation::ValidateLoadedWorld(&World);
		for (const FWacomRunSceneBindingDiagnostic& Diagnostic :
			SharedReport.Diagnostics)
		{
			if (Diagnostic.Severity
				== EWacomRunSceneBindingDiagnosticSeverity::Error)
			{
				OutErrors.Add(Diagnostic.Message.ToString());
			}
		}

		int32 DescriptorCount = 0;
		int32 AnchorCount = 0;
		int32 PathCount = 0;
		int32 BranchCount = 0;
		int32 ContentHostCount = 0;
		int32 EnemyHostCount = 0;
		int32 ViewpointCount = 0;
		TSet<FName> AnchorIds;
		TSet<FName> PathIds;
		TSet<FName> BranchIds;
		TSet<FName> HostNodeIds;
		TSet<FName> PersistentIds;
		for (TActorIterator<AActor> It(&World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor->ActorHasTag(TEXT("Wacom.Generated.RunExploration")))
			{
				OutErrors.Add(TEXT("Production map contains Debug generated ownership"));
			}
			if (const AWacomRunFloorSceneDescriptorActor* Descriptor =
				Cast<AWacomRunFloorSceneDescriptorActor>(Actor))
			{
				++DescriptorCount;
				if (Descriptor->FloorDefinition != &Floor)
				{
					OutErrors.Add(TEXT("Scene descriptor FloorDefinition mismatch"));
				}
			}
			if (const AWacomRunMapNodeAnchorActor* Anchor =
				Cast<AWacomRunMapNodeAnchorActor>(Actor))
			{
				++AnchorCount;
				AnchorIds.Add(Anchor->NodeId);
			}
			if (const AWacomRunPathSegmentActor* Path =
				Cast<AWacomRunPathSegmentActor>(Actor))
			{
				++PathCount;
				PathIds.Add(Path->EdgeId);
			}
			if (const AWacomRunPathBranchTargetActor* Branch =
				Cast<AWacomRunPathBranchTargetActor>(Actor))
			{
				++BranchCount;
				BranchIds.Add(Branch->EdgeId);
			}
			if (Cast<AWacomBattleEnemyActor>(Actor))
			{
				++EnemyHostCount;
			}
			if (Cast<AWacomFirstPersonViewpointActor>(Actor))
			{
				++ViewpointCount;
			}
			TArray<UWacomRunMapNodeBindingComponent*> Bindings;
			Actor->GetComponents<UWacomRunMapNodeBindingComponent>(Bindings);
			for (const UWacomRunMapNodeBindingComponent* Binding : Bindings)
			{
				++ContentHostCount;
				if (!Binding || HostNodeIds.Contains(Binding->NodeId))
				{
					OutErrors.Add(TEXT("Duplicate/null content Host binding"));
					continue;
				}
				HostNodeIds.Add(Binding->NodeId);
				const FName PersistentId = ReadPersistentId(*Actor);
				if (PersistentId != PersistentIdForNode(Binding->NodeId)
					|| PersistentIds.Contains(PersistentId))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Content Host PersistentId mismatch/duplicate: %s"),
						*Binding->NodeId.ToString()));
				}
				PersistentIds.Add(PersistentId);
			}
			if (const ABattleTriggerActor* Battle = Cast<ABattleTriggerActor>(Actor))
			{
				if (!Battle->EncounterDefinition
					|| Battle->SceneEnemyHostSlots.Num()
						!= Battle->EncounterDefinition->EnemySlots.Num())
				{
					OutErrors.Add(TEXT("Battle Host Encounter slot count mismatch"));
					continue;
				}
				for (const FEncounterEnemySlot& EncounterSlot :
					Battle->EncounterDefinition->EnemySlots)
				{
					const FWacomBattleSceneEnemyHostSlot* SceneSlot =
						Battle->SceneEnemyHostSlots.FindByPredicate(
							[&EncounterSlot](
								const FWacomBattleSceneEnemyHostSlot& Candidate)
							{
								return Candidate.EnemySlotId
									== EncounterSlot.EnemySlotId;
							});
					if (!SceneSlot || !SceneSlot->SceneEnemyHost
						|| SceneSlot->SceneEnemyHost->EnemyDefinition
							!= EncounterSlot.EnemyDefinition
						|| SceneSlot->SceneEnemyHost->EnemySlotId
							!= EncounterSlot.EnemySlotId)
					{
						OutErrors.Add(FString::Printf(
							TEXT("Battle scene slot mismatch: %s"),
							*EncounterSlot.EnemySlotId.ToString()));
					}
				}
			}
		}

		if (DescriptorCount != 1 || AnchorCount != 20 || PathCount != 21
			|| BranchCount != 4 || ContentHostCount != 16
			|| EnemyHostCount != 8 || ViewpointCount != 11)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Scene counts expected 1/20/21/4/16/8/11, got %d/%d/%d/%d/%d/%d/%d"),
				DescriptorCount, AnchorCount, PathCount, BranchCount,
				ContentHostCount, EnemyHostCount, ViewpointCount));
		}
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			if (!AnchorIds.Contains(Node.NodeId))
			{
				OutErrors.Add(TEXT("Missing Anchor: ") + Node.NodeId.ToString());
			}
			if (Node.NodeType != EWacomMapNodeType::Navigation
				&& !HostNodeIds.Contains(Node.NodeId))
			{
				OutErrors.Add(TEXT("Missing content Host: ") + Node.NodeId.ToString());
			}
		}
		for (const FWacomMapEdgeDefinition& Edge : Floor.Edges)
		{
			if (!PathIds.Contains(Edge.EdgeId))
			{
				OutErrors.Add(TEXT("Missing Path: ") + Edge.EdgeId.ToString());
			}
		}
		const TSet<FName> ExpectedBranches =
		{
			TEXT("Edge.Route.A.01"), TEXT("Edge.Route.B.01"),
			TEXT("Edge.Route.C.01"), TEXT("Edge.Route.D.01"),
		};
		if (BranchIds.Difference(ExpectedBranches).Num() > 0
			|| ExpectedBranches.Difference(BranchIds).Num() > 0)
		{
			OutErrors.Add(TEXT("BranchTarget EdgeId set mismatch"));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateForbiddenDependencyClosure(
		const FString& RootPackage,
		TArray<FString>& OutErrors)
	{
		FAssetRegistryModule& Module = FModuleManager::LoadModuleChecked<
			FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& Registry = Module.Get();
		TArray<FName> Queue = {FName(*RootPackage)};
		TSet<FName> Seen;
		for (int32 Index = 0; Index < Queue.Num(); ++Index)
		{
			const FName PackageName = Queue[Index];
			if (PackageName.IsNone() || Seen.Contains(PackageName))
			{
				continue;
			}
			Seen.Add(PackageName);
			const FString PackageString = PackageName.ToString();
			if (PackageString.StartsWith(TEXT("/Game/")))
			{
				const bool bForbidden =
					PackageString.Contains(TEXT("/Debug/"), ESearchCase::IgnoreCase)
					|| PackageString.Contains(TEXT("/Authoring/"), ESearchCase::IgnoreCase)
					|| PackageString.Contains(TEXT("/Test/"), ESearchCase::IgnoreCase)
					|| PackageString.Contains(TEXT("BadgeDisplayTests"), ESearchCase::IgnoreCase)
					|| PackageString == TEXT("/Game/Wacom/Maps/L_Exploration");
				if (bForbidden)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Forbidden Production dependency: %s -> %s"),
						*RootPackage, *PackageString));
				}
			}
			TArray<FAssetDependency> Dependencies;
			Registry.GetDependencies(
				FAssetIdentifier(PackageName), Dependencies,
				UE::AssetRegistry::EDependencyCategory::Package);
			for (const FAssetDependency& Dependency : Dependencies)
			{
				const FName DependencyPackage = Dependency.AssetId.PackageName;
				if (!DependencyPackage.IsNone()
					&& DependencyPackage.ToString().StartsWith(TEXT("/Game/"))
					&& !Seen.Contains(DependencyPackage))
				{
					Queue.Add(DependencyPackage);
				}
			}
		}
		return OutErrors.IsEmpty();
	}

	TArray<FFormalFloor1ProductionSceneManifestEntry> BuildManifest()
	{
		return
		{
			{EFormalFloor1ProductionSceneGroup::Floor, FloorPackage,
				TEXT("Floor.Main.01"), UWacomFloorMapDefinition::StaticClass()},
			{EFormalFloor1ProductionSceneGroup::EnemyHosts, BrushSnakeHostPackage,
				TEXT("EnemyHost.SerpentWood.BrushSnake"), UBlueprint::StaticClass()},
			{EFormalFloor1ProductionSceneGroup::EnemyHosts, MoltGuardHostPackage,
				TEXT("EnemyHost.SerpentWood.MoltGuard"), UBlueprint::StaticClass()},
			{EFormalFloor1ProductionSceneGroup::EnemyHosts, RootStalkerHostPackage,
				TEXT("EnemyHost.SerpentWood.RootStalker"), UBlueprint::StaticClass()},
			{EFormalFloor1ProductionSceneGroup::EnemyHosts, GuardianHostPackage,
				TEXT("EnemyHost.SerpentWood.ShallowGuardian"), UBlueprint::StaticClass()},
			{EFormalFloor1ProductionSceneGroup::Scene, FloorEntranceMarkerPackage,
				TEXT("Graybox.FloorEntranceMarker"), UBlueprint::StaticClass()},
			{EFormalFloor1ProductionSceneGroup::Scene, WorldPackage,
				TEXT("World.Floor.Main.01"), UWorld::StaticClass()},
		};
	}

	FString GroupToString(const EFormalFloor1ProductionSceneGroup Group)
	{
		switch (Group)
		{
		case EFormalFloor1ProductionSceneGroup::Floor: return TEXT("Floor");
		case EFormalFloor1ProductionSceneGroup::EnemyHosts: return TEXT("EnemyHosts");
		case EFormalFloor1ProductionSceneGroup::Scene: return TEXT("Scene");
		case EFormalFloor1ProductionSceneGroup::All:
		default: return TEXT("All");
		}
	}

	FString StateToString(const EFormalFloor1ProductionSceneEntryState State)
	{
		switch (State)
		{
		case EFormalFloor1ProductionSceneEntryState::Missing: return TEXT("Missing");
		case EFormalFloor1ProductionSceneEntryState::Existing: return TEXT("Existing");
		case EFormalFloor1ProductionSceneEntryState::Created: return TEXT("Created");
		case EFormalFloor1ProductionSceneEntryState::Failed: return TEXT("Failed");
		case EFormalFloor1ProductionSceneEntryState::NotProcessed:
		default: return TEXT("NotProcessed");
		}
	}

	bool IsSelected(
		const FFormalFloor1ProductionSceneManifestEntry& Entry,
		const EFormalFloor1ProductionSceneGroup Group)
	{
		return Group == EFormalFloor1ProductionSceneGroup::All
			|| Entry.Group == Group;
	}

	bool ValidateLoadedManifestEntry(
		const FFormalFloor1ProductionSceneManifestEntry& Entry,
		UObject& Object,
		const bool bCompareSeedDefaults,
		TArray<FString>& OutErrors)
	{
		if (Entry.Group == EFormalFloor1ProductionSceneGroup::Floor)
		{
			UWacomFloorMapDefinition* Floor = Cast<UWacomFloorMapDefinition>(&Object);
			const bool bValid = Floor && ValidateFormalFloor1ProductionFloor(
				*Floor, bCompareSeedDefaults, OutErrors);
			return ValidateForbiddenDependencyClosure(
				Entry.PackagePath, OutErrors) && bValid;
		}
		if (Entry.Group == EFormalFloor1ProductionSceneGroup::EnemyHosts)
		{
			const FEnemyHostSeed* Seed = FindEnemyHostSeed(Entry.PackagePath);
			UBlueprint* Blueprint = Cast<UBlueprint>(&Object);
			if (!Seed || !Blueprint)
			{
				OutErrors.Add(TEXT("Enemy Host manifest seed/object mismatch"));
				return false;
			}
			const bool bValid =
				ValidateEnemyHostBlueprint(*Blueprint, *Seed, OutErrors);
			return ValidateForbiddenDependencyClosure(
				Entry.PackagePath, OutErrors) && bValid;
		}
		if (Entry.PackagePath == FloorEntranceMarkerPackage)
		{
			UBlueprint* Blueprint = Cast<UBlueprint>(&Object);
			if (!Blueprint)
			{
				OutErrors.Add(TEXT("FloorEntrance marker is not a Blueprint"));
				return false;
			}
			const bool bValid =
				ValidateFloorEntranceMarkerBlueprint(*Blueprint, OutErrors);
			return ValidateForbiddenDependencyClosure(
				Entry.PackagePath, OutErrors) && bValid;
		}
		if (Entry.PackagePath == WorldPackage)
		{
			UWorld* World = Cast<UWorld>(&Object);
			UWacomFloorMapDefinition* Floor = LoadObject<UWacomFloorMapDefinition>(
				nullptr, *ObjectPathForPackage(FloorPackage));
			if (!World || !Floor)
			{
				OutErrors.Add(TEXT("Production World or Floor could not be loaded"));
				return false;
			}
			const bool bValid = ValidateProductionWorld(
				*World, *Floor, OutErrors);
			return ValidateForbiddenDependencyClosure(
				Entry.PackagePath, OutErrors) && bValid;
		}
		OutErrors.Add(TEXT("Unhandled manifest entry: ") + Entry.PackagePath);
		return false;
	}

	bool WriteReportJson(FFormalFloor1ProductionSceneBuildReport& Report)
	{
		if (Report.ReportPath.IsEmpty())
		{
			Report.ReportPath = FPaths::ProjectSavedDir()
				/ TEXT("FormalFloor1ProductionScene")
				/ FString::Printf(TEXT("%s-report.json"),
					*GroupToString(Report.Options.Group));
		}
		else if (FPaths::IsRelative(Report.ReportPath))
		{
			Report.ReportPath = FPaths::ConvertRelativePathToFull(
				FPaths::ProjectDir(), Report.ReportPath);
		}
		IFileManager::Get().MakeDirectory(*FPaths::GetPath(Report.ReportPath), true);

		TSharedRef<FJsonObject> Root = MakeShared<FJsonObject>();
		Root->SetNumberField(TEXT("schemaVersion"), ReportSchemaVersion);
		Root->SetStringField(TEXT("timestampUtc"), FDateTime::UtcNow().ToIso8601());
		Root->SetStringField(TEXT("projectPath"),
			FPaths::ConvertRelativePathToFull(FPaths::ProjectDir()));
		Root->SetStringField(TEXT("group"), GroupToString(Report.Options.Group));
		Root->SetBoolField(TEXT("seedMissing"), Report.Options.bSeedMissing);
		Root->SetBoolField(TEXT("compareSeedDefaults"),
			Report.Options.bCompareSeedDefaults);
		Root->SetNumberField(TEXT("manifestCount"), Report.ManifestCount);
		Root->SetNumberField(TEXT("selectedCount"), Report.SelectedCount);
		Root->SetNumberField(TEXT("createdCount"), Report.CreatedCount);
		Root->SetNumberField(TEXT("existingCount"), Report.ExistingCount);
		Root->SetNumberField(TEXT("missingCount"), Report.MissingCount);
		Root->SetNumberField(TEXT("failedCount"), Report.FailedCount);
		Root->SetNumberField(TEXT("savedCount"), Report.SavedCount);
		Root->SetNumberField(TEXT("exitCode"), Report.ExitCode);
		Root->SetStringField(TEXT("failureCategory"), Report.FailureCategory);
		TArray<TSharedPtr<FJsonValue>> JsonEntries;
		for (const FFormalFloor1ProductionSceneEntryReport& Entry : Report.Entries)
		{
			TSharedRef<FJsonObject> JsonEntry = MakeShared<FJsonObject>();
			JsonEntry->SetStringField(TEXT("package"), Entry.PackagePath);
			JsonEntry->SetStringField(TEXT("class"), Entry.ClassName);
			JsonEntry->SetStringField(TEXT("stableId"), Entry.StableId.ToString());
			JsonEntry->SetStringField(TEXT("state"), StateToString(Entry.State));
			JsonEntry->SetBoolField(TEXT("saved"), Entry.bSaved);
			TArray<TSharedPtr<FJsonValue>> Diagnostics;
			for (const FString& Diagnostic : Entry.Diagnostics)
			{
				Diagnostics.Add(MakeShared<FJsonValueString>(Diagnostic));
			}
			JsonEntry->SetArrayField(TEXT("diagnostics"), Diagnostics);
			JsonEntries.Add(MakeShared<FJsonValueObject>(JsonEntry));
		}
		Root->SetArrayField(TEXT("entries"), JsonEntries);
		FString Json;
		const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
		return FJsonSerializer::Serialize(Root, Writer)
			&& FFileHelper::SaveStringToFile(Json, *Report.ReportPath);
	}
}

namespace Wacom::ContentBuilder
{
	const TArray<FFormalFloor1ProductionSceneManifestEntry>&
	GetFormalFloor1ProductionSceneManifest()
	{
		static const TArray<FFormalFloor1ProductionSceneManifestEntry> Manifest =
			BuildManifest();
		return Manifest;
	}

	bool ParseFormalFloor1ProductionSceneOptions(
		const TArray<FString>& Arguments,
		FFormalFloor1ProductionSceneOptions& OutOptions,
		TArray<FString>& OutErrors)
	{
		OutOptions = FFormalFloor1ProductionSceneOptions();
		bool bSawGroup = false;
		bool bSawReport = false;
		for (FString Argument : Arguments)
		{
			while (Argument.RemoveFromStart(TEXT("-"))) {}
			if (Argument.Equals(TEXT("Inspect"), ESearchCase::IgnoreCase))
			{
				continue;
			}
			if (Argument.Equals(TEXT("SeedMissing"), ESearchCase::IgnoreCase))
			{
				if (OutOptions.bSeedMissing)
				{
					OutErrors.Add(TEXT("SeedMissing was specified more than once"));
				}
				OutOptions.bSeedMissing = true;
				continue;
			}
			if (Argument.Equals(TEXT("CompareSeedDefaults"), ESearchCase::IgnoreCase))
			{
				if (OutOptions.bCompareSeedDefaults)
				{
					OutErrors.Add(TEXT("CompareSeedDefaults was specified more than once"));
				}
				OutOptions.bCompareSeedDefaults = true;
				continue;
			}
			if (Argument.StartsWith(TEXT("Group="), ESearchCase::IgnoreCase))
			{
				const FString Value = Argument.Mid(FCString::Strlen(TEXT("Group=")));
				if (bSawGroup)
				{
					OutErrors.Add(TEXT("Group was specified more than once"));
				}
				bSawGroup = true;
				if (Value.Equals(TEXT("Floor"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ProductionSceneGroup::Floor;
				}
				else if (Value.Equals(TEXT("EnemyHosts"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ProductionSceneGroup::EnemyHosts;
				}
				else if (Value.Equals(TEXT("Scene"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ProductionSceneGroup::Scene;
				}
				else if (Value.Equals(TEXT("All"), ESearchCase::IgnoreCase))
				{
					OutOptions.Group = EFormalFloor1ProductionSceneGroup::All;
				}
				else
				{
					OutErrors.Add(FString::Printf(TEXT("Invalid Group: %s"), *Value));
				}
				continue;
			}
			if (Argument.StartsWith(TEXT("Report="), ESearchCase::IgnoreCase))
			{
				const FString Value = Argument.Mid(FCString::Strlen(TEXT("Report=")));
				if (bSawReport)
				{
					OutErrors.Add(TEXT("Report was specified more than once"));
				}
				bSawReport = true;
				OutOptions.ReportPath = Value.TrimQuotes();
				if (OutOptions.ReportPath.IsEmpty())
				{
					OutErrors.Add(TEXT("Report path cannot be empty"));
				}
				continue;
			}
			OutErrors.Add(FString::Printf(TEXT("Unknown argument: %s"), *Argument));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1ProductionSceneManifest(TArray<FString>& OutErrors)
	{
		const auto& Manifest = GetFormalFloor1ProductionSceneManifest();
		if (Manifest.Num() != 7)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Expected 7 manifest entries, got %d"), Manifest.Num()));
		}
		TSet<FString> Packages;
		TSet<FName> StableIds;
		int32 FloorCount = 0;
		int32 EnemyHostCount = 0;
		int32 SceneCount = 0;
		for (const FFormalFloor1ProductionSceneManifestEntry& Entry : Manifest)
		{
			if (!FPackageName::IsValidLongPackageName(Entry.PackagePath))
			{
				OutErrors.Add(TEXT("Invalid package path: ") + Entry.PackagePath);
			}
			if (!Entry.PackagePath.StartsWith(TEXT("/Game/Wacom/")))
			{
				OutErrors.Add(TEXT("Package outside Wacom root: ") + Entry.PackagePath);
			}
			for (const TCHAR* Forbidden :
				{TEXT("Debug"), TEXT("Authoring"), TEXT("Test"), TEXT("BadgeDisplayTests")})
			{
				if (Entry.PackagePath.Contains(Forbidden, ESearchCase::IgnoreCase))
				{
					OutErrors.Add(FString::Printf(TEXT("Forbidden identity %s: %s"),
						Forbidden, *Entry.PackagePath));
				}
			}
			if (Packages.Contains(Entry.PackagePath))
			{
				OutErrors.Add(TEXT("Duplicate package: ") + Entry.PackagePath);
			}
			Packages.Add(Entry.PackagePath);
			if (Entry.StableId.IsNone() || StableIds.Contains(Entry.StableId))
			{
				OutErrors.Add(TEXT("Missing/duplicate stable id: ")
					+ Entry.StableId.ToString());
			}
			StableIds.Add(Entry.StableId);
			if (!Entry.AssetClass)
			{
				OutErrors.Add(TEXT("Missing asset class: ") + Entry.PackagePath);
			}
			switch (Entry.Group)
			{
			case EFormalFloor1ProductionSceneGroup::Floor: ++FloorCount; break;
			case EFormalFloor1ProductionSceneGroup::EnemyHosts: ++EnemyHostCount; break;
			case EFormalFloor1ProductionSceneGroup::Scene: ++SceneCount; break;
			case EFormalFloor1ProductionSceneGroup::All: break;
			}
		}
		if (FloorCount != 1 || EnemyHostCount != 4 || SceneCount != 2)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Expected group counts 1/4/2, got %d/%d/%d"),
				FloorCount, EnemyHostCount, SceneCount));
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1ProductionFloor(
		const UWacomFloorMapDefinition& Floor,
		const bool bCompareSeedDefaults,
		TArray<FString>& OutErrors)
	{
		AppendSharedFloorErrors(Floor, OutErrors);
		if (Floor.FloorId != TEXT("Floor.Main.01"))
		{
			OutErrors.Add(TEXT("FloorId must be Floor.Main.01"));
		}
		if (Floor.EntryNodeId != TEXT("Node.Entry"))
		{
			OutErrors.Add(TEXT("EntryNodeId must be Node.Entry"));
		}
		if (Floor.Nodes.Num() != NodeSeeds().Num()
			|| Floor.Edges.Num() != EdgeSeeds().Num())
		{
			OutErrors.Add(FString::Printf(
				TEXT("Expected 20 nodes / 21 edges, got %d / %d"),
				Floor.Nodes.Num(), Floor.Edges.Num()));
		}
		TMap<EWacomMapNodeType, int32> TypeCounts;
		for (const FWacomMapNodeDefinition& Node : Floor.Nodes)
		{
			++TypeCounts.FindOrAdd(Node.NodeType);
		}
		const TMap<EWacomMapNodeType, int32> ExpectedTypeCounts =
		{
			{EWacomMapNodeType::Navigation, 4},
			{EWacomMapNodeType::Encounter, 6},
			{EWacomMapNodeType::RunEvent, 4},
			{EWacomMapNodeType::Treasure, 4},
			{EWacomMapNodeType::Shop, 1},
			{EWacomMapNodeType::FloorEntrance, 1},
		};
		for (const auto& Pair : ExpectedTypeCounts)
		{
			if (TypeCounts.FindRef(Pair.Key) != Pair.Value)
			{
				OutErrors.Add(TEXT("Floor node type distribution mismatch"));
				break;
			}
		}

		TArray<FString> ExpectedErrors;
		TStrongObjectPtr<UWacomFloorMapDefinition> Expected(
			NewObject<UWacomFloorMapDefinition>(GetTransientPackage()));
		ConfigureExpectedFloor(*Expected.Get(), ExpectedErrors);
		OutErrors.Append(ExpectedErrors);
		if (Floor.Nodes.Num() == Expected->Nodes.Num())
		{
			for (int32 Index = 0; Index < Floor.Nodes.Num(); ++Index)
			{
				const FWacomMapNodeDefinition& ActualNode = Floor.Nodes[Index];
				const FWacomMapNodeDefinition& ExpectedNode = Expected->Nodes[Index];
				if (ActualNode.NodeId != ExpectedNode.NodeId
					|| ActualNode.NodeType != ExpectedNode.NodeType
					|| ActualNode.bAllowsCamp != ExpectedNode.bAllowsCamp
					|| ActualNode.LandmarkVisibility != ExpectedNode.LandmarkVisibility
					|| ActualNode.Content.Encounter.EncounterDefinition
						!= ExpectedNode.Content.Encounter.EncounterDefinition
					|| ActualNode.Content.Encounter.bBoss
						!= ExpectedNode.Content.Encounter.bBoss
					|| ActualNode.Content.RunEvent.RunEventDefinition
						!= ExpectedNode.Content.RunEvent.RunEventDefinition
					|| ActualNode.Content.Shop.ShopDefinition
						!= ExpectedNode.Content.Shop.ShopDefinition
					|| ActualNode.Content.Treasure.PickupDefinition
						!= ExpectedNode.Content.Treasure.PickupDefinition
					|| ActualNode.Content.FloorEntrance.TargetFloorId
						!= ExpectedNode.Content.FloorEntrance.TargetFloorId
					|| ActualNode.Content.FloorEntrance.RequiredCredentialIds
						!= ExpectedNode.Content.FloorEntrance.RequiredCredentialIds)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Stable node contract mismatch at index %d (%s)"),
						Index, *ExpectedNode.NodeId.ToString()));
				}
			}
		}
		if (Floor.Edges.Num() == Expected->Edges.Num())
		{
			for (int32 Index = 0; Index < Floor.Edges.Num(); ++Index)
			{
				const FWacomMapEdgeDefinition& ActualEdge = Floor.Edges[Index];
				const FWacomMapEdgeDefinition& ExpectedEdge = Expected->Edges[Index];
				if (ActualEdge.EdgeId != ExpectedEdge.EdgeId
					|| ActualEdge.FromNodeId != ExpectedEdge.FromNodeId
					|| ActualEdge.ToNodeId != ExpectedEdge.ToNodeId)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Stable edge contract mismatch at index %d (%s)"),
						Index, *ExpectedEdge.EdgeId.ToString()));
				}
			}
		}
		const TSet<FName> Reachable = BuildReachable(Floor, Floor.EntryNodeId);
		if (Reachable.Num() != Floor.Nodes.Num())
		{
			OutErrors.Add(TEXT("Not every Floor node is reachable from Entry"));
		}
		const TSet<FName> WithoutKey =
			BuildReachable(Floor, Floor.EntryNodeId, TEXT("Node.Key.01"));
		if (WithoutKey.Contains(TEXT("Node.Guardian.01"))
			|| WithoutKey.Contains(TEXT("Node.Exit.01")))
		{
			OutErrors.Add(TEXT("Node.Key.01 must dominate Guardian and Exit"));
		}
		if (bCompareSeedDefaults)
		{
			CompareStrictEditableProperties(Floor, *Expected.Get(), OutErrors);
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateFormalFloor1ProductionTransientFloor(TArray<FString>& OutErrors)
	{
		TStrongObjectPtr<UWacomFloorMapDefinition> Floor(
			NewObject<UWacomFloorMapDefinition>(GetTransientPackage()));
		if (!Floor.IsValid() || !ConfigureExpectedFloor(*Floor.Get(), OutErrors))
		{
			return false;
		}
		return ValidateFormalFloor1ProductionFloor(*Floor.Get(), true, OutErrors);
	}

	int32 RunFormalFloor1ProductionSceneBuilder(
		const TArray<FString>& Arguments,
		FFormalFloor1ProductionSceneBuildReport* OutReport)
	{
		FFormalFloor1ProductionSceneBuildReport Report;
		TArray<FString> ParseErrors;
		if (!ParseFormalFloor1ProductionSceneOptions(
			Arguments, Report.Options, ParseErrors))
		{
			Report.ExitCode = 2;
			Report.FailureCategory = TEXT("Arguments");
			if (OutReport) { *OutReport = Report; }
			return Report.ExitCode;
		}
		Report.ReportPath = Report.Options.ReportPath;
		const auto& Manifest = GetFormalFloor1ProductionSceneManifest();
		Report.ManifestCount = Manifest.Num();
		TArray<FString> ManifestErrors;
		if (!ValidateFormalFloor1ProductionSceneManifest(ManifestErrors))
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Manifest");
			WriteReportJson(Report);
			if (OutReport) { *OutReport = Report; }
			return Report.ExitCode;
		}

		TArray<int32> SelectedIndices;
		TMap<FString, UObject*> LoadedObjects;
		bool bPreflightFailed = false;
		for (int32 Index = 0; Index < Manifest.Num(); ++Index)
		{
			const auto& Entry = Manifest[Index];
			if (!IsSelected(Entry, Report.Options.Group))
			{
				continue;
			}
			SelectedIndices.Add(Index);
			auto& EntryReport = Report.Entries.AddDefaulted_GetRef();
			EntryReport.PackagePath = Entry.PackagePath;
			EntryReport.ClassName = GetNameSafe(Entry.AssetClass);
			EntryReport.StableId = Entry.StableId;
			if (!FPackageName::DoesPackageExist(Entry.PackagePath))
			{
				EntryReport.State = EFormalFloor1ProductionSceneEntryState::Missing;
				++Report.MissingCount;
				continue;
			}
			UObject* Existing = LoadObject<UObject>(
				nullptr, *ObjectPathForPackage(Entry.PackagePath));
			if (!Existing || Existing->GetClass() != Entry.AssetClass)
			{
				EntryReport.State = EFormalFloor1ProductionSceneEntryState::Failed;
				EntryReport.Diagnostics.Add(FString::Printf(
					TEXT("Existing package failed expected %s load/class check"),
					*GetNameSafe(Entry.AssetClass)));
				++Report.FailedCount;
				bPreflightFailed = true;
				continue;
			}
			EntryReport.State = EFormalFloor1ProductionSceneEntryState::Existing;
			LoadedObjects.Add(Entry.PackagePath, Existing);
		}
		Report.SelectedCount = SelectedIndices.Num();

		if (!Report.Options.bSeedMissing)
		{
			for (int32 SelectedIndex = 0; SelectedIndex < SelectedIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedIndices[SelectedIndex]];
				auto& EntryReport = Report.Entries[SelectedIndex];
				if (EntryReport.State == EFormalFloor1ProductionSceneEntryState::Existing)
				{
					TArray<FString> Errors;
					if (!ValidateLoadedManifestEntry(
						Entry, *LoadedObjects.FindChecked(Entry.PackagePath),
						Report.Options.bCompareSeedDefaults, Errors))
					{
						EntryReport.State = EFormalFloor1ProductionSceneEntryState::Failed;
						EntryReport.Diagnostics.Append(Errors);
						++Report.FailedCount;
					}
					else
					{
						++Report.ExistingCount;
					}
				}
				else if (EntryReport.State == EFormalFloor1ProductionSceneEntryState::Missing)
				{
					EntryReport.Diagnostics.Add(TEXT("Package is missing in inspect-only mode"));
				}
			}
			Report.ExitCode = (Report.MissingCount == 0 && Report.FailedCount == 0)
				? 0 : 1;
			Report.FailureCategory = Report.ExitCode == 0
				? TEXT("") : TEXT("Validation");
		}
		else if (bPreflightFailed)
		{
			Report.ExitCode = 1;
			Report.FailureCategory = TEXT("Preflight");
		}
		else
		{
			Report.MissingCount = 0;
			for (int32 SelectedIndex = 0; SelectedIndex < SelectedIndices.Num(); ++SelectedIndex)
			{
				const auto& Entry = Manifest[SelectedIndices[SelectedIndex]];
				auto& EntryReport = Report.Entries[SelectedIndex];
				if (EntryReport.State == EFormalFloor1ProductionSceneEntryState::Existing)
				{
					TArray<FString> Errors;
					if (!ValidateLoadedManifestEntry(
						Entry, *LoadedObjects.FindChecked(Entry.PackagePath),
						Report.Options.bCompareSeedDefaults, Errors))
					{
						EntryReport.State = EFormalFloor1ProductionSceneEntryState::Failed;
						EntryReport.Diagnostics.Append(Errors);
						++Report.FailedCount;
						Report.ExitCode = 1;
						Report.FailureCategory = TEXT("Validation");
						break;
					}
					++Report.ExistingCount;
					continue;
				}

				TArray<FString> Errors;
				UObject* CreatedObject = nullptr;
				UPackage* Package = nullptr;
				bool bSaved = false;
				if (Entry.Group == EFormalFloor1ProductionSceneGroup::Floor)
				{
					Package = CreatePackage(*Entry.PackagePath);
					const FName AssetName(
						*FPackageName::GetLongPackageAssetName(Entry.PackagePath));
					UWacomFloorMapDefinition* Floor = Package
						? NewObject<UWacomFloorMapDefinition>(Package, AssetName,
							RF_Public | RF_Standalone | RF_Transactional)
						: nullptr;
					if (Floor && ConfigureExpectedFloor(*Floor, Errors)
						&& ValidateFormalFloor1ProductionFloor(*Floor, true, Errors))
					{
						CreatedObject = Floor;
					}
				}
				else if (Entry.Group == EFormalFloor1ProductionSceneGroup::EnemyHosts)
				{
					const FEnemyHostSeed* Seed = FindEnemyHostSeed(Entry.PackagePath);
					CreatedObject = Seed
						? CreateEnemyHostBlueprint(*Seed, Errors) : nullptr;
					Package = CreatedObject ? CreatedObject->GetOutermost() : nullptr;
				}
				else if (Entry.PackagePath == FloorEntranceMarkerPackage)
				{
					CreatedObject = CreateFloorEntranceMarkerBlueprint(Errors);
					Package = CreatedObject ? CreatedObject->GetOutermost() : nullptr;
				}
				else if (Entry.PackagePath == WorldPackage)
				{
					UWacomFloorMapDefinition* Floor =
						LoadObject<UWacomFloorMapDefinition>(
							nullptr, *ObjectPathForPackage(FloorPackage));
					UWorld* World = Floor
						? UEditorLoadingAndSavingUtils::NewBlankMap(false)
						: nullptr;
					if (World && ConfigureProductionWorld(*World, *Floor, Errors)
						&& ValidateProductionWorld(*World, *Floor, Errors))
					{
						CreatedObject = World;
						bSaved = UEditorLoadingAndSavingUtils::SaveMap(
							World, Entry.PackagePath);
					}
				}
				if (!CreatedObject || !Errors.IsEmpty())
				{
					EntryReport.State = EFormalFloor1ProductionSceneEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					if (EntryReport.Diagnostics.IsEmpty())
					{
						EntryReport.Diagnostics.Add(TEXT("Could not create/configure asset"));
					}
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Create");
					break;
				}
				if (Entry.PackagePath != WorldPackage)
				{
					bSaved = SaveAssetPackage(
						Package, CreatedObject, Entry.PackagePath);
				}
				if (!bSaved)
				{
					EntryReport.State = EFormalFloor1ProductionSceneEntryState::Failed;
					EntryReport.Diagnostics.Add(TEXT("SavePackage/SaveMap failed"));
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Save");
					break;
				}
				UPackage::WaitForAsyncFileWrites();
				UObject* Reloaded = LoadObject<UObject>(
					nullptr, *ObjectPathForPackage(Entry.PackagePath));
				Errors.Reset();
				if (!Reloaded || !ValidateLoadedManifestEntry(
					Entry, *Reloaded, true, Errors))
				{
					EntryReport.State = EFormalFloor1ProductionSceneEntryState::Failed;
					EntryReport.Diagnostics.Append(Errors);
					EntryReport.Diagnostics.Add(TEXT("Post-save reload validation failed"));
					++Report.FailedCount;
					Report.ExitCode = 3;
					Report.FailureCategory = TEXT("Reload");
					break;
				}
				EntryReport.State = EFormalFloor1ProductionSceneEntryState::Created;
				EntryReport.bSaved = true;
				++Report.CreatedCount;
				++Report.SavedCount;
			}
		}

		if (!WriteReportJson(Report))
		{
			Report.ExitCode = 3;
			Report.FailureCategory = TEXT("ReportWrite");
		}
		UE_LOG(LogTemp, Display,
			TEXT("[FormalFloor1ProductionScene] Group=%s Created=%d Existing=%d Missing=%d Failed=%d Saved=%d Report=%s Exit=%d"),
			*GroupToString(Report.Options.Group), Report.CreatedCount,
			Report.ExistingCount, Report.MissingCount, Report.FailedCount,
			Report.SavedCount, *Report.ReportPath, Report.ExitCode);
		if (OutReport)
		{
			*OutReport = Report;
		}
		return Report.ExitCode;
	}
}
