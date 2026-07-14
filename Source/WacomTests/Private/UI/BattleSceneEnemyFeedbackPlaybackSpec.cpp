// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperSprite.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "WacomBattleEnemyPartPresentationTestAccess.h"

#include <limits>

namespace WacomBattleSceneEnemyFeedbackPlaybackSpec
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

	AWacomBattleEnemyPartActor* SpawnPart(UWorld& World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		AWacomBattleEnemyPartActor* Part = World.SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (Part)
		{
			Part->PartId = TEXT("Test.Part.Head");
			Part->PartSlotId = TEXT("Head");
			Part->SetEnemySlotId(TEXT("Enemy"));
			Part->RefreshAuthoringState();
		}
		return Part;
	}

	FWacomBattlePresentationTargetCue MakeCue(
		EWacomBattlePresentationTargetCueKind Kind,
		float DurationSeconds)
	{
		FWacomBattlePresentationTargetCue Cue;
		Cue.CueKind = Kind;
		Cue.SourceEventType = EBattleEventType::None;
		Cue.Duration = DurationSeconds;
		return Cue;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyCuePlaybackPrioritySpec,
	"Wacom.UI.Battle.BattleSceneEnemyPresentation.CuePlaybackPriorityProgressAndPersistentScale",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyCuePlaybackPrioritySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyFeedbackPlaybackSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleEnemyPartActor* Part =
		WacomBattleSceneEnemyFeedbackPlaybackSpec::SpawnPart(*World);
	if (!TestNotNull(TEXT("Part actor"), Part))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part))
		{
			Part->Destroy();
		}
	};

	UWacomBattleEnemyPartPresentationComponent* Presentation = Part->GetPresentationComponent();
	if (!TestNotNull(TEXT("Presentation component"), Presentation))
	{
		return false;
	}

	Part->CueHoldSeconds = 0.23f;
	Part->TargetableAffordanceScale = 1.07f;
	Part->HoverProbeScale = 1.03f;
	Part->RefreshAuthoringState();
	const FVector AuthoredScale = Part->GetVisualLayersRoot()->GetRelativeScale3D();

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyFeedbackPlaybackSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::TargetConfirmed,
			0.0f));
	FWacomBattleEnemyPartPresentationDebugView View =
		Presentation->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("confirm cue starts"), View.bCuePlaybackActive);
	TestEqual(TEXT("confirm cue kind"), View.ActiveCueKind, FName(TEXT("TargetConfirmed")));
	TestEqual(TEXT("invalid duration uses authored fallback"), View.CuePlaybackDurationSeconds, 0.23f);
	TestEqual(TEXT("accepted cue increments count"), View.CuePlayCount, 1);
	TestEqual(TEXT("semantic cue does not change authored scale"),
		Part->GetVisualLayersRoot()->GetRelativeScale3D(), AuthoredScale);

	FWacomBattleEnemyPartPresentationTestAccess::TickCuePlayback(*Presentation, 0.115f);
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestTrue(TEXT("progress advances"),
		View.CuePlaybackProgress > 0.49f && View.CuePlaybackProgress < 0.51f);

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyFeedbackPlaybackSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::DamageDealt,
			0.30f));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("higher priority damage replaces confirm"), View.ActiveCueKind, FName(TEXT("Damage")));
	TestEqual(TEXT("replacement increments count"), View.CuePlayCount, 2);

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyFeedbackPlaybackSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::TargetConfirmed,
			0.10f));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("lower priority cue does not interrupt"), View.ActiveCueKind, FName(TEXT("Damage")));
	TestEqual(TEXT("ignored cue does not increment count"), View.CuePlayCount, 2);

	FWacomBattleEnemyPartPresentationTestAccess::TickCuePlayback(*Presentation, 0.10f);
	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyFeedbackPlaybackSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::DamageDealt,
			0.20f));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("same priority restarts"), View.CuePlayCount, 3);
	TestEqual(TEXT("same priority restart resets progress"), View.CuePlaybackProgress, 0.0f);

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyFeedbackPlaybackSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::EnemyPartHpEmptied,
			0.12f));
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("destroyed replaces damage"), View.ActiveCueKind, FName(TEXT("Destroyed")));
	TestEqual(TEXT("destroyed increments count"), View.CuePlayCount, 4);

	FWacomBattleEnemyPartPresentationTestAccess::TickCuePlayback(*Presentation, 0.12f);
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("completed cue becomes inactive"), View.bCuePlaybackActive);
	TestEqual(TEXT("completed cue reports full progress"), View.CuePlaybackProgress, 1.0f);
	TestFalse(TEXT("completed cue disables component tick"), Presentation->IsComponentTickEnabled());

	const FWacomInteractionTargetHandle HoverHandle =
		([Part]()
		{
			Part->GetInteractionTargetComponent()->SetTargetId(FGuid::NewGuid());
			Part->GetInteractionTargetComponent()->SetBattlePartSlotIdentity(
				TEXT("Encounter"), TEXT("Enemy"), TEXT("Head"));
			return Part->GetInteractionTargetComponent()->BuildWorldTargetHandle();
		})();
	Presentation->SetHoverProbeState(HoverHandle, TEXT("Test"));
	TestEqual(TEXT("hover applies over authored scale"),
		Part->GetVisualLayersRoot()->GetRelativeScale3D(), AuthoredScale * 1.03f);
	Presentation->SetTargetableAffordance(true);
	TestEqual(TEXT("targetable has priority over hover"),
		Part->GetVisualLayersRoot()->GetRelativeScale3D(), AuthoredScale * 1.07f);
	Presentation->SetDragTargetPreviewState(
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("drag preview pixel frame suppresses targetable scale"),
		Part->GetVisualLayersRoot()->GetRelativeScale3D(), AuthoredScale);
	Presentation->ResetBattlePresentationFeedback();
	TestEqual(TEXT("reset restores authored scale"),
		Part->GetVisualLayersRoot()->GetRelativeScale3D(), AuthoredScale);
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("reset clears cue playback"), View.bCuePlaybackActive);
	TestFalse(TEXT("reset leaves tick disabled"), Presentation->IsComponentTickEnabled());

	Presentation->PlayBattlePresentationCue(
		WacomBattleSceneEnemyFeedbackPlaybackSpec::MakeCue(
			EWacomBattlePresentationTargetCueKind::DamageDealt,
			0.25f));
	Presentation->ForceCompleteBattlePresentationCue();
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestFalse(TEXT("force complete deactivates cue"), View.bCuePlaybackActive);
	TestEqual(TEXT("force complete reports full progress"), View.CuePlaybackProgress, 1.0f);
	Presentation->ClearRuntimePartFacts();
	View = Presentation->GetBattleEnemyPartPresentationDebugView();
	TestEqual(TEXT("unbind reset clears playback kind"), View.ActiveCueKind, FName(TEXT("None")));
	TestEqual(TEXT("unbind reset clears progress"), View.CuePlaybackProgress, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyImpactAnchorSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPresentation.ImpactAnchorSupportsHitOnlyAndVisualLayers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyImpactAnchorSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyFeedbackPlaybackSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomBattleEnemyPartActor* Part =
		WacomBattleSceneEnemyFeedbackPlaybackSpec::SpawnPart(*World);
	if (!TestNotNull(TEXT("Part actor"), Part))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Part))
		{
			Part->Destroy();
		}
	};

	USceneComponent* ImpactAnchor = Part->GetImpactAnchorComponent();
	if (!TestNotNull(TEXT("Every part owns an impact anchor"), ImpactAnchor))
	{
		return false;
	}
	TestEqual(TEXT("Default anchor is centered on hit bounds"),
		ImpactAnchor->GetRelativeLocation(), FVector::ZeroVector);

	Part->ImpactAnchorRelativeLocation = FVector(12.f, -4.f, 27.f);
	Part->VisualLayers.Reset();
	Part->SetHostVisualContext(true);
	Part->RefreshAuthoringState();
	FWacomBattleSceneEnemyPartDebugView View = Part->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Hit-only mode remains active"), View.VisualAuthoringMode, FName(TEXT("HitOnly")));
	TestTrue(TEXT("Hit-only mode resolves impact anchor"), View.bImpactAnchorReady);
	TestEqual(TEXT("Hit-only custom anchor offset"),
		View.ImpactAnchorRelativeLocation, FVector(12.f, -4.f, 27.f));
	TestEqual(TEXT("Presentation receives the same anchor"),
		View.PresentationDebugView.ImpactAnchorName, FName(TEXT("ImpactAnchor")));

	FWacomBattleEnemyPartVisualLayer VisualLayer;
	VisualLayer.LayerId = TEXT("Main");
	VisualLayer.Sprite = NewObject<UPaperSprite>(Part);
	Part->VisualLayers = { VisualLayer };
	Part->SetHostVisualContext(false);
	Part->RefreshAuthoringState();
	View = Part->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Visual-layer mode remains active"),
		View.VisualAuthoringMode, FName(TEXT("VisualLayers")));
	TestTrue(TEXT("Visual-layer mode resolves impact anchor"), View.bImpactAnchorReady);

#if WITH_EDITOR
	Part->ImpactAnchorRelativeLocation.X = std::numeric_limits<float>::infinity();
	FDataValidationContext ValidationContext;
	const EDataValidationResult ValidationResult = Part->IsDataValid(ValidationContext);
	TestEqual(TEXT("Non-finite anchor offset is invalid"),
		ValidationResult, EDataValidationResult::Invalid);
#endif
	return true;
}
