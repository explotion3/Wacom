// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomWorldShopActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Cards/CardDefinition.h"
#include "Components/ChildActorComponent.h"
#include "Components/ActorComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Components/WacomWorldShopLayoutAnchorComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/InheritableComponentHandler.h"
#include "Engine/Level.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomMapTypes.h"
#include "Shops/ShopDefinition.h"
#include "UI/Shop/WacomWorldShopPresentationHost.h"

namespace WacomWorldShopFormalAssetSpec
{
	constexpr const TCHAR* BlueprintObjectPath =
		TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomWorldShop.BP_WacomWorldShop");
	constexpr const TCHAR* MainMapObjectPath =
		TEXT("/Game/Wacom/Maps/L_Exploration.L_Exploration");
	constexpr const TCHAR* ShopObjectPath =
		TEXT("/Game/Wacom/Data/Shops/DA_Shop_LevelAuthoringSnake.DA_Shop_LevelAuthoringSnake");
	constexpr const TCHAR* FloorObjectPath =
		TEXT("/Game/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01.DA_Floor_LevelAuthoring_01");

	struct FExpectedOffer
	{
		const TCHAR* CardObjectPath = nullptr;
		int32 Price = 0;
	};

	const TArray<FExpectedOffer>& ExpectedOffers()
	{
		static const TArray<FExpectedOffer> Offers =
		{
			{
				TEXT("/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang.DA_Card_PoisonFang"),
				0
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ChifuGongyi.DA_Card_ChifuGongyi"),
				2
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/DA_Card_ZhaoguangMudie.DA_Card_ZhaoguangMudie"),
				2
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_PoisonNeedle.DA_Card_Starter_PoisonNeedle"),
				2
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard.DA_Card_Starter_ChitinWard"),
				1
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_AntennaSearch.DA_Card_Starter_AntennaSearch"),
				2
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut.DA_Card_Starter_MoltCut"),
				2
			},
			{
				TEXT("/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_LightHusk.DA_Card_Starter_LightHusk"),
				1
			}
		};
		return Offers;
	}

	UStaticMeshComponent* FindPresentationMesh(
		const UBlueprint& Blueprint,
		const FName VariableName)
	{
		if (!Blueprint.SimpleConstructionScript)
		{
			return nullptr;
		}
		for (const USCS_Node* Node :
			Blueprint.SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetVariableName() == VariableName)
			{
				return Cast<UStaticMeshComponent>(Node->ComponentTemplate);
			}
		}
		return nullptr;
	}

	const USCS_Node* FindPresentationNode(
		const UBlueprint& Blueprint,
		const FName VariableName)
	{
		if (!Blueprint.SimpleConstructionScript)
		{
			return nullptr;
		}
		for (const USCS_Node* Node :
			Blueprint.SimpleConstructionScript->GetAllNodes())
		{
			if (Node && Node->GetVariableName() == VariableName)
			{
				return Node;
			}
		}
		return nullptr;
	}

	void VerifyPresentationMesh(
		FAutomationTestBase& Test,
		const TCHAR* Label,
		const UStaticMeshComponent* Mesh,
		const FVector& ExpectedLocation,
		const FVector& ExpectedScale)
	{
		if (!Test.TestNotNull(Label, Mesh))
		{
			return;
		}
		Test.TestTrue(
			FString::Printf(TEXT("%s uses Engine cube"), Label),
			Mesh->GetStaticMesh()
				&& Mesh->GetStaticMesh()->GetPathName()
					== TEXT("/Engine/BasicShapes/Cube.Cube"));
		Test.TestTrue(
			FString::Printf(TEXT("%s location"), Label),
			Mesh->GetRelativeLocation().Equals(ExpectedLocation));
		Test.TestTrue(
			FString::Printf(TEXT("%s scale"), Label),
			Mesh->GetRelativeScale3D().Equals(ExpectedScale));
		Test.TestTrue(
			FString::Printf(TEXT("%s has no collision"), Label),
			Mesh->GetCollisionEnabled() == ECollisionEnabled::NoCollision);
		Test.TestFalse(
			FString::Printf(TEXT("%s does not overlap"), Label),
			Mesh->GetGenerateOverlapEvents());
		Test.TestFalse(
			FString::Printf(TEXT("%s does not cast shadow"), Label),
			Mesh->CastShadow);
		Test.TestFalse(
			FString::Printf(TEXT("%s does not affect navigation"), Label),
			Mesh->CanEverAffectNavigation());
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFormalBrowseProfileAndMapInstanceContractSpec,
	"Wacom.UI.WorldShop.FormalAsset.BrowseProfileAndMapInstanceContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFormalBrowseProfileAndMapInstanceContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopFormalAssetSpec;

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, BlueprintObjectPath);
	UWorld* MainWorld = LoadObject<UWorld>(nullptr, MainMapObjectPath);
	UShopDefinition* Shop = LoadObject<UShopDefinition>(nullptr, ShopObjectPath);
	if (!TestNotNull(TEXT("Formal world shop Blueprint loads"), Blueprint)
		|| !TestNotNull(TEXT("L_Exploration loads"), MainWorld)
		|| !TestNotNull(TEXT("Authoring ShopDefinition loads"), Shop))
	{
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	if (!TestNotNull(TEXT("Blueprint generated class"),
			Blueprint->GeneratedClass.Get()))
	{
		return false;
	}

	const AWacomWorldShopActor* BlueprintDefaults =
		Cast<AWacomWorldShopActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Blueprint CDO is a formal world shop"),
			BlueprintDefaults))
	{
		return false;
	}

	if (UBlueprintGeneratedClass* BlueprintClass =
		Cast<UBlueprintGeneratedClass>(Blueprint->GeneratedClass.Get()))
	{
		if (const UInheritableComponentHandler* Handler =
			BlueprintClass->GetInheritableComponentHandler(false))
		{
			TArray<UActorComponent*> OverrideTemplates;
			Handler->GetAllTemplates(
				OverrideTemplates,
				/*bIncludeTransientTemplates*/ true);
			for (const UActorComponent* Template : OverrideTemplates)
			{
				if (!Template
					|| !Template->GetName().StartsWith(
						TEXT("OfferLayoutAnchor_")))
				{
					continue;
				}

				TestFalse(
					*FString::Printf(
						TEXT("Inherited template %s has no stale Primitive/BodyInstance data"),
						*Template->GetName()),
					Template->IsA<UPrimitiveComponent>());
			}
		}
	}

	const auto VerifyRunLiveProfile =
		[this](
			const TCHAR* Label,
			const bool bOverride)
		{
			TestFalse(
				*FString::Printf(
					TEXT("%s inherits the current Run live look profile"),
					Label),
				bOverride);
		};

	VerifyRunLiveProfile(
		TEXT("Blueprint CDO"),
		BlueprintDefaults->bOverrideCursorLookProfile);
	TestTrue(TEXT("Blueprint uses the enlarged 0.13 card scale"),
		FMath::IsNearlyEqual(
			BlueprintDefaults->CardWorldScale,
			0.13f));
	TestEqual(TEXT("Blueprint inherits eight visual layout anchors"),
		BlueprintDefaults->GetOfferLayoutAnchorsSorted().Num(), 8);
	for (const UWacomWorldShopLayoutAnchorComponent* Anchor :
		BlueprintDefaults->GetOfferLayoutAnchorsSorted())
	{
		TestTrue(TEXT("Blueprint layout preview matches 0.13 card size"),
			Anchor
				&& Anchor->GetCardPreviewSizeCm().Equals(
					FVector2D(93.6f, 126.88f),
					0.01f));
	}
	TestTrue(TEXT("Blueprint keeps the 220 cm close browse preset"),
		FMath::IsNearlyEqual(
			BlueprintDefaults->CloseBrowsePresetDistanceCm,
			220.0f));

	TArray<AWacomWorldShopActor*> FormalShops;
	for (AActor* Actor : MainWorld->PersistentLevel->Actors)
	{
		if (AWacomWorldShopActor* FormalShop =
			Cast<AWacomWorldShopActor>(Actor))
		{
			FormalShops.Add(FormalShop);
		}
	}
	TestEqual(TEXT("L_Exploration owns exactly one formal world shop"),
		FormalShops.Num(), 1);
	if (FormalShops.Num() != 1)
	{
		return false;
	}

	AWacomWorldShopActor* FormalShop = FormalShops[0];
	TestTrue(TEXT("Map instance uses exact BP_WacomWorldShop class"),
		FormalShop->GetClass() == Blueprint->GeneratedClass);
	TestEqual(TEXT("Map instance preserves PersistentId"),
		FormalShop->PersistentId,
		FName(TEXT("Shop.Test.001")));
	TestTrue(TEXT("Map instance preserves ShopDefinition"),
		FormalShop->ShopDefinition == Shop);
	TestEqual(TEXT("Map instance preserves its authority label"),
		FormalShop->GetActorLabel(),
		FString(TEXT("BP_WacomShopTriggerActor")));
	TestTrue(TEXT("Map instance preserves its authority location"),
		FormalShop->GetActorLocation().Equals(
			FVector(-440.0f, 140.0f, 120.0f),
			0.001f));
	TestTrue(TEXT("Map instance preserves its authority rotation"),
		FormalShop->GetActorRotation().Equals(
			FRotator(0.0f, 155.0f, 0.0f),
			0.001f));
	TestTrue(TEXT("Map instance preserves its authority scale"),
		FormalShop->GetActorScale3D().Equals(
			FVector(1.0f, 1.222f, 1.0f),
			0.001f));
	TestTrue(TEXT("Map instance preserves its 200 cm trigger radius"),
		FMath::IsNearlyEqual(FormalShop->TriggerRadius, 200.0f));

	const UWacomRunMapNodeBindingComponent* Binding =
		FormalShop->GetRunMapNodeBindingComponent();
	TestTrue(TEXT("Map instance preserves Shop.Snake binding"),
		Binding && Binding->NodeId == FName(TEXT("Shop.Snake")));
	TestTrue(TEXT("Map instance binding remains Shop"),
		Binding && Binding->NodeType == EWacomMapNodeType::Shop);
	VerifyRunLiveProfile(
		TEXT("Map instance"),
		FormalShop->bOverrideCursorLookProfile);
	TestTrue(TEXT("Map instance inherits the enlarged card scale"),
		FMath::IsNearlyEqual(FormalShop->CardWorldScale, 0.13f));

	const TArray<UWacomWorldShopLayoutAnchorComponent*> BlueprintAnchors =
		BlueprintDefaults->GetOfferLayoutAnchorsSorted();
	const TArray<UWacomWorldShopLayoutAnchorComponent*> MapAnchors =
		FormalShop->GetOfferLayoutAnchorsSorted();
	TestEqual(
		TEXT("Blueprint CDO and map instance expose the same anchor count"),
		MapAnchors.Num(),
		BlueprintAnchors.Num());
	for (int32 Index = 0;
		Index < FMath::Min(BlueprintAnchors.Num(), MapAnchors.Num());
		++Index)
	{
		const UWacomWorldShopLayoutAnchorComponent* BlueprintAnchor =
			BlueprintAnchors[Index];
		const UWacomWorldShopLayoutAnchorComponent* MapAnchor =
			MapAnchors[Index];
		if (!TestNotNull(
				*FString::Printf(
					TEXT("Blueprint anchor %02d exists"),
					Index + 1),
				BlueprintAnchor)
			|| !TestNotNull(
				*FString::Printf(
					TEXT("Map anchor %02d exists"),
					Index + 1),
				MapAnchor))
		{
			continue;
		}

		const FTransform BlueprintLayout =
			BlueprintAnchor->GetRelativeTransform();
		const FTransform MapLayout =
			MapAnchor->GetRelativeTransform();
		TestTrue(
			*FString::Printf(
				TEXT("Map anchor %02d inherits the Blueprint-authored layout"),
				Index + 1),
			MapLayout.Equals(BlueprintLayout, 0.01f));
	}

	UChildActorComponent* ViewpointComponent =
		FormalShop->GetShopEntryViewpointComponent();
	USceneComponent* FocusComponent =
		FormalShop->GetShopFocusAnchorComponent();
	const FWacomWorldShopPresentationHost PresentationHost =
		FormalShop->BuildPresentationHost();
	TestTrue(TEXT("Map instance is its own formal presentation host"),
		PresentationHost.IsOwnedBy(FormalShop));
	TestEqual(TEXT("Map instance exposes eight actual offer anchors"),
		PresentationHost.GetEnabledOfferAnchorsSorted().Num(), 8);
	TestTrue(TEXT("Map instance formal host validates for eight offers"),
		PresentationHost.ValidateForOfferCount(8).bValid);
	if (!TestNotNull(TEXT("Map instance keeps its Viewpoint component"),
			ViewpointComponent)
		|| !TestNotNull(TEXT("Map instance keeps its Focus component"),
			FocusComponent))
	{
		return false;
	}

	const float ViewpointDistanceCm = FVector::Distance(
		ViewpointComponent->GetComponentLocation(),
		FocusComponent->GetComponentLocation());
	const FVector FocusDirection =
		(FocusComponent->GetComponentLocation()
			- ViewpointComponent->GetComponentLocation()).GetSafeNormal();
	const float FocusAlignment = FVector::DotProduct(
		ViewpointComponent->GetForwardVector(),
		FocusDirection);
	TestTrue(TEXT("Map composition remains inside supported tuning range"),
		ViewpointDistanceCm >= 180.0f && ViewpointDistanceCm <= 320.0f);
	TestTrue(TEXT("Map Viewpoint remains aligned to ShopFocus"),
		FocusAlignment > 0.995f);

	AddInfo(FString::Printf(
		TEXT("Formal world shop asset audit: Label=%s Location=%s Rotation=%s Scale=%s TriggerRadius=%.2fcm ViewpointDistance=%.2fcm FocusAlignment=%.5f"),
		*FormalShop->GetActorLabel(),
		*FormalShop->GetActorLocation().ToString(),
		*FormalShop->GetActorRotation().ToString(),
		*FormalShop->GetActorScale3D().ToString(),
		FormalShop->TriggerRadius,
		ViewpointDistanceCm,
		FocusAlignment));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFormalDefinitionAndBlueprintContractSpec,
	"Wacom.UI.WorldShop.FormalAsset.DefinitionAndBlueprintContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFormalDefinitionAndBlueprintContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopFormalAssetSpec;

	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
			TEXT("AssetRegistry"));
	IAssetRegistry& AssetRegistry = AssetRegistryModule.Get();
	const TArray<TPair<FName, FTopLevelAssetPath>> AssetContracts =
	{
		{
			FName(TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomWorldShop")),
			UBlueprint::StaticClass()->GetClassPathName()
		},
		{
			FName(TEXT("/Game/Wacom/Data/Shops/DA_Shop_LevelAuthoringSnake")),
			UShopDefinition::StaticClass()->GetClassPathName()
		}
	};
	for (const TPair<FName, FTopLevelAssetPath>& Contract : AssetContracts)
	{
		TArray<FAssetData> Assets;
		AssetRegistry.GetAssetsByPackageName(Contract.Key, Assets);
		TestEqual(
			*FString::Printf(TEXT("One AssetRegistry entry for %s"),
				*Contract.Key.ToString()),
			Assets.Num(),
			1);
		if (Assets.Num() == 1)
		{
			TestEqual(
				*FString::Printf(TEXT("Exact asset class for %s"),
					*Contract.Key.ToString()),
				Assets[0].AssetClassPath,
				Contract.Value);
		}
	}

	UBlueprint* Blueprint = LoadObject<UBlueprint>(nullptr, BlueprintObjectPath);
	UShopDefinition* Shop = LoadObject<UShopDefinition>(nullptr, ShopObjectPath);
	UWacomFloorMapDefinition* Floor =
		LoadObject<UWacomFloorMapDefinition>(nullptr, FloorObjectPath);
	if (!TestNotNull(TEXT("Formal world shop Blueprint loads"), Blueprint)
		|| !TestNotNull(TEXT("Authoring ShopDefinition loads"), Shop)
		|| !TestNotNull(TEXT("Authoring Floor loads"), Floor))
	{
		return false;
	}

	FKismetEditorUtilities::CompileBlueprint(Blueprint);
	TestTrue(TEXT("Formal world shop Blueprint compiles"),
		Blueprint->Status != BS_Error);
	TestTrue(TEXT("Blueprint parent is exact formal C++ actor"),
		Blueprint->ParentClass == AWacomWorldShopActor::StaticClass());
	TestNotNull(TEXT("Blueprint generated class"),
		Blueprint->GeneratedClass.Get());

	int32 EventGraphNodeCount = 0;
	bool bOnlyUnwiredDefaultEvents = true;
	for (const UEdGraph* Graph : Blueprint->UbergraphPages)
	{
		if (!Graph)
		{
			continue;
		}
		EventGraphNodeCount += Graph->Nodes.Num();
		for (const UEdGraphNode* Node : Graph->Nodes)
		{
			if (!Node)
			{
				bOnlyUnwiredDefaultEvents = false;
				continue;
			}
			bOnlyUnwiredDefaultEvents &=
				Node->GetClass()->GetFName() == TEXT("K2Node_Event");
			for (const UEdGraphPin* Pin : Node->Pins)
			{
				bOnlyUnwiredDefaultEvents &= Pin && Pin->LinkedTo.IsEmpty();
			}
		}
	}
	TestEqual(TEXT("Blueprint keeps only the three generated event stubs"),
		EventGraphNodeCount, 3);
	TestTrue(TEXT("Generated event stubs have no business wiring"),
		bOnlyUnwiredDefaultEvents);
	TestNotNull(TEXT("Blueprint owns an SCS"),
		Blueprint->SimpleConstructionScript.Get());
	if (Blueprint->SimpleConstructionScript)
	{
		TestEqual(TEXT("Blueprint owns exactly two presentation components"),
			Blueprint->SimpleConstructionScript->GetAllNodes().Num(), 2);
	}

	const USCS_Node* BackboardNode =
		FindPresentationNode(*Blueprint, TEXT("Backboard"));
	const USCS_Node* CounterNode =
		FindPresentationNode(*Blueprint, TEXT("Counter"));
	TestTrue(TEXT("Backboard attaches to native PresentationRoot"),
		BackboardNode
			&& BackboardNode->bIsParentComponentNative
			&& BackboardNode->ParentComponentOrVariableName
				== TEXT("PresentationRoot"));
	TestTrue(TEXT("Counter attaches to native PresentationRoot"),
		CounterNode
			&& CounterNode->bIsParentComponentNative
			&& CounterNode->ParentComponentOrVariableName
				== TEXT("PresentationRoot"));
	VerifyPresentationMesh(
		*this,
		TEXT("Backboard"),
		FindPresentationMesh(*Blueprint, TEXT("Backboard")),
		FVector(-35.0, 0.0, 35.0),
		FVector(0.2, 4.0, 2.8));
	VerifyPresentationMesh(
		*this,
		TEXT("Counter"),
		FindPresentationMesh(*Blueprint, TEXT("Counter")),
		FVector(25.0, 0.0, -115.0),
		FVector(1.0, 4.6, 0.8));

	TestEqual(TEXT("Authoring shop identity"), Shop->ShopId,
		FName(TEXT("Shop.LevelAuthoringSnake")));
	TestEqual(TEXT("Authoring shop display name"),
		Shop->DisplayName.ToString(), FString(TEXT("行商营帐")));
	TestEqual(TEXT("Authoring shop has eight purchase offers"),
		Shop->Offers.Num(), ExpectedOffers().Num());
	for (int32 Index = 0;
		Index < FMath::Min(Shop->Offers.Num(), ExpectedOffers().Num());
		++Index)
	{
		const FExpectedOffer& Expected = ExpectedOffers()[Index];
		UCardDefinition* ExpectedCard =
			LoadObject<UCardDefinition>(nullptr, Expected.CardObjectPath);
		TestNotNull(
			*FString::Printf(TEXT("Offer %d card loads"), Index + 1),
			ExpectedCard);
		TestTrue(
			*FString::Printf(TEXT("Offer %d exact card"), Index + 1),
			Shop->Offers[Index].CardDefinition == ExpectedCard);
		TestEqual(
			*FString::Printf(TEXT("Offer %d exact price"), Index + 1),
			Shop->Offers[Index].Price,
			Expected.Price);
	}
	TestFalse(TEXT("Authoring shop upgrade service remains disabled"),
		Shop->CardUpgradeService.bEnabled);
	TestEqual(TEXT("Disabled upgrade service has no price rows"),
		Shop->CardUpgradeService.Prices.Num(), 0);

	const FWacomMapNodeDefinition* ShopNode =
		Floor->FindNode(TEXT("Shop.Snake"));
	TestNotNull(TEXT("Authoring Floor still owns Shop.Snake"), ShopNode);
	TestTrue(TEXT("Shop.Snake keeps Shop node type"),
		ShopNode && ShopNode->NodeType == EWacomMapNodeType::Shop);
	TestTrue(TEXT("Shop.Snake references the new authoring shop"),
		ShopNode && ShopNode->Content.Shop.ShopDefinition == Shop);
	return true;
}

#endif
