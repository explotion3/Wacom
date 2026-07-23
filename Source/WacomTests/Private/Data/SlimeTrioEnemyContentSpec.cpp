// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomEditor/Private/ContentBuilders/SlimeTrioBuilder.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Data/EnemyHostComponentTestHelpers.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Encounters/EncounterDefinition.h"
#include "Engine/Blueprint.h"
#include "Engine/BlueprintGeneratedClass.h"
#include "Engine/SCS_Node.h"
#include "Engine/SimpleConstructionScript.h"
#include "Engine/Texture2D.h"
#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Modules/ModuleManager.h"
#include "Misc/DataValidation.h"
#include "PaperFlipbook.h"
#include "PaperSprite.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEnemyPanelWidget.h"

namespace WacomSlimeTrioEnemyContentSpec
{
	const TCHAR* DataRoot = TEXT("/Game/Wacom/Data/Enemies/SlimeTrio");
	const TCHAR* PlaceholderRoot =
		TEXT("/Game/Wacom/Art/Placeholders/Enemies/SlimeTrio");
	const TCHAR* HostPath =
		TEXT("/Game/Wacom/Core/Enemy/BP_EnemyHost_SlimeTrio.BP_EnemyHost_SlimeTrio");
	const TCHAR* EncounterPath =
		TEXT("/Game/Wacom/Data/Encounters/DA_Encounter_SlimeTrioSingle.DA_Encounter_SlimeTrioSingle");

	template <typename T>
	T* LoadAsset(const FString& Path)
	{
		return LoadObject<T>(nullptr, *Path);
	}

	template <typename T>
	T* LoadDataAsset(const TCHAR* AssetName)
	{
		const FString Path = FString::Printf(
			TEXT("%s/%s.%s"), DataRoot, AssetName, AssetName);
		return LoadAsset<T>(Path);
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

	bool IsUnderRoot(FName PackageName, const TCHAR* Root)
	{
		const FString PackageString = PackageName.ToString();
		const FString RootString(Root);
		return PackageString == RootString
			|| PackageString.StartsWith(RootString + TEXT("/"));
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataSlimeTrioAssetContractSpec,
	"Wacom.Data.Enemy.SlimeTrio.AssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataSlimeTrioAssetContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomSlimeTrioEnemyContentSpec;
	UEnemyBehaviorDefinition* Behavior =
		LoadDataAsset<UEnemyBehaviorDefinition>(TEXT("DA_Behavior_SlimeTrio"));
	UEnemyPartDefinition* Left =
		LoadDataAsset<UEnemyPartDefinition>(TEXT("DA_Part_SlimeTrio_Left"));
	UEnemyPartDefinition* Core =
		LoadDataAsset<UEnemyPartDefinition>(TEXT("DA_Part_SlimeTrio_Core"));
	UEnemyPartDefinition* Right =
		LoadDataAsset<UEnemyPartDefinition>(TEXT("DA_Part_SlimeTrio_Right"));
	UEnemyDefinition* Enemy =
		LoadDataAsset<UEnemyDefinition>(TEXT("DA_Enemy_SlimeTrio"));
	UEncounterDefinition* Encounter =
		LoadAsset<UEncounterDefinition>(EncounterPath);
	if (!TestNotNull(TEXT("Behavior loads"), Behavior)
		|| !TestNotNull(TEXT("Left Part loads"), Left)
		|| !TestNotNull(TEXT("Core Part loads"), Core)
		|| !TestNotNull(TEXT("Right Part loads"), Right)
		|| !TestNotNull(TEXT("Enemy loads"), Enemy)
		|| !TestNotNull(TEXT("Encounter loads"), Encounter))
	{
		return false;
	}

	TestEqual(TEXT("EnemyId"), Enemy->EnemyId, FName(TEXT("Enemy.SlimeTrio")));
	TestEqual(TEXT("BehaviorId"), Behavior->BehaviorId,
		FName(TEXT("SlimeTrio.Behavior")));
	TestTrue(TEXT("Enemy references Behavior"),
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
		{ TEXT("Left"), TEXT("SlimeTrio.Left"), Left, 12, 1,
			TEXT("SlimeTrio.Left.Sequence") },
		{ TEXT("Core"), TEXT("SlimeTrio.Core"), Core, 20, 2,
			TEXT("SlimeTrio.Core.Sequence") },
		{ TEXT("Right"), TEXT("SlimeTrio.Right"), Right, 12, 1,
			TEXT("SlimeTrio.Right.Sequence") },
	};
	TestEqual(TEXT("Three ordered PartSlots"), Enemy->Parts.Num(), 3);
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
		TestTrue(FString::Printf(TEXT("Part %d definition"), Index),
			Slot.PartDef.Get() == Expected.Part);
		TestEqual(TEXT("Stable PartId"), Expected.Part->PartId, Expected.PartId);
		TestEqual(TEXT("Part HP"), Expected.Part->MaxHp, Expected.MaxHp);
		TestEqual(TEXT("Part XP"),
			Expected.Part->ExperienceReward, Expected.Experience);
		TestNull(TEXT("No Aid reward"), Expected.Part->AidRewardCard.Get());
		TestNull(TEXT("No Destroy reward"), Expected.Part->DestroyRewardCard.Get());
		TestNull(TEXT("No legacy reward"),
			Expected.Part->KnockdownRewardCard.Get());
	}

	struct FIntentExpectation
	{
		FName IntentId;
		int32 Initiative;
		int32 Magnitude;
		FGameplayTag EffectType;
		FGameplayTag Target;
	};
	const TMap<FName, TArray<FIntentExpectation>> ExpectedIntents = {
		{ TEXT("SlimeTrio.Left.Sequence"), {
			{ TEXT("SlimeTrio.Left.Bump"), 2, 3,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("SlimeTrio.Left.Coat"), 3, 3,
				WacomTags::Status_Shield, WacomTags::Target_Self },
		} },
		{ TEXT("SlimeTrio.Core.Sequence"), {
			{ TEXT("SlimeTrio.Core.Slam"), 4, 6,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("SlimeTrio.Core.Harden"), 3, 5,
				WacomTags::Status_Shield, WacomTags::Target_Self },
		} },
		{ TEXT("SlimeTrio.Right.Sequence"), {
			{ TEXT("SlimeTrio.Right.Bump"), 2, 3,
				WacomTags::Effect_Damage, WacomTags::Target_Player },
			{ TEXT("SlimeTrio.Right.ToxicSpit"), 4, 1,
				WacomTags::Effect_ApplyStatus_Poison, WacomTags::Target_Player },
		} },
	};
	TestEqual(TEXT("One behavior phase"), Behavior->Phases.Num(), 1);
	if (Behavior->Phases.Num() == 1)
	{
		const TArray<FName> ExpectedSetOrder = {
			TEXT("SlimeTrio.Left.Sequence"),
			TEXT("SlimeTrio.Core.Sequence"),
			TEXT("SlimeTrio.Right.Sequence"),
		};
		TestEqual(TEXT("Three ordered IntentSets"),
			Behavior->Phases[0].IntentSets.Num(), 3);
		for (int32 Index = 0;
			Index < Behavior->Phases[0].IntentSets.Num()
				&& Index < ExpectedSetOrder.Num();
			++Index)
		{
			TestEqual(FString::Printf(TEXT("IntentSet %d order"), Index),
				Behavior->Phases[0].IntentSets[Index].IntentSetId,
				ExpectedSetOrder[Index]);
		}
	}
	for (const TPair<FName, TArray<FIntentExpectation>>& Pair : ExpectedIntents)
	{
		const FWacomEnemyIntentSetDefinition* IntentSet =
			FindIntentSet(*Behavior, Pair.Key);
		if (!TestNotNull(
			FString::Printf(TEXT("%s loads"), *Pair.Key.ToString()), IntentSet))
		{
			continue;
		}
		TestEqual(TEXT("Intent count"), IntentSet->Intents.Num(), 2);
		for (int32 Index = 0;
			Index < IntentSet->Intents.Num() && Index < Pair.Value.Num();
			++Index)
		{
			const FIntentDefinition& Intent = IntentSet->Intents[Index].Intent;
			const FIntentExpectation& Expected = Pair.Value[Index];
			TestEqual(TEXT("IntentId"), Intent.IntentId, Expected.IntentId);
			TestEqual(TEXT("Initiative"), Intent.Initiative, Expected.Initiative);
			TestEqual(TEXT("One effect"), Intent.Effects.Num(), 1);
			if (Intent.Effects.Num() == 1)
			{
				TestEqual(TEXT("Magnitude"),
					Intent.Effects[0].Magnitude, Expected.Magnitude);
				TestTrue(TEXT("Effect type"),
					Intent.Effects[0].EffectType == Expected.EffectType);
				TestTrue(TEXT("Effect target"),
					Intent.Effects[0].Target == Expected.Target);
			}
		}
	}

	TestEqual(TEXT("Encounter id"), Encounter->EncounterDefinitionId,
		FName(TEXT("Encounter.SlimeTrio.Single")));
	TestEqual(TEXT("Encounter has one slot"), Encounter->EnemySlots.Num(), 1);
	if (Encounter->EnemySlots.Num() == 1)
	{
		TestEqual(TEXT("EnemySlotId"), Encounter->EnemySlots[0].EnemySlotId,
			FName(TEXT("Enemy")));
		TestTrue(TEXT("Encounter references SlimeTrio"),
			Encounter->EnemySlots[0].EnemyDefinition.Get() == Enemy);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomDataSlimeTrioHostAndPlaceholderArtSpec,
	"Wacom.Data.Enemy.SlimeTrio.HostAndPlaceholderArt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataSlimeTrioHostAndPlaceholderArtSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomSlimeTrioEnemyContentSpec;
	UBlueprint* Blueprint = LoadAsset<UBlueprint>(HostPath);
	if (!TestNotNull(TEXT("Host Blueprint loads"), Blueprint)
		|| !TestNotNull(TEXT("Host generated class"),
			Blueprint ? Blueprint->GeneratedClass.Get() : nullptr))
	{
		return false;
	}
	AWacomBattleEnemyActor* Host = Cast<AWacomBattleEnemyActor>(
		Blueprint->GeneratedClass->GetDefaultObject());
	if (!TestNotNull(TEXT("Host CDO"), Host))
	{
		return false;
	}

	TestEqual(TEXT("EnemySlotId"), Host->EnemySlotId, FName(TEXT("Enemy")));
	TestTrue(TEXT("Uses project default multi-part panel"),
		!Host->EnemyPanelWidgetClass);
	TestNotNull(TEXT("Formal impact Style"), Host->DefaultImpactStyle.Get());
	TestNotNull(TEXT("Formal target preview Style"),
		Host->DefaultTargetPreviewStyle.Get());

	const FWacomBattleSceneEnemyHostAuthoringReport Report =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
	TestTrue(TEXT("Authoring Report is Ready"), Report.bAuthoringReady);
	TestEqual(TEXT("Exactly three Part components"), Report.PartComponentCount, 3);
	FDataValidationContext HostValidation;
	TestEqual(TEXT("Host validation"), Host->IsDataValid(HostValidation),
		EDataValidationResult::Valid);

	struct FPresentationExpectation
	{
		FName SlotId;
		FName PartId;
		FVector Location;
		float Scale;
		float StartTime;
		FLinearColor Tint;
		int32 SortOrder;
		const TCHAR* DestroyedName;
		int32 DestroyedSourceFrame;
	};
	const TArray<FPresentationExpectation> Expectations = {
		{ TEXT("Left"), TEXT("SlimeTrio.Left"), FVector(-88, 8, -6),
			0.90f, 0.00f,
			FLinearColor(0.84f, 1.0f, 0.90f, 1.0f), 10,
			TEXT("PF_Enemy_SlimeTrioPlaceholder_Destroyed_Left"), 1 },
		{ TEXT("Core"), TEXT("SlimeTrio.Core"), FVector(0, 0, 8),
			1.10f, 0.04f,
			FLinearColor::White, 20,
			TEXT("PF_Enemy_SlimeTrioPlaceholder_Destroyed_Core"), 2 },
		{ TEXT("Right"), TEXT("SlimeTrio.Right"), FVector(88, -8, -6),
			0.90f, 0.08f,
			FLinearColor(0.86f, 0.92f, 1.0f, 1.0f), 30,
			TEXT("PF_Enemy_SlimeTrioPlaceholder_Destroyed_Right"), 3 },
	};
	const TArray<Wacom::Tests::EnemyHostComponents::FPartTemplates> OrderedParts =
		Wacom::Tests::EnemyHostComponents::OrderByDefinition(
			*Blueprint, Host->EnemyDefinition);
	TestEqual(TEXT("SCS Part count"), OrderedParts.Num(), 3);
	UPaperFlipbook* ExpectedIdle = LoadAsset<UPaperFlipbook>(FString::Printf(
		TEXT("%s/Flipbooks/PF_Enemy_SlimeTrioPlaceholder_Idle.PF_Enemy_SlimeTrioPlaceholder_Idle"),
		PlaceholderRoot));
	TestNotNull(TEXT("Shared Placeholder Idle loads"), ExpectedIdle);
	for (int32 Index = 0;
		Index < OrderedParts.Num() && Index < Expectations.Num();
		++Index)
	{
		TestEqual(FString::Printf(TEXT("SCS order %d"), Index),
			OrderedParts[Index].Part ? OrderedParts[Index].Part->PartSlotId : NAME_None,
			Expectations[Index].SlotId);
	}
	for (const FPresentationExpectation& Expected : Expectations)
	{
		const Wacom::Tests::EnemyHostComponents::FPartTemplates Templates =
			Wacom::Tests::EnemyHostComponents::Find(*Blueprint, Expected.SlotId);
		UWacomBattleEnemyPartComponent* Part = Templates.Part;
		if (!TestNotNull(
			FString::Printf(TEXT("%s PartActor"), *Expected.SlotId.ToString()),
			Part))
		{
			continue;
		}
		TestEqual(TEXT("Derived PartId"), Part->PartId, Expected.PartId);
		TestEqual(TEXT("Part viewport transform"),
			Part->GetRelativeLocation(), Expected.Location);
		if (TestNotNull(TEXT("ImpactAnchor"), Templates.ImpactAnchor))
		{
			TestEqual(TEXT("ImpactAnchor transform"),
				Templates.ImpactAnchor->GetRelativeLocation(), FVector::ZeroVector);
		}
		TestEqual(TEXT("Destroyed swap marker"),
			Part->DestroyedVisualSwapNormalizedTime, 0.35f);
		TestNull(TEXT("No action animation Style"),
			Part->PartAnimationStyle.Get());
		UWacomBattleEnemyPartFlipbookLayerComponent* Layer = Templates.Flipbook;
		if (TestNotNull(TEXT("One typed Flipbook layer"), Layer))
		{
			TestEqual(TEXT("Stable LayerId"), Layer->LayerId,
				FName(*FString::Printf(TEXT("SlimeTrio.%s.Main"),
					*Expected.SlotId.ToString())));
			TestTrue(TEXT("All parts share Idle"),
				Layer->GetFlipbook() == ExpectedIdle);
			TestEqual(TEXT("Idle offset"), Layer->InitialPlaybackPositionSeconds,
				Expected.StartTime);
			TestEqual(TEXT("Visual scale"), Layer->GetRelativeScale3D(),
				FVector(Expected.Scale));
			TestEqual(TEXT("Visual tint"), Layer->GetSpriteColor(), Expected.Tint);
			TestEqual(TEXT("Sort order"), Layer->TranslucencySortPriority, Expected.SortOrder);
			UPaperFlipbook* Destroyed = Layer->DestroyedFlipbook.Get();
			if (TestNotNull(TEXT("Destroyed Flipbook"), Destroyed))
			{
				TestEqual(TEXT("Destroyed name"), Destroyed->GetName(),
					FString(Expected.DestroyedName));
				TestEqual(TEXT("Destroyed is single-frame"),
					Destroyed->GetNumKeyFrames(), 1);
				UPaperSprite* ExpectedDestroyedSprite = LoadAsset<UPaperSprite>(
					FString::Printf(
						TEXT("%s/Sprites/SPR_Enemy_SlimeTrioPlaceholder_Idle_%02d.SPR_Enemy_SlimeTrioPlaceholder_Idle_%02d"),
						PlaceholderRoot,
						Expected.DestroyedSourceFrame,
						Expected.DestroyedSourceFrame));
				TestNotNull(TEXT("Destroyed source Sprite"), ExpectedDestroyedSprite);
				if (Destroyed->GetNumKeyFrames() == 1)
				{
					TestTrue(TEXT("Destroyed uses configured source frame"),
						Destroyed->GetKeyFrameChecked(0).Sprite
							== ExpectedDestroyedSprite);
				}
			}
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
	TestEqual(TEXT("Independent Placeholder closure contains nine assets"),
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
			TestFalse(TEXT("No /Game/Art dependency"),
				IsUnderRoot(Dependency.AssetId.PackageName, TEXT("/Game/Art")));
			TestFalse(TEXT("No /Game/Asset dependency"),
				IsUnderRoot(Dependency.AssetId.PackageName, TEXT("/Game/Asset")));
			TestFalse(TEXT("No DreamMaterials dependency"),
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
	FWacomDataSlimeTrioBuilderIdempotenceSpec,
	"Wacom.Data.Enemy.SlimeTrio.BuilderIdempotence",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomDataSlimeTrioBuilderIdempotenceSpec::RunTest(
	const FString& /*Parameters*/)
{
	const Wacom::ContentBuilder::FSlimeTrioBuildResult Result =
		Wacom::ContentBuilder::BuildSlimeTrioContent();
	TestTrue(TEXT("Builder succeeds from committed Placeholder"),
		Result.IsSuccess());
	TestEqual(TEXT("Builder error count"), Result.Errors.Num(), 0);
	TestFalse(TEXT("Second build has no semantic changes"), Result.bChanged);
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
				Wacom::Tests::EnemyHostComponents::Collect(*Result.HostBlueprint).Num(), 3);
		}
	}
	return true;
}
