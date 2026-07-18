// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomEditor/Private/ContentBuilders/SnakeBuilder.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Cards/CardDefinition.h"
#include "Components/ChildActorComponent.h"
#include "Encounters/EncounterDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/Texture2D.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/GeneratedBattleContentTestAssets.h"
#include "Modules/ModuleManager.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

namespace WacomSnakeEnemyContentSpec
{
	const TCHAR* HostPath =
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake.BP_EnemyHost_Snake");
	const TCHAR* EncounterPath =
		TEXT("/Game/Wacom/Data/Encounters/DA_Encounter_SnakeSingle.DA_Encounter_SnakeSingle");
	const TCHAR* PlaceholderRoot =
		TEXT("/Game/Wacom/Art/Placeholders/Enemies/Snake");

	template <typename T>
	T* LoadAsset(const TCHAR* Path)
	{
		return LoadObject<T>(nullptr, Path);
	}

	const FWacomEnemyIntentSetDefinition* FindIntentSet(
		const UEnemyBehaviorDefinition& Behavior,
		FName IntentSetId)
	{
		for (const FWacomEnemyPhaseDefinition& Phase : Behavior.Phases)
		{
			if (const FWacomEnemyIntentSetDefinition* IntentSet =
				Phase.IntentSets.FindByPredicate(
					[IntentSetId](const FWacomEnemyIntentSetDefinition& Candidate)
					{
						return Candidate.IntentSetId == IntentSetId;
					}))
			{
				return IntentSet;
			}
		}
		return nullptr;
	}

	AWacomBattleEnemyPartActor* FindPart(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId)
	{
		for (AWacomBattleEnemyPartActor* Part : Host.GetBattleEnemyPartActors())
		{
			if (Part && Part->PartSlotId == PartSlotId)
			{
				return Part;
			}
		}
		return nullptr;
	}

	UChildActorComponent* FindPartComponent(
		AWacomBattleEnemyActor& Host,
		const AWacomBattleEnemyPartActor& Part)
	{
		UBlueprintGeneratedClass* BlueprintClass =
			Cast<UBlueprintGeneratedClass>(Host.GetClass());
		if (!BlueprintClass || !BlueprintClass->SimpleConstructionScript)
		{
			return nullptr;
		}
		for (USCS_Node* Node : BlueprintClass->SimpleConstructionScript->GetAllNodes())
		{
			UChildActorComponent* Component = Node
				? Cast<UChildActorComponent>(
					Node->GetActualComponentTemplate(BlueprintClass))
				: nullptr;
			if (Component && Component->GetChildActorTemplate() == &Part)
			{
				return Component;
			}
		}
		return nullptr;
	}

	bool IsUnderRoot(FName PackageName, const TCHAR* Root)
	{
		const FString PackageString = PackageName.ToString();
		const FString RootString(Root);
		return PackageString == RootString
			|| PackageString.StartsWith(RootString + TEXT("/"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataSnakeAssetContractSpec,
	"Wacom.Data.Enemy.Snake.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataSnakeAssetContractSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomSnakeEnemyContentSpec;
	UCardDefinition* Reward = FWacomGeneratedBattleContentAssets::LoadPoisonFang(*this);
	UEnemyDefinition* Enemy = FWacomGeneratedBattleContentAssets::LoadSnake(*this);
	UEnemyBehaviorDefinition* Behavior =
		FWacomGeneratedBattleContentAssets::LoadSnakeBehavior(*this);
	UEnemyPartDefinition* Head = FWacomGeneratedBattleContentAssets::LoadSnakeHead(*this);
	UEnemyPartDefinition* Body = FWacomGeneratedBattleContentAssets::LoadSnakeBody(*this);
	UEnemyPartDefinition* Tail = FWacomGeneratedBattleContentAssets::LoadSnakeTail(*this);
	UEncounterDefinition* Encounter = LoadAsset<UEncounterDefinition>(EncounterPath);
	if (!Reward || !Enemy || !Behavior || !Head || !Body || !Tail
		|| !TestNotNull(TEXT("Snake encounter loads"), Encounter))
	{
		return false;
	}

	TestEqual(TEXT("Reward CardId remains PoisonFang"), Reward->CardId,
		FName(TEXT("PoisonFang")));
	TestEqual(TEXT("PoisonFang cost"), Reward->BaseCost, 0);
	TestEqual(TEXT("PoisonFang target mode"), Reward->TargetMode,
		ECardTargetMode::SingleEnemyPart);
	TestEqual(TEXT("PoisonFang effect count"), Reward->Effects.Num(), 1);
	if (Reward->Effects.Num() == 1)
	{
		TestTrue(TEXT("PoisonFang applies poison"),
			Reward->Effects[0].EffectType == WacomTags::Effect_ApplyStatus_Poison);
		TestEqual(TEXT("PoisonFang magnitude"), Reward->Effects[0].Magnitude, 1);
		TestTrue(TEXT("PoisonFang targets one enemy part"),
			Reward->Effects[0].Target == WacomTags::Target_SingleEnemyPart);
	}
	TestEqual(TEXT("Snake EnemyId remains stable"), Enemy->EnemyId,
		FName(TEXT("Snake")));
	TestEqual(TEXT("Snake BehaviorId remains stable"), Behavior->BehaviorId,
		FName(TEXT("Snake.Behavior")));
	TestTrue(TEXT("Snake still references its Behavior"),
		Enemy->DefaultBehavior.Get() == Behavior);

	struct FPartExpectation
	{
		FName SlotId;
		FName PartId;
		UEnemyPartDefinition* Part;
		int32 MaxHp;
		int32 Experience;
		FName IntentSetId;
	};
	const TArray<FPartExpectation> PartExpectations = {
		{ TEXT("Head"), TEXT("Snake.Head"), Head, 16, 3, TEXT("Snake.Head.Sequence") },
		{ TEXT("Body"), TEXT("Snake.Body"), Body, 22, 2, TEXT("Snake.Body.Sequence") },
		{ TEXT("Tail"), TEXT("Snake.Tail"), Tail, 10, 2, TEXT("Snake.Tail.Sequence") },
	};
	TestEqual(TEXT("Snake keeps three ordered PartSlots"),
		Enemy->Parts.Num(), PartExpectations.Num());
	for (int32 Index = 0;
		Index < Enemy->Parts.Num() && Index < PartExpectations.Num();
		++Index)
	{
		const FPartExpectation& Expected = PartExpectations[Index];
		const FEnemyPartSlot& Slot = Enemy->Parts[Index];
		TestEqual(FString::Printf(TEXT("Part %d SlotId"), Index),
			Slot.PartSlotId, Expected.SlotId);
		TestEqual(FString::Printf(TEXT("Part %d IntentSetId"), Index),
			Slot.InitialIntentSetId, Expected.IntentSetId);
		TestTrue(FString::Printf(TEXT("Part %d definition reference"), Index),
			Slot.PartDef.Get() == Expected.Part);
		TestEqual(FString::Printf(TEXT("Part %d PartId"), Index),
			Expected.Part->PartId, Expected.PartId);
		TestEqual(FString::Printf(TEXT("Part %d HP"), Index),
			Expected.Part->MaxHp, Expected.MaxHp);
		TestEqual(FString::Printf(TEXT("Part %d experience"), Index),
			Expected.Part->ExperienceReward, Expected.Experience);
		TestTrue(FString::Printf(TEXT("Part %d PoisonFang reward"), Index),
			Expected.Part->KnockdownRewardCard.Get() == Reward);
	}

	struct FIntentExpectation
	{
		FName IntentId;
		int32 Initiative;
		int32 Resistance;
		int32 Magnitude;
		FGameplayTag EffectType;
		FGameplayTag Target;
	};
	TestEqual(TEXT("Snake keeps one behavior phase"), Behavior->Phases.Num(), 1);
	if (Behavior->Phases.Num() == 1)
	{
		TestEqual(TEXT("Default phase id"), Behavior->Phases[0].PhaseId,
			FName(TEXT("Default")));
		TestEqual(TEXT("Snake keeps three ordered IntentSets"),
			Behavior->Phases[0].IntentSets.Num(), 3);
		const TArray<FName> ExpectedIntentSetOrder = {
			TEXT("Snake.Head.Sequence"),
			TEXT("Snake.Body.Sequence"),
			TEXT("Snake.Tail.Sequence"),
		};
		for (int32 Index = 0;
			Index < Behavior->Phases[0].IntentSets.Num()
				&& Index < ExpectedIntentSetOrder.Num();
			++Index)
		{
			TestEqual(FString::Printf(TEXT("IntentSet %d order"), Index),
				Behavior->Phases[0].IntentSets[Index].IntentSetId,
				ExpectedIntentSetOrder[Index]);
		}
	}
	const TMap<FName, TArray<FIntentExpectation>> ExpectedIntents = {
		{ TEXT("Snake.Head.Sequence"), {
			{ TEXT("Snake.Head.Bite"), 3, 6, 6,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("Snake.Head.Venom"), 5, 0, 2,
				WacomTags::Effect_ApplyStatus_Poison, WacomTags::Target_Player },
			{ TEXT("Snake.Head.Strike"), 4, 8, 8,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("Snake.Head.CoiledGuard"), 2, 0, 4,
				WacomTags::Status_Shield, WacomTags::Target_Self },
		} },
		{ TEXT("Snake.Body.Sequence"), {
			{ TEXT("Snake.Body.Constrict"), 4, 0, 1,
				WacomTags::Effect_ApplyStatus_Slow, WacomTags::Target_Player },
			{ TEXT("Snake.Body.Harden"), 2, 0, 5,
				WacomTags::Status_Shield, WacomTags::Target_Self },
			{ TEXT("Snake.Body.Slam"), 3, 5, 5,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("Snake.Body.VenomMist"), 5, 0, 1,
				WacomTags::Effect_ApplyStatus_Poison, WacomTags::Target_Player },
		} },
		{ TEXT("Snake.Tail.Sequence"), {
			{ TEXT("Snake.Tail.Sweep"), 1, 3, 3,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("Snake.Tail.Lash"), 2, 5, 5,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("Snake.Tail.Whip"), 3, 4, 4,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("Snake.Tail.Brace"), 2, 0, 3,
				WacomTags::Status_Shield, WacomTags::Target_Self },
			{ TEXT("Snake.Tail.Tangle"), 4, 0, 1,
				WacomTags::Effect_ApplyStatus_Slow, WacomTags::Target_Player },
		} },
	};
	int32 IntentCount = 0;
	for (const TPair<FName, TArray<FIntentExpectation>>& Pair : ExpectedIntents)
	{
		const FWacomEnemyIntentSetDefinition* IntentSet =
			FindIntentSet(*Behavior, Pair.Key);
		if (!TestNotNull(
			FString::Printf(TEXT("%s loads"), *Pair.Key.ToString()), IntentSet))
		{
			continue;
		}
		TestEqual(FString::Printf(TEXT("%s intent count"), *Pair.Key.ToString()),
			IntentSet->Intents.Num(), Pair.Value.Num());
		IntentCount += IntentSet->Intents.Num();
		for (int32 Index = 0;
			Index < IntentSet->Intents.Num() && Index < Pair.Value.Num();
			++Index)
		{
			const FIntentDefinition& Intent = IntentSet->Intents[Index].Intent;
			const FIntentExpectation& Expected = Pair.Value[Index];
			TestEqual(FString::Printf(TEXT("Intent %s id"), *Expected.IntentId.ToString()),
				Intent.IntentId, Expected.IntentId);
			TestEqual(FString::Printf(TEXT("Intent %s initiative"), *Expected.IntentId.ToString()),
				Intent.Initiative, Expected.Initiative);
			TestEqual(FString::Printf(TEXT("Intent %s resistance"), *Expected.IntentId.ToString()),
				Intent.ResistanceValue, Expected.Resistance);
			TestEqual(FString::Printf(TEXT("Intent %s effect count"), *Expected.IntentId.ToString()),
				Intent.Effects.Num(), 1);
			if (Intent.Effects.Num() == 1)
			{
				TestEqual(FString::Printf(TEXT("Intent %s magnitude"), *Expected.IntentId.ToString()),
					Intent.Effects[0].Magnitude, Expected.Magnitude);
				TestTrue(FString::Printf(TEXT("Intent %s effect type"), *Expected.IntentId.ToString()),
					Intent.Effects[0].EffectType == Expected.EffectType);
				TestTrue(FString::Printf(TEXT("Intent %s target"), *Expected.IntentId.ToString()),
					Intent.Effects[0].Target == Expected.Target);
			}
		}
	}
	TestEqual(TEXT("Snake keeps all 13 Intents"), IntentCount, 13);
	TestEqual(TEXT("Encounter id remains stable"), Encounter->EncounterDefinitionId,
		FName(TEXT("Encounter.Snake.Single")));
	TestEqual(TEXT("Encounter keeps one Enemy slot"), Encounter->EnemySlots.Num(), 1);
	if (Encounter->EnemySlots.Num() == 1)
	{
		TestEqual(TEXT("Encounter EnemySlotId remains stable"),
			Encounter->EnemySlots[0].EnemySlotId, FName(TEXT("Enemy")));
		TestTrue(TEXT("Encounter references Snake"),
			Encounter->EnemySlots[0].EnemyDefinition.Get() == Enemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataSnakeHostAndPlaceholderArtSpec,
	"Wacom.Data.Enemy.Snake.HostAndPlaceholderArt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataSnakeHostAndPlaceholderArtSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomSnakeEnemyContentSpec;
	UBlueprint* Blueprint = LoadAsset<UBlueprint>(HostPath);
	if (!TestNotNull(TEXT("Snake Host Blueprint loads"), Blueprint)
		|| !TestNotNull(TEXT("Snake Host generated class"),
			Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
	{
		return false;
	}
	AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
		Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Snake Host CDO"), Host))
	{
		return false;
	}

	TestEqual(TEXT("Snake Host mode is MultiPartVisualLayers"),
		Host->HostAuthoringMode,
		EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers);
	TestEqual(TEXT("Snake Host EnemySlotId"), Host->EnemySlotId,
		FName(TEXT("Enemy")));
	TestTrue(TEXT("Snake Host references generated Snake definition"),
		Host->EnemyDefinition.Get()
			== FWacomGeneratedBattleContentAssets::LoadSnake(*this));
	TestNull(TEXT("Snake Host has no Sprite"), Host->HostSprite.Get());
	TestNull(TEXT("Snake Host has no Flipbook"), Host->HostFlipbook.Get());
	TestNull(TEXT("Snake Host has no semantic Host animation Style"),
		Host->HostAnimationStyle.Get());
	TestTrue(TEXT("Snake uses project default multi-part panel"),
		!Host->EnemyPanelWidgetClass);
	TestNotNull(TEXT("Snake Host uses formal impact Style"),
		Host->DefaultImpactStyle.Get());
	TestNotNull(TEXT("Snake Host uses formal target preview Style"),
		Host->DefaultTargetPreviewStyle.Get());
	TestTrue(TEXT("Snake badge stagger enabled"),
		Host->bApplyAttachedPartBadgeStagger);
	TestEqual(TEXT("Snake badge horizontal step"),
		Host->BadgeStaggerHorizontalStep, 28.0f);
	TestEqual(TEXT("Snake badge vertical step"),
		Host->BadgeStaggerVerticalStep, 18.0f);

	const FWacomBattleSceneEnemyHostAuthoringReport Report =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
	TestTrue(TEXT("Snake Host Authoring Report is Ready"), Report.bAuthoringReady);
	TestEqual(TEXT("Snake Host has exactly three PartActors"),
		Report.PartActorCount, 3);

	struct FPresentationExpectation
	{
		FName SlotId;
		FName PartId;
		FVector Location;
		FVector Bounds;
		float Scale;
		float StartTime;
		FLinearColor Tint;
		int32 SortOrder;
		const TCHAR* DestroyedName;
	};
	const TArray<FPresentationExpectation> Expectations = {
		{ TEXT("Head"), TEXT("Snake.Head"), FVector(96, -6, 16),
			FVector(42, 38, 42), 0.85f, 0.00f,
			FLinearColor(1.0f, 0.85f, 0.85f, 1.0f), 30,
			TEXT("PF_Enemy_SnakePlaceholder_Destroyed_Head") },
		{ TEXT("Body"), TEXT("Snake.Body"), FVector::ZeroVector,
			FVector(62, 46, 42), 1.00f, 0.04f,
			FLinearColor::White, 20,
			TEXT("PF_Enemy_SnakePlaceholder_Destroyed_Body") },
		{ TEXT("Tail"), TEXT("Snake.Tail"), FVector(-92, 16, -8),
			FVector(48, 34, 34), 0.70f, 0.08f,
			FLinearColor(0.82f, 0.90f, 1.0f, 1.0f), 10,
			TEXT("PF_Enemy_SnakePlaceholder_Destroyed_Tail") },
	};
	const TArray<AWacomBattleEnemyPartActor*> OrderedParts =
		Host->GetBattleEnemyPartActors();
	TestEqual(TEXT("Snake SCS Part order count"), OrderedParts.Num(), 3);
	for (int32 Index = 0;
		Index < OrderedParts.Num() && Index < Expectations.Num();
		++Index)
	{
		TestEqual(FString::Printf(TEXT("Snake SCS Part order %d"), Index),
			OrderedParts[Index] ? OrderedParts[Index]->PartSlotId : NAME_None,
			Expectations[Index].SlotId);
	}
	UPaperFlipbook* ExpectedIdle = LoadAsset<UPaperFlipbook>(
		TEXT("/Game/Wacom/Art/Placeholders/Enemies/Snake/Flipbooks/PF_Enemy_SnakePlaceholder_Idle.PF_Enemy_SnakePlaceholder_Idle"));
	TestNotNull(TEXT("Shared Snake Placeholder Idle loads"), ExpectedIdle);
	for (const FPresentationExpectation& Expected : Expectations)
	{
		AWacomBattleEnemyPartActor* Part = FindPart(*Host, Expected.SlotId);
		if (!TestNotNull(
			FString::Printf(TEXT("%s PartActor"), *Expected.SlotId.ToString()),
			Part))
		{
			continue;
		}
		TestEqual(TEXT("Derived PartId"), Part->PartId, Expected.PartId);
		TestEqual(TEXT("HitBounds"), Part->HitBoundsExtent, Expected.Bounds);
		TestEqual(TEXT("ImpactAnchor"), Part->ImpactAnchorRelativeLocation,
			FVector::ZeroVector);
		TestEqual(TEXT("Destroyed swap marker"),
			Part->DestroyedVisualSwapNormalizedTime, 0.35f);
		TestEqual(TEXT("One VisualLayer per Snake Part"),
			Part->VisualLayers.Num(), 1);
		if (Part->VisualLayers.Num() == 1)
		{
			const FWacomBattleEnemyPartVisualLayer& Layer = Part->VisualLayers[0];
			TestEqual(TEXT("Stable LayerId"), Layer.LayerId,
				FName(*FString::Printf(TEXT("Snake.%s.Main"),
					*Expected.SlotId.ToString())));
			TestEqual(TEXT("Layer mode"), Layer.LayerMode,
				EWacomBattleEnemyPartVisualLayerMode::Flipbook);
			TestNotNull(TEXT("Idle Flipbook"), Layer.Flipbook.Get());
			TestTrue(TEXT("All Snake parts share the formal Placeholder Idle"),
				Layer.Flipbook.Get() == ExpectedIdle);
			TestEqual(TEXT("Idle offset"), Layer.FlipbookStartTimeSeconds,
				Expected.StartTime);
			TestEqual(TEXT("Visual scale"), Layer.RelativeScale3D,
				FVector(Expected.Scale));
			TestEqual(TEXT("Visual tint"), Layer.Tint, Expected.Tint);
			TestEqual(TEXT("Visual sort order"), Layer.SortOrder,
				Expected.SortOrder);
			TestNotNull(TEXT("Destroyed Flipbook"),
				Layer.DestroyedFlipbook.Get());
			if (Layer.DestroyedFlipbook)
			{
				TestEqual(TEXT("Destroyed Flipbook name"),
					Layer.DestroyedFlipbook->GetName(), FString(Expected.DestroyedName));
				TestEqual(TEXT("Destroyed Flipbook is single-frame"),
					Layer.DestroyedFlipbook->GetNumKeyFrames(), 1);
			}
		}
		UChildActorComponent* Component = FindPartComponent(*Host, *Part);
		if (TestNotNull(TEXT("Part SCS component"), Component))
		{
			TestEqual(TEXT("Part SCS relative location"),
				Component->GetRelativeLocation(), Expected.Location);
		}
	}

	FAssetRegistryModule& RegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry& Registry = RegistryModule.Get();
	Registry.SearchAllAssets(/*bSynchronousSearch*/ true);
	TArray<FAssetData> PlaceholderAssets;
	Registry.GetAssetsByPath(
		FName(PlaceholderRoot), PlaceholderAssets,
		/*bRecursive*/ true,
		/*bIncludeOnlyOnDiskAssets*/ true);
	TestEqual(TEXT("Snake placeholder closure contains nine assets"),
		PlaceholderAssets.Num(), 9);
	int32 FlipbookCount = 0;
	int32 SpriteCount = 0;
	int32 TextureCount = 0;
	for (const FAssetData& Asset : PlaceholderAssets)
	{
		FlipbookCount += Asset.AssetClassPath
			== UPaperFlipbook::StaticClass()->GetClassPathName();
		SpriteCount += Asset.AssetClassPath
			== UPaperSprite::StaticClass()->GetClassPathName();
		TextureCount += Asset.AssetClassPath
			== UTexture2D::StaticClass()->GetClassPathName();
		TArray<FAssetDependency> Dependencies;
		Registry.GetDependencies(
			FAssetIdentifier(Asset.PackageName), Dependencies,
			UE::AssetRegistry::EDependencyCategory::Package);
		for (const FAssetDependency& Dependency : Dependencies)
		{
			TestFalse(TEXT("Placeholder art has no /Game/Art dependency"),
				IsUnderRoot(Dependency.AssetId.PackageName, TEXT("/Game/Art")));
			TestFalse(TEXT("Placeholder art has no /Game/Asset dependency"),
				IsUnderRoot(Dependency.AssetId.PackageName, TEXT("/Game/Asset")));
			TestFalse(TEXT("Placeholder art has no /Game/DreamMaterials dependency"),
				IsUnderRoot(
					Dependency.AssetId.PackageName, TEXT("/Game/DreamMaterials")));
		}
	}
	TestEqual(TEXT("Placeholder Flipbook count"), FlipbookCount, 4);
	TestEqual(TEXT("Placeholder Sprite count"), SpriteCount, 4);
	TestEqual(TEXT("Placeholder Texture count"), TextureCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataSnakeBuilderIdempotenceSpec,
	"Wacom.Data.Enemy.Snake.BuilderIdempotence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataSnakeBuilderIdempotenceSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomSnakeEnemyContentSpec;
	UEnemyPartDefinition* Head =
		FWacomGeneratedBattleContentAssets::LoadSnakeHead(*this);
	UEnemyPartDefinition* Body =
		FWacomGeneratedBattleContentAssets::LoadSnakeBody(*this);
	UEnemyPartDefinition* Tail =
		FWacomGeneratedBattleContentAssets::LoadSnakeTail(*this);
	if (!Head || !Body || !Tail)
	{
		return false;
	}

	const TArray<UEnemyPartDefinition*> Parts = { Head, Body, Tail };
	const bool bAnyLegacyReward = Parts.ContainsByPredicate(
		[](const UEnemyPartDefinition* Part)
		{
			return Part->KnockdownRewardCard != nullptr;
		});
	if (bAnyLegacyReward)
	{
		for (const UEnemyPartDefinition* Part : Parts)
		{
			TestNotNull(TEXT("Authorized legacy Snake reward remains available"),
				Part->KnockdownRewardCard.Get());
			TestNull(TEXT("Legacy Snake part has no explicit Aid reward"),
				Part->AidRewardCard.Get());
			TestNull(TEXT("Legacy Snake part has no explicit Destroy reward"),
				Part->DestroyRewardCard.Get());
		}
		AddInfo(TEXT(
			"Snake builder idempotence is deferred until the authorized reward-field asset migration."));
		return true;
	}

	const Wacom::ContentBuilder::FSnakeBuildResult Result =
		Wacom::ContentBuilder::BuildSnakeContent();
	TestTrue(TEXT("Snake Builder succeeds from committed formal Placeholder"),
		Result.IsSuccess());
	TestEqual(TEXT("Snake Builder error count"), Result.Errors.Num(), 0);
	TestFalse(TEXT("Second Snake build has no semantic changes"), Result.bChanged);
	if (Result.HostBlueprint)
	{
		TestFalse(TEXT("Idempotent build does not dirty Host package"),
			Result.HostBlueprint->GetOutermost()->IsDirty());
		AWacomBattleEnemyActor* Host = Result.HostBlueprint->GeneratedClass
			? Cast<AWacomBattleEnemyActor>(
				Result.HostBlueprint->GeneratedClass->GetDefaultObject())
			: nullptr;
		if (TestNotNull(TEXT("Idempotent Host CDO"), Host))
		{
			TestEqual(TEXT("Idempotent Host keeps three SCS parts"),
				Host->GetBattleEnemyPartActors().Num(), 3);
		}
	}
	return true;
}
