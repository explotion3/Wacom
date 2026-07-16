// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemyPackArtPromotion.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Engine/Texture2D.h"
#include "IAssetTools.h"
#include "Modules/ModuleManager.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"

namespace
{
	const FString SourceRoot = TEXT("/Game/Art/PaperAssets/Party/BattleWarrior");
	const FString FormalRoot = TEXT("/Game/Wacom/Art/Enemies/TrainingWarrior");

	struct FAnimationPromotionSpec
	{
		const TCHAR* SourceState;
		const TCHAR* FormalState;
		int32 FrameCount;
	};

	const TArray<FAnimationPromotionSpec>& GetAnimationSpecs()
	{
		static const TArray<FAnimationPromotionSpec> Specs = {
			{ TEXT("Idle"), TEXT("Idle"), 6 },
			{ TEXT("Attack"), TEXT("Attack"), 5 },
			{ TEXT("Block"), TEXT("Block"), 4 },
			{ TEXT("Cleave"), TEXT("Cleave"), 11 },
			{ TEXT("Downed"), TEXT("Destroyed"), 4 },
		};
		return Specs;
	}

	bool IsPackageUnderRoot(const FString& PackageName, const FString& Root)
	{
		return PackageName == Root || PackageName.StartsWith(Root + TEXT("/"));
	}

	FString MakeObjectPath(const FString& PackageName)
	{
		return PackageName + TEXT(".") + FPackageName::GetLongPackageAssetName(PackageName);
	}

	FString SourceFlipbookPackage(const TCHAR* State)
	{
		return FString::Printf(TEXT("%s/BattleWarrior__%s"), *SourceRoot, State);
	}

	FString FormalFlipbookPackage(const TCHAR* State)
	{
		return FString::Printf(
			TEXT("%s/Flipbooks/PF_Enemy_TrainingWarrior_%s"),
			*FormalRoot,
			State);
	}

	FString FormalSpritePackage(const TCHAR* State, int32 FrameIndex)
	{
		return FString::Printf(
			TEXT("%s/Sprites/SPR_Enemy_TrainingWarrior_%s_%02d"),
			*FormalRoot,
			State,
			FrameIndex);
	}

	void AddExpectedFormalPackages(
		TMap<FString, UClass*>& OutExpectedPackages)
	{
		OutExpectedPackages.Add(
			FormalRoot + TEXT("/Textures/T_Enemy_TrainingWarrior"),
			UTexture2D::StaticClass());
		for (const FAnimationPromotionSpec& Spec : GetAnimationSpecs())
		{
			OutExpectedPackages.Add(
				FormalFlipbookPackage(Spec.FormalState),
				UPaperFlipbook::StaticClass());
			for (int32 FrameIndex = 0; FrameIndex < Spec.FrameCount; ++FrameIndex)
			{
				OutExpectedPackages.Add(
					FormalSpritePackage(Spec.FormalState, FrameIndex),
					UPaperSprite::StaticClass());
			}
		}
	}

	UObject* LoadExactAsset(const FString& PackageName)
	{
		return StaticLoadObject(
			UObject::StaticClass(),
			nullptr,
			*MakeObjectPath(PackageName));
	}

	bool ValidateExpectedPackages(
		IAssetRegistry& AssetRegistry,
		const TMap<FString, UClass*>& ExpectedPackages,
		TArray<FString>& OutErrors)
	{
		bool bValid = true;
		for (const TPair<FString, UClass*>& Entry : ExpectedPackages)
		{
			UObject* Asset = LoadExactAsset(Entry.Key);
			if (!Asset)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Missing formal asset: %s"), *Entry.Key));
				bValid = false;
				continue;
			}
			if (!Asset->IsA(Entry.Value))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Formal asset has class %s, expected %s: %s"),
					*GetNameSafe(Asset->GetClass()),
					*GetNameSafe(Entry.Value),
					*Entry.Key));
				bValid = false;
			}
		}

		const TArray<FString> ForbiddenRoots = {
			TEXT("/Game/Art"),
			TEXT("/Game/Asset"),
			TEXT("/Game/DreamMaterials"),
		};
		for (const TPair<FString, UClass*>& Entry : ExpectedPackages)
		{
			TArray<FAssetDependency> Dependencies;
			AssetRegistry.GetDependencies(
				FAssetIdentifier(FName(*Entry.Key)),
				Dependencies,
				UE::AssetRegistry::EDependencyCategory::Package);
			for (const FAssetDependency& Dependency : Dependencies)
			{
				const FString DependencyName =
					Dependency.AssetId.PackageName.ToString();
				for (const FString& ForbiddenRoot : ForbiddenRoots)
				{
					if (IsPackageUnderRoot(DependencyName, ForbiddenRoot))
					{
						OutErrors.Add(FString::Printf(
							TEXT("Formal asset %s still depends on ignored package %s"),
							*Entry.Key,
							*DependencyName));
						bValid = false;
					}
				}
			}
		}
		return bValid;
	}

	bool BuildSourceClosure(
		IAssetRegistry& AssetRegistry,
		TSet<FName>& OutPackages,
		TArray<FString>& OutErrors)
	{
		TArray<FName> Queue;
		for (const FAnimationPromotionSpec& Spec : GetAnimationSpecs())
		{
			const FName PackageName(*SourceFlipbookPackage(Spec.SourceState));
			Queue.Add(PackageName);
			OutPackages.Add(PackageName);
		}

		for (int32 QueueIndex = 0; QueueIndex < Queue.Num(); ++QueueIndex)
		{
			const FName SourcePackage = Queue[QueueIndex];
			TArray<FAssetDependency> Dependencies;
			AssetRegistry.GetDependencies(
				FAssetIdentifier(SourcePackage),
				Dependencies,
				UE::AssetRegistry::EDependencyCategory::Package);
			for (const FAssetDependency& Dependency : Dependencies)
			{
				const FString TargetPackage =
					Dependency.AssetId.PackageName.ToString();
				if (!TargetPackage.StartsWith(TEXT("/Game/")))
				{
					continue;
				}
				if (!IsPackageUnderRoot(TargetPackage, SourceRoot))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Source dependency escapes BattleWarrior: %s -> %s"),
						*SourcePackage.ToString(),
						*TargetPackage));
					continue;
				}
				const FName TargetName(*TargetPackage);
				if (!OutPackages.Contains(TargetName))
				{
					OutPackages.Add(TargetName);
					Queue.Add(TargetName);
				}
			}
		}
		return OutErrors.IsEmpty();
	}

	const FAnimationPromotionSpec* FindSpecBySourceState(
		const FString& SourceState)
	{
		for (const FAnimationPromotionSpec& Spec : GetAnimationSpecs())
		{
			if (SourceState == Spec.SourceState)
			{
				return &Spec;
			}
		}
		return nullptr;
	}

	bool BuildCopyMap(
		const TSet<FName>& SourcePackages,
		TMap<FString, FString>& OutCopyMap,
		TArray<FString>& OutErrors)
	{
		const FString TexturePackage = SourceRoot + TEXT("/Textures/BattleWarrior");
		for (FName SourcePackageName : SourcePackages)
		{
			const FString SourcePackage = SourcePackageName.ToString();
			FString DestinationPackage;
			if (SourcePackage == TexturePackage)
			{
				DestinationPackage =
					FormalRoot + TEXT("/Textures/T_Enemy_TrainingWarrior");
			}
			else if (SourcePackage.StartsWith(SourceRoot + TEXT("/Frames/")))
			{
				FString AssetName = FPackageName::GetLongPackageAssetName(SourcePackage);
				if (!AssetName.RemoveFromStart(TEXT("BattleWarrior__"))
					|| !AssetName.RemoveFromEnd(TEXT("_aseprite")))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Unsupported BattleWarrior sprite name: %s"),
						*SourcePackage));
					continue;
				}
				FString SourceState;
				FString FrameText;
				if (!AssetName.Split(
					TEXT("_"),
					&SourceState,
					&FrameText,
					ESearchCase::CaseSensitive,
					ESearchDir::FromEnd))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Unsupported BattleWarrior sprite frame: %s"),
						*SourcePackage));
					continue;
				}
				const FAnimationPromotionSpec* Spec =
					FindSpecBySourceState(SourceState);
				if (!Spec || !FrameText.IsNumeric())
				{
					OutErrors.Add(FString::Printf(
						TEXT("Sprite is outside selected animation states: %s"),
						*SourcePackage));
					continue;
				}
				const int32 FrameIndex = FCString::Atoi(*FrameText);
				if (FrameIndex < 0 || FrameIndex >= Spec->FrameCount)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Sprite frame is outside expected range: %s"),
						*SourcePackage));
					continue;
				}
				DestinationPackage =
					FormalSpritePackage(Spec->FormalState, FrameIndex);
			}
			else
			{
				const FString AssetName = FPackageName::GetLongPackageAssetName(SourcePackage);
				FString SourceState = AssetName;
				if (!SourceState.RemoveFromStart(TEXT("BattleWarrior__")))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Unsupported BattleWarrior dependency: %s"),
						*SourcePackage));
					continue;
				}
				const FAnimationPromotionSpec* Spec =
					FindSpecBySourceState(SourceState);
				if (!Spec)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Flipbook is outside selected animation states: %s"),
						*SourcePackage));
					continue;
				}
				DestinationPackage = FormalFlipbookPackage(Spec->FormalState);
			}
			OutCopyMap.Add(SourcePackage, DestinationPackage);
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateSourceAssets(
		const TSet<FName>& SourcePackages,
		TArray<FString>& OutErrors)
	{
		for (FName PackageName : SourcePackages)
		{
			UObject* Asset = LoadExactAsset(PackageName.ToString());
			if (!Asset)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Missing source asset: %s"), *PackageName.ToString()));
				continue;
			}
			if (!Asset->IsA<UPaperFlipbook>()
				&& !Asset->IsA<UPaperSprite>()
				&& !Asset->IsA<UTexture2D>())
			{
				OutErrors.Add(FString::Printf(
					TEXT("Unsupported source asset class %s: %s"),
					*GetNameSafe(Asset->GetClass()),
					*PackageName.ToString()));
			}
		}
		return OutErrors.IsEmpty();
	}

	IAssetRegistry& GetAssetRegistry()
	{
		FAssetRegistryModule& Module =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& Registry = Module.Get();
		Registry.SearchAllAssets(/*bSynchronousSearch*/ true);
		return Registry;
	}
}

namespace Wacom::ContentBuilder
{
	FEnemyPackArtPromotionResult ValidateTrainingWarriorFormalArt()
	{
		FEnemyPackArtPromotionResult Result;
		TMap<FString, UClass*> ExpectedPackages;
		AddExpectedFormalPackages(ExpectedPackages);
		Result.ExpectedAssetCount = ExpectedPackages.Num();
		Result.bSucceeded = ValidateExpectedPackages(
			GetAssetRegistry(),
			ExpectedPackages,
			Result.Errors);
		return Result;
	}

	FEnemyPackArtPromotionResult PromoteTrainingWarriorArt(bool bForceRefresh)
	{
		FEnemyPackArtPromotionResult ExistingResult =
			ValidateTrainingWarriorFormalArt();
		if (ExistingResult.bSucceeded && !bForceRefresh)
		{
			return ExistingResult;
		}

		FEnemyPackArtPromotionResult Result;
		Result.ExpectedAssetCount = ExistingResult.ExpectedAssetCount;
		IAssetRegistry& AssetRegistry = GetAssetRegistry();
		TSet<FName> SourcePackages;
		BuildSourceClosure(AssetRegistry, SourcePackages, Result.Errors);
		ValidateSourceAssets(SourcePackages, Result.Errors);

		TMap<FString, FString> CopyMap;
		BuildCopyMap(SourcePackages, CopyMap, Result.Errors);
		if (CopyMap.Num() != Result.ExpectedAssetCount)
		{
			Result.Errors.Add(FString::Printf(
				TEXT("Source closure produced %d mapped assets; expected %d"),
				CopyMap.Num(),
				Result.ExpectedAssetCount));
		}
		if (!Result.Errors.IsEmpty())
		{
			return Result;
		}

		IAssetTools& AssetTools =
			FModuleManager::LoadModuleChecked<FAssetToolsModule>(TEXT("AssetTools")).Get();
		if (!AssetTools.AdvancedCopyPackages(
			CopyMap,
			/*bForceAutosave*/ true,
			/*bCopyOverAllDestinationOverlaps*/ true))
		{
			Result.Errors.Add(TEXT("IAssetTools::AdvancedCopyPackages failed"));
			return Result;
		}
		Result.bCopiedAssets = true;
		TMap<FString, UClass*> ExpectedPackages;
		AddExpectedFormalPackages(ExpectedPackages);
		for (const TPair<FString, UClass*>& Entry : ExpectedPackages)
		{
			UObject* Asset = LoadExactAsset(Entry.Key);
			if (!Asset || !Wacom::ContentBuilder::SaveAssetPackage(
				Asset->GetPackage(), Asset, Entry.Key))
			{
				Result.Errors.Add(FString::Printf(
					TEXT("Could not persist promoted formal asset: %s"),
					*Entry.Key));
			}
		}
		UPackage::WaitForAsyncFileWrites();
		if (!Result.Errors.IsEmpty())
		{
			return Result;
		}

		AssetRegistry.ScanPathsSynchronous(
			{ FormalRoot },
			/*bForceRescan*/ true,
			/*bIgnoreDenyListScanFilters*/ true);
		FEnemyPackArtPromotionResult Validation =
			ValidateTrainingWarriorFormalArt();
		Result.bSucceeded = Validation.bSucceeded;
		Result.Errors = MoveTemp(Validation.Errors);
		return Result;
	}
}
