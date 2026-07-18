// Copyright Wacom. All Rights Reserved.

#include "Testing/WacomFormalFloor1ContentAutomationTestView.h"

#include "Cards/CardDefinition.h"
#include "ContentBuilders/FormalFloor1ContentBuilder.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"

namespace
{
	FString GroupToString(
		const Wacom::ContentBuilder::EFormalFloor1ContentGroup Group)
	{
		switch (Group)
		{
		case Wacom::ContentBuilder::EFormalFloor1ContentGroup::Cards:
			return TEXT("Cards");
		case Wacom::ContentBuilder::EFormalFloor1ContentGroup::EnemyGraph:
			return TEXT("EnemyGraph");
		case Wacom::ContentBuilder::EFormalFloor1ContentGroup::NodeDefinitions:
			return TEXT("NodeDefinitions");
		case Wacom::ContentBuilder::EFormalFloor1ContentGroup::All:
		default:
			return TEXT("All");
		}
	}
}

FWacomFormalFloor1ContentAutomationSummary
FWacomFormalFloor1ContentAutomationTestView::GetManifestSummary()
{
	using namespace Wacom::ContentBuilder;
	FWacomFormalFloor1ContentAutomationSummary Summary;
	for (const FFormalFloor1ContentManifestEntry& Entry :
		GetFormalFloor1ContentManifest())
	{
		++Summary.TotalCount;
		Summary.PackagePaths.Add(Entry.PackagePath);
		Summary.StableIds.Add(Entry.StableId);
		if (Entry.AssetClass == UCardDefinition::StaticClass()) { ++Summary.CardCount; }
		else if (Entry.AssetClass == UEnemyBehaviorDefinition::StaticClass()) { ++Summary.BehaviorCount; }
		else if (Entry.AssetClass == UEnemyPartDefinition::StaticClass()) { ++Summary.PartCount; }
		else if (Entry.AssetClass == UEnemyDefinition::StaticClass()) { ++Summary.EnemyCount; }
		else if (Entry.AssetClass == UEncounterDefinition::StaticClass()) { ++Summary.EncounterCount; }
		else if (Entry.AssetClass == UWacomRunEventDefinition::StaticClass()) { ++Summary.EventCount; }
		else if (Entry.AssetClass == UWacomRunPickupDefinition::StaticClass()) { ++Summary.PickupCount; }
		else if (Entry.AssetClass == UShopDefinition::StaticClass()) { ++Summary.ShopCount; }

		switch (Entry.Group)
		{
		case EFormalFloor1ContentGroup::Cards: ++Summary.CardsGroupCount; break;
		case EFormalFloor1ContentGroup::EnemyGraph: ++Summary.EnemyGraphGroupCount; break;
		case EFormalFloor1ContentGroup::NodeDefinitions: ++Summary.NodeDefinitionsGroupCount; break;
		case EFormalFloor1ContentGroup::All: break;
		}
	}
	return Summary;
}

bool FWacomFormalFloor1ContentAutomationTestView::ValidateManifest(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor1ContentManifest(OutErrors);
}

bool FWacomFormalFloor1ContentAutomationTestView::ValidateTransientDefaults(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor1TransientDefaults(OutErrors);
}

bool FWacomFormalFloor1ContentAutomationTestView::ValidateComparatorBoundaries(
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor1ComparatorBoundaries(OutErrors);
}

bool FWacomFormalFloor1ContentAutomationTestView::ValidateLoadedAssets(
	const bool bCompareSeedDefaults,
	TArray<FString>& OutErrors)
{
	return Wacom::ContentBuilder::ValidateFormalFloor1LoadedAssets(
		bCompareSeedDefaults, OutErrors);
}

bool FWacomFormalFloor1ContentAutomationTestView::ParseArguments(
	const TArray<FString>& Arguments,
	FString& OutCanonicalGroup,
	bool& bOutSeedMissing,
	bool& bOutCompareSeedDefaults,
	FString& OutReportPath,
	TArray<FString>& OutErrors)
{
	Wacom::ContentBuilder::FFormalFloor1ContentOptions Options;
	const bool bParsed = Wacom::ContentBuilder::ParseFormalFloor1ContentOptions(
		Arguments, Options, OutErrors);
	OutCanonicalGroup = GroupToString(Options.Group);
	bOutSeedMissing = Options.bSeedMissing;
	bOutCompareSeedDefaults = Options.bCompareSeedDefaults;
	OutReportPath = Options.ReportPath;
	return bParsed;
}
