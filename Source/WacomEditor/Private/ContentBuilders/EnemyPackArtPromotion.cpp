// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/EnemyPackArtPromotion.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "AssetToolsModule.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Engine/Texture2D.h"
#include "IAssetTools.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"
#include "PackageTools.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "UObject/UnrealType.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	struct FAnimationPromotionSpec
	{
		const TCHAR* SourceState;
		const TCHAR* FormalState;
		int32 FrameCount;
	};

	struct FEnemyArtPromotionManifest
	{
		const TCHAR* Label;
		FString SourceRoot;
		FString TargetRoot;
		FString SourceAssetPrefix;
		FString TargetAssetPrefix;
		FString SourceTextureName;
		FString TargetTextureName;
		TArray<FAnimationPromotionSpec> Animations;
		bool bBuildSnakeDestroyedFlipbooks = false;
	};

	struct FSnakeDestroyedFlipbookSpec
	{
		const TCHAR* PartName;
		int32 SourceFrameIndex;
	};

	const TArray<FSnakeDestroyedFlipbookSpec>& GetSnakeDestroyedSpecs()
	{
		static const TArray<FSnakeDestroyedFlipbookSpec> Specs = {
			{ TEXT("Head"), 3 },
			{ TEXT("Body"), 2 },
			{ TEXT("Tail"), 1 },
		};
		return Specs;
	}

	const FEnemyArtPromotionManifest& GetTrainingWarriorManifest()
	{
		static const FEnemyArtPromotionManifest Manifest = {
			TEXT("TrainingWarrior"),
			TEXT("/Game/Art/PaperAssets/Party/BattleWarrior"),
			TEXT("/Game/Wacom/Art/Enemies/TrainingWarrior"),
			TEXT("BattleWarrior__"),
			TEXT("TrainingWarrior"),
			TEXT("BattleWarrior"),
			TEXT("T_Enemy_TrainingWarrior"),
			{
				{ TEXT("Idle"), TEXT("Idle"), 6 },
				{ TEXT("Attack"), TEXT("Attack"), 5 },
				{ TEXT("Block"), TEXT("Block"), 4 },
				{ TEXT("Cleave"), TEXT("Cleave"), 11 },
				{ TEXT("Downed"), TEXT("Destroyed"), 4 },
			},
			false,
		};
		return Manifest;
	}

	const FEnemyArtPromotionManifest& GetSnakePlaceholderManifest()
	{
		static const FEnemyArtPromotionManifest Manifest = {
			TEXT("SnakePlaceholder"),
			TEXT("/Game/Art/PaperAssets/Enemies/Slime"),
			TEXT("/Game/Wacom/Art/Placeholders/Enemies/Snake"),
			TEXT("Slime__"),
			TEXT("SnakePlaceholder"),
			TEXT("Slime"),
			TEXT("T_Enemy_SnakePlaceholder_Slime"),
			{
				{ TEXT("Idle"), TEXT("Idle"), 4 },
			},
			true,
		};
		return Manifest;
	}

	bool IsPackageUnderRoot(const FString& PackageName, const FString& Root)
	{
		return PackageName == Root || PackageName.StartsWith(Root + TEXT("/"));
	}

	FString SourceFlipbookPackage(
		const FEnemyArtPromotionManifest& Manifest,
		const FAnimationPromotionSpec& Spec)
	{
		return FString::Printf(
			TEXT("%s/%s%s"),
			*Manifest.SourceRoot,
			*Manifest.SourceAssetPrefix,
			Spec.SourceState);
	}

	FString SourceSpritePackage(
		const FEnemyArtPromotionManifest& Manifest,
		const FAnimationPromotionSpec& Spec,
		int32 FrameIndex)
	{
		return FString::Printf(
			TEXT("%s/Frames/%s%s_%d_aseprite"),
			*Manifest.SourceRoot,
			*Manifest.SourceAssetPrefix,
			Spec.SourceState,
			FrameIndex);
	}

	FString TargetFlipbookPackage(
		const FEnemyArtPromotionManifest& Manifest,
		const FAnimationPromotionSpec& Spec)
	{
		return FString::Printf(
			TEXT("%s/Flipbooks/PF_Enemy_%s_%s"),
			*Manifest.TargetRoot,
			*Manifest.TargetAssetPrefix,
			Spec.FormalState);
	}

	FString TargetSpritePackage(
		const FEnemyArtPromotionManifest& Manifest,
		const FAnimationPromotionSpec& Spec,
		int32 FrameIndex)
	{
		return FString::Printf(
			TEXT("%s/Sprites/SPR_Enemy_%s_%s_%02d"),
			*Manifest.TargetRoot,
			*Manifest.TargetAssetPrefix,
			Spec.FormalState,
			FrameIndex);
	}

	FString SnakeDestroyedFlipbookPackage(const TCHAR* PartName)
	{
		return FString::Printf(
			TEXT("%s/Flipbooks/PF_Enemy_SnakePlaceholder_Destroyed_%s"),
			*GetSnakePlaceholderManifest().TargetRoot,
			PartName);
	}

	void BuildManifestMaps(
		const FEnemyArtPromotionManifest& Manifest,
		TMap<FString, FString>& OutCopyMap,
		TMap<FString, UClass*>& OutSourceClasses,
		TMap<FString, UClass*>& OutTargetClasses)
	{
		const FString SourceTexture = FString::Printf(
			TEXT("%s/Textures/%s"),
			*Manifest.SourceRoot,
			*Manifest.SourceTextureName);
		const FString TargetTexture = FString::Printf(
			TEXT("%s/Textures/%s"),
			*Manifest.TargetRoot,
			*Manifest.TargetTextureName);
		OutCopyMap.Add(SourceTexture, TargetTexture);
		OutSourceClasses.Add(SourceTexture, UTexture2D::StaticClass());
		OutTargetClasses.Add(TargetTexture, UTexture2D::StaticClass());

		for (const FAnimationPromotionSpec& Spec : Manifest.Animations)
		{
			const FString SourceFlipbook = SourceFlipbookPackage(Manifest, Spec);
			const FString TargetFlipbook = TargetFlipbookPackage(Manifest, Spec);
			OutCopyMap.Add(SourceFlipbook, TargetFlipbook);
			OutSourceClasses.Add(SourceFlipbook, UPaperFlipbook::StaticClass());
			OutTargetClasses.Add(TargetFlipbook, UPaperFlipbook::StaticClass());
			for (int32 FrameIndex = 0; FrameIndex < Spec.FrameCount; ++FrameIndex)
			{
				const FString SourceSprite = SourceSpritePackage(
					Manifest, Spec, FrameIndex);
				const FString TargetSprite = TargetSpritePackage(
					Manifest, Spec, FrameIndex);
				OutCopyMap.Add(SourceSprite, TargetSprite);
				OutSourceClasses.Add(SourceSprite, UPaperSprite::StaticClass());
				OutTargetClasses.Add(TargetSprite, UPaperSprite::StaticClass());
			}
		}

		if (Manifest.bBuildSnakeDestroyedFlipbooks)
		{
			for (const FSnakeDestroyedFlipbookSpec& Spec : GetSnakeDestroyedSpecs())
			{
				OutTargetClasses.Add(
					SnakeDestroyedFlipbookPackage(Spec.PartName),
					UPaperFlipbook::StaticClass());
			}
		}
	}

	UObject* LoadExactAsset(const FString& PackageName)
	{
		return StaticLoadObject(
			UObject::StaticClass(),
			nullptr,
			*Wacom::ContentBuilder::MakeObjectPath(PackageName));
	}

	IAssetRegistry& GetAssetRegistry()
	{
		FAssetRegistryModule& Module =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& Registry = Module.Get();
		Registry.SearchAllAssets(/*bSynchronousSearch*/ true);
		Registry.WaitForCompletion();
		return Registry;
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
					TEXT("Missing promoted asset: %s"), *Entry.Key));
				bValid = false;
				continue;
			}
			if (!Asset->IsA(Entry.Value))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Promoted asset has class %s, expected %s: %s"),
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
							TEXT("Promoted asset %s still depends on ignored package %s"),
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
		const FEnemyArtPromotionManifest& Manifest,
		TSet<FName>& OutPackages,
		TArray<FString>& OutErrors)
	{
		TArray<FName> Queue;
		for (const FAnimationPromotionSpec& Spec : Manifest.Animations)
		{
			const FName PackageName(*SourceFlipbookPackage(Manifest, Spec));
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
				if (!IsPackageUnderRoot(TargetPackage, Manifest.SourceRoot))
				{
					OutErrors.Add(FString::Printf(
						TEXT("%s source dependency escapes its source root: %s -> %s"),
						Manifest.Label,
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

	bool ValidateSourceClosure(
		const FEnemyArtPromotionManifest& Manifest,
		const TSet<FName>& SourcePackages,
		const TMap<FString, UClass*>& ExpectedSourceClasses,
		TArray<FString>& OutErrors)
	{
		for (FName PackageName : SourcePackages)
		{
			const FString PackageString = PackageName.ToString();
			UClass* const* ExpectedClass = ExpectedSourceClasses.Find(PackageString);
			if (!ExpectedClass)
			{
				OutErrors.Add(FString::Printf(
					TEXT("%s source closure contains an unmapped package: %s"),
					Manifest.Label,
					*PackageString));
				continue;
			}
			UObject* Asset = LoadExactAsset(PackageString);
			if (!Asset || !Asset->IsA(*ExpectedClass))
			{
				OutErrors.Add(FString::Printf(
					TEXT("%s source asset is missing or has an unexpected class: %s"),
					Manifest.Label,
					*PackageString));
			}
		}
		for (const TPair<FString, UClass*>& Entry : ExpectedSourceClasses)
		{
			if (!SourcePackages.Contains(FName(*Entry.Key)))
			{
				OutErrors.Add(FString::Printf(
					TEXT("%s selected source asset is outside the dependency closure: %s"),
					Manifest.Label,
					*Entry.Key));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool UnloadLoadedTargetPackages(
		const TMap<FString, UClass*>& TargetClasses,
		TArray<FString>& OutErrors)
	{
		TArray<UPackage*> LoadedPackages;
		for (const TPair<FString, UClass*>& Entry : TargetClasses)
		{
			if (UPackage* Package = FindPackage(nullptr, *Entry.Key))
			{
				ResetLoaders(Package);
				LoadedPackages.AddUnique(Package);
			}
		}
		if (LoadedPackages.IsEmpty())
		{
			return true;
		}

		UPackageTools::FUnloadPackageParams UnloadParams(LoadedPackages);
		UnloadParams.bUnloadDirtyPackages = true;
		UnloadParams.bResetTransBuffer = true;
		UPackageTools::UnloadPackages(UnloadParams);
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		for (const TPair<FString, UClass*>& Entry : TargetClasses)
		{
			if (FindPackage(nullptr, *Entry.Key))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not unload existing promoted package before replacement: %s"),
					*Entry.Key));
			}
		}
		return OutErrors.IsEmpty();
	}

	bool FixupPromotedManifestReferences(
		const FEnemyArtPromotionManifest& Manifest,
		TArray<FString>& OutErrors)
	{
		UTexture2D* TargetTexture = Cast<UTexture2D>(LoadExactAsset(FString::Printf(
			TEXT("%s/Textures/%s"),
			*Manifest.TargetRoot,
			*Manifest.TargetTextureName)));
		if (!TargetTexture)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Missing promoted texture for %s"), Manifest.Label));
			return false;
		}

		FSoftObjectProperty* SourceTextureProperty = FindFProperty<FSoftObjectProperty>(
			UPaperSprite::StaticClass(),
			UPaperSprite::GetSourceTextureMemberName());
		FObjectPropertyBase* BakedTextureProperty = FindFProperty<FObjectPropertyBase>(
			UPaperSprite::StaticClass(), TEXT("BakedSourceTexture"));
		FObjectPropertyBase* SourceTextureCacheProperty = FindFProperty<FObjectPropertyBase>(
			UPaperSprite::StaticClass(), TEXT("SourceTextureCacheNeverSerialized"));
		if (!SourceTextureProperty || !BakedTextureProperty || !SourceTextureCacheProperty)
		{
			OutErrors.Add(TEXT("Could not resolve PaperSprite texture properties for promoted reference fixup"));
			return false;
		}

		for (const FAnimationPromotionSpec& Spec : Manifest.Animations)
		{
			TArray<UPaperSprite*> TargetSprites;
			TargetSprites.Reserve(Spec.FrameCount);
			for (int32 FrameIndex = 0; FrameIndex < Spec.FrameCount; ++FrameIndex)
			{
				const FString SpritePackage = TargetSpritePackage(
					Manifest, Spec, FrameIndex);
				UPaperSprite* Sprite = Cast<UPaperSprite>(LoadExactAsset(SpritePackage));
				if (!Sprite)
				{
					OutErrors.Add(FString::Printf(
						TEXT("Missing promoted sprite during reference fixup: %s"),
						*SpritePackage));
					continue;
				}
				TargetSprites.Add(Sprite);

				const FSoftObjectPtr* SourceTextureValue =
					SourceTextureProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Sprite);
				const bool bNeedsTextureFixup = !SourceTextureValue
					|| SourceTextureValue->Get() != TargetTexture
					|| BakedTextureProperty->GetObjectPropertyValue_InContainer(Sprite)
						!= TargetTexture;
				if (bNeedsTextureFixup)
				{
					Sprite->Modify();
					FSoftObjectPtr* MutableSourceTexture =
						SourceTextureProperty->ContainerPtrToValuePtr<FSoftObjectPtr>(Sprite);
					*MutableSourceTexture = FSoftObjectPtr(TargetTexture);
					BakedTextureProperty->SetObjectPropertyValue_InContainer(
						Sprite, TargetTexture);
					SourceTextureCacheProperty->SetObjectPropertyValue_InContainer(
						Sprite, TargetTexture);
					Sprite->PostEditChange();
					if (!SaveAssetPackage(Sprite->GetPackage(), Sprite, SpritePackage))
					{
						OutErrors.Add(FString::Printf(
							TEXT("Could not save promoted sprite after reference fixup: %s"),
							*SpritePackage));
					}
				}
			}

			const FString SourceFlipbookPath = SourceFlipbookPackage(Manifest, Spec);
			const FString TargetFlipbookPath = TargetFlipbookPackage(Manifest, Spec);
			UPaperFlipbook* SourceFlipbook = Cast<UPaperFlipbook>(
				LoadExactAsset(SourceFlipbookPath));
			UPaperFlipbook* TargetFlipbook = Cast<UPaperFlipbook>(
				LoadExactAsset(TargetFlipbookPath));
			if (!SourceFlipbook || !TargetFlipbook
				|| SourceFlipbook->GetNumKeyFrames() != Spec.FrameCount
				|| TargetSprites.Num() != Spec.FrameCount)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not establish promoted flipbook frame contract: %s"),
					*TargetFlipbookPath));
				continue;
			}

			bool bNeedsFlipbookFixup = TargetFlipbook->GetNumKeyFrames()
				!= Spec.FrameCount
				|| !FMath::IsNearlyEqual(
					TargetFlipbook->GetFramesPerSecond(),
					SourceFlipbook->GetFramesPerSecond());
			for (int32 FrameIndex = 0;
				!bNeedsFlipbookFixup && FrameIndex < Spec.FrameCount;
				++FrameIndex)
			{
				bNeedsFlipbookFixup =
					TargetFlipbook->GetKeyFrameChecked(FrameIndex).Sprite
						!= TargetSprites[FrameIndex]
					|| TargetFlipbook->GetKeyFrameChecked(FrameIndex).FrameRun
						!= SourceFlipbook->GetKeyFrameChecked(FrameIndex).FrameRun;
			}
			if (bNeedsFlipbookFixup)
			{
				TargetFlipbook->Modify();
				FScopedFlipbookMutator Mutator(TargetFlipbook);
				Mutator.FramesPerSecond = SourceFlipbook->GetFramesPerSecond();
				Mutator.KeyFrames.Reset(Spec.FrameCount);
				for (int32 FrameIndex = 0; FrameIndex < Spec.FrameCount; ++FrameIndex)
				{
					FPaperFlipbookKeyFrame& Frame =
						Mutator.KeyFrames.AddDefaulted_GetRef();
					Frame.Sprite = TargetSprites[FrameIndex];
					Frame.FrameRun =
						SourceFlipbook->GetKeyFrameChecked(FrameIndex).FrameRun;
				}
				if (!SaveAssetPackage(
					TargetFlipbook->GetPackage(),
					TargetFlipbook,
					TargetFlipbookPath))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Could not save promoted flipbook after reference fixup: %s"),
						*TargetFlipbookPath));
				}
			}
		}
		return OutErrors.IsEmpty();
	}

	bool BuildSnakeDestroyedFlipbooks(
		bool& bOutChanged,
		TArray<FString>& OutErrors)
	{
		const FEnemyArtPromotionManifest& Manifest = GetSnakePlaceholderManifest();
		const FAnimationPromotionSpec& IdleSpec = Manifest.Animations[0];
		for (const FSnakeDestroyedFlipbookSpec& Spec : GetSnakeDestroyedSpecs())
		{
			UPaperSprite* SourceSprite = Cast<UPaperSprite>(LoadExactAsset(
				TargetSpritePackage(Manifest, IdleSpec, Spec.SourceFrameIndex)));
			if (!SourceSprite)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Missing Snake placeholder source sprite for %s destroyed flipbook"),
					Spec.PartName));
				continue;
			}

			const FString PackagePath = SnakeDestroyedFlipbookPackage(Spec.PartName);
			const FName AssetName(*FPackageName::GetLongPackageAssetName(PackagePath));
			UPackage* Package = FindOrCreatePackage(PackagePath);
			if (!Package)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not create Snake placeholder destroyed package: %s"),
					*PackagePath));
				continue;
			}

			UObject* ExistingObject = StaticFindObject(
				UObject::StaticClass(), Package, *AssetName.ToString());
			UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(ExistingObject);
			if (ExistingObject && !Flipbook)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Snake placeholder destroyed asset has an unexpected class: %s"),
					*PackagePath));
				continue;
			}

			bool bCreated = false;
			if (!Flipbook)
			{
				Flipbook = NewObject<UPaperFlipbook>(
					Package,
					AssetName,
					RF_Public | RF_Standalone | RF_Transactional);
				bCreated = Flipbook != nullptr;
			}
			if (!Flipbook)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not create Snake placeholder destroyed flipbook: %s"),
					*PackagePath));
				continue;
			}

			const bool bHasExpectedFrame = Flipbook->GetNumKeyFrames() == 1
				&& Flipbook->GetKeyFrameChecked(0).Sprite == SourceSprite
				&& Flipbook->GetKeyFrameChecked(0).FrameRun == 1;
			const bool bNeedsUpdate = bCreated
				|| !bHasExpectedFrame
				|| !FMath::IsNearlyEqual(Flipbook->GetFramesPerSecond(), 1.0f);
			if (bNeedsUpdate)
			{
				Flipbook->Modify();
				FScopedFlipbookMutator Mutator(Flipbook);
				Mutator.FramesPerSecond = 1.0f;
				Mutator.KeyFrames.Reset();
				FPaperFlipbookKeyFrame& Frame = Mutator.KeyFrames.AddDefaulted_GetRef();
				Frame.Sprite = SourceSprite;
				Frame.FrameRun = 1;
				if (!SaveAssetPackage(Package, Flipbook, PackagePath))
				{
					OutErrors.Add(FString::Printf(
						TEXT("Could not save Snake placeholder destroyed flipbook: %s"),
						*PackagePath));
					continue;
				}
				bOutChanged = true;
			}
		}
		return OutErrors.IsEmpty();
	}

	bool ValidateSnakeDestroyedFlipbooks(TArray<FString>& OutErrors)
	{
		const FEnemyArtPromotionManifest& Manifest = GetSnakePlaceholderManifest();
		const FAnimationPromotionSpec& IdleSpec = Manifest.Animations[0];
		for (const FSnakeDestroyedFlipbookSpec& Spec : GetSnakeDestroyedSpecs())
		{
			UPaperFlipbook* Flipbook = Cast<UPaperFlipbook>(LoadExactAsset(
				SnakeDestroyedFlipbookPackage(Spec.PartName)));
			UPaperSprite* ExpectedSprite = Cast<UPaperSprite>(LoadExactAsset(
				TargetSpritePackage(Manifest, IdleSpec, Spec.SourceFrameIndex)));
			if (!Flipbook || Flipbook->GetNumKeyFrames() != 1
				|| Flipbook->GetKeyFrameChecked(0).Sprite != ExpectedSprite
				|| Flipbook->GetKeyFrameChecked(0).FrameRun != 1
				|| !FMath::IsNearlyEqual(Flipbook->GetFramesPerSecond(), 1.0f)
				|| Flipbook->GetTotalDuration() <= 0.0f)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Snake placeholder destroyed flipbook contract is invalid: %s"),
					Spec.PartName));
			}
		}
		return OutErrors.IsEmpty();
	}

	FEnemyPackArtPromotionResult ValidateManifestTargets(
		const FEnemyArtPromotionManifest& Manifest)
	{
		FEnemyPackArtPromotionResult Result;
		TMap<FString, FString> CopyMap;
		TMap<FString, UClass*> SourceClasses;
		TMap<FString, UClass*> TargetClasses;
		BuildManifestMaps(Manifest, CopyMap, SourceClasses, TargetClasses);
		Result.ExpectedAssetCount = TargetClasses.Num();
		Result.bSucceeded = ValidateExpectedPackages(
			GetAssetRegistry(), TargetClasses, Result.Errors);
		if (Result.bSucceeded && Manifest.bBuildSnakeDestroyedFlipbooks)
		{
			Result.bSucceeded = ValidateSnakeDestroyedFlipbooks(Result.Errors);
		}
		return Result;
	}

	FEnemyPackArtPromotionResult PromoteManifest(
		const FEnemyArtPromotionManifest& Manifest,
		bool bForceRefresh)
	{
		FEnemyPackArtPromotionResult ExistingResult = ValidateManifestTargets(Manifest);
		if (ExistingResult.bSucceeded && !bForceRefresh)
		{
			return ExistingResult;
		}

		FEnemyPackArtPromotionResult Result;
		Result.ExpectedAssetCount = ExistingResult.ExpectedAssetCount;
		IAssetRegistry& AssetRegistry = GetAssetRegistry();
		TMap<FString, FString> CopyMap;
		TMap<FString, UClass*> SourceClasses;
		TMap<FString, UClass*> TargetClasses;
		BuildManifestMaps(Manifest, CopyMap, SourceClasses, TargetClasses);

		TSet<FName> SourcePackages;
		BuildSourceClosure(AssetRegistry, Manifest, SourcePackages, Result.Errors);
		ValidateSourceClosure(Manifest, SourcePackages, SourceClasses, Result.Errors);
		UnloadLoadedTargetPackages(TargetClasses, Result.Errors);
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
		FixupPromotedManifestReferences(Manifest, Result.Errors);
		UPackage::WaitForAsyncFileWrites();
		if (!Result.Errors.IsEmpty())
		{
			return Result;
		}

		AssetRegistry.ScanPathsSynchronous(
			{ Manifest.TargetRoot },
			/*bForceRescan*/ true,
			/*bIgnoreDenyListScanFilters*/ true);
		if (Manifest.bBuildSnakeDestroyedFlipbooks)
		{
			bool bGeneratedChanged = false;
			BuildSnakeDestroyedFlipbooks(bGeneratedChanged, Result.Errors);
			UPackage::WaitForAsyncFileWrites();
			AssetRegistry.ScanPathsSynchronous(
				{ Manifest.TargetRoot },
				/*bForceRescan*/ true,
				/*bIgnoreDenyListScanFilters*/ true);
		}
		if (!Result.Errors.IsEmpty())
		{
			return Result;
		}

		FEnemyPackArtPromotionResult Validation = ValidateManifestTargets(Manifest);
		Result.bSucceeded = Validation.bSucceeded;
		Result.Errors = MoveTemp(Validation.Errors);
		return Result;
	}
}

namespace Wacom::ContentBuilder
{
	FEnemyPackArtPromotionResult ValidateTrainingWarriorFormalArt()
	{
		return ValidateManifestTargets(GetTrainingWarriorManifest());
	}

	FEnemyPackArtPromotionResult PromoteTrainingWarriorArt(bool bForceRefresh)
	{
		return PromoteManifest(GetTrainingWarriorManifest(), bForceRefresh);
	}

	FEnemyPackArtPromotionResult ValidateSnakePlaceholderArt()
	{
		return ValidateManifestTargets(GetSnakePlaceholderManifest());
	}

	FEnemyPackArtPromotionResult PromoteSnakePlaceholderArt(bool bForceRefresh)
	{
		return PromoteManifest(GetSnakePlaceholderManifest(), bForceRefresh);
	}
}
