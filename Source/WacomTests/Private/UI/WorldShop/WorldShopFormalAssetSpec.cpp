// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomWorldShopActor.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Cards/CardDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "EdGraph/EdGraph.h"
#include "EdGraph/EdGraphNode.h"
#include "EdGraph/EdGraphPin.h"
#include "Engine/Blueprint.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/StaticMesh.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomMapTypes.h"
#include "Shops/ShopDefinition.h"

namespace WacomWorldShopFormalAssetSpec
{
	constexpr const TCHAR* BlueprintObjectPath =
		TEXT("/Game/Wacom/Maps/SceneActor/BP_WacomWorldShop.BP_WacomWorldShop");
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
