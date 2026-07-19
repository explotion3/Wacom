// Copyright Wacom. All Rights Reserved.

#include "Commandlets/WacomBuildEnemyPackCommandlet.h"

#include "ContentBuilders/EnemyPackArtPromotion.h"
#include "ContentBuilders/SnakeBuilder.h"
#include "ContentBuilders/SlimeTrioBuilder.h"
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
	const bool bPromotePlaceholderArt =
		FParse::Param(*Params, TEXT("PromotePlaceholderArt"));
	const bool bForceArtRefresh =
		FParse::Param(*Params, TEXT("ForceArtRefresh"));
	const bool bTrainingWarrior =
		PackName.Equals(TEXT("TrainingWarrior"), ESearchCase::IgnoreCase);
	const bool bSnake = PackName.Equals(TEXT("Snake"), ESearchCase::IgnoreCase);
	const bool bSlimeTrio =
		PackName.Equals(TEXT("SlimeTrio"), ESearchCase::IgnoreCase);
	if (!bTrainingWarrior && !bSnake && !bSlimeTrio)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] Unsupported or missing -Pack. Expected TrainingWarrior, Snake, or SlimeTrio"));
		return 1;
	}
	if (bTrainingWarrior && bPromotePlaceholderArt)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] TrainingWarrior does not accept -PromotePlaceholderArt"));
		return 2;
	}
	if ((bSnake || bSlimeTrio) && bPromoteArt)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] %s formal art is not available; use -PromotePlaceholderArt for the release-blocked Slime proxy"),
			*PackName);
		return 2;
	}
	const bool bHasMatchingPromotionFlag = bTrainingWarrior
		? bPromoteArt
		: bPromotePlaceholderArt;
	if (bForceArtRefresh && !bHasMatchingPromotionFlag)
	{
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] -ForceArtRefresh requires this pack's promotion flag"));
		return 2;
	}

	Wacom::ContentBuilder::FEnemyPackArtPromotionResult ArtResult;
	if (bTrainingWarrior)
	{
		ArtResult = bPromoteArt
			? Wacom::ContentBuilder::PromoteTrainingWarriorArt(bForceArtRefresh)
			: Wacom::ContentBuilder::ValidateTrainingWarriorFormalArt();
	}
	else if (bSnake)
	{
		ArtResult = bPromotePlaceholderArt
			? Wacom::ContentBuilder::PromoteSnakePlaceholderArt(bForceArtRefresh)
			: Wacom::ContentBuilder::ValidateSnakePlaceholderArt();
	}
	else
	{
		ArtResult = bPromotePlaceholderArt
			? Wacom::ContentBuilder::PromoteSlimeTrioPlaceholderArt(
				bForceArtRefresh)
			: Wacom::ContentBuilder::ValidateSlimeTrioPlaceholderArt();
	}
	if (!ArtResult.bSucceeded)
	{
		for (const FString& Error : ArtResult.Errors)
		{
			UE_LOG(LogTemp, Error, TEXT("[WacomBuildEnemyPack][Art] %s"), *Error);
		}
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] Pack art is incomplete. Use the matching explicit promotion flag with the authorized local source available."));
		return 3;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildEnemyPack] Pack art valid (%d assets, copied=%s)"),
		ArtResult.ExpectedAssetCount,
		ArtResult.bCopiedAssets ? TEXT("true") : TEXT("false"));

	if (bTrainingWarrior)
	{
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

	if (bSnake)
	{
		Wacom::ContentBuilder::FSnakeBuildResult BuildResult =
			Wacom::ContentBuilder::BuildSnakeContent();
		if (!BuildResult.IsSuccess())
		{
			for (const FString& Error : BuildResult.Errors)
			{
				UE_LOG(LogTemp, Error,
					TEXT("[WacomBuildEnemyPack][Content] %s"), *Error);
			}
			UE_LOG(LogTemp, Error,
				TEXT("[WacomBuildEnemyPack] Snake content build failed"));
			return 4;
		}
		UE_LOG(LogTemp, Display,
			TEXT("[WacomBuildEnemyPack] Snake built successfully (changed=%s, placeholder=true)"),
			BuildResult.bChanged ? TEXT("true") : TEXT("false"));
		return 0;
	}

	Wacom::ContentBuilder::FSlimeTrioBuildResult BuildResult =
		Wacom::ContentBuilder::BuildSlimeTrioContent();
	if (!BuildResult.IsSuccess())
	{
		for (const FString& Error : BuildResult.Errors)
		{
			UE_LOG(LogTemp, Error,
				TEXT("[WacomBuildEnemyPack][Content] %s"), *Error);
		}
		UE_LOG(LogTemp, Error,
			TEXT("[WacomBuildEnemyPack] SlimeTrio content build failed"));
		return 4;
	}
	UE_LOG(LogTemp, Display,
		TEXT("[WacomBuildEnemyPack] SlimeTrio built successfully (changed=%s, placeholder=true)"),
		BuildResult.bChanged ? TEXT("true") : TEXT("false"));
	return 0;
}
