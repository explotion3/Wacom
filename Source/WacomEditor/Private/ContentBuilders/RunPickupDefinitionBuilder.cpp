// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunPickupDefinitionBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Pickups/RunPickupDefinition.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	UCardDefinition* LoadGeneratedCard(const FString& ObjectPath)
	{
		UCardDefinition* Card = LoadObject<UCardDefinition>(nullptr, *ObjectPath);
		if (!Card)
		{
			UE_LOG(LogTemp, Error, TEXT("[RunPickupDefinitionBuilder] Failed to load card asset: %s"), *ObjectPath);
		}
		return Card;
	}

	UWacomRunPickupDefinition* BuildDebugGoldPickupDefinition()
	{
		const FString PackagePath = MakePackagePath(PickupsRoot(), TEXT("DA_Pickup_DebugGold3"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UWacomRunPickupDefinition* Definition =
			CreateOrReplaceAsset<UWacomRunPickupDefinition>(Pkg, TEXT("DA_Pickup_DebugGold3"));
		if (!Definition) { return nullptr; }

		Definition->PickupId = TEXT("Pickup.Debug.Gold3");
		Definition->RewardType = EWacomRunPickupRewardType::Gold;
		Definition->GoldAmount = 3;
		Definition->CardDefinition = nullptr;

		SaveAssetPackage(Pkg, Definition, PackagePath);
		return Definition;
	}

	UWacomRunPickupDefinition* BuildDebugPoisonFangPickupDefinition(UCardDefinition* PoisonFang)
	{
		const FString PackagePath = MakePackagePath(PickupsRoot(), TEXT("DA_Pickup_DebugPoisonFang"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UWacomRunPickupDefinition* Definition =
			CreateOrReplaceAsset<UWacomRunPickupDefinition>(Pkg, TEXT("DA_Pickup_DebugPoisonFang"));
		if (!Definition) { return nullptr; }

		Definition->PickupId = TEXT("Pickup.Debug.PoisonFang");
		Definition->RewardType = EWacomRunPickupRewardType::Card;
		Definition->GoldAmount = 1;
		Definition->CardDefinition = PoisonFang;

		SaveAssetPackage(Pkg, Definition, PackagePath);
		return Definition;
	}
}

namespace Wacom::ContentBuilder
{
	UWacomRunPickupDefinition* BuildRunPickupDefinitionContent()
	{
		UCardDefinition* PoisonFang = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(RewardCardsRoot(), TEXT("DA_Card_PoisonFang"))));
		if (!PoisonFang)
		{
			return nullptr;
		}

		UWacomRunPickupDefinition* Gold = BuildDebugGoldPickupDefinition();
		UWacomRunPickupDefinition* PoisonFangPickup =
			BuildDebugPoisonFangPickupDefinition(PoisonFang);
		return (Gold && PoisonFangPickup) ? Gold : nullptr;
	}
}
