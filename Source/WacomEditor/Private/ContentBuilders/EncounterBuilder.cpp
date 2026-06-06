// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EncounterBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"

namespace Wacom::ContentBuilder
{
	UEncounterDefinition* BuildEncounterContent(UEnemyDefinition* SnakeEnemy)
	{
		if (!SnakeEnemy)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[EncounterBuilder] Snake enemy is required to build DA_Encounter_SnakeSingle"));
			return nullptr;
		}

		const FString PackagePath = MakePackagePath(EncountersRoot(), TEXT("DA_Encounter_SnakeSingle"));
		UPackage* Package = FindOrCreatePackage(PackagePath);
		if (!Package)
		{
			return nullptr;
		}

		UEncounterDefinition* Encounter =
			CreateOrReplaceAsset<UEncounterDefinition>(Package, TEXT("DA_Encounter_SnakeSingle"));
		if (!Encounter)
		{
			return nullptr;
		}

		Encounter->EncounterDefinitionId = TEXT("Encounter.Snake.Single");
		Encounter->DisplayName = FText::FromString(TEXT("Snake"));

		FEncounterEnemySlot EnemySlot;
		EnemySlot.EnemySlotId = TEXT("Enemy");
		EnemySlot.EnemyDefinition = SnakeEnemy;
		Encounter->EnemySlots = { EnemySlot };

		SaveAssetPackage(Package, Encounter, PackagePath);
		return Encounter;
	}
}
