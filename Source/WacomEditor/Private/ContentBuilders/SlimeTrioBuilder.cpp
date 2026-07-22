// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/SlimeTrioBuilder.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
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

namespace
{
	using namespace Wacom::ContentBuilder;

	const FString SlimeTrioDataRoot =
		TEXT("/Game/Wacom/Data/Enemies/SlimeTrio");
	const FString SlimeTrioPlaceholderArtRoot =
		TEXT("/Game/Wacom/Art/Placeholders/Enemies/SlimeTrio");
	const FString SlimeTrioHostPackage =
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_SlimeTrio");

	struct FSlimeTrioPartPresentationSpec
	{
		FName PartSlotId;
		FName PartId;
		FVector RelativeLocation;
		FVector HitBoundsExtent;
		float VisualScale = 1.0f;
		float IdleOffsetSeconds = 0.0f;
		FLinearColor Tint = FLinearColor::White;
		int32 SortOrder = 0;
		const TCHAR* DestroyedFlipbookName = nullptr;
	};

	const TArray<FSlimeTrioPartPresentationSpec>& GetPartPresentationSpecs()
	{
		static const TArray<FSlimeTrioPartPresentationSpec> Specs = {
			{
				TEXT("Left"), TEXT("SlimeTrio.Left"),
				FVector(-88.0f, 8.0f, -6.0f), FVector(46.0f, 38.0f, 40.0f),
				0.90f, 0.00f, FLinearColor(0.84f, 1.0f, 0.90f, 1.0f), 10,
				TEXT("PF_Enemy_SlimeTrioPlaceholder_Destroyed_Left")
			},
			{
				TEXT("Core"), TEXT("SlimeTrio.Core"),
				FVector(0.0f, 0.0f, 8.0f), FVector(56.0f, 44.0f, 48.0f),
				1.10f, 0.04f, FLinearColor::White, 20,
				TEXT("PF_Enemy_SlimeTrioPlaceholder_Destroyed_Core")
			},
			{
				TEXT("Right"), TEXT("SlimeTrio.Right"),
				FVector(88.0f, -8.0f, -6.0f), FVector(46.0f, 38.0f, 40.0f),
				0.90f, 0.08f, FLinearColor(0.86f, 0.92f, 1.0f, 1.0f), 30,
				TEXT("PF_Enemy_SlimeTrioPlaceholder_Destroyed_Right")
			},
		};
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

	bool ApplyPartPresentation(
		UBlueprint& Blueprint,
		const FSlimeTrioPartPresentationSpec& Spec,
		UPaperFlipbook& IdleFlipbook,
		UPaperFlipbook& DestroyedFlipbook)
	{
		Wacom::EnemyHostComponentBuilder::FFlipbookPartSpec ComponentSpec;
		ComponentSpec.PartSlotId = Spec.PartSlotId;
		ComponentSpec.PartId = Spec.PartId;
		ComponentSpec.LayerId = FName(*FString::Printf(
			TEXT("SlimeTrio.%s.Main"), *Spec.PartSlotId.ToString()));
		ComponentSpec.RelativeLocation = Spec.RelativeLocation;
		ComponentSpec.HitBoundsExtent = Spec.HitBoundsExtent;
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
		bool& bOutChanged,
		TArray<FString>& OutErrors)
	{
		UPackage* Package = FindOrCreatePackage(SlimeTrioHostPackage);
		if (!Package)
		{
			OutErrors.Add(TEXT("Could not create SlimeTrio Host package"));
			return nullptr;
		}

		const FName AssetName(TEXT("BP_EnemyHost_SlimeTrio"));
		UObject* ExistingObject = StaticFindObject(
			UObject::StaticClass(), Package, *AssetName.ToString());
		UBlueprint* Blueprint = Cast<UBlueprint>(ExistingObject);
		bool bChanged = false;
		if (ExistingObject && !Blueprint)
		{
			OutErrors.Add(FString::Printf(
				TEXT("Existing SlimeTrio Host object has unexpected class %s"),
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
			|| !Blueprint->GeneratedClass->IsChildOf(
				AWacomBattleEnemyActor::StaticClass()))
		{
			OutErrors.Add(TEXT("SlimeTrio Host Blueprint is invalid"));
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
			OutErrors.Add(
				TEXT("SlimeTrio Host requires a valid CDO and the formal Pixel styles"));
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
			OutErrors.Add(TEXT("SlimeTrio Host part synchronization failed"));
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
					TEXT("SlimeTrio Host compile after part synchronization failed"));
				return nullptr;
			}
			Host = Cast<AWacomBattleEnemyActor>(
				Blueprint->GeneratedClass->GetDefaultObject());
		}

		if (!Host)
		{
			OutErrors.Add(TEXT("SlimeTrio Host CDO was not regenerated"));
			return nullptr;
		}
		for (const FSlimeTrioPartPresentationSpec& Spec : GetPartPresentationSpecs())
		{
			UPaperFlipbook* DestroyedFlipbook = LoadGeneratedAsset<UPaperFlipbook>(
				SlimeTrioPlaceholderArtRoot / TEXT("Flipbooks")
				/ Spec.DestroyedFlipbookName);
			if (!Wacom::EnemyHostComponentBuilder::FindPartTemplates(
					*Blueprint, Spec.PartSlotId).IsComplete()
				|| !DestroyedFlipbook)
			{
				OutErrors.Add(FString::Printf(
					TEXT("SlimeTrio Host part contract is invalid: Slot=%s Part=%s Component=%s ActualPartId=%s ExpectedPartId=%s Destroyed=%s"),
					*Spec.PartSlotId.ToString(),
					TEXT("<missing>"),
					TEXT("<missing>"),
					TEXT("<missing>"),
					*Spec.PartId.ToString(),
					*GetNameSafe(DestroyedFlipbook)));
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
			OutErrors.Add(TEXT("SlimeTrio Host Blueprint compile failed"));
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
				TEXT("SlimeTrio Host authoring report is %s with %d parts"),
				*Report.AuthoringState.ToString(),
				Report.PartComponentCount));
			return nullptr;
		}
		if (bChanged
			&& !SaveAssetPackage(Package, Blueprint, SlimeTrioHostPackage))
		{
			OutErrors.Add(TEXT("SlimeTrio Host Blueprint save failed"));
			return nullptr;
		}
		UPackage::WaitForAsyncFileWrites();

		UBlueprint* PersistedBlueprint =
			LoadGeneratedAsset<UBlueprint>(SlimeTrioHostPackage);
		if (!PersistedBlueprint || !PersistedBlueprint->GeneratedClass)
		{
			OutErrors.Add(TEXT("SlimeTrio Host Blueprint reload verification failed"));
			return nullptr;
		}
		bOutChanged |= bChanged;
		return PersistedBlueprint;
	}
}

namespace Wacom::ContentBuilder
{
	FSlimeTrioBuildResult BuildSlimeTrioContent()
	{
		FSlimeTrioBuildResult Result;
		UPaperFlipbook* IdleFlipbook = LoadGeneratedAsset<UPaperFlipbook>(
			SlimeTrioPlaceholderArtRoot
			/ TEXT("Flipbooks/PF_Enemy_SlimeTrioPlaceholder_Idle"));
		if (!IdleFlipbook)
		{
			Result.Errors.Add(
				TEXT("SlimeTrio placeholder art is incomplete; run WacomBuildEnemyPack -Pack=SlimeTrio -PromotePlaceholderArt"));
			return Result;
		}

		Result.Behavior = BuildDataAsset<UEnemyBehaviorDefinition>(
			MakePackagePath(SlimeTrioDataRoot, TEXT("DA_Behavior_SlimeTrio")),
			TEXT("DA_Behavior_SlimeTrio"),
			[](UEnemyBehaviorDefinition& Behavior)
			{
				Behavior.BehaviorId = TEXT("SlimeTrio.Behavior");
				Behavior.InitialPhaseId = TEXT("Default");
				FWacomEnemyPhaseDefinition Phase;
				Phase.PhaseId = TEXT("Default");
				Phase.IntentSets = {
					MakeSequenceIntentSet(
						TEXT("SlimeTrio.Left.Sequence"), TEXT("Left"),
						{
							MakeBehaviorIntent(
								TEXT("SlimeTrio.Left.Bump"), TEXT("Bump"),
								2, MakeDamage(3)),
							MakeBehaviorIntent(
								TEXT("SlimeTrio.Left.Coat"), TEXT("Coat"),
								3, MakeShieldSelf(3)),
						}),
					MakeSequenceIntentSet(
						TEXT("SlimeTrio.Core.Sequence"), TEXT("Core"),
						{
							MakeBehaviorIntent(
								TEXT("SlimeTrio.Core.Slam"), TEXT("Slam"),
								4, MakeDamage(6)),
							MakeBehaviorIntent(
								TEXT("SlimeTrio.Core.Harden"), TEXT("Harden"),
								3, MakeShieldSelf(5)),
						}),
					MakeSequenceIntentSet(
						TEXT("SlimeTrio.Right.Sequence"), TEXT("Right"),
						{
							MakeBehaviorIntent(
								TEXT("SlimeTrio.Right.Bump"), TEXT("Bump"),
								2, MakeDamage(3)),
							MakeBehaviorIntent(
								TEXT("SlimeTrio.Right.ToxicSpit"), TEXT("Toxic Spit"),
								4, MakePoisonOnPlayer(1)),
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

		auto BuildPart = [&Result](
			const TCHAR* AssetName,
			FName PartId,
			const TCHAR* DisplayName,
			int32 MaxHp,
			int32 ExperienceReward)
		{
			return BuildDataAsset<UEnemyPartDefinition>(
				MakePackagePath(SlimeTrioDataRoot, AssetName),
				FName(AssetName),
				[PartId, DisplayName, MaxHp, ExperienceReward](
					UEnemyPartDefinition& Part)
				{
					Part.PartId = PartId;
					Part.DisplayName = FText::FromString(DisplayName);
					Part.MaxHp = MaxHp;
					Part.ExperienceReward = ExperienceReward;
					Part.AidRewardCard = nullptr;
					Part.DestroyRewardCard = nullptr;
					Part.KnockdownRewardCard = nullptr;
				},
				Result.bChanged,
				Result.Errors);
		};
		Result.LeftPart = BuildPart(
			TEXT("DA_Part_SlimeTrio_Left"), TEXT("SlimeTrio.Left"),
			TEXT("Left Slime"), 12, 1);
		Result.CorePart = BuildPart(
			TEXT("DA_Part_SlimeTrio_Core"), TEXT("SlimeTrio.Core"),
			TEXT("Core Slime"), 20, 2);
		Result.RightPart = BuildPart(
			TEXT("DA_Part_SlimeTrio_Right"), TEXT("SlimeTrio.Right"),
			TEXT("Right Slime"), 12, 1);
		if (!Result.LeftPart || !Result.CorePart || !Result.RightPart)
		{
			return Result;
		}

		UEnemyBehaviorDefinition* Behavior = Result.Behavior;
		UEnemyPartDefinition* Left = Result.LeftPart;
		UEnemyPartDefinition* Core = Result.CorePart;
		UEnemyPartDefinition* Right = Result.RightPart;
		Result.Enemy = BuildDataAsset<UEnemyDefinition>(
			MakePackagePath(SlimeTrioDataRoot, TEXT("DA_Enemy_SlimeTrio")),
			TEXT("DA_Enemy_SlimeTrio"),
			[Behavior, Left, Core, Right](UEnemyDefinition& Enemy)
			{
				Enemy.EnemyId = TEXT("Enemy.SlimeTrio");
				Enemy.DisplayName = FText::FromString(TEXT("Slime Trio"));
				Enemy.DefaultBehavior = Behavior;
				Enemy.DefaultPhaseId = TEXT("Default");
				FEnemyPartSlot LeftSlot;
				LeftSlot.PartSlotId = TEXT("Left");
				LeftSlot.PartDef = Left;
				LeftSlot.InitialIntentSetId = TEXT("SlimeTrio.Left.Sequence");
				FEnemyPartSlot CoreSlot;
				CoreSlot.PartSlotId = TEXT("Core");
				CoreSlot.PartDef = Core;
				CoreSlot.InitialIntentSetId = TEXT("SlimeTrio.Core.Sequence");
				FEnemyPartSlot RightSlot;
				RightSlot.PartSlotId = TEXT("Right");
				RightSlot.PartDef = Right;
				RightSlot.InitialIntentSetId = TEXT("SlimeTrio.Right.Sequence");
				Enemy.Parts = {
					MoveTemp(LeftSlot), MoveTemp(CoreSlot), MoveTemp(RightSlot) };
			},
			Result.bChanged,
			Result.Errors);
		if (!Result.Enemy)
		{
			return Result;
		}

		UEnemyDefinition* Enemy = Result.Enemy;
		Result.Encounter = BuildDataAsset<UEncounterDefinition>(
			MakePackagePath(
				EncountersRoot(), TEXT("DA_Encounter_SlimeTrioSingle")),
			TEXT("DA_Encounter_SlimeTrioSingle"),
			[Enemy](UEncounterDefinition& Encounter)
			{
				Encounter.EncounterDefinitionId = TEXT("Encounter.SlimeTrio.Single");
				Encounter.DisplayName = FText::FromString(TEXT("Slime Trio"));
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
			Result.bChanged,
			Result.Errors);
		return Result;
	}
}
