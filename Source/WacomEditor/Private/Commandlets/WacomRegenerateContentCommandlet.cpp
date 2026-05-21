// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomRegenerateContentCommandlet.h"
#include "ContentBuilders/BugGirlBuilder.h"
#include "ContentBuilders/ShopBuilder.h"
#include "ContentBuilders/SnakeBuilder.h"

#include "Characters/CharacterDefinition.h"
#include "Enemies/EnemyDefinition.h"
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

	UEnemyDefinition* Snake = Wacom::ContentBuilder::BuildSnakeContent();
	if (!Snake)
	{
		UE_LOG(LogTemp, Error, TEXT("[WacomRegenerateContent] BuildSnakeContent failed"));
		return 1;
	}
	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Snake built"));

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

	UE_LOG(LogTemp, Display, TEXT("[WacomRegenerateContent] Done"));
	return 0;
}
