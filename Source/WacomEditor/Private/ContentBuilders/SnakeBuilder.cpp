// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SnakeBuilder.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "ContentBuilders/ContentBuilderHelpers.h"
#include "ContentBuilders/EnemyHostComponentBuilderHelpers.h"
#include "Encounters/EncounterDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Enemies/IntentDefinition.h"
#include "Enemies/IntentEffect.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"
#include "UObject/SavePackage.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	const FString SnakeDataRoot = TEXT("/Game/Wacom/Data/Enemies/Snake");
	const FString SnakePlaceholderArtRoot =
		TEXT("/Game/Wacom/Art/Placeholders/Enemies/Snake");
	const FString SnakeHostPackage =
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake");
	const FString SnakeDebugHostPackage =
		TEXT("/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug");

	struct FSnakePartPresentationSpec
	{
		FName PartSlotId;
		FName PartId;
		FVector RelativeLocation;
		float VisualScale = 1.0f;
		float IdleOffsetSeconds = 0.0f;
		FLinearColor Tint = FLinearColor::White;
		int32 SortOrder = 0;
		const TCHAR* DestroyedFlipbookName = nullptr;
	};

	const TArray<FSnakePartPresentationSpec>& GetSnakePartPresentationSpecs()
	{
		static const TArray<FSnakePartPresentationSpec> Specs = {
			{
				TEXT("Head"), TEXT("Snake.Head"),
				FVector(96.0f, -6.0f, 16.0f),
				0.85f, 0.0f, FLinearColor(1.0f, 0.85f, 0.85f, 1.0f), 30,
				TEXT("PF_Enemy_SnakePlaceholder_Destroyed_Head")
			},
			{
				TEXT("Body"), TEXT("Snake.Body"),
				FVector::ZeroVector,
				1.0f, 0.04f, FLinearColor::White, 20,
				TEXT("PF_Enemy_SnakePlaceholder_Destroyed_Body")
			},
			{
				TEXT("Tail"), TEXT("Snake.Tail"),
				FVector(-92.0f, 16.0f, -8.0f),
				0.70f, 0.08f, FLinearColor(0.82f, 0.90f, 1.0f, 1.0f), 10,
				TEXT("PF_Enemy_SnakePlaceholder_Destroyed_Tail")
			},
		};
		return Specs;
	}

	const TArray<FSnakePartPresentationSpec>& GetSnakeDebugPartPresentationSpecs()
	{
		static const TArray<FSnakePartPresentationSpec> Specs = []
		{
			TArray<FSnakePartPresentationSpec> Result =
				GetSnakePartPresentationSpecs();
			for (FSnakePartPresentationSpec& Spec : Result)
			{
				if (Spec.PartSlotId == FName(TEXT("Head")))
				{
					Spec.RelativeLocation = FVector(-154.0f, -6.0f, 46.0f);
				}
				else if (Spec.PartSlotId == FName(TEXT("Body")))
				{
					Spec.RelativeLocation = FVector(0.0f, 0.0f, 70.0f);
				}
				else if (Spec.PartSlotId == FName(TEXT("Tail")))
				{
					Spec.RelativeLocation = FVector(118.0f, 16.0f, 72.0f);
				}
			}
			return Result;
		}();
		return Specs;
	}

	template <typename T>
	T* LoadGeneratedAsset(const FString& PackagePath)
	{
		return LoadObject<T>(nullptr, *MakeObjectPath(PackagePath));
	}

	FIntentEffect MakeDamage(int32 Amount)
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Effect_Damage;
		Effect.Magnitude = Amount;
		Effect.Target = WacomTags::Target_Player;
		return Effect;
	}

	FIntentEffect MakePoisonOnPlayer(int32 Stacks)
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Effect.Magnitude = Stacks;
		Effect.Target = WacomTags::Target_Player;
		return Effect;
	}

	FIntentEffect MakeSlowOnPlayer(int32 Stacks)
	{
		FIntentEffect Effect;
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Slow;
		Effect.Magnitude = Stacks;
		Effect.Target = WacomTags::Target_Player;
		return Effect;
	}

	FIntentEffect MakeShieldSelf(int32 Amount)
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
		FIntentEffect Effect)
	{
		FWacomEnemyBehaviorIntent Entry;
		Entry.Intent.IntentId = IntentId;
		Entry.Intent.DisplayName = FText::FromString(DisplayName);
		Entry.Intent.Initiative = Initiative;
		Entry.Intent.Effects = { MoveTemp(Effect) };
		return Entry;
	}

	FWacomEnemyIntentSetDefinition MakeSequenceIntentSet(
		FName IntentSetId,
		FName PartSlotId,
		TArray<FWacomEnemyBehaviorIntent> Intents)
	{
		FWacomEnemyIntentSetDefinition IntentSet;
		IntentSet.IntentSetId = IntentSetId;
		IntentSet.AppliesToPartSlotId = PartSlotId;
		IntentSet.SelectorMode = EWacomEnemyIntentSelectorMode::Sequence;
		IntentSet.Intents = MoveTemp(Intents);
		return IntentSet;
	}

	FCardEffect MakePoisonOnEnemyPart(int32 Stacks)
	{
		FCardEffect Effect;
		Effect.EffectType = WacomTags::Effect_ApplyStatus_Poison;
		Effect.Magnitude = Stacks;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	bool ApplyPartPresentation(
		UBlueprint& Blueprint,
		const FSnakePartPresentationSpec& Spec,
		UPaperFlipbook& IdleFlipbook,
		UPaperFlipbook& DestroyedFlipbook)
	{
		Wacom::EnemyHostComponentBuilder::FFlipbookPartSpec ComponentSpec;
		ComponentSpec.PartSlotId = Spec.PartSlotId;
		ComponentSpec.PartId = Spec.PartId;
		ComponentSpec.LayerId = FName(*FString::Printf(
			TEXT("Snake.%s.Main"), *Spec.PartSlotId.ToString()));
		ComponentSpec.RelativeLocation = Spec.RelativeLocation;
		ComponentSpec.VisualScale = FVector(Spec.VisualScale);
		ComponentSpec.IdleOffsetSeconds = Spec.IdleOffsetSeconds;
		ComponentSpec.Tint = Spec.Tint;
		ComponentSpec.SortOrder = Spec.SortOrder;
		ComponentSpec.IdleFlipbook = &IdleFlipbook;
		ComponentSpec.DestroyedFlipbook = &DestroyedFlipbook;
		return Wacom::EnemyHostComponentBuilder::ApplyFlipbookPart(
			Blueprint, ComponentSpec);
	}

	UBlueprint* BuildHostBlueprint(
		UEnemyDefinition& Enemy,
		UPaperFlipbook& IdleFlipbook,
		const TArray<FSnakePartPresentationSpec>& PresentationSpecs,
		const FString& HostPackage,
		FName HostAssetName,
		const TCHAR* HostLabel,
		bool& bOutChanged,
		TArray<FString>& OutErrors)
	{
		UPackage* Package = FindOrCreatePackage(HostPackage);
		if (!Package)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Could not create %s package"), HostLabel));
			return nullptr;
		}

		UObject* ExistingObject = StaticFindObject(
			UObject::StaticClass(), Package, *HostAssetName.ToString());
		UBlueprint* Blueprint = Cast<UBlueprint>(ExistingObject);
		bool bChanged = false;
		if (ExistingObject && !Blueprint)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Existing Snake Host object has unexpected class %s"),
				*GetNameSafe(ExistingObject->GetClass())));
			return nullptr;
		}
		if (!Blueprint)
		{
			Blueprint = FKismetEditorUtilities::CreateBlueprint(
				AWacomBattleEnemyActor::StaticClass(),
				Package,
				HostAssetName,
				BPTYPE_Normal,
				UBlueprint::StaticClass(),
				UBlueprintGeneratedClass::StaticClass());
			bChanged = Blueprint != nullptr;
		}
		if (!Blueprint || !Blueprint->GeneratedClass
			|| !Blueprint->GeneratedClass->IsChildOf(
				AWacomBattleEnemyActor::StaticClass()))
		{
			OutErrors.Add(TEXT("Snake Host Blueprint is invalid"));
			return nullptr;
		}

		AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
		UWacomBattleEnemyPartImpactStyle* ImpactStyle =
			LoadGeneratedAsset<UWacomBattleEnemyPartImpactStyle>(
				TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartImpactStyle_Pixel"));
		UWacomBattleEnemyPartTargetPreviewStyle* TargetPreviewStyle =
			LoadGeneratedAsset<UWacomBattleEnemyPartTargetPreviewStyle>(
				TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartTargetPreviewStyle_PixelLock"));
		if (!Host || !ImpactStyle || !TargetPreviewStyle)
		{
			OutErrors.Add(TEXT("Snake Host requires a valid CDO and the formal Pixel styles"));
			return nullptr;
		}

		bChanged |= AssignIfDifferent(
			*Host, Host->EnemyDefinition, TObjectPtr<UEnemyDefinition>(&Enemy));
		bChanged |= AssignIfDifferent(*Host, Host->EnemySlotId, FName(TEXT("Enemy")));
		bChanged |= AssignIfDifferent(
			*Host,
			Host->DefaultImpactStyle,
			TObjectPtr<UWacomBattleEnemyPartImpactStyle>(ImpactStyle));
		bChanged |= AssignIfDifferent(
			*Host,
			Host->DefaultTargetPreviewStyle,
			TObjectPtr<UWacomBattleEnemyPartTargetPreviewStyle>(TargetPreviewStyle));
		bChanged |= AssignIfDifferent(
			*Host,
			Host->EnemyPanelWidgetClass,
			TSubclassOf<UWacomBattleEnemyPanelWidget>());

		TArray<AWacomBattleEnemyActor*> HostsToSync = { Host };
		const TArray<FWacomBattleSceneEnemyHostSyncResult> SyncResults =
			FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(HostsToSync);
		if (SyncResults.Num() != 1
			|| SyncResults[0].ResultCode == FName(TEXT("ApplyFailed"))
			|| SyncResults[0].ResultCode == FName(TEXT("PartiallyApplied")))
		{
			OutErrors.Add(TEXT("Snake Host part synchronization failed"));
			return nullptr;
		}
		bChanged |= SyncResults[0].bChanged;
		if (SyncResults[0].bChanged)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
			FKismetEditorUtilities::CompileBlueprint(Blueprint);
			if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
			{
				OutErrors.Add(TEXT("Snake Host compile after part synchronization failed"));
				return nullptr;
			}
			Host = Cast<AWacomBattleEnemyActor>(
				Blueprint->GeneratedClass->GetDefaultObject());
		}

		if (!Host)
		{
			OutErrors.Add(TEXT("Snake Host CDO was not regenerated"));
			return nullptr;
		}
		for (const FSnakePartPresentationSpec& Spec : PresentationSpecs)
		{
			UPaperFlipbook* DestroyedFlipbook = LoadGeneratedAsset<UPaperFlipbook>(
				SnakePlaceholderArtRoot / TEXT("Flipbooks") / Spec.DestroyedFlipbookName);
			if (!Wacom::EnemyHostComponentBuilder::FindPartTemplates(
					*Blueprint, Spec.PartSlotId).IsComplete()
				|| !DestroyedFlipbook)
			{
				OutErrors.Add(FString::Printf(
					TEXT("Snake Host part or destroyed visual is invalid: %s"),
					*Spec.PartSlotId.ToString()));
				return nullptr;
			}
			bChanged |= ApplyPartPresentation(
				*Blueprint, Spec, IdleFlipbook, *DestroyedFlipbook);
		}

		if (bChanged)
		{
			FBlueprintEditorUtils::MarkBlueprintAsModified(Blueprint);
		}
		FKismetEditorUtilities::CompileBlueprint(Blueprint);
		if (Blueprint->Status == BS_Error || !Blueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("Snake Host Blueprint compile failed"));
			return nullptr;
		}
		Host = Cast<AWacomBattleEnemyActor>(
			Blueprint->GeneratedClass->GetDefaultObject());
		const FWacomBattleSceneEnemyHostAuthoringReport Report = Host
			? FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host)
			: FWacomBattleSceneEnemyHostAuthoringReport{};
		if (!Host || !Report.bAuthoringReady || Report.PartComponentCount != 3)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Snake Host authoring report is %s with %d parts"),
				*Report.AuthoringState.ToString(),
				Report.PartComponentCount));
			return nullptr;
		}
		if (bChanged && !SaveAssetPackage(Package, Blueprint, HostPackage))
		{
			OutErrors.Add(TEXT("Snake Host Blueprint save failed"));
			return nullptr;
		}
		UPackage::WaitForAsyncFileWrites();

		UBlueprint* PersistedBlueprint =
			LoadGeneratedAsset<UBlueprint>(HostPackage);
		if (!PersistedBlueprint || !PersistedBlueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("Snake Host Blueprint reload verification failed"));
			return nullptr;
		}
		bOutChanged |= bChanged;
		return PersistedBlueprint;
	}
}

namespace Wacom::ContentBuilder
{
	FSnakeBuildResult BuildSnakeContent()
	{
		FSnakeBuildResult Result;
		UPaperFlipbook* IdleFlipbook = LoadGeneratedAsset<UPaperFlipbook>(
			SnakePlaceholderArtRoot
			/ TEXT("Flipbooks/PF_Enemy_SnakePlaceholder_Idle"));
		if (!IdleFlipbook)
		{
			Result.Errors.Add(
				TEXT("Snake placeholder art is incomplete; run WacomBuildEnemyPack -Pack=Snake -PromotePlaceholderArt"));
			return Result;
		}

		Result.RewardCard = BuildDataAsset<UCardDefinition>(
			MakePackagePath(RewardCardsRoot(), TEXT("DA_Card_PoisonFang")),
			TEXT("DA_Card_PoisonFang"),
			[](UCardDefinition& Card)
			{
				Card.CardId = TEXT("PoisonFang");
				Card.DisplayName = FText::FromString(TEXT("毒牙"));
				Card.Description = FText::FromString(
					TEXT("对一个敌方部位施加 {Effect.0} 中毒。"));
				Card.BaseCost = 0;
				Card.Rarity = WacomTags::Card_Rarity_White;
				Card.Keywords.Reset();
				Card.TargetMode = ECardTargetMode::SingleEnemyPart;
				Card.Physique = FCardPhysique{};
				Card.Effects = { MakePoisonOnEnemyPart(1) };
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
			MakePackagePath(SnakeDataRoot, TEXT("DA_Behavior_Snake")),
			TEXT("DA_Behavior_Snake"),
			[](UEnemyBehaviorDefinition& Behavior)
			{
				Behavior.BehaviorId = TEXT("Snake.Behavior");
				Behavior.InitialPhaseId = TEXT("Default");
				FWacomEnemyPhaseDefinition Phase;
				Phase.PhaseId = TEXT("Default");
				Phase.IntentSets = {
					MakeSequenceIntentSet(
						TEXT("Snake.Head.Sequence"), TEXT("Head"),
						{
							MakeBehaviorIntent(TEXT("Snake.Head.Bite"), TEXT("Bite"), 3, MakeDamage(6)),
							MakeBehaviorIntent(TEXT("Snake.Head.Venom"), TEXT("Venom"), 5, MakePoisonOnPlayer(2)),
							MakeBehaviorIntent(TEXT("Snake.Head.Strike"), TEXT("Strike"), 4, MakeDamage(8)),
							MakeBehaviorIntent(TEXT("Snake.Head.CoiledGuard"), TEXT("Coiled Guard"), 2, MakeShieldSelf(4)),
						}),
					MakeSequenceIntentSet(
						TEXT("Snake.Body.Sequence"), TEXT("Body"),
						{
							MakeBehaviorIntent(TEXT("Snake.Body.Constrict"), TEXT("Constrict"), 4, MakeSlowOnPlayer(1)),
							MakeBehaviorIntent(TEXT("Snake.Body.Harden"), TEXT("Harden"), 2, MakeShieldSelf(5)),
							MakeBehaviorIntent(TEXT("Snake.Body.Slam"), TEXT("Slam"), 3, MakeDamage(5)),
							MakeBehaviorIntent(TEXT("Snake.Body.VenomMist"), TEXT("Venom Mist"), 5, MakePoisonOnPlayer(1)),
						}),
					MakeSequenceIntentSet(
						TEXT("Snake.Tail.Sequence"), TEXT("Tail"),
						{
							MakeBehaviorIntent(TEXT("Snake.Tail.Sweep"), TEXT("Sweep"), 1, MakeDamage(3)),
							MakeBehaviorIntent(TEXT("Snake.Tail.Lash"), TEXT("Lash"), 2, MakeDamage(5)),
							MakeBehaviorIntent(TEXT("Snake.Tail.Whip"), TEXT("Whip"), 3, MakeDamage(4)),
							MakeBehaviorIntent(TEXT("Snake.Tail.Brace"), TEXT("Brace"), 2, MakeShieldSelf(3)),
							MakeBehaviorIntent(TEXT("Snake.Tail.Tangle"), TEXT("Tangle"), 4, MakeSlowOnPlayer(1)),
						}),
				};
				Behavior.Phases = { MoveTemp(Phase) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Behavior)
		{
			return Result;
		}

		UCardDefinition* RewardCard = Result.RewardCard;
		auto BuildPart = [&Result, RewardCard](
			const TCHAR* AssetName,
			FName PartId,
			const TCHAR* DisplayName,
			int32 MaxHp,
			int32 ExperienceReward)
		{
			return BuildDataAsset<UEnemyPartDefinition>(
				MakePackagePath(SnakeDataRoot, AssetName),
				FName(AssetName),
				[RewardCard, PartId, DisplayName, MaxHp, ExperienceReward](
					UEnemyPartDefinition& Part)
				{
					Part.PartId = PartId;
					Part.DisplayName = FText::FromString(DisplayName);
					Part.MaxHp = MaxHp;
					Part.ExperienceReward = ExperienceReward;
					Part.AidRewardCard = RewardCard;
					Part.DestroyRewardCard = RewardCard;
					Part.KnockdownRewardCard = nullptr;
				},
				Result.bChanged,
				Result.Errors);
		};
		Result.HeadPart = BuildPart(
			TEXT("DA_Part_Snake_Head"), TEXT("Snake.Head"), TEXT("Snake Head"), 16, 3);
		Result.BodyPart = BuildPart(
			TEXT("DA_Part_Snake_Body"), TEXT("Snake.Body"), TEXT("Snake Body"), 22, 2);
		Result.TailPart = BuildPart(
			TEXT("DA_Part_Snake_Tail"), TEXT("Snake.Tail"), TEXT("Snake Tail"), 10, 2);
		if (!Result.HeadPart || !Result.BodyPart || !Result.TailPart)
		{
			return Result;
		}

		UEnemyBehaviorDefinition* Behavior = Result.Behavior;
		UEnemyPartDefinition* Head = Result.HeadPart;
		UEnemyPartDefinition* Body = Result.BodyPart;
		UEnemyPartDefinition* Tail = Result.TailPart;
		Result.Enemy = BuildDataAsset<UEnemyDefinition>(
			MakePackagePath(SnakeDataRoot, TEXT("DA_Enemy_Snake")),
			TEXT("DA_Enemy_Snake"),
			[Behavior, Head, Body, Tail](UEnemyDefinition& Enemy)
			{
				Enemy.EnemyId = TEXT("Snake");
				Enemy.DisplayName = FText::FromString(TEXT("Snake"));
				Enemy.DefaultBehavior = Behavior;
				Enemy.DefaultPhaseId = TEXT("Default");
				FEnemyPartSlot HeadSlot;
				HeadSlot.PartSlotId = TEXT("Head");
				HeadSlot.PartDef = Head;
				HeadSlot.InitialIntentSetId = TEXT("Snake.Head.Sequence");
				FEnemyPartSlot BodySlot;
				BodySlot.PartSlotId = TEXT("Body");
				BodySlot.PartDef = Body;
				BodySlot.InitialIntentSetId = TEXT("Snake.Body.Sequence");
				FEnemyPartSlot TailSlot;
				TailSlot.PartSlotId = TEXT("Tail");
				TailSlot.PartDef = Tail;
				TailSlot.InitialIntentSetId = TEXT("Snake.Tail.Sequence");
				Enemy.Parts = {
					MoveTemp(HeadSlot), MoveTemp(BodySlot), MoveTemp(TailSlot) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Enemy)
		{
			return Result;
		}

		UEnemyDefinition* Enemy = Result.Enemy;
		Result.Encounter = BuildDataAsset<UEncounterDefinition>(
			MakePackagePath(EncountersRoot(), TEXT("DA_Encounter_SnakeSingle")),
			TEXT("DA_Encounter_SnakeSingle"),
			[Enemy](UEncounterDefinition& Encounter)
			{
				Encounter.EncounterDefinitionId = TEXT("Encounter.Snake.Single");
				Encounter.DisplayName = FText::FromString(TEXT("Snake"));
				FEncounterEnemySlot Slot;
				Slot.EnemySlotId = TEXT("Enemy");
				Slot.EnemyDefinition = Enemy;
				Encounter.EnemySlots = { MoveTemp(Slot) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Encounter)
		{
			return Result;
		}

		Result.HostBlueprint = BuildHostBlueprint(
			*Result.Enemy,
			*IdleFlipbook,
			GetSnakePartPresentationSpecs(),
			SnakeHostPackage,
			TEXT("BP_EnemyHost_Snake"),
			TEXT("Snake Host"),
			Result.bChanged,
			Result.Errors);
		if (!Result.HostBlueprint)
		{
			return Result;
		}
		Result.DebugHostBlueprint = BuildHostBlueprint(
			*Result.Enemy,
			*IdleFlipbook,
			GetSnakeDebugPartPresentationSpecs(),
			SnakeDebugHostPackage,
			TEXT("BP_SnakeHost_Debug"),
			TEXT("Debug Snake Host"),
			Result.bChanged,
			Result.Errors);
		return Result;
	}
}
