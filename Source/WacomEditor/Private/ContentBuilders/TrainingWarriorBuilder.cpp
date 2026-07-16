// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/TrainingWarriorBuilder.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyHostAnimationStyle.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "Encounters/EncounterDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PaperFlipbook.h"
#include "Tags/WacomGameplayTags.h"
#include "UObject/SavePackage.h"
#include "UObject/UnrealType.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	const FString TrainingWarriorDataRoot =
		TEXT("/Game/Wacom/Data/Enemies/TrainingWarrior");
	const FString TrainingWarriorArtRoot =
		TEXT("/Game/Wacom/Art/Enemies/TrainingWarrior");
	const FString TrainingWarriorHostPackage =
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_TrainingWarrior");

	template <typename T>
	T* LoadFormalAsset(const FString& PackagePath)
	{
		return LoadObject<T>(nullptr, *MakeObjectPath(PackagePath));
	}

	bool CopyEditedProperties(UObject& Target, const UObject& Expected)
	{
		TArray<FProperty*> ChangedProperties;
		for (TFieldIterator<FProperty> It(Target.GetClass()); It; ++It)
		{
			FProperty* Property = *It;
			if (!Property->HasAnyPropertyFlags(CPF_Edit)
				|| Property->HasAnyPropertyFlags(
					CPF_Transient | CPF_DuplicateTransient
					| CPF_NonPIEDuplicateTransient | CPF_Deprecated))
			{
				continue;
			}
			if (!Property->Identical_InContainer(&Target, &Expected, PPF_None))
			{
				ChangedProperties.Add(Property);
			}
		}
		if (ChangedProperties.IsEmpty())
		{
			return false;
		}

		Target.Modify();
		for (FProperty* Property : ChangedProperties)
		{
			Property->CopyCompleteValue_InContainer(&Target, &Expected);
		}
		return true;
	}

	template <typename T, typename ConfigureExpectedType>
	T* BuildDataAsset(
		const FString& PackagePath,
		FName AssetName,
		ConfigureExpectedType&& ConfigureExpected,
		bool& bOutChanged,
		TArray<FString>& OutErrors)
	{
		UPackage* Package = FindOrCreatePackage(PackagePath);
		if (!Package)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Could not create package %s"), *PackagePath));
			return nullptr;
		}

		UObject* ExistingObject = StaticFindObject(
			UObject::StaticClass(), Package, *AssetName.ToString());
		T* Asset = Cast<T>(ExistingObject);
		bool bCreated = false;
		if (ExistingObject && !Asset)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Existing object has unexpected class %s: %s"),
				*GetNameSafe(ExistingObject->GetClass()),
				*PackagePath));
			return nullptr;
		}
		if (!Asset)
		{
			Asset = NewObject<T>(
				Package,
				AssetName,
				RF_Public | RF_Standalone | RF_Transactional);
			bCreated = Asset != nullptr;
		}
		if (!Asset)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Could not create asset %s"), *PackagePath));
			return nullptr;
		}

		TStrongObjectPtr<T> Expected(NewObject<T>(GetTransientPackage()));
		ConfigureExpected(*Expected.Get());
		const bool bChanged = bCreated || CopyEditedProperties(*Asset, *Expected.Get());
		if (bChanged)
		{
			if (!SaveAssetPackage(Package, Asset, PackagePath))
			{
				OutErrors.Add(FString::Printf(
					TEXT("Could not save asset %s"), *PackagePath));
				return nullptr;
			}
			bOutChanged = true;
		}
		return Asset;
	}

	FCardEffect MakeAllEnemyPartsDamage(int32 Amount)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = Amount;
		Effect.Target = WacomTags::Target_AllEnemyParts;
		return Effect;
	}

	FIntentEffect MakePlayerDamage(int32 Amount)
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = Amount;
		Effect.Target = WacomTags::Target_Player;
		return Effect;
	}

	FIntentEffect MakeSelfShield(int32 Amount)
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Status_Shield;
		Effect.Magnitude = Amount;
		Effect.Target = WacomTags::Target_Self;
		return Effect;
	}

	FWacomEnemyBehaviorIntent MakeBehaviorIntent(
		FName IntentId,
		const TCHAR* DisplayName,
		int32 Initiative,
		int32 Resistance,
		FIntentEffect Effect)
	{
		FWacomEnemyBehaviorIntent Result;
		Result.Intent.IntentId = IntentId;
		Result.Intent.DisplayName = FText::FromString(DisplayName);
		Result.Intent.Initiative = Initiative;
		Result.Intent.ResistanceValue = Resistance;
		Result.Intent.Effects = { MoveTemp(Effect) };
		return Result;
	}

	template <typename T>
	bool AssignIfDifferent(UObject& Owner, T& Target, const T& Value)
	{
		if (Target == Value)
		{
			return false;
		}
		Owner.Modify();
		Target = Value;
		return true;
	}

	bool SaveBlueprint(
		UPackage& Package,
		UBlueprint& Blueprint,
		const FString& PackagePath)
	{
		return SaveAssetPackage(&Package, &Blueprint, PackagePath);
	}

	UBlueprint* BuildHostBlueprint(
		UEnemyDefinition& Enemy,
		UWacomBattleEnemyHostAnimationStyle& AnimationStyle,
		UPaperFlipbook& IdleFlipbook,
		bool& bOutChanged,
		TArray<FString>& OutErrors)
	{
		UPackage* Package = FindOrCreatePackage(TrainingWarriorHostPackage);
		if (!Package)
		{
			OutErrors.Add(TEXT("Could not create TrainingWarrior Host package"));
			return nullptr;
		}

		const FName AssetName(TEXT("BP_EnemyHost_TrainingWarrior"));
		UObject* ExistingObject = StaticFindObject(
			UObject::StaticClass(), Package, *AssetName.ToString());
		UBlueprint* Blueprint = Cast<UBlueprint>(ExistingObject);
		bool bChanged = false;
		if (ExistingObject && !Blueprint)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Existing Host object has unexpected class %s"),
				*GetNameSafe(ExistingObject->GetClass())));
			return nullptr;
		}
		if (!Blueprint)
		{
			Blueprint = FKismetEditorUtilities::CreateBlueprint(
				AWacomBattleEnemyActor::StaticClass(),
				Package,
				AssetName,
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			bChanged = Blueprint != nullptr;
		}
		if (!Blueprint || !Blueprint->GeneratedClass
			|| !Blueprint->GeneratedClass->IsChildOf(AWacomBattleEnemyActor::StaticClass()))
		{
			OutErrors.Add(TEXT("TrainingWarrior Host Blueprint is invalid"));
			return nullptr;
		}

		AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
		if (!Host)
		{
			OutErrors.Add(TEXT("TrainingWarrior Host CDO is invalid"));
			return nullptr;
		}

		UWacomBattleEnemyPartImpactStyle* ImpactStyle =
			LoadObject<UWacomBattleEnemyPartImpactStyle>(
				nullptr,
				TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartImpactStyle_Pixel.DA_BattleEnemyPartImpactStyle_Pixel"));
		UWacomBattleEnemyPartTargetPreviewStyle* TargetPreviewStyle =
			LoadObject<UWacomBattleEnemyPartTargetPreviewStyle>(
				nullptr,
				TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartTargetPreviewStyle_PixelLock.DA_BattleEnemyPartTargetPreviewStyle_PixelLock"));
		if (!ImpactStyle || !TargetPreviewStyle)
		{
			OutErrors.Add(TEXT("TrainingWarrior Host requires the formal Pixel impact and target preview styles"));
			return nullptr;
		}

		bChanged |= AssignIfDifferent(*Host, Host->EnemyDefinition, TObjectPtr<UEnemyDefinition>(&Enemy));
		bChanged |= AssignIfDifferent(*Host, Host->EnemySlotId, FName(TEXT("Enemy")));
		bChanged |= AssignIfDifferent(
			*Host,
			Host->HostAuthoringMode,
			EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual);
		bChanged |= AssignIfDifferent(*Host, Host->DefaultImpactStyle, TObjectPtr<UWacomBattleEnemyPartImpactStyle>(ImpactStyle));
		bChanged |= AssignIfDifferent(*Host, Host->DefaultTargetPreviewStyle, TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle>(TargetPreviewStyle));
		bChanged |= AssignIfDifferent(
			*Host,
			Host->HostVisualMode,
			EWacomBattleEnemyHostVisualMode::Flipbook);
		bChanged |= AssignIfDifferent(*Host, Host->HostSprite, TObjectPtr<UPaperSprite>(nullptr));
		bChanged |= AssignIfDifferent(*Host, Host->HostFlipbook, TObjectPtr<UPaperFlipbook>(&IdleFlipbook));
		bChanged |= AssignIfDifferent(*Host, Host->HostAnimationStyle, TObjectPtr<UWacomBattleEnemyHostAnimationStyle>(&AnimationStyle));
		bChanged |= AssignIfDifferent(*Host, Host->HostFlipbookPlayRate, 1.0f);
		bChanged |= AssignIfDifferent(*Host, Host->bLoopHostFlipbook, true);
		bChanged |= AssignIfDifferent(*Host, Host->HostFlipbookStartTimeSeconds, 0.0f);
		bChanged |= AssignIfDifferent(*Host, Host->bAutoPlayHostFlipbook, true);
		bChanged |= AssignIfDifferent(*Host, Host->bHostVisualVisible, true);

		TArray<AWacomBattleEnemyActor*> HostsToSync = { Host };
		const TArray<FWacomBattleSceneEnemyHostSyncResult> SyncResults =
			FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(HostsToSync);
		if (SyncResults.Num() != 1
			|| SyncResults[0].ResultCode == FName(TEXT("ApplyFailed"))
			|| SyncResults[0].ResultCode == FName(TEXT("PartiallyApplied")))
		{
			OutErrors.Add(TEXT("TrainingWarrior Host part synchronization failed"));
			return nullptr;
		}
		bChanged |= SyncResults[0].bChanged;
		if (SyncResults[0].bChanged)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
			{
				OutErrors.Add(
					TEXT("TrainingWarrior Host Blueprint compile after part synchronization failed"));
				return nullptr;
			}
			Host = Cast<AWacomBattleEnemyActor>(
				Blueprint->GeneratedClass->GetDefaultObject());
			if (!Host)
			{
				OutErrors.Add(
					TEXT("TrainingWarrior Host CDO was not regenerated after part synchronization"));
				return nullptr;
			}
		}

		const TArray<AWacomBattleEnemyPartActor*> Parts =
			Host->GetBattleEnemyPartActors();
		if (Parts.Num() != 1 || !Parts[0]
			|| Parts[0]->PartSlotId != FName(TEXT("Body"))
			|| Parts[0]->PartId != FName(TEXT("TrainingWarrior.Body")))
		{
			OutErrors.Add(TEXT("TrainingWarrior Host must contain exactly one derived Body PartActor"));
			return nullptr;
		}

		AWacomBattleEnemyPartActor& BodyPart = *Parts[0];
		bool bPartChanged = false;
		bPartChanged |= AssignIfDifferent(
			BodyPart, BodyPart.HitBoundsExtent, FVector(55.0, 45.0, 55.0));
		bPartChanged |= AssignIfDifferent(
			BodyPart, BodyPart.ImpactAnchorRelativeLocation, FVector::ZeroVector);
		if (!BodyPart.VisualLayers.IsEmpty())
		{
			BodyPart.Modify();
			BodyPart.VisualLayers.Reset();
			bPartChanged = true;
		}
		if (bPartChanged)
		{
			BodyPart.RefreshAuthoringState();
			bChanged = true;
		}

		const FWacomBattleSceneEnemyHostAuthoringReport Report =
			FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
		if (!Report.bAuthoringReady || Report.PartActorCount != 1)
		{
			OutErrors.Add(FString::Printf(
				TEXT("TrainingWarrior Host authoring report is %s with %d parts"),
				*Report.AuthoringState.ToString(),
				Report.PartActorCount));
			return nullptr;
		}

		if (bChanged)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("TrainingWarrior Host Blueprint compile failed"));
			return nullptr;
		}
		if (bChanged && !SaveBlueprint(*Package, *Blueprint, TrainingWarriorHostPackage))
		{
			OutErrors.Add(TEXT("TrainingWarrior Host Blueprint save failed"));
			return nullptr;
		}
		UPackage::WaitForAsyncFileWrites();

		UBlueprint* PersistedBlueprint = LoadFormalAsset<UBlueprint>(
			TrainingWarriorHostPackage);
		if (!PersistedBlueprint || !PersistedBlueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("TrainingWarrior Host Blueprint reload verification failed"));
			return nullptr;
		}
		bOutChanged |= bChanged;
		return PersistedBlueprint;
	}
}

namespace Wacom::ContentBuilder
{
	FTrainingWarriorBuildResult BuildTrainingWarriorContent()
	{
		FTrainingWarriorBuildResult Result;
		UPaperFlipbook* IdleFlipbook = LoadFormalAsset<UPaperFlipbook>(
			TrainingWarriorArtRoot + TEXT("/Flipbooks/PF_Enemy_TrainingWarrior_Idle"));
		UPaperFlipbook* AttackFlipbook = LoadFormalAsset<UPaperFlipbook>(
			TrainingWarriorArtRoot + TEXT("/Flipbooks/PF_Enemy_TrainingWarrior_Attack"));
		UPaperFlipbook* BlockFlipbook = LoadFormalAsset<UPaperFlipbook>(
			TrainingWarriorArtRoot + TEXT("/Flipbooks/PF_Enemy_TrainingWarrior_Block"));
		UPaperFlipbook* CleaveFlipbook = LoadFormalAsset<UPaperFlipbook>(
			TrainingWarriorArtRoot + TEXT("/Flipbooks/PF_Enemy_TrainingWarrior_Cleave"));
		UPaperFlipbook* DestroyedFlipbook = LoadFormalAsset<UPaperFlipbook>(
			TrainingWarriorArtRoot + TEXT("/Flipbooks/PF_Enemy_TrainingWarrior_Destroyed"));
		if (!IdleFlipbook || !AttackFlipbook || !BlockFlipbook
			|| !CleaveFlipbook || !DestroyedFlipbook)
		{
			Result.Errors.Add(
				TEXT("Formal TrainingWarrior Flipbooks are incomplete; run WacomBuildEnemyPack -Pack=TrainingWarrior -PromoteArt"));
			return Result;
		}

		Result.RewardCard = BuildDataAsset<UCardDefinition>(
			MakePackagePath(RewardCardsRoot(), TEXT("DA_Card_BrokenCleave")),
			TEXT("DA_Card_BrokenCleave"),
			[](UCardDefinition& Card)
			{
				Card.CardId = TEXT("Reward.BrokenCleave");
				Card.DisplayName = FText::FromString(TEXT("残缺横斩"));
				Card.Description = FText::FromString(
					TEXT("对所有存活敌方部位造成 {Effect.0} 点伤害。"));
				Card.CardIllustration = nullptr;
				Card.BaseCost = 1;
				Card.Rarity = WacomTags::Card_Rarity_White;
				Card.Keywords.Reset();
				Card.Keywords.AddTag(WacomTags::Card_Keyword_Weapon);
				Card.TargetMode = ECardTargetMode::AllEnemyParts;
				Card.HandCardTargetFilter = FWacomHandCardTargetFilter{};
				Card.Physique = FCardPhysique{};
				Card.Effects = { MakeAllEnemyPartsDamage(3) };
				Card.PerfectReleaseEffects.Reset();
				Card.ZoneHooks.Reset();
				Card.Passives.Reset();
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.RewardCard)
		{
			return Result;
		}

		Result.Behavior = BuildDataAsset<UEnemyBehaviorDefinition>(
			MakePackagePath(TrainingWarriorDataRoot, TEXT("DA_Behavior_TrainingWarrior")),
			TEXT("DA_Behavior_TrainingWarrior"),
			[](UEnemyBehaviorDefinition& Behavior)
			{
				Behavior.BehaviorId = TEXT("TrainingWarrior.Behavior");
				Behavior.InitialPhaseId = TEXT("Default");
				FWacomEnemyIntentSetDefinition IntentSet;
				IntentSet.IntentSetId = TEXT("TrainingWarrior.Body.Sequence");
				IntentSet.AppliesToPartSlotId = TEXT("Body");
				IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
				IntentSet.Intents = {
					MakeBehaviorIntent(
						TEXT("TrainingWarrior.Body.Attack"),
						TEXT("Attack"), 3, 4, MakePlayerDamage(4)),
					MakeBehaviorIntent(
						TEXT("TrainingWarrior.Body.Guard"),
						TEXT("Guard"), 2, 0, MakeSelfShield(4)),
					MakeBehaviorIntent(
						TEXT("TrainingWarrior.Body.Cleave"),
						TEXT("Cleave"), 4, 7, MakePlayerDamage(7)),
				};
				FWacomEnemyPhaseDefinition Phase;
				Phase.PhaseId = TEXT("Default");
				Phase.IntentSets = { MoveTemp(IntentSet) };
				Behavior.Phases = { MoveTemp(Phase) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Behavior)
		{
			return Result;
		}

		UCardDefinition* RewardCard = Result.RewardCard;
		Result.BodyPart = BuildDataAsset<UEnemyPartDefinition>(
			MakePackagePath(TrainingWarriorDataRoot, TEXT("DA_Part_TrainingWarrior_Body")),
			TEXT("DA_Part_TrainingWarrior_Body"),
			[RewardCard](UEnemyPartDefinition& Part)
			{
				Part.PartId = TEXT("TrainingWarrior.Body");
				Part.DisplayName = FText::FromString(TEXT("Training Warrior Body"));
				Part.MaxHp = 24;
				Part.ExperienceReward = 3;
				Part.KnockdownRewardCard = RewardCard;
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.BodyPart)
		{
			return Result;
		}

		UEnemyBehaviorDefinition* Behavior = Result.Behavior;
		UEnemyPartDefinition* BodyPart = Result.BodyPart;
		Result.Enemy = BuildDataAsset<UEnemyDefinition>(
			MakePackagePath(TrainingWarriorDataRoot, TEXT("DA_Enemy_TrainingWarrior")),
			TEXT("DA_Enemy_TrainingWarrior"),
			[Behavior, BodyPart](UEnemyDefinition& Enemy)
			{
				Enemy.EnemyId = TEXT("Enemy.TrainingWarrior");
				Enemy.DisplayName = FText::FromString(TEXT("Training Warrior"));
				Enemy.DefaultBehavior = Behavior;
				Enemy.DefaultPhaseId = TEXT("Default");
				FEnemyPartSlot BodySlot;
				BodySlot.PartSlotId = TEXT("Body");
				BodySlot.PartDef = BodyPart;
				BodySlot.InitialIntentSetId =
					TEXT("TrainingWarrior.Body.Sequence");
				Enemy.Parts = { MoveTemp(BodySlot) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Enemy)
		{
			return Result;
		}

		UEnemyDefinition* Enemy = Result.Enemy;
		Result.Encounter = BuildDataAsset<UEncounterDefinition>(
			MakePackagePath(EncountersRoot(), TEXT("DA_Encounter_TrainingWarriorSingle")),
			TEXT("DA_Encounter_TrainingWarriorSingle"),
			[Enemy](UEncounterDefinition& Encounter)
			{
				Encounter.EncounterDefinitionId =
					TEXT("Encounter.TrainingWarrior.Single");
				Encounter.DisplayName = FText::FromString(TEXT("Training Warrior"));
				FEncounterEnemySlot EnemySlot;
				EnemySlot.EnemySlotId = TEXT("Enemy");
				EnemySlot.EnemyDefinition = Enemy;
				Encounter.EnemySlots = { MoveTemp(EnemySlot) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Encounter)
		{
			return Result;
		}

		Result.AnimationStyle =
			BuildDataAsset<UWacomBattleEnemyHostAnimationStyle>(
				MakePackagePath(
					TrainingWarriorDataRoot,
					TEXT("DA_EnemyHostAnimation_TrainingWarrior")),
				TEXT("DA_EnemyHostAnimation_TrainingWarrior"),
				[AttackFlipbook, BlockFlipbook, CleaveFlipbook, DestroyedFlipbook](
					UWacomBattleEnemyHostAnimationStyle& Style)
				{
					Style.DefaultActionClip.Flipbook = AttackFlipbook;
					Style.DefaultActionClip.PlayRate = 0.75f;
					Style.ActionClipsByIntentId.Reset();
					FWacomBattleEnemyHostAnimationClip GuardClip;
					GuardClip.Flipbook = BlockFlipbook;
					GuardClip.PlayRate = 1.0f;
					Style.ActionClipsByIntentId.Add(
						TEXT("TrainingWarrior.Body.Guard"), GuardClip);
					FWacomBattleEnemyHostAnimationClip CleaveClip;
					CleaveClip.Flipbook = CleaveFlipbook;
					CleaveClip.PlayRate = 0.75f;
					Style.ActionClipsByIntentId.Add(
						TEXT("TrainingWarrior.Body.Cleave"), CleaveClip);
					Style.DestroyedClip.Flipbook = DestroyedFlipbook;
					Style.DestroyedClip.PlayRate = 0.75f;
				},
				Result.bChanged,
				Result.Errors);
		if (!Result.AnimationStyle)
		{
			return Result;
		}

		Result.HostBlueprint = BuildHostBlueprint(
			*Result.Enemy,
			*Result.AnimationStyle,
			*IdleFlipbook,
			Result.bChanged,
			Result.Errors);
		return Result;
	}
}
