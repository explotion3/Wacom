// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildEnemyPackCommandlet.h"

#include "ContentBuilders/EnemyPackArtPromotion.h"
#include "ContentBuilders/TrainingWarriorBuilder.h"
#include "Misc/Parse.h"

UWacomBuildEnemyPackCommandlet::UWacomBuildEnemyPackCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UWacomBuildEnemyPackCommandlet::Main(const FString& Params)
{
	FString PackName;
	FParse::Value(*Params, TEXT("Pack="), PackName);
	const bool bPromoteArt = FParse::Param(*Params, TEXT("PromoteArt"));
	const bool bForceArtRefresh =
		FParse::Param(*Params, TEXT("ForceArtRefresh"));
	if (!PackName.Equals(TEXT("TrainingWarrior"), ESearchCase::IgnoreCase))
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] Unsupported or missing -Pack. Expected -Pack=TrainingWarrior"));
		return 1;
	}
	if (bForceArtRefresh && !bPromoteArt)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] -ForceArtRefresh requires -PromoteArt"));
		return 2;
	}

	Wacom::ContentBuilder::FEnemyPackArtPromotionResult ArtResult =
		bPromoteArt
			? Wacom::ContentBuilder::PromoteTrainingWarriorArt(bForceArtRefresh)
			: Wacom::ContentBuilder::ValidateTrainingWarriorFormalArt();
	if (!ArtResult.bSucceeded)
	{
		for (const FString& Error : ArtResult.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyPack][Art] %s"), *Error);
		}
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] Formal art is incomplete. Use -PromoteArt with the authorized local source available."));
		return 3;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildEnemyPack] Formal art valid (%d assets, copied=%s)"),
		ArtResult.ExpectedAssetCount,
		ArtResult.bCopiedAssets ? TEXT("true") : TEXT("false"));

	Wacom::ContentBuilder::FTrainingWarriorBuildResult BuildResult =
		Wacom::ContentBuilder::BuildTrainingWarriorContent();
	if (!BuildResult.IsSuccess())
	{
		for (const FString& Error : BuildResult.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyPack][Content] %s"), *Error);
		}
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] TrainingWarrior content build failed"));
		return 4;
	}

	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildEnemyPack] TrainingWarrior built successfully (changed=%s)"),
		BuildResult.bChanged ? TEXT("true") : TEXT("false"));
	return 0;
}
