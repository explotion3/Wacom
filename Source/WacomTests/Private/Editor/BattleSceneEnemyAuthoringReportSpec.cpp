// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Actors/WacomBattleSceneEnemyAuthoringReport.h"
#include "Authoring/WacomBattleSceneEnemyHostAuthoring.h"
#include "Components/ChildActorComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Editor.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyAuthoringReportSpec
{
	struct FDefinitionFixture
	{
		TStrongObjectPtr<UEnemyDefinition> Enemy;
		TArray<TStrongObjectPtr<UEnemyPartDefinition>> Parts;
	};

	FDefinitionFixture MakeDefinition(bool bIncludeInvalidSlot = false)
	{
		FDefinitionFixture Result;
		Result.Enemy.Reset(NewObject<UEnemyDefinition>(
			GetTransientPackage(), NAME_None, RF_Transient));
		Result.Enemy->EnemyId = TEXT("Test.Enemy.AuthoringReport");
		for (const TPair<FName, FName>& PartSpec :
			TArray<TPair<FName, FName>>{
				{ TEXT("Head"), TEXT("Snake.Head") },
				{ TEXT("Body"), TEXT("Snake.Body") },
				{ TEXT("Tail"), TEXT("Snake.Tail") } })
		{
			UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(
				GetTransientPackage(), NAME_None, RF_Transient);
			Part->PartId = PartSpec.Value;
			Part->MaxHp = 10;
			Result.Parts.Add(TStrongObjectPtr<UEnemyPartDefinition>(Part));
			FEnemyPartSlot Slot;
			Slot.PartSlotId = PartSpec.Key;
			Slot.PartDef = Part;
			Result.Enemy->Parts.Add(Slot);
		}
		if (bIncludeInvalidSlot)
		{
			FEnemyPartSlot InvalidSlot;
			InvalidSlot.PartSlotId = TEXT("Broken");
			Result.Enemy->Parts.Add(InvalidSlot);
		}
		return Result;
	}

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

	AWacomBattleEnemyPartActor* AddLivePart(
		AWacomBattleEnemyActor& Host,
		FName ComponentName,
		FName PartSlotId,
		FName PartId)
	{
		UChildActorComponent* Component = NewObject<UChildActorComponent>(
			&Host, ComponentName, RF_Transient | RF_Transactional);
		Host.AddInstanceComponent(Component);
		Component->CreationMethod = EComponentCreationMethod::Instance;
		Component->SetupAttachment(Host.GetRootComponent());
		Component->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
		Component->RegisterComponent();
		AWacomBattleEnemyPartActor* Part =
			Cast<AWacomBattleEnemyPartActor>(Component->GetChildActor());
		if (Part)
		{
			Part->PartSlotId = PartSlotId;
			Part->PartId = PartId;
		}
		return Part;
	}

	TStrongObjectPtr<AWacomBattleEnemyActor> MakeTemplateHost(
		UEnemyDefinition& EnemyDefinition)
	{
		TStrongObjectPtr<AWacomBattleEnemyActor> Host(NewObject<AWacomBattleEnemyActor>(
			GetTransientPackage(),
			NAME_None,
			RF_ArchetypeObject | RF_Transactional));
		Host->EnemyDefinition = &EnemyDefinition;
		return Host;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorBattleSceneEnemyAuthoringReportReadOnlySpec,
	"Wacom.Editor.BattleSceneEnemyAuthoringReport.EvaluationAndValidationAreReadOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorBattleSceneEnemyAuthoringReportReadOnlySpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringReportSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FDefinitionFixture Definition = MakeDefinition(true);
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host))
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

	Host->EnemyDefinition = Definition.Enemy.Get();
	Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
	Host->HostFlipbook = NewObject<UPaperFlipbook>(Host);
	AWacomBattleEnemyPartActor* Head = AddLivePart(
		*Host, TEXT("Head"), TEXT("Head"), TEXT("Wrong.Head"));
	AWacomBattleEnemyPartActor* DuplicateHead = AddLivePart(
		*Host, TEXT("DuplicateHead"), TEXT("Head"), TEXT("Snake.Head"));
	AWacomBattleEnemyPartActor* Body = AddLivePart(
		*Host, TEXT("Body"), TEXT("Body"), TEXT("Wrong.Body"));
	AWacomBattleEnemyPartActor* Legacy = AddLivePart(
		*Host, TEXT("LegacyWing"), TEXT("LegacyWing"), TEXT("Snake.LegacyWing"));
	if (!TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Duplicate Head"), DuplicateHead)
		|| !TestNotNull(TEXT("Body"), Body)
		|| !TestNotNull(TEXT("Legacy"), Legacy))
	{
		return false;
	}
	Host->RefreshBattleEnemyPartAuthoringState();
	UPaperFlipbookComponent* HostVisual =
		Host->GetGeneratedHostFlipbookVisualComponent();
	if (!TestNotNull(TEXT("Host visual"), HostVisual))
	{
		return false;
	}
	HostVisual->Stop();
	HostVisual->SetPlaybackPosition(0.04f, false);

	const FName HeadPartIdBefore = Head->PartId;
	const int32 ComponentCountBefore = Host->GetComponents().Num();
	const uint32 TopologyRevisionBefore = Host->GetRuntimePartTopologyRevision();
	const FName LastSyncBefore = Host->AuthoringLastPartSyncResult;
	const float PlaybackPositionBefore = HostVisual->GetPlaybackPosition();
	const bool bHostPackageDirtyBefore = Host->GetPackage()->IsDirty();
	const bool bPartPackageDirtyBefore = Head->GetPackage()->IsDirty();

	const FWacomBattleSceneEnemyHostAuthoringReport FirstReport =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
	const FWacomBattleSceneEnemyHostAuthoringReport SecondReport =
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host);
	FDataValidationContext ValidationContext;
	Host->IsDataValid(ValidationContext);

	TestEqual(TEXT("Plan skips ambiguous duplicate and contains two additions"),
		FirstReport.SyncPlan.Num(), 2);
	if (FirstReport.SyncPlan.Num() == 2)
	{
		TestEqual(TEXT("Plan first updates Body"),
			FirstReport.SyncPlan[0].PartSlotId, FName(TEXT("Body")));
		TestEqual(TEXT("Plan second adds Tail"),
			FirstReport.SyncPlan[1].PartSlotId, FName(TEXT("Tail")));
	}
	TestTrue(TEXT("Duplicate slot diagnosed"),
		FirstReport.IdentityAudit.DuplicatePartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Definition mismatch diagnosed without ambiguous write plan"),
		FirstReport.IdentityAudit.PartDefinitionMismatchSlotIds.Contains(TEXT("Body")));
	TestTrue(TEXT("Unknown slot diagnosed"),
		FirstReport.IdentityAudit.UnknownPartSlotIds.Contains(TEXT("LegacyWing")));
	TestTrue(TEXT("Missing Definition slot diagnosed"),
		FirstReport.IdentityAudit.MissingDefinitionPartSlotIds.Contains(TEXT("Tail")));
	TestTrue(TEXT("Invalid definition slot diagnosed"),
		FirstReport.InvalidDefinitionPartSlotIds.Contains(TEXT("Broken")));
	TestTrue(TEXT("Surplus actors are diagnosed without deletion"),
		FirstReport.IdentityAudit.SurplusPartActorNames.Contains(Legacy->GetName())
		&& FirstReport.IdentityAudit.SurplusPartActorNames.Num() >= 2);
	TestEqual(TEXT("Repeated report is stable"),
		SecondReport.SyncPlan.Num(), FirstReport.SyncPlan.Num());

	TestEqual(TEXT("Evaluation does not derive PartId"), Head->PartId, HeadPartIdBefore);
	TestEqual(TEXT("Evaluation does not create components"),
		Host->GetComponents().Num(), ComponentCountBefore);
	TestTrue(TEXT("Evaluation keeps Host visual component"),
		Host->GetGeneratedHostFlipbookVisualComponent() == HostVisual);
	TestEqual(TEXT("Evaluation keeps playback position"),
		HostVisual->GetPlaybackPosition(), PlaybackPositionBefore);
	TestEqual(TEXT("Evaluation keeps topology revision"),
		Host->GetRuntimePartTopologyRevision(), TopologyRevisionBefore);
	TestEqual(TEXT("Evaluation keeps last sync result"),
		Host->AuthoringLastPartSyncResult, LastSyncBefore);
	TestEqual(TEXT("Evaluation keeps Host package dirty state"),
		Host->GetPackage()->IsDirty(), bHostPackageDirtyBefore);
	TestEqual(TEXT("Evaluation keeps Part package dirty state"),
		Head->GetPackage()->IsDirty(), bPartPackageDirtyBefore);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorBattleSceneEnemyAuthoringReportBatchSyncSpec,
	"Wacom.Editor.BattleSceneEnemyAuthoringReport.BatchSyncIsIdempotent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorBattleSceneEnemyAuthoringReportBatchSyncSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringReportSpec;
	FDefinitionFixture Definition = MakeDefinition();
	TStrongObjectPtr<AWacomBattleEnemyActor> FirstHost =
		MakeTemplateHost(*Definition.Enemy);
	TStrongObjectPtr<AWacomBattleEnemyActor> SecondHost =
		MakeTemplateHost(*Definition.Enemy);
	TArray<AWacomBattleEnemyActor*> Hosts = { FirstHost.Get(), SecondHost.Get() };
	const TArray<FWacomBattleSceneEnemyHostSyncResult> FirstResults =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(Hosts);
	TestEqual(TEXT("Batch returns one result per Host"), FirstResults.Num(), 2);
	TestEqual(TEXT("First Host receives three parts"),
		FirstHost->GetBattleEnemyPartActors().Num(), 3);
	TestEqual(TEXT("Second Host receives three parts"),
		SecondHost->GetBattleEnemyPartActors().Num(), 3);
	if (!TestNotNull(TEXT("Editor transaction owner"), GEditor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		GEditor->ResetTransaction(FText::FromString(
			TEXT("Clean up Scene Enemy batch authoring test")));
	};
	TestTrue(TEXT("One undo reverts the complete multi-Host batch"),
		GEditor->UndoTransaction());
	TestEqual(TEXT("Undo removes First Host instance parts"),
		FirstHost->GetBattleEnemyPartActors().Num(), 0);
	TestEqual(TEXT("Undo removes Second Host instance parts"),
		SecondHost->GetBattleEnemyPartActors().Num(), 0);
	TestTrue(TEXT("One redo reapplies the complete multi-Host batch"),
		GEditor->RedoTransaction());
	TestEqual(TEXT("Redo restores First Host instance parts"),
		FirstHost->GetBattleEnemyPartActors().Num(), 3);
	TestEqual(TEXT("Redo restores Second Host instance parts"),
		SecondHost->GetBattleEnemyPartActors().Num(), 3);

	const TArray<AWacomBattleEnemyPartActor*> FirstPartsBefore =
		FirstHost->GetBattleEnemyPartActors();
	const TArray<AWacomBattleEnemyPartActor*> SecondPartsBefore =
		SecondHost->GetBattleEnemyPartActors();
	const TArray<FWacomBattleSceneEnemyHostSyncResult> SecondResults =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(Hosts);
	TestEqual(TEXT("Second batch returns one result per Host"), SecondResults.Num(), 2);
	for (const FWacomBattleSceneEnemyHostSyncResult& Result : SecondResults)
	{
		TestEqual(TEXT("Second batch reports no changes"),
			Result.ResultCode, FName(TEXT("NoChanges")));
	}
	for (const AWacomBattleEnemyPartActor* Part : FirstPartsBefore)
	{
		TestTrue(TEXT("First Host component identity remains stable"),
			FirstHost->GetBattleEnemyPartActors().Contains(Part));
	}
	for (const AWacomBattleEnemyPartActor* Part : SecondPartsBefore)
	{
		TestTrue(TEXT("Second Host component identity remains stable"),
			SecondHost->GetBattleEnemyPartActors().Contains(Part));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomEditorBattleSceneEnemyAuthoringReportLevelInstanceTransactionSpec,
	"Wacom.Editor.BattleSceneEnemyAuthoringReport.LevelInstanceSyncSupportsUndoRedo",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomEditorBattleSceneEnemyAuthoringReportLevelInstanceTransactionSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringReportSpec;
	if (!TestNotNull(TEXT("Editor transaction owner"), GEditor))
	{
		return false;
	}
	UWorld* World = GEditor->GetEditorWorldContext().World();
	if (!TestNotNull(TEXT("Editor world"), World))
	{
		return false;
	}
	if (!TestFalse(TEXT("Authoring world is not a game world"), World->IsGameWorld()))
	{
		return false;
	}

	FDefinitionFixture Definition = MakeDefinition();
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient | RF_Transactional;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		FVector::ZeroVector, FRotator::ZeroRotator, SpawnParams);
	if (!TestNotNull(TEXT("Level Host instance"), Host))
	{
		return false;
	}
	UPackage* WorldPackage = World->GetOutermost();
	const bool bWorldPackageDirtyBefore =
		WorldPackage && WorldPackage->IsDirty();
	ON_SCOPE_EXIT
	{
		if (IsValid(Host))
		{
			Host->Destroy();
		}
		GEditor->ResetTransaction(FText::FromString(
			TEXT("Clean up Scene Enemy level instance authoring test")));
		if (WorldPackage)
		{
			WorldPackage->SetDirtyFlag(bWorldPackageDirtyBefore);
		}
	};

	Host->EnemyDefinition = Definition.Enemy.Get();
	TArray<AWacomBattleEnemyActor*> Hosts = { Host };
	const TArray<FWacomBattleSceneEnemyHostSyncResult> Results =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(Hosts);
	TestEqual(TEXT("Level instance sync returns one result"), Results.Num(), 1);
	if (Results.Num() == 1)
	{
		TestEqual(TEXT("Level instance sync applies"),
			Results[0].ResultCode, FName(TEXT("Applied")));
	}
	TestEqual(TEXT("Level instance receives three live parts"),
		Host->GetBattleEnemyPartActors().Num(), 3);

	TestTrue(TEXT("Undo removes transactional instance components"),
		GEditor->UndoTransaction());
	TestEqual(TEXT("Level instance has no parts after undo"),
		Host->GetBattleEnemyPartActors().Num(), 0);
	TestTrue(TEXT("Redo restores transactional instance components"),
		GEditor->RedoTransaction());
	TestEqual(TEXT("Level instance restores all parts after redo"),
		Host->GetBattleEnemyPartActors().Num(), 3);
	TestTrue(TEXT("Redo restores complete derived identity"),
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host).SyncPlan.IsEmpty());

	Definition.Parts[1]->PartId = TEXT("Snake.Body.Revised");
	const TArray<FWacomBattleSceneEnemyHostSyncResult> UpdateResults =
		FWacomBattleSceneEnemyHostAuthoring::SyncPartsFromDefinition(Hosts);
	TestEqual(TEXT("Definition identity change updates the level instance"),
		UpdateResults.Num(), 1);
	if (UpdateResults.Num() == 1)
	{
		TestTrue(TEXT("Body identity update is reported"),
			UpdateResults[0].UpdatedPartSlotIds.Contains(TEXT("Body")));
	}
	TestTrue(TEXT("Updated identity leaves no pending plan"),
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host).SyncPlan.IsEmpty());
	TestTrue(TEXT("Undo reverts the stored level-instance identity update"),
		GEditor->UndoTransaction());
	TestFalse(TEXT("Undo exposes the reverted identity against the revised definition"),
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host).SyncPlan.IsEmpty());
	TestTrue(TEXT("Redo restores the stored level-instance identity update"),
		GEditor->RedoTransaction());
	TestTrue(TEXT("Redo restores the revised identity without a pending plan"),
		FWacomBattleSceneEnemyHostAuthoringEvaluator::Build(*Host).SyncPlan.IsEmpty());
	return true;
}
