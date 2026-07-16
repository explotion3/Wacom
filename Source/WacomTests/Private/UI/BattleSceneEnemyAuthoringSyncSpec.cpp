// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/ChildActorComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperSprite.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyAuthoringSyncSpec
{
	struct FDefinitionFixture
	{
		TStrongObjectPtr<UEnemyDefinition> Enemy;
		TArray<TStrongObjectPtr<UEnemyPartDefinition>> Parts;
	};

	struct FPartTemplateRef
	{
		UChildActorComponent* Component = nullptr;
		AWacomBattleEnemyPartActor* Part = nullptr;
	};

	FDefinitionFixture MakeDefinition()
	{
		FDefinitionFixture Result;
		Result.Enemy.Reset(NewObject<UEnemyDefinition>(
			GetTransientPackage(), NAME_None, RF_Transient));
		Result.Enemy->EnemyId = TEXT("Test.Enemy.AuthoringSync");

		const TArray<TPair<FName, FName>> PartSpecs = {
			{ TEXT("Head"), TEXT("Snake.Head") },
			{ TEXT("Body"), TEXT("Snake.Body") },
			{ TEXT("Tail"), TEXT("Snake.Tail") }
		};
		for (const TPair<FName, FName>& PartSpec : PartSpecs)
		{
			UEnemyPartDefinition* PartDefinition = NewObject<UEnemyPartDefinition>(
				GetTransientPackage(), NAME_None, RF_Transient);
			PartDefinition->PartId = PartSpec.Value;
			PartDefinition->MaxHp = 20;
			Result.Parts.Add(TStrongObjectPtr<UEnemyPartDefinition>(PartDefinition));

			FEnemyPartSlot Slot;
			Slot.PartSlotId = PartSpec.Key;
			Slot.PartDef = PartDefinition;
			Result.Enemy->Parts.Add(Slot);
		}
		return Result;
	}

	TStrongObjectPtr<AWacomBattleEnemyActor> MakeTemplateHost(
		UEnemyDefinition& EnemyDefinition)
	{
		TStrongObjectPtr<AWacomBattleEnemyActor> Host(NewObject<AWacomBattleEnemyActor>(
			GetTransientPackage(),
			NAME_None,
			RF_ArchetypeObject | RF_Transactional));
		Host->EnemyDefinition = &EnemyDefinition;
		Host->EnemySlotId = TEXT("Enemy");
		return Host;
	}

	FPartTemplateRef AddPartTemplate(
		AWacomBattleEnemyActor& Host,
		FName ComponentName,
		FName PartSlotId,
		FName PartId)
	{
		FPartTemplateRef Result;
		Result.Component = NewObject<UChildActorComponent>(
			&Host,
			ComponentName,
			RF_ArchetypeObject | RF_Transactional);
		if (!Result.Component)
		{
			return Result;
		}

		Result.Component->SetupAttachment(Host.GetRootComponent());
		Host.AddInstanceComponent(Result.Component);
		Result.Component->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
		Result.Part = Cast<AWacomBattleEnemyPartActor>(
			Result.Component->GetChildActorTemplate());
		if (Result.Part)
		{
			Result.Part->PartSlotId = PartSlotId;
			Result.Part->PartId = PartId;
		}
		return Result;
	}

	TArray<UChildActorComponent*> GetPartComponents(AWacomBattleEnemyActor& Host)
	{
		TInlineComponentArray<UChildActorComponent*> Components;
		Host.GetComponents(Components);

		TArray<UChildActorComponent*> PartComponents;
		for (UChildActorComponent* Component : Components)
		{
			if (Component
				&& Component->GetChildActorClass()
				&& Component->GetChildActorClass()->IsChildOf(
					AWacomBattleEnemyPartActor::StaticClass()))
			{
				PartComponents.Add(Component);
			}
		}
		return PartComponents;
	}

	FPartTemplateRef FindPartTemplateBySlot(
		AWacomBattleEnemyActor& Host,
		FName PartSlotId)
	{
		for (UChildActorComponent* Component : GetPartComponents(Host))
		{
			AWacomBattleEnemyPartActor* Part = Cast<AWacomBattleEnemyPartActor>(
				Component->GetChildActorTemplate());
			if (Part && Part->PartSlotId == PartSlotId)
			{
				return { Component, Part };
			}
		}
		return {};
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

	bool IssuesContain(const TArray<FText>& Issues, const TCHAR* ExpectedText)
	{
		for (const FText& Issue : Issues)
		{
			if (Issue.ToString().Contains(ExpectedText))
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyAuthoringSyncCreatesAndPreservesSpec,
	"Wacom.UI.Battle.BattleSceneEnemyAuthoringSync.CreatesMissingPartsDerivesPartIdAndPreservesAuthoredState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyAuthoringSyncCreatesAndPreservesSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringSyncSpec;
	FDefinitionFixture Definition = MakeDefinition();
	TStrongObjectPtr<AWacomBattleEnemyActor> Host = MakeTemplateHost(*Definition.Enemy);
	Host->HostAuthoringMode =
		EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;

	FPartTemplateRef Head = AddPartTemplate(
		*Host,
		TEXT("AuthoredHead"),
		TEXT("Head"),
		TEXT("Wrong.Head"));
	if (!TestNotNull(TEXT("Head component"), Head.Component)
		|| !TestNotNull(TEXT("Head template"), Head.Part))
	{
		return false;
	}

	const FTransform AuthoredTransform(
		FRotator(0.0, 17.0, -4.0),
		FVector(96.0, -6.0, 16.0),
		FVector(1.25, 0.9, 1.1));
	Head.Component->SetRelativeTransform(AuthoredTransform);
	Head.Part->HitBoundsExtent = FVector(71.0, 53.0, 41.0);
	Head.Part->ImpactAnchorRelativeLocation = FVector(7.0, -3.0, 11.0);
	FWacomBattleEnemyPartVisualLayer VisualLayer;
	VisualLayer.LayerId = TEXT("Head.Authored");
	VisualLayer.Sprite = NewObject<UPaperSprite>(Head.Part);
	Head.Part->VisualLayers = { VisualLayer };
	UPaperSprite* AuthoredSprite = VisualLayer.Sprite;

	Host->SyncEnemyPartsFromDefinition();

	TestEqual(TEXT("Sync applied"),
		Host->AuthoringLastPartSyncResult,
		FName(TEXT("Applied")));
	TestTrue(TEXT("Head PartId is derived from definition"),
		Head.Part->PartId == FName(TEXT("Snake.Head")));
	TestTrue(TEXT("Existing component transform is preserved"),
		Head.Component->GetRelativeTransform().Equals(AuthoredTransform));
	TestEqual(TEXT("Existing HitBounds is preserved"),
		Head.Part->HitBoundsExtent,
		FVector(71.0, 53.0, 41.0));
	TestEqual(TEXT("Existing ImpactAnchor is preserved"),
		Head.Part->ImpactAnchorRelativeLocation,
		FVector(7.0, -3.0, 11.0));
	TestEqual(TEXT("Existing VisualLayers count is preserved"),
		Head.Part->VisualLayers.Num(),
		1);
	TestTrue(TEXT("Existing VisualLayers resource is preserved"),
		Head.Part->VisualLayers[0].Sprite == AuthoredSprite);
	TestEqual(TEXT("Authoring mode is preserved"),
		Host->HostAuthoringMode,
		EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers);

	const FPartTemplateRef Body = FindPartTemplateBySlot(*Host, TEXT("Body"));
	const FPartTemplateRef Tail = FindPartTemplateBySlot(*Host, TEXT("Tail"));
	TestEqual(TEXT("Definition produces three components"),
		GetPartComponents(*Host).Num(),
		3);
	TestNotNull(TEXT("Body component is generated"), Body.Component);
	TestNotNull(TEXT("Body template is generated"), Body.Part);
	TestNotNull(TEXT("Tail component is generated"), Tail.Component);
	TestNotNull(TEXT("Tail template is generated"), Tail.Part);
	if (Body.Component && Body.Part && Tail.Component && Tail.Part)
	{
		TestTrue(TEXT("Body starts at zero relative transform"),
			Body.Component->GetRelativeTransform().Equals(FTransform::Identity));
		TestTrue(TEXT("Tail starts at zero relative transform"),
			Tail.Component->GetRelativeTransform().Equals(FTransform::Identity));
		TestEqual(TEXT("Body PartId is derived"),
			Body.Part->PartId,
			FName(TEXT("Snake.Body")));
		TestEqual(TEXT("Tail PartId is derived"),
			Tail.Part->PartId,
			FName(TEXT("Snake.Tail")));
	}
	TestTrue(TEXT("Added slots are reported"),
		Host->AuthoringLastAddedPartSlotIds.Contains(TEXT("Body"))
		&& Host->AuthoringLastAddedPartSlotIds.Contains(TEXT("Tail")));
	TestTrue(TEXT("Corrected slot is reported"),
		Host->AuthoringLastUpdatedPartSlotIds.Contains(TEXT("Head")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyAuthoringSyncIdempotentSurplusSpec,
	"Wacom.UI.Battle.BattleSceneEnemyAuthoringSync.IsIdempotentAndPreservesSurplusParts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyAuthoringSyncIdempotentSurplusSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringSyncSpec;
	FDefinitionFixture Definition = MakeDefinition();
	TStrongObjectPtr<AWacomBattleEnemyActor> Host = MakeTemplateHost(*Definition.Enemy);
	Host->SyncEnemyPartsFromDefinition();

	const TArray<UChildActorComponent*> FirstComponents = GetPartComponents(*Host);
	Host->SyncEnemyPartsFromDefinition();
	const TArray<UChildActorComponent*> SecondComponents = GetPartComponents(*Host);
	TestEqual(TEXT("Second sync reports no changes"),
		Host->AuthoringLastPartSyncResult,
		FName(TEXT("NoChanges")));
	TestEqual(TEXT("Second sync keeps component count"),
		SecondComponents.Num(),
		FirstComponents.Num());
	for (UChildActorComponent* Component : FirstComponents)
	{
		TestTrue(TEXT("Second sync keeps component identity"),
			SecondComponents.Contains(Component));
	}
	TestTrue(TEXT("Second sync has no added slots"),
		Host->AuthoringLastAddedPartSlotIds.IsEmpty());
	TestTrue(TEXT("Second sync has no updated slots"),
		Host->AuthoringLastUpdatedPartSlotIds.IsEmpty());

	FPartTemplateRef DuplicateHead = AddPartTemplate(
		*Host,
		TEXT("DuplicateHead"),
		TEXT("Head"),
		TEXT("Snake.Head"));
	FPartTemplateRef LegacyPart = AddPartTemplate(
		*Host,
		TEXT("LegacyWing"),
		TEXT("LegacyWing"),
		TEXT("Snake.LegacyWing"));
	if (!TestNotNull(TEXT("Duplicate head template"), DuplicateHead.Part)
		|| !TestNotNull(TEXT("Legacy part template"), LegacyPart.Part))
	{
		return false;
	}

	const FString DuplicateActorName = DuplicateHead.Part->GetName();
	const FString LegacyActorName = LegacyPart.Part->GetName();
	Host->SyncEnemyPartsFromDefinition();

	TestEqual(TEXT("Surplus-only sync does not rewrite topology"),
		Host->AuthoringLastPartSyncResult,
		FName(TEXT("NoChanges")));
	TestEqual(TEXT("Surplus components are preserved"),
		GetPartComponents(*Host).Num(),
		5);
	TestTrue(TEXT("Duplicate slot is diagnosed"),
		Host->AuthoringDuplicatePartSlotIds.Contains(TEXT("Head")));
	TestTrue(TEXT("Unknown slot is diagnosed"),
		Host->AuthoringUnknownPartSlotIds.Contains(TEXT("LegacyWing")));
	TestTrue(TEXT("Duplicate actor is marked surplus"),
		Host->AuthoringSurplusPartActorNames.Contains(DuplicateActorName));
	TestTrue(TEXT("Unknown actor is marked surplus"),
		Host->AuthoringSurplusPartActorNames.Contains(LegacyActorName));
	TestFalse(TEXT("Duplicate topology is not authoring-ready"),
		Host->bAuthoringReady);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyAuthoringSyncSkipsInvalidDefinitionSlotsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyAuthoringSync.SkipsInvalidDefinitionSlotsWithoutGuessing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyAuthoringSyncSkipsInvalidDefinitionSlotsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringSyncSpec;
	FDefinitionFixture Definition = MakeDefinition();
	FEnemyPartSlot DuplicateBody = Definition.Enemy->Parts[1];
	Definition.Enemy->Parts.Add(DuplicateBody);
	FEnemyPartSlot MissingDefinition;
	MissingDefinition.PartSlotId = TEXT("Broken");
	Definition.Enemy->Parts.Add(MissingDefinition);

	TStrongObjectPtr<AWacomBattleEnemyActor> Host = MakeTemplateHost(*Definition.Enemy);
	Host->SyncEnemyPartsFromDefinition();

	TestEqual(TEXT("Valid slots apply while invalid slots are reported"),
		Host->AuthoringLastPartSyncResult,
		FName(TEXT("AppliedWithInvalidDefinitionSlots")));
	TestTrue(TEXT("Duplicate definition slot is reported"),
		Host->AuthoringLastInvalidDefinitionPartSlotIds.Contains(TEXT("Body")));
	TestTrue(TEXT("Missing PartDefinition slot is reported"),
		Host->AuthoringLastInvalidDefinitionPartSlotIds.Contains(TEXT("Broken")));
	TestEqual(TEXT("Only valid unique slots are generated"),
		GetPartComponents(*Host).Num(),
		2);
	TestNotNull(TEXT("Head remains generated"),
		FindPartTemplateBySlot(*Host, TEXT("Head")).Part);
	TestNotNull(TEXT("Tail remains generated"),
		FindPartTemplateBySlot(*Host, TEXT("Tail")).Part);
	TestNull(TEXT("Duplicate body is not guessed"),
		FindPartTemplateBySlot(*Host, TEXT("Body")).Part);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyAuthoringModeValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyAuthoringSync.AuthoringModesDriveDiagnosticsOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyAuthoringModeValidationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleSceneEnemyAuthoringSyncSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FDefinitionFixture Definition = MakeDefinition();
	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host = World->SpawnActor<AWacomBattleEnemyActor>(
		AWacomBattleEnemyActor::StaticClass(),
		FTransform::Identity,
		SpawnParams);
	if (!TestNotNull(TEXT("Host"), Host))
	{
		return false;
	}

	TArray<AWacomBattleEnemyPartActor*> Parts;
	ON_SCOPE_EXIT
	{
		for (AWacomBattleEnemyPartActor* Part : Parts)
		{
			if (IsValid(Part))
			{
				Part->Destroy();
			}
		}
		if (IsValid(Host))
		{
			Host->Destroy();
		}
	};

	Host->EnemyDefinition = Definition.Enemy.Get();
	for (const FEnemyPartSlot& Slot : Definition.Enemy->Parts)
	{
		AWacomBattleEnemyPartActor* Part = World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!TestNotNull(TEXT("Part"), Part))
		{
			return false;
		}
		Part->PartSlotId = Slot.PartSlotId;
		Part->PartId = Slot.PartDef ? Slot.PartDef->PartId : NAME_None;
		Part->AttachToActor(Host, FAttachmentTransformRules::KeepWorldTransform);
		Parts.Add(Part);
	}

	TArray<FText> Warnings;
	TArray<FText> Errors;
	FDataValidationContext SimpleContext;
	Host->HostAuthoringMode =
		EWacomBattleEnemyHostAuthoringMode::SimpleHostVisual;
	Host->IsDataValid(SimpleContext);
	SimpleContext.SplitIssues(Warnings, Errors);
	TestTrue(TEXT("Simple mode reports missing host visual"),
		IssuesContain(Warnings, TEXT("SimpleHostVisual")));

	FDataValidationContext MultiPartContext;
	Host->HostAuthoringMode =
		EWacomBattleEnemyHostAuthoringMode::MultiPartVisualLayers;
	Host->IsDataValid(MultiPartContext);
	MultiPartContext.SplitIssues(Warnings, Errors);
	TestTrue(TEXT("Multi-part mode reports missing visual layers"),
		IssuesContain(Warnings, TEXT("MultiPartVisualLayers")));
	TestEqual(TEXT("Mode diagnostics do not create visual layers"),
		Parts[0]->VisualLayers.Num(),
		0);
	return true;
}
