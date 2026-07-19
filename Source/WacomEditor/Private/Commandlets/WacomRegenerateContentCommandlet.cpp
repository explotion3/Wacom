// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomRegenerateContentCommandlet.h"
#include "ContentBuilders/BugGirlBuilder.h"
#include "ContentBuilders/RunPickupBlueprintBuilder.h"
#include "ContentBuilders/RunPickupDefinitionBuilder.h"
#include "ContentBuilders/RunWorldCardInteractionDefinitionBuilder.h"
#include "ContentBuilders/RunEventBuilder.h"
#include "ContentBuilders/ShopBuilder.h"
#include "ContentBuilders/SnakeBuilder.h"
#include "ContentBuilders/SlimeTrioBuilder.h"
#include "ContentBuilders/TrainingWarriorBuilder.h"

#include "Characters/CharacterDefinition.h"
#include "Encounters/EncounterDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Engine/Blueprint.h"
#include "Interactions/RunWorldCardInteractionDefinition.h"
#include "Pickups/RunPickupDefinition.h"
#include "Shops/ShopDefinition.h"

UWacomRegenerateContentCommandlet::UWacomRegenerateContentCommandlet()
{
	IsClient  = false;
	IsServer  = false;
	IsEditor  = true;
	LogToConsole = true;
}

int32 UWacomRegenerateContentCommandlet::Main(const FString& /*Params*/)
{
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Start"));

	const Wacom::ContentBuilder::FSnakeBuildResult Snake =
		Wacom::ContentBuilder::BuildSnakeContent();
	if (!Snake.IsSuccess())
	{
		for (const FString& Error : Snake.Errors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomRegenerateContent] Snake: %s"), *Error);
		}
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildSnakeContent failed"));
		return 1;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRegenerateContent] Snake built (changed=%s)"),
		Snake.bChanged ? TEXT("true") : TEXT("false"));

	const Wacom::ContentBuilder::FSlimeTrioBuildResult SlimeTrio =
		Wacom::ContentBuilder::BuildSlimeTrioContent();
	if (!SlimeTrio.IsSuccess())
	{
		for (const FString& Error : SlimeTrio.Errors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomRegenerateContent] SlimeTrio: %s"), *Error);
		}
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRegenerateContent] BuildSlimeTrioContent failed"));
		return 10;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRegenerateContent] SlimeTrio built (changed=%s)"),
		SlimeTrio.bChanged ? TEXT("true") : TEXT("false"));

	const Wacom::ContentBuilder::FTrainingWarriorBuildResult TrainingWarrior =
		Wacom::ContentBuilder::BuildTrainingWarriorContent();
	if (!TrainingWarrior.IsSuccess())
	{
		for (const FString& Error : TrainingWarrior.Errors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomRegenerateContent] TrainingWarrior: %s"), *Error);
		}
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRegenerateContent] BuildTrainingWarriorContent failed"));
		return 9;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRegenerateContent] TrainingWarrior built (changed=%s)"),
		TrainingWarrior.bChanged ? TEXT("true") : TEXT("false"));

	UCharacterDefinition* BugGirl = Wacom::ContentBuilder::BuildBugGirlContent();
	if (!BugGirl)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildBugGirlContent failed"));
		return 2;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] BugGirl built"));

	UShopDefinition* DebugShop = Wacom::ContentBuilder::BuildShopContent();
	if (!DebugShop)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildShopContent failed"));
		return 3;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Shops built"));

	UWacomRunEventDefinition* DebugEvent = Wacom::ContentBuilder::BuildRunEventContent();
	if (!DebugEvent)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildRunEventContent failed"));
		return 4;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Run events built"));

	UWacomRunPickupDefinition* DebugPickup = Wacom::ContentBuilder::BuildRunPickupDefinitionContent();
	if (!DebugPickup)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildRunPickupDefinitionContent failed"));
		return 5;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Pickup definitions built"));

	UWacomRunWorldCardInteractionDefinition* DebugWorldCardInteraction =
		Wacom::ContentBuilder::BuildRunWorldCardInteractionDefinitionContent();
	if (!DebugWorldCardInteraction)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomRegenerateContent] BuildRunWorldCardInteractionDefinitionContent failed"));
		return 6;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomRegenerateContent] Run world card interaction definitions built"));

	UBlueprint* RewardPickupBlueprint = Wacom::ContentBuilder::BuildRunPickupBlueprintContent();
	if (!RewardPickupBlueprint)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildRunPickupBlueprintContent failed"));
		return 7;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Pickup Blueprints built"));

	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Done"));
	return 0;
}
