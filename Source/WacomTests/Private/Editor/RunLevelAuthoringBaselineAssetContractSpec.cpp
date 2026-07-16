// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Actors/WacomRunMapNodeAnchorActor.h"
#include "Actors/WacomRunPathBranchTargetActor.h"
#include "Actors/WacomRunPathSegmentActor.h"
#include "Components/SplineComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Engine/Blueprint.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/WacomGameMode.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"

namespace WacomRunLevelAuthoringBaselineAssetContract
{
	constexpr const TCHAR* MainMapObjectPath =
		TEXT("/Game/Wacom/Maps/L_Exploration.L_Exploration");
	constexpr const TCHAR* DebugMapObjectPath =
		TEXT("/Game/Wacom/Maps/Debug/L_RunExploration_Debug.L_RunExploration_Debug");
	constexpr const TCHAR* AuthoringFloorObjectPath =
		TEXT("/Game/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01.DA_Floor_LevelAuthoring_01");
	constexpr const TCHAR* AuthoringJourneyObjectPath =
		TEXT("/Game/Wacom/Data/Map/Authoring/DA_Journey_LevelAuthoring.DA_Journey_LevelAuthoring");
	constexpr const TCHAR* MainGameModeObjectPath =
		TEXT("/Game/Wacom/Core/GameModes/GM_Wacom.GM_Wacom");
	constexpr const TCHAR* DebugGameModeObjectPath =
		TEXT("/Game/Wacom/Debug/GameModes/GM_WacomRunDebug.GM_WacomRunDebug");
	const FName GeneratedTag = TEXT("Wacom.Generated.RunExploration");

	struct FActorRecord
	{
		FGuid Guid;
		FTransform Transform = FTransform::Identity;
		TArray<FVector> SplinePoints;
	};

	struct FWorldRecord
	{
		int32 DescriptorCount = 0;
		const UWacomFloorMapDefinition* DescriptorFloor = nullptr;
		TSubclassOf<AGameModeBase> GameModeOverride;
		TMap<FName, FActorRecord> Anchors;
		TMap<FName, FActorRecord> Paths;
		TMap<FName, FActorRecord> Branches;
		TSet<FName> ContentHostNodeIds;
		int32 GeneratedRunActorCount = 0;
	};

	TMap<FString, FGuid> BuildFormalMapBaselineGuids()
	{
		TMap<FString, FGuid> BaselineGuids;
		BaselineGuids.Add(TEXT("Anchor:Battle.Snake"), FGuid(0x3E39091E, 0x41E2C3D6, 0x472C45BB, 0x6FC10AE6));
		BaselineGuids.Add(TEXT("Anchor:Entry"), FGuid(0x0FF1D519, 0x48768CB4, 0x6F058682, 0xD02FE767));
		BaselineGuids.Add(TEXT("Anchor:Event.FlagReward"), FGuid(0x87D77A3C, 0x44602693, 0x611153B9, 0xB139DB2E));
		BaselineGuids.Add(TEXT("Anchor:Event.SnakeGift"), FGuid(0x4F94EDCF, 0x47BE1D9F, 0x067F719A, 0x3294A4C4));
		BaselineGuids.Add(TEXT("Anchor:Junction"), FGuid(0x876D206E, 0x40FC1555, 0xC1AA05A6, 0xC21AF184));
		BaselineGuids.Add(TEXT("Anchor:Shop.Snake"), FGuid(0x06998AE5, 0x4E766FB5, 0xF431FB86, 0x087AD928));
		BaselineGuids.Add(TEXT("Anchor:Treasure.KeyChest"), FGuid(0xA775417C, 0x4831E1A7, 0x73E79AA5, 0x895E3856));
		BaselineGuids.Add(TEXT("Anchor:Treasure.PoisonFang"), FGuid(0x00304F00, 0x4472AF35, 0x96CCBF9D, 0xA4A3263E));
		BaselineGuids.Add(TEXT("Branch:JunctionToChest"), FGuid(0x3EA6E530, 0x4B194FDB, 0xCB04A798, 0x25E2C972));
		BaselineGuids.Add(TEXT("Branch:JunctionToEventFlag"), FGuid(0xCFDAAF74, 0x4EF4BC12, 0xCC7E4DA0, 0x6D3E2359));
		BaselineGuids.Add(TEXT("Branch:JunctionToPickup"), FGuid(0x9A7B3196, 0x4D7305AD, 0x8589EC82, 0x10D0EF63));
		BaselineGuids.Add(TEXT("Path:BattleToShop"), FGuid(0x68921465, 0x4212AFF3, 0x89EE87B9, 0x5DB23C60));
		BaselineGuids.Add(TEXT("Path:EntryToBattle"), FGuid(0x77CEE924, 0x413B2322, 0x50AF40B5, 0x3CC7D56A));
		BaselineGuids.Add(TEXT("Path:EventSnakeToJunction"), FGuid(0xFCCDC2EF, 0x4735A875, 0x3CA6FAB4, 0x4A40E931));
		BaselineGuids.Add(TEXT("Path:JunctionToChest"), FGuid(0x40B85291, 0x465CF16D, 0x7B1A7EB4, 0x54388D3C));
		BaselineGuids.Add(TEXT("Path:JunctionToEventFlag"), FGuid(0x972E8169, 0x4FC7E182, 0x64F133A9, 0xE2C1A4FE));
		BaselineGuids.Add(TEXT("Path:JunctionToPickup"), FGuid(0xBD4646A9, 0x43CF1372, 0xECD7DB88, 0x0353FB1A));
		BaselineGuids.Add(TEXT("Path:ShopToEventSnake"), FGuid(0xFA8D7020, 0x4CFA5E91, 0x8D979080, 0x70447BFA));
		return BaselineGuids;
	}

	FActorRecord MakeActorRecord(const AActor& Actor)
	{
		FActorRecord Record;
#if WITH_EDITORONLY_DATA
		Record.Guid = Actor.GetActorGuid();
#endif
		Record.Transform = Actor.GetActorTransform();
		return Record;
	}

	FWorldRecord BuildWorldRecord(UWorld& World)
	{
		FWorldRecord Record;
		if (const AWorldSettings* WorldSettings = World.GetWorldSettings())
		{
			Record.GameModeOverride = WorldSettings->DefaultGameMode;
		}
		for (AActor* Actor : World.PersistentLevel->Actors)
		{
			if (!IsValid(Actor))
			{
				continue;
			}
			if (const AWacomRunFloorSceneDescriptorActor* Descriptor =
				Cast<AWacomRunFloorSceneDescriptorActor>(Actor))
			{
				++Record.DescriptorCount;
				Record.DescriptorFloor = Descriptor->GetFloorDefinition();
			}
			if (Actor->ActorHasTag(GeneratedTag)
				&& (Actor->IsA<AWacomRunMapNodeAnchorActor>()
					|| Actor->IsA<AWacomRunPathSegmentActor>()
					|| Actor->IsA<AWacomRunPathBranchTargetActor>()))
			{
				++Record.GeneratedRunActorCount;
			}
			if (const AWacomRunMapNodeAnchorActor* Anchor =
				Cast<AWacomRunMapNodeAnchorActor>(Actor))
			{
				Record.Anchors.Add(Anchor->NodeId, MakeActorRecord(*Anchor));
			}
			else if (const AWacomRunPathSegmentActor* Path =
				Cast<AWacomRunPathSegmentActor>(Actor))
			{
				FActorRecord PathRecord = MakeActorRecord(*Path);
				if (const USplineComponent* Spline = Path->GetPathSpline())
				{
					for (int32 Index = 0; Index < Spline->GetNumberOfSplinePoints(); ++Index)
					{
						PathRecord.SplinePoints.Add(Spline->GetLocationAtSplinePoint(
							Index,
							ESplineCoordinateSpace::World));
					}
				}
				Record.Paths.Add(Path->EdgeId, MoveTemp(PathRecord));
			}
			else if (const AWacomRunPathBranchTargetActor* Branch =
				Cast<AWacomRunPathBranchTargetActor>(Actor))
			{
				Record.Branches.Add(Branch->EdgeId, MakeActorRecord(*Branch));
			}

			TInlineComponentArray<UWacomRunMapNodeBindingComponent*> Bindings;
			Actor->GetComponents(Bindings);
			for (const UWacomRunMapNodeBindingComponent* Binding : Bindings)
			{
				if (Binding && !Binding->NodeId.IsNone())
				{
					Record.ContentHostNodeIds.Add(Binding->NodeId);
				}
			}
		}
		return Record;
	}

	template <typename TMapType>
	void VerifyActorIdentityCatalog(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const TMapType& MainCatalog,
		const TMapType& DebugCatalog)
	{
		Test.TestEqual(
			FString::Printf(TEXT("%s actor count is preserved"), Label),
			MainCatalog.Num(),
			DebugCatalog.Num());
		for (const TPair<FName, FActorRecord>& Pair : MainCatalog)
		{
			const FActorRecord* DebugRecord = DebugCatalog.Find(Pair.Key);
			if (!Test.TestNotNull(
				FString::Printf(TEXT("%s identity %s exists in Debug clone"),
					Label, *Pair.Key.ToString()),
				DebugRecord))
			{
				continue;
			}
			Test.TestTrue(
				FString::Printf(TEXT("%s %s formal Actor GUID is valid"),
					Label, *Pair.Key.ToString()),
				Pair.Value.Guid.IsValid());
			Test.TestTrue(
				FString::Printf(TEXT("%s %s Debug Actor GUID is valid"),
					Label, *Pair.Key.ToString()),
				DebugRecord->Guid.IsValid());
		}
	}

	template <typename TMapType>
	void VerifyFormalBaselineGuids(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const TMapType& Catalog,
		const TMap<FString, FGuid>& BaselineGuids)
	{
		for (const TPair<FName, FActorRecord>& Pair : Catalog)
		{
			const FString Key = FString::Printf(
				TEXT("%s:%s"), Label, *Pair.Key.ToString());
			const FGuid* BaselineGuid = BaselineGuids.Find(Key);
			if (Test.TestNotNull(
				FString::Printf(TEXT("Formal baseline GUID exists for %s"), *Key),
				BaselineGuid))
			{
				Test.TestEqual(
					FString::Printf(TEXT("Formal Actor GUID is preserved for %s"), *Key),
					Pair.Value.Guid,
					*BaselineGuid);
			}
		}
	}

	TSet<FGuid> CollectActorGuids(const FWorldRecord& Record)
	{
		TSet<FGuid> Result;
		for (const TPair<FName, FActorRecord>& Pair : Record.Anchors)
			Result.Add(Pair.Value.Guid);
		for (const TPair<FName, FActorRecord>& Pair : Record.Paths)
			Result.Add(Pair.Value.Guid);
		for (const TPair<FName, FActorRecord>& Pair : Record.Branches)
			Result.Add(Pair.Value.Guid);
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunLevelAuthoringBaselineAssetContractSpec,
	"Wacom.UI.RunSceneBinding.AuthoringBaseline.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunLevelAuthoringBaselineAssetContractSpec::RunTest(
	const FString& Parameters)
{
	using namespace WacomRunLevelAuthoringBaselineAssetContract;

	UWorld* MainWorld = LoadObject<UWorld>(nullptr, MainMapObjectPath);
	UWorld* DebugWorld = LoadObject<UWorld>(nullptr, DebugMapObjectPath);
	UWacomFloorMapDefinition* AuthoringFloor =
		LoadObject<UWacomFloorMapDefinition>(nullptr, AuthoringFloorObjectPath);
	UWacomJourneyDefinition* AuthoringJourney =
		LoadObject<UWacomJourneyDefinition>(nullptr, AuthoringJourneyObjectPath);
	UBlueprint* MainGameMode = LoadObject<UBlueprint>(nullptr, MainGameModeObjectPath);
	UBlueprint* DebugGameMode = LoadObject<UBlueprint>(nullptr, DebugGameModeObjectPath);
	if (!TestNotNull(TEXT("Main Run map"), MainWorld)
		|| !TestNotNull(TEXT("Debug Run map"), DebugWorld)
		|| !TestNotNull(TEXT("Authoring Floor"), AuthoringFloor)
		|| !TestNotNull(TEXT("Authoring Journey"), AuthoringJourney)
		|| !TestNotNull(TEXT("Main GameMode Blueprint"), MainGameMode)
		|| !TestNotNull(TEXT("Debug GameMode Blueprint"), DebugGameMode))
	{
		return false;
	}

	TestEqual(TEXT("Authoring Floor identity"), AuthoringFloor->FloorId,
		FName(TEXT("Floor.Authoring.01")));
	TestEqual(TEXT("Authoring Journey identity"), AuthoringJourney->JourneyId,
		FName(TEXT("Journey.Authoring")));
	TestEqual(TEXT("Authoring Journey owns exactly one Floor"),
		AuthoringJourney->Floors.Num(), 1);
	TestTrue(TEXT("Authoring Journey references the Authoring Floor"),
		AuthoringJourney->Floors.Num() == 1
			&& AuthoringJourney->Floors[0] == AuthoringFloor);

	const AWacomGameMode* MainGameModeCDO = MainGameMode->GeneratedClass
		? Cast<AWacomGameMode>(MainGameMode->GeneratedClass->GetDefaultObject())
		: nullptr;
	const AWacomGameMode* DebugGameModeCDO = DebugGameMode->GeneratedClass
		? Cast<AWacomGameMode>(DebugGameMode->GeneratedClass->GetDefaultObject())
		: nullptr;
	TestTrue(TEXT("Main GM references Authoring Journey"),
		MainGameModeCDO
			&& MainGameModeCDO->DefaultJourneyDefinition == AuthoringJourney);
	TestTrue(TEXT("Debug GM references Debug Journey"),
		DebugGameModeCDO
			&& DebugGameModeCDO->DefaultJourneyDefinition
			&& DebugGameModeCDO->DefaultJourneyDefinition->JourneyId
				== FName(TEXT("Journey.Debug")));

	const FWorldRecord MainRecord = BuildWorldRecord(*MainWorld);
	const FWorldRecord DebugRecord = BuildWorldRecord(*DebugWorld);
	TestEqual(TEXT("Main Anchor identity count"), MainRecord.Anchors.Num(), 8);
	TestEqual(TEXT("Main Path identity count"), MainRecord.Paths.Num(), 7);
	TestEqual(TEXT("Main Branch identity count"), MainRecord.Branches.Num(), 3);
	TestEqual(TEXT("Main content host identity count"),
		MainRecord.ContentHostNodeIds.Num(), 6);
	const TMap<FString, FGuid> BaselineGuids = BuildFormalMapBaselineGuids();
	TestEqual(TEXT("Formal baseline GUID contract count"), BaselineGuids.Num(), 18);
	VerifyFormalBaselineGuids(
		*this, TEXT("Anchor"), MainRecord.Anchors, BaselineGuids);
	VerifyFormalBaselineGuids(
		*this, TEXT("Path"), MainRecord.Paths, BaselineGuids);
	VerifyFormalBaselineGuids(
		*this, TEXT("Branch"), MainRecord.Branches, BaselineGuids);
	const TSet<FGuid> MainGuids = CollectActorGuids(MainRecord);
	const TSet<FGuid> DebugGuids = CollectActorGuids(DebugRecord);
	TestEqual(TEXT("Formal Run Actor GUIDs are unique"), MainGuids.Num(), 18);
	TestEqual(TEXT("Debug Run Actor GUIDs are unique"), DebugGuids.Num(), 18);
	bool bMapGuidsAreIndependent = true;
	for (const FGuid& DebugGuid : DebugGuids)
		bMapGuidsAreIndependent &= !MainGuids.Contains(DebugGuid);
	TestTrue(TEXT("Debug clone owns independent Actor GUIDs"),
		bMapGuidsAreIndependent);
	TestEqual(TEXT("Main map has exactly one Descriptor"), MainRecord.DescriptorCount, 1);
	TestEqual(TEXT("Debug map has exactly one Descriptor"), DebugRecord.DescriptorCount, 1);
	TestTrue(TEXT("Main Descriptor references Authoring Floor"),
		MainRecord.DescriptorFloor == AuthoringFloor);
	TestTrue(TEXT("Debug Descriptor references Debug Floor"),
		DebugRecord.DescriptorFloor
			&& DebugRecord.DescriptorFloor->FloorId == FName(TEXT("Floor.Debug.01")));
	TestTrue(TEXT("Main map uses GM_Wacom"),
		MainRecord.GameModeOverride == MainGameMode->GeneratedClass);
	TestTrue(TEXT("Debug map uses GM_WacomRunDebug"),
		DebugRecord.GameModeOverride == DebugGameMode->GeneratedClass);
	TestEqual(TEXT("Main Run actors have no Debug generated ownership"),
		MainRecord.GeneratedRunActorCount, 0);
	TestTrue(TEXT("Debug clone retains explicit Debug generated ownership"),
		DebugRecord.GeneratedRunActorCount > 0);
	bool bContentHostIdentitiesMatch =
		MainRecord.ContentHostNodeIds.Num() == DebugRecord.ContentHostNodeIds.Num();
	for (const FName NodeId : MainRecord.ContentHostNodeIds)
	{
		bContentHostIdentitiesMatch &= DebugRecord.ContentHostNodeIds.Contains(NodeId);
	}
	TestTrue(TEXT("Content host identities are preserved"),
		bContentHostIdentitiesMatch);

	// Debug map 与正式图共享逻辑身份和制作合同，但允许独立摆放调试几何。
	// 端点、方向和绑定合法性由 Run scene validator/path tests 负责。
	VerifyActorIdentityCatalog(
		*this, TEXT("Anchor"), MainRecord.Anchors, DebugRecord.Anchors);
	VerifyActorIdentityCatalog(
		*this, TEXT("Path"), MainRecord.Paths, DebugRecord.Paths);
	VerifyActorIdentityCatalog(
		*this, TEXT("Branch"), MainRecord.Branches, DebugRecord.Branches);
	return true;
}

#endif
