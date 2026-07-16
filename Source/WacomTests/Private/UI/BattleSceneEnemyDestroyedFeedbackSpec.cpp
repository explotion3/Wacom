// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleEnemyPartImpactStyle.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "Snapshots/EnemySnapshot.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "WacomBattleEnemyPartPresentationTestAccess.h"

#include <limits>

#if WITH_AUTOMATION_TESTS

namespace WacomBattleSceneEnemyDestroyedFeedbackSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	UPaperFlipbook* MakeFlipbook(UObject* Outer, FName Name, int32 FrameRun = 4)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer, Name);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 10.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = FrameRun;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	FWacomBattlePresentationTargetCue MakeDestroyedCue(int32 Seed = 731)
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = EWacomBattlePresentationTargetCueKind::EnemyPartHpEmptied;
		Cue.SourceEventType = EBattleEventType::EnemyPartHpEmptied;
		Cue.Duration = 0.30f;
		Cue.Seed = Seed;
		return Cue;
	}

	template <typename TComponent>
	TComponent* FindGeneratedVisualLayer(AActor& Actor)
	{
		TInlineComponentArray<TComponent*> Components;
		Actor.GetComponents(Components);
		for (TComponent* Component : Components)
		{
			if (Component && Component->GetName().StartsWith(TEXT("VisualLayer_")))
			{
				return Component;
			}
		}
		return nullptr;
	}

	struct FSpawnedScenePart
	{
		AWacomBattleEnemyActor* Host = nullptr;
		AWacomBattleEnemyPartActor* Part = nullptr;
	};

	FSpawnedScenePart SpawnScenePart(UWorld& World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		FSpawnedScenePart Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(), FTransform::Identity, SpawnParams);
		Result.Part = World.SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(), FTransform::Identity, SpawnParams);
		if (Result.Part)
		{
			Result.Part->PartId = TEXT("Test.Part.Body");
			Result.Part->PartSlotId = TEXT("Body");
			Result.Part->SetEnemySlotId(TEXT("Enemy"));
			if (Result.Host)
			{
				Result.Part->AttachToActor(
					Result.Host,
					FAttachmentTransformRules::KeepWorldTransform);
			}
		}
		return Result;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyDestroyedFeedbackValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyDestroyedFeedback.AuthoringValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyDestroyedFeedbackValidationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyDestroyedFeedbackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const FSpawnedScenePart ScenePart = SpawnScenePart(*World);
	AWacomBattleEnemyActor* Host = ScenePart.Host;
	AWacomBattleEnemyPartActor* Part = ScenePart.Part;
	if (!TestNotNull(TEXT("Scene enemy Part"), Part))
	{
		if (IsValid(Host)) { Host->Destroy(); }
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
	};

	FWacomBattleEnemyPartVisualLayer MismatchedLayer;
	MismatchedLayer.LayerId = TEXT("Body.Invalid");
	MismatchedLayer.Sprite = NewObject<UPaperSprite>(Part);
	MismatchedLayer.DestroyedFlipbook = MakeFlipbook(Part, TEXT("WrongModeDestroyed"));
	Part->VisualLayers = { MismatchedLayer };
	Part->DestroyedVisualSwapNormalizedTime = std::numeric_limits<float>::quiet_NaN();
	FDataValidationContext MismatchContext;
	TestEqual(TEXT("Mode mismatch and non-finite marker are invalid"),
		Part->IsDataValid(MismatchContext), EDataValidationResult::Invalid);

	FWacomBattleEnemyPartVisualLayer InvalidRateLayer;
	InvalidRateLayer.LayerId = TEXT("Body.InvalidRate");
	InvalidRateLayer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
	InvalidRateLayer.Flipbook = MakeFlipbook(Part, TEXT("AuthoredForInvalidRate"));
	InvalidRateLayer.DestroyedFlipbook = MakeFlipbook(Part, TEXT("DestroyedForInvalidRate"));
	InvalidRateLayer.DestroyedFlipbookPlayRate = 0.0f;
	Part->VisualLayers = { InvalidRateLayer };
	Part->DestroyedVisualSwapNormalizedTime = 0.35f;
	FDataValidationContext InvalidRateContext;
	TestEqual(TEXT("Non-positive Destroyed Flipbook rate is invalid"),
		Part->IsDataValid(InvalidRateContext), EDataValidationResult::Invalid);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyDestroyedFeedbackVisualSwapSpec,
	"Wacom.UI.Battle.BattleSceneEnemyDestroyedFeedback.InPlaceSwapAtSemanticCueMarker",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyDestroyedFeedbackVisualSwapSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyDestroyedFeedbackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const FSpawnedScenePart ScenePart = SpawnScenePart(*World);
	AWacomBattleEnemyActor* Host = ScenePart.Host;
	AWacomBattleEnemyPartActor* Part = ScenePart.Part;
	if (!TestNotNull(TEXT("Scene enemy Host"), Host)
		|| !TestNotNull(TEXT("Scene enemy Part"), Part))
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
	};

	UPaperSprite* AuthoredSprite = NewObject<UPaperSprite>(Part, TEXT("AuthoredSprite"));
	UPaperSprite* DestroyedSprite = NewObject<UPaperSprite>(Part, TEXT("DestroyedSprite"));
	UPaperSprite* IntactSprite = NewObject<UPaperSprite>(Part, TEXT("IntactSprite"));
	UPaperFlipbook* AuthoredFlipbook = MakeFlipbook(Part, TEXT("AuthoredFlipbook"));
	UPaperFlipbook* DestroyedFlipbook = MakeFlipbook(Part, TEXT("DestroyedFlipbook"));

	FWacomBattleEnemyPartVisualLayer SpriteLayer;
	SpriteLayer.LayerId = TEXT("Body.Static");
	SpriteLayer.Sprite = AuthoredSprite;
	SpriteLayer.DestroyedSprite = DestroyedSprite;
	FWacomBattleEnemyPartVisualLayer FlipbookLayer;
	FlipbookLayer.LayerId = TEXT("Body.Animated");
	FlipbookLayer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
	FlipbookLayer.Flipbook = AuthoredFlipbook;
	FlipbookLayer.DestroyedFlipbook = DestroyedFlipbook;
	FlipbookLayer.FlipbookPlayRate = 0.8f;
	FlipbookLayer.DestroyedFlipbookPlayRate = 1.25f;
	FlipbookLayer.bLoopFlipbook = true;
	FlipbookLayer.bAutoPlayFlipbook = false;
	FlipbookLayer.FlipbookStartTimeSeconds = 0.1f;
	FWacomBattleEnemyPartVisualLayer IntactLayer;
	IntactLayer.LayerId = TEXT("Body.Intact");
	IntactLayer.Sprite = IntactSprite;
	Part->VisualLayers = { SpriteLayer, FlipbookLayer, IntactLayer };
	Part->DestroyedVisualSwapNormalizedTime = 0.35f;
	Part->ImpactStyleOverride = NewObject<UWacomBattleEnemyPartImpactStyle>(Part);
	Part->RefreshAuthoringState();

	UPaperSpriteComponent* SpriteComponent = nullptr;
	UPaperFlipbookComponent* FlipbookComponent = FindGeneratedVisualLayer<UPaperFlipbookComponent>(*Part);
	UPaperSpriteComponent* IntactSpriteComponent = nullptr;
	TInlineComponentArray<UPaperSpriteComponent*> SpriteComponents;
	Part->GetComponents(SpriteComponents);
	for (UPaperSpriteComponent* Candidate : SpriteComponents)
	{
		if (Candidate && Candidate->GetSprite() == AuthoredSprite)
		{
			SpriteComponent = Candidate;
		}
		else if (Candidate && Candidate->GetSprite() == IntactSprite)
		{
			IntactSpriteComponent = Candidate;
		}
	}
	if (!TestNotNull(TEXT("Generated Sprite component"), SpriteComponent)
		|| !TestNotNull(TEXT("Generated Flipbook component"), FlipbookComponent)
		|| !TestNotNull(TEXT("Generated layer without a destroyed resource"), IntactSpriteComponent))
	{
		return false;
	}
	const int32 TopologyRevisionBefore = Host->GetRuntimePartTopologyRevision();
	const int32 ComponentCountBefore = Part->GetComponents().Num();

	UWacomBattleEnemyPartPresentationComponent* Presentation = Part->GetPresentationComponent();
	FWacomBattleEnemyPartPresentationTestAccess::SetAccessibility(*Presentation, 0.0f, true);
	Presentation->PlayBattlePresentationCue(MakeDestroyedCue());
	FWacomBattleEnemyPartPresentationDebugView PresentationView =
		Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("Destroyed maps to EffectKind 3"), PresentationView.LastImpactEffectKindValue, 3);
	TestEqual(TEXT("Destroyed keeps stable seed"), PresentationView.LastImpactSeed, 731);
	TestEqual(TEXT("Destroyed request kind is diagnosed"),
		PresentationView.LastImpactEffectKind, FName(TEXT("Destroyed")));
	TestTrue(TEXT("Destroyed uses Style intensity"),
		FMath::IsNearlyEqual(PresentationView.LastImpactIntensity, 1.35f));
	TestTrue(TEXT("Destroyed snapshots Reduced Motion"), PresentationView.bImpactReducedMotion);
	TestTrue(TEXT("Flash Off only zeros decorative intensity"),
		FMath::IsNearlyZero(PresentationView.ImpactDecorativeIntensity));

	FWacomBattleEnemyPartPresentationTestAccess::TickCuePlayback(*Presentation, 0.104f);
	TestEqual(TEXT("Sprite stays authored before 35 percent"), SpriteComponent->GetSprite(), AuthoredSprite);
	TestEqual(TEXT("Flipbook stays authored before 35 percent"), FlipbookComponent->GetFlipbook(), AuthoredFlipbook);

	FWacomBattleEnemyPartPresentationTestAccess::TickCuePlayback(*Presentation, 0.002f);
	TestEqual(TEXT("Sprite swaps after crossing 35 percent"), SpriteComponent->GetSprite(), DestroyedSprite);
	TestEqual(TEXT("Flipbook swaps after crossing 35 percent"), FlipbookComponent->GetFlipbook(), DestroyedFlipbook);
	TestEqual(TEXT("Layer without a destroyed resource stays authored"),
		IntactSpriteComponent->GetSprite(), IntactSprite);
	TestFalse(TEXT("Destroyed Flipbook does not loop"), FlipbookComponent->IsLooping());
	TestTrue(TEXT("Destroyed Flipbook uses authored terminal rate"),
		FMath::IsNearlyEqual(FlipbookComponent->GetPlayRate(), 1.25f));
	UPaperSpriteComponent* DestroyedSpriteComponent = nullptr;
	TInlineComponentArray<UPaperSpriteComponent*> SpriteComponentsAfterSwap;
	Part->GetComponents(SpriteComponentsAfterSwap);
	for (UPaperSpriteComponent* Candidate : SpriteComponentsAfterSwap)
	{
		if (Candidate && Candidate->GetSprite() == DestroyedSprite)
		{
			DestroyedSpriteComponent = Candidate;
			break;
		}
	}
	TestEqual(TEXT("Sprite component is reused"), DestroyedSpriteComponent, SpriteComponent);
	TestEqual(TEXT("Flipbook component is reused"),
		FindGeneratedVisualLayer<UPaperFlipbookComponent>(*Part), FlipbookComponent);
	TestEqual(TEXT("Component count is unchanged"), Part->GetComponents().Num(), ComponentCountBefore);
	TestEqual(TEXT("Host topology revision is unchanged"),
		Host->GetRuntimePartTopologyRevision(), TopologyRevisionBefore);

	const FWacomBattleSceneEnemyPartDebugView FirstApplyView = Part->GetBattleSceneEnemyPartDebugView();
	TestTrue(TEXT("Destroyed terminal state is diagnosed"), FirstApplyView.bDestroyedVisualStateApplied);
	TestEqual(TEXT("Both configured layers were replaced"), FirstApplyView.DestroyedVisualLayerCount, 2);
	TestEqual(TEXT("Terminal state applied once"), FirstApplyView.DestroyedVisualApplyCount, 1);

	Presentation->PlayBattlePresentationCue(MakeDestroyedCue(732));
	Presentation->ForceCompleteBattlePresentationCue();
	TestEqual(TEXT("Repeated cue remains idempotent"),
		Part->GetBattleSceneEnemyPartDebugView().DestroyedVisualApplyCount, 1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyDestroyedFeedbackLifecycleSpec,
	"Wacom.UI.Battle.BattleSceneEnemyDestroyedFeedback.SnapshotFallbackAndBattleLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyDestroyedFeedbackLifecycleSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyDestroyedFeedbackSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const FSpawnedScenePart ScenePart = SpawnScenePart(*World);
	AWacomBattleEnemyActor* Host = ScenePart.Host;
	AWacomBattleEnemyPartActor* Part = ScenePart.Part;
	if (!TestNotNull(TEXT("Scene enemy Host"), Host)
		|| !TestNotNull(TEXT("Scene enemy Part"), Part))
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part)) { Part->Destroy(); }
		if (IsValid(Host)) { Host->Destroy(); }
	};

	UPaperSprite* AuthoredSprite = NewObject<UPaperSprite>(Part, TEXT("AuthoredSnapshotSprite"));
	UPaperSprite* DestroyedSprite = NewObject<UPaperSprite>(Part, TEXT("DestroyedSnapshotSprite"));
	FWacomBattleEnemyPartVisualLayer Layer;
	Layer.LayerId = TEXT("Body.Static");
	Layer.Sprite = AuthoredSprite;
	Layer.DestroyedSprite = DestroyedSprite;
	Part->VisualLayers = { Layer };
	Part->bEnableImpactFeedback = false;
	Part->DestroyedVisualSwapNormalizedTime = 0.35f;
	Part->RefreshAuthoringState();

	UPaperSpriteComponent* SpriteComponent = FindGeneratedVisualLayer<UPaperSpriteComponent>(*Part);
	if (!TestNotNull(TEXT("Generated Sprite component"), SpriteComponent))
	{
		return false;
	}
	UWacomBattleEnemyPartPresentationComponent* Presentation = Part->GetPresentationComponent();
	Presentation->PlayBattlePresentationCue(MakeDestroyedCue());
	FWacomBattleEnemyPartPresentationTestAccess::TickCuePlayback(*Presentation, 0.11f);
	TestEqual(TEXT("Disabled particles do not suppress terminal visual"),
		SpriteComponent->GetSprite(), DestroyedSprite);
	TestEqual(TEXT("Disabled particles do not issue an impact request"),
		Presentation->GetBattleEnemyPartPresentationDebugView().LastImpactEffectKindValue,
		INDEX_NONE);
	Presentation->ResetBattlePresentationFeedback();
	TestEqual(TEXT("Presentation reset keeps an applied terminal state"),
		SpriteComponent->GetSprite(), DestroyedSprite);

	Host->ResetRuntimeScenePresentationForBattle();
	TestEqual(TEXT("New battle first takeover restores authored Sprite"),
		SpriteComponent->GetSprite(), AuthoredSprite);
	TestFalse(TEXT("New battle clears terminal state"),
		Part->GetBattleSceneEnemyPartDebugView().bDestroyedVisualStateApplied);

	FEnemyPartSnapshot InitialDestroyed;
	InitialDestroyed.InstanceId = FGuid::NewGuid();
	InitialDestroyed.PartSlotId = TEXT("Body");
	InitialDestroyed.bDestroyed = true;
	Presentation->CacheRuntimePartFacts(TEXT("Test.Part.Body"), InitialDestroyed);
	TestEqual(TEXT("Initial destroyed Snapshot restores terminal state without replay"),
		SpriteComponent->GetSprite(), DestroyedSprite);
	TestEqual(TEXT("Initial destroyed Snapshot does not play particles"),
		Presentation->GetBattleEnemyPartPresentationDebugView().ImpactEffectPlayCount, 0);
	Presentation->ClearRuntimePartFacts();
	TestEqual(TEXT("Source clear keeps the already-applied terminal Sprite"),
		SpriteComponent->GetSprite(), DestroyedSprite);

	Host->ResetRuntimeScenePresentationForBattle();
	FEnemyPartSnapshot Alive;
	Alive.InstanceId = FGuid::NewGuid();
	Alive.PartSlotId = TEXT("Body");
	Alive.bDestroyed = false;
	Presentation->CacheRuntimePartFacts(TEXT("Test.Part.Body"), Alive);
	FEnemyPartSnapshot LiveDestroyed = Alive;
	LiveDestroyed.bDestroyed = true;
	Presentation->CacheRuntimePartFacts(TEXT("Test.Part.Body"), LiveDestroyed);
	TestEqual(TEXT("Live false-to-true Snapshot waits for its semantic Cue"),
		SpriteComponent->GetSprite(), AuthoredSprite);
	TestTrue(TEXT("Live transition wait is diagnosed"),
		Presentation->GetBattleEnemyPartPresentationDebugView().bAwaitingDestroyedCueForRuntimeFacts);
	Presentation->PlayBattlePresentationCue(MakeDestroyedCue(733));
	Presentation->ForceCompleteBattlePresentationCue();
	TestEqual(TEXT("Forced cue completion applies terminal state first"),
		SpriteComponent->GetSprite(), DestroyedSprite);
	return true;
}

#endif
