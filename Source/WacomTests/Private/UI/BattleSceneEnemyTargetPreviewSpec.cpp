// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartTargetPreviewPlayback.h"
#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartTargetPreviewFeedbackController.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "../../../WacomApp/Private/Components/WacomBattleEnemySceneRuntimeComponent.h"
#include "Components/SceneComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "NiagaraComponent.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyTargetPreviewSpec
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

	UWacomBattleEnemyPartComponent* AddPart(AWacomBattleEnemyActor& Host)
	{
		UWacomBattleEnemyPartComponent* Part = NewObject<UWacomBattleEnemyPartComponent>(
			&Host, TEXT("Part_TargetPreview"), RF_Transient | RF_Transactional);
		Host.AddInstanceComponent(Part);
		Part->CreationMethod = EComponentCreationMethod::Instance;
		Part->SetupAttachment(Host.GetRootComponent());
		Part->PartSlotId = TEXT("Body");
		Part->SetDerivedPartId(TEXT("TargetPreview.Body"));
		Part->RegisterComponent();
		Host.NotifyEnemySceneComponentTopologyChanged();
		return Part;
	}

	FBattleSnapshot BuildSnapshot(
		const UEnemyDefinition& EnemyDefinition,
		const UEnemyPartDefinition& PartDefinition)
	{
		FBattleSnapshot Snapshot;
		Snapshot.EncounterId = TEXT("Encounter.TargetPreview");
		Snapshot.Phase = EBattlePhase::PlayerAction;
		FEnemySnapshot& Enemy = Snapshot.Enemies.AddDefaulted_GetRef();
		Enemy.Definition = &EnemyDefinition;
		Enemy.EncounterId = Snapshot.EncounterId;
		Enemy.EnemySlotId = TEXT("Enemy");
		FEnemyPartSnapshot& Part = Enemy.Parts.AddDefaulted_GetRef();
		Part.InstanceId = FGuid::NewGuid();
		Part.Definition = &PartDefinition;
		Part.EncounterId = Snapshot.EncounterId;
		Part.EnemySlotId = Enemy.EnemySlotId;
		Part.PartSlotId = TEXT("Body");
		Part.Identity = FBattlePartSlotIdentity::Make(
			Part.EncounterId, Part.EnemySlotId, Part.PartSlotId);
		Part.CurrentHp = 10;
		Part.MaxHp = 10;
		return Snapshot;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetPreviewPlaybackSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetPreview.PlaybackEnterHoldExitAndValidity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetPreviewPlaybackSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleEnemyPartTargetPreviewPlayback Playback;
	TestTrue(TEXT("valid preview begins"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Valid,
		0.18f,
		0.10f,
		0.90f,
		false));
	TestTrue(TEXT("preview active"), Playback.GetView().bActive);
	TestEqual(TEXT("valid kind"), Playback.GetView().Kind,
		EWacomBattleEnemyPartTargetPreviewKind::Valid);
	TestEqual(TEXT("enter starts from zero"), Playback.GetView().Amount, 0.0f);

	FWacomBattleEnemyPartTargetPreviewPlaybackView View = Playback.Tick(0.09f);
	TestTrue(TEXT("enter amount advances"), View.Amount > 0.0f && View.Amount < 1.0f);
	View = Playback.Tick(0.09f);
	TestEqual(TEXT("enter reaches hold"), View.Phase,
		EWacomBattleEnemyPartTargetPreviewPhase::Holding);
	TestEqual(TEXT("hold amount is one"), View.Amount, 1.0f);

	View = Playback.Tick(0.225f);
	TestTrue(TEXT("valid hold produces weak pulse phase"), View.Pulse > 0.9f);
	TestFalse(TEXT("same valid target does not restart"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Valid,
		0.18f,
		0.10f,
		0.90f,
		false));
	TestEqual(TEXT("same target remains holding"), Playback.GetView().Phase,
		EWacomBattleEnemyPartTargetPreviewPhase::Holding);

	TestTrue(TEXT("switching to invalid restarts preview"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Invalid,
		0.18f,
		0.10f,
		0.90f,
		false));
	View = Playback.Tick(0.18f);
	View = Playback.Tick(0.25f);
	TestEqual(TEXT("invalid preview never breathes"), View.Pulse, 0.0f);

	TestTrue(TEXT("switching to available begins a distinct semantic mode"), Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Available,
		0.12f,
		0.10f,
		0.90f,
		false));
	TestEqual(TEXT("available mode restarts from zero"), Playback.GetView().Amount, 0.0f);
	View = Playback.Tick(0.12f);
	View = Playback.Tick(0.45f);
	TestEqual(TEXT("available mode remains static"), View.Pulse, 0.0f);
	TestEqual(TEXT("available debug name"),
		FWacomBattleEnemyPartTargetPreviewPlayback::KindToName(View.Kind),
		FName(TEXT("Available")));

	Playback.BeginExit();
	View = Playback.Tick(0.05f);
	TestTrue(TEXT("exit fades without instant clear"), View.bActive && View.Amount > 0.0f);
	View = Playback.Tick(0.05f);
	TestFalse(TEXT("exit completes"), View.bActive);
	TestEqual(TEXT("exit clears amount"), View.Amount, 0.0f);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetPreviewReducedMotionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetPreview.ReducedMotionUsesStaticSemanticFrame",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetPreviewReducedMotionSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleEnemyPartTargetPreviewPlayback Playback;
	Playback.Begin(
		EWacomBattleEnemyPartTargetPreviewKind::Valid,
		0.18f,
		0.10f,
		0.90f,
		true);
	const FWacomBattleEnemyPartTargetPreviewPlaybackView View = Playback.Tick(0.45f);
	TestTrue(TEXT("reduced preview remains active"), View.bActive);
	TestEqual(TEXT("reduced preview is immediately fully visible"), View.Amount, 1.0f);
	TestEqual(TEXT("reduced preview has no pulse"), View.Pulse, 0.0f);
	TestEqual(TEXT("reduced preview holds without enter motion"), View.Phase,
		EWacomBattleEnemyPartTargetPreviewPhase::Holding);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetPreviewStateCompositionSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetPreview.AvailableHoverPriorityAndRestore",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetPreviewStateCompositionSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyTargetPreviewSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	TStrongObjectPtr<UEnemyPartDefinition> PartDefinition(NewObject<UEnemyPartDefinition>(
		GetTransientPackage(), NAME_None, RF_Transient));
	PartDefinition->PartId = TEXT("TargetPreview.Body");
	PartDefinition->MaxHp = 10;
	TStrongObjectPtr<UEnemyDefinition> EnemyDefinition(NewObject<UEnemyDefinition>(
		GetTransientPackage(), NAME_None, RF_Transient));
	EnemyDefinition->EnemyId = TEXT("Enemy.TargetPreview");
	FEnemyPartSlot& Slot = EnemyDefinition->Parts.AddDefaulted_GetRef();
	Slot.PartSlotId = TEXT("Body");
	Slot.PartDef = PartDefinition.Get();

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Enemy host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = EnemyDefinition.Get();
	Host->EnemySlotId = TEXT("Enemy");
	UWacomBattleEnemyPartComponent* Part = AddPart(*Host);
	if (!TestNotNull(TEXT("Enemy part"), Part))
	{
		return false;
	}
	FWacomEnemySceneRuntimeAutomationTestView::InitializeBinding(
		*Host, TEXT("Encounter.TargetPreview"), TEXT("Enemy"));
	const FBattleSnapshot Snapshot = BuildSnapshot(*EnemyDefinition, *PartDefinition);
	TestTrue(TEXT("Part binds before preview composition"),
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(*Host, *Part, Snapshot));

	TestEqual(TEXT("idle has no preview"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("None")));
	FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
		*Host, *Part, true, true);
	TestEqual(TEXT("targetable part becomes available"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("Available")));

	FWacomEnemySceneRuntimeAutomationTestView::SetDragTargetPreview(
		*Host,
		*Part,
		EWacomFirstPersonCardDragTargetFeedbackState::ValidWorldTarget);
	TestEqual(TEXT("valid hover overrides available"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("ValidHover")));
	FWacomEnemySceneRuntimeAutomationTestView::ClearDragTargetPreview(*Host, *Part);
	TestEqual(TEXT("leaving valid hover restores available"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("Available")));

	FWacomEnemySceneRuntimeAutomationTestView::SetDragTargetPreview(
		*Host,
		*Part,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	TestEqual(TEXT("invalid hover overrides available"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("InvalidHover")));
	FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
		*Host, *Part, true, false);
	TestEqual(TEXT("concrete invalid hover remains highest priority"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("InvalidHover")));
	FWacomEnemySceneRuntimeAutomationTestView::ClearDragTargetPreview(*Host, *Part);
	TestEqual(TEXT("clearing selection removes preview"),
		FWacomEnemySceneRuntimeAutomationTestView::GetDesiredTargetPreviewKind(*Part),
		FName(TEXT("None")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyTargetPreviewPerPartNiagaraOwnershipSpec,
	"Wacom.UI.Battle.BattleSceneEnemyTargetPreview.MultiplePartsOwnIndependentNiagaraComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyTargetPreviewPerPartNiagaraOwnershipSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyTargetPreviewSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	UWacomBattleEnemyPartTargetPreviewStyle* Style =
		LoadObject<UWacomBattleEnemyPartTargetPreviewStyle>(
			nullptr,
			TEXT("/Game/Wacom/UI/Battle/WorldImpact/DA_BattleEnemyPartTargetPreviewStyle_PixelLock.DA_BattleEnemyPartTargetPreviewStyle_PixelLock"));
	if (!TestNotNull(TEXT("Target preview style"), Style))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		FVector::ZeroVector,
		FRotator::ZeroRotator,
		SpawnParams);
	if (!TestNotNull(TEXT("Enemy host"), Host))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	UWacomBattleEnemySceneRuntimeComponent* Runtime = Host->GetEnemySceneRuntimeComponent();
	if (!TestNotNull(TEXT("Enemy scene runtime"), Runtime))
	{
		return false;
	}

	auto AddAnchor = [Host](const FName Name)
	{
		USceneComponent* Anchor = NewObject<USceneComponent>(Host, Name, RF_Transient);
		Host->AddInstanceComponent(Anchor);
		Anchor->SetupAttachment(Host->GetRootComponent());
		Anchor->RegisterComponent();
		return Anchor;
	};
	USceneComponent* FirstAnchor = AddAnchor(TEXT("TargetPreviewAnchor_First"));
	USceneComponent* SecondAnchor = AddAnchor(TEXT("TargetPreviewAnchor_Second"));

	FWacomBattleEnemyPartTargetPreviewPlaybackView PlaybackView;
	PlaybackView.bActive = true;
	PlaybackView.Kind = EWacomBattleEnemyPartTargetPreviewKind::Available;
	PlaybackView.Phase = EWacomBattleEnemyPartTargetPreviewPhase::Holding;
	PlaybackView.Amount = 1.0f;

	FWacomBattleEnemyPartTargetPreviewFeedbackController FirstController;
	FWacomBattleEnemyPartTargetPreviewFeedbackController SecondController;
	TestTrue(TEXT("First part starts target preview"), FirstController.BeginOrUpdate(
		*Runtime, FirstAnchor, nullptr, Style, PlaybackView, 1.0f));
	TestTrue(TEXT("Second part starts target preview"), SecondController.BeginOrUpdate(
		*Runtime, SecondAnchor, nullptr, Style, PlaybackView, 1.0f));

	TArray<UNiagaraComponent*> NiagaraComponents;
	Host->GetComponents(NiagaraComponents);
	NiagaraComponents.RemoveAll([](const UNiagaraComponent* Component)
	{
		return !Component
			|| !Component->GetName().StartsWith(TEXT("WacomEnemyPartTargetPreviewNiagara"));
	});
	TestEqual(TEXT("Each part owns one preview Niagara component"), NiagaraComponents.Num(), 2);
	if (NiagaraComponents.Num() == 2)
	{
		TestNotEqual(TEXT("Part preview components are distinct"),
			NiagaraComponents[0],
			NiagaraComponents[1]);
		TestNotEqual(TEXT("Part preview components use distinct UObject names"),
			NiagaraComponents[0]->GetFName(),
			NiagaraComponents[1]->GetFName());
	}

	FirstController.ResetImmediate(false);
	TestTrue(TEXT("Stopping one part does not deactivate the other"),
		SecondController.GetDebugView().bEffectActive);
	SecondController.ResetImmediate(true);
	FirstController.ResetImmediate(true);
	return true;
}
