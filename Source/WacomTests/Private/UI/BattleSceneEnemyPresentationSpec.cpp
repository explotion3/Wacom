// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Misc/DataValidation.h"
#include "Misc/ScopeExit.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleSceneEnemyPresentationSpec
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

	UEnemyDefinition* MakeThreePartDefinition(
		UObject* Outer,
		TArray<TStrongObjectPtr<UEnemyPartDefinition>>& OutParts)
	{
		UEnemyDefinition* Enemy =
			NewObject<UEnemyDefinition>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
		if (!Enemy)
		{
			return nullptr;
		}

		Enemy->EnemyId = TEXT("Test.Enemy.ScenePresentation");
		const TArray<TPair<FName, FName>> PartSpecs = {
			{ TEXT("Test.Part.Head"), TEXT("Head") },
			{ TEXT("Test.Part.Body"), TEXT("Body") },
			{ TEXT("Test.Part.Tail"), TEXT("Tail") },
		};
		for (const TPair<FName, FName>& PartSpec : PartSpecs)
		{
			UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(Enemy, NAME_None, RF_Transient);
			Part->PartId = PartSpec.Key;
			Part->MaxHp = 20;
			OutParts.Add(TStrongObjectPtr<UEnemyPartDefinition>(Part));

			FEnemyPartSlot Slot;
			Slot.PartSlotId = PartSpec.Value;
			Slot.PartDef = Part;
			Enemy->Parts.Add(Slot);
		}
		return Enemy;
	}

	struct FSceneEnemyPresentationHost
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
		TStrongObjectPtr<UEnemyDefinition> Enemy;
		TArray<TStrongObjectPtr<UEnemyPartDefinition>> PartDefinitions;
	};

	void DestroySceneEnemyPresentationHost(FSceneEnemyPresentationHost& SceneHost)
	{
		for (AWacomBattleEnemyPartActor* PartActor : SceneHost.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		SceneHost.Parts.Reset();

		if (IsValid(SceneHost.Host))
		{
			SceneHost.Host->Destroy();
		}
		SceneHost.Host = nullptr;
	}

	FSceneEnemyPresentationHost SpawnThreePartHost(UWorld& World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyPresentationHost Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Enemy.Reset(MakeThreePartDefinition(Result.Host, Result.PartDefinitions));
		Result.Host->EnemyDefinition = Result.Enemy.Get();
		Result.Host->EnemySlotId = TEXT("Enemy");

		const TArray<TPair<FName, FName>> PartSpecs = {
			{ TEXT("Test.Part.Head"), TEXT("Head") },
			{ TEXT("Test.Part.Body"), TEXT("Body") },
			{ TEXT("Test.Part.Tail"), TEXT("Tail") },
		};
		for (int32 Index = 0; Index < PartSpecs.Num(); ++Index)
		{
			AWacomBattleEnemyPartActor* PartActor =
				World.SpawnActor<AWacomBattleEnemyPartActor>(
					AWacomBattleEnemyPartActor::StaticClass(),
					FTransform(FVector(120.0f * static_cast<float>(Index + 1), 0.0f, 0.0f)),
					SpawnParams);
			if (!PartActor)
			{
				continue;
			}

			PartActor->PartId = PartSpecs[Index].Key;
			PartActor->PartSlotId = PartSpecs[Index].Value;
			PartActor->AttachToActor(Result.Host, FAttachmentTransformRules::KeepWorldTransform);
			Result.Parts.Add(PartActor);
		}

		Result.Host->RefreshBattleEnemyPartAuthoringState();
		return Result;
	}

	EDataValidationResult ValidateObjectForTest(
		const UObject* Object,
		TArray<FText>& OutWarnings,
		TArray<FText>& OutErrors)
	{
		OutWarnings.Reset();
		OutErrors.Reset();
		if (!Object)
		{
			OutErrors.Add(FText::FromString(TEXT("Missing object")));
			return EDataValidationResult::Invalid;
		}

		FDataValidationContext Context;
		const EDataValidationResult Result = Object->IsDataValid(Context);
		Context.SplitIssues(OutWarnings, OutErrors);
		return Result;
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
	FWacomUIBattleSceneEnemyPresentationHostVisualHitOnlySpec,
	"Wacom.UI.Battle.BattleSceneEnemyPresentation.HostVisualHitOnlyPartsAreLegal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPresentationHostVisualHitOnlySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyPresentationSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleSceneEnemyPresentationSpec::FSceneEnemyPresentationHost SceneHost =
		WacomBattleSceneEnemyPresentationSpec::SpawnThreePartHost(*World);
	if (!TestNotNull(TEXT("Host"), SceneHost.Host)
		|| !TestEqual(TEXT("Part count"), SceneHost.Parts.Num(), 3))
	{
		WacomBattleSceneEnemyPresentationSpec::DestroySceneEnemyPresentationHost(SceneHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyPresentationSpec::DestroySceneEnemyPresentationHost(SceneHost);
	};

	SceneHost.Host->HostSprite = NewObject<UPaperSprite>(SceneHost.Host);
	SceneHost.Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::StaticSprite;
	for (AWacomBattleEnemyPartActor* PartActor : SceneHost.Parts)
	{
		PartActor->VisualLayers.Reset();
	}
	SceneHost.Host->RefreshBattleEnemyPartAuthoringState();

	const FWacomBattleSceneEnemyDebugView HostView = SceneHost.Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host is ready"), HostView.AuthoringState, FName(TEXT("Ready")));
	TestTrue(TEXT("Host visual is active"), HostView.bUsingHostVisual);
	TestEqual(TEXT("Host visual mode"), HostView.HostVisualMode, FName(TEXT("StaticSprite")));
	TestEqual(TEXT("Generated host visual"), HostView.GeneratedHostVisualComponentCount, 1);

	for (AWacomBattleEnemyPartActor* PartActor : SceneHost.Parts)
	{
		const FWacomBattleSceneEnemyPartDebugView PartView =
			PartActor->GetBattleSceneEnemyPartDebugView();
		TestEqual(TEXT("Part is hit-only"), PartView.VisualAuthoringMode, FName(TEXT("HitOnly")));
		TestEqual(TEXT("Part authoring state"), PartView.AuthoringState, FName(TEXT("HitOnly")));
		TestTrue(TEXT("Part remains target ready"), PartView.bAuthoringReady);
		TestTrue(TEXT("Part has host visual context"), PartView.bUsingHostVisual);
		TestTrue(TEXT("Part reports hit-only visual"), PartView.bHitOnlyVisual);
		TestEqual(TEXT("Hit bounds remain query-only"),
			PartActor->GetHitBounds()->GetCollisionEnabled(),
			ECollisionEnabled::QueryOnly);
		TestEqual(TEXT("Feedback target stays per-part root"),
			PartView.FeedbackTargetName,
			FName(TEXT("VisualLayersRoot")));
	}

	TArray<FText> Warnings;
	TArray<FText> Errors;
	const EDataValidationResult HostValidation =
		WacomBattleSceneEnemyPresentationSpec::ValidateObjectForTest(SceneHost.Host, Warnings, Errors);
	TestEqual(TEXT("Host-visual hit-only host validates"), HostValidation, EDataValidationResult::Valid);
	TestEqual(TEXT("Host-visual hit-only host has no errors"), Errors.Num(), 0);
	TestFalse(TEXT("Host-visual hit-only host has no no-art warning"),
		WacomBattleSceneEnemyPresentationSpec::IssuesContain(Warnings, TEXT("只有命中体")));

	for (AWacomBattleEnemyPartActor* PartActor : SceneHost.Parts)
	{
		const EDataValidationResult PartValidation =
			WacomBattleSceneEnemyPresentationSpec::ValidateObjectForTest(PartActor, Warnings, Errors);
		TestEqual(TEXT("Hit-only part validates"), PartValidation, EDataValidationResult::Valid);
		TestEqual(TEXT("Hit-only part has no errors"), Errors.Num(), 0);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPresentationPerPartVisualSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPresentation.PerPartVisualLayersOverrideHostHitOnly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPresentationPerPartVisualSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyPresentationSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleSceneEnemyPresentationSpec::FSceneEnemyPresentationHost SceneHost =
		WacomBattleSceneEnemyPresentationSpec::SpawnThreePartHost(*World);
	AWacomBattleEnemyPartActor* Head = SceneHost.Parts.Num() > 0 ? SceneHost.Parts[0] : nullptr;
	if (!TestNotNull(TEXT("Host"), SceneHost.Host)
		|| !TestNotNull(TEXT("Head"), Head))
	{
		WacomBattleSceneEnemyPresentationSpec::DestroySceneEnemyPresentationHost(SceneHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyPresentationSpec::DestroySceneEnemyPresentationHost(SceneHost);
	};

	SceneHost.Host->HostSprite = NewObject<UPaperSprite>(SceneHost.Host);
	FWacomBattleEnemyPartVisualLayer Layer;
	Layer.LayerId = TEXT("Head.Main");
	Layer.Sprite = NewObject<UPaperSprite>(Head);
	Layer.RelativeLocation = FVector(8.0f, -3.0f, 12.0f);
	Layer.RelativeScale3D = FVector(1.25f, 1.10f, 1.0f);
	Layer.SortOrder = 7;
	Layer.Tint = FLinearColor(0.7f, 0.9f, 1.0f, 0.85f);
	Head->VisualLayers = { Layer };
	SceneHost.Host->RefreshBattleEnemyPartAuthoringState();

	const FWacomBattleSceneEnemyPartDebugView HeadView = Head->GetBattleSceneEnemyPartDebugView();
	TestEqual(TEXT("Visual layers authoring mode"),
		HeadView.VisualAuthoringMode,
		FName(TEXT("VisualLayers")));
	TestEqual(TEXT("Visual layers authoring state"),
		HeadView.AuthoringState,
		FName(TEXT("UsingVisualLayers")));
	TestFalse(TEXT("Visual layer part is not hit-only"), HeadView.bHitOnlyVisual);
	TestTrue(TEXT("Visual layer part still sees host visual context"), HeadView.bUsingHostVisual);
	TestEqual(TEXT("Generated visual layer count"), HeadView.GeneratedVisualLayerComponentCount, 1);
	TestEqual(TEXT("Registered visual layer count"), HeadView.RegisteredVisualLayerComponentCount, 1);
	TestTrue(TEXT("Feedback target remains visual layers root"),
		Head->GetPresentationComponent()->FeedbackTargetComponent == Head->GetVisualLayersRoot());

	TArray<UPaperSpriteComponent*> SpriteComponents;
	Head->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	UPaperSpriteComponent* GeneratedSprite = nullptr;
	for (UPaperSpriteComponent* SpriteComponent : SpriteComponents)
	{
		if (SpriteComponent && SpriteComponent->GetSprite() == Layer.Sprite)
		{
			GeneratedSprite = SpriteComponent;
			break;
		}
	}
	TestNotNull(TEXT("Generated sprite component exists"), GeneratedSprite);
	TestEqual(TEXT("Host remains ready"),
		SceneHost.Host->GetBattleSceneEnemyDebugView().AuthoringState,
		FName(TEXT("Ready")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPresentationValidationSpec,
	"Wacom.UI.Battle.BattleSceneEnemyPresentation.ValidationRejectsBrokenTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPresentationValidationSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyPresentationSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	WacomBattleSceneEnemyPresentationSpec::FSceneEnemyPresentationHost SceneHost =
		WacomBattleSceneEnemyPresentationSpec::SpawnThreePartHost(*World);
	AWacomBattleEnemyPartActor* Head = SceneHost.Parts.Num() > 0 ? SceneHost.Parts[0] : nullptr;
	AWacomBattleEnemyPartActor* Body = SceneHost.Parts.Num() > 1 ? SceneHost.Parts[1] : nullptr;
	if (!TestNotNull(TEXT("Host"), SceneHost.Host)
		|| !TestNotNull(TEXT("Head"), Head)
		|| !TestNotNull(TEXT("Body"), Body))
	{
		WacomBattleSceneEnemyPresentationSpec::DestroySceneEnemyPresentationHost(SceneHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleSceneEnemyPresentationSpec::DestroySceneEnemyPresentationHost(SceneHost);
	};

	TArray<FText> Warnings;
	TArray<FText> Errors;

	Head->PartId = NAME_None;
	Head->PartSlotId = NAME_None;
	Head->RefreshAuthoringState();
	EDataValidationResult Result =
		WacomBattleSceneEnemyPresentationSpec::ValidateObjectForTest(Head, Warnings, Errors);
	TestEqual(TEXT("Missing key part is invalid"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Missing key error"), WacomBattleSceneEnemyPresentationSpec::IssuesContain(Errors, TEXT("PartId")));

	Head->PartId = TEXT("Test.Part.Head");
	Head->PartSlotId = TEXT("Head");
	Head->HitBoundsExtent = FVector(0.0f, 42.0f, 42.0f);
	Head->RefreshAuthoringState();
	Result = WacomBattleSceneEnemyPresentationSpec::ValidateObjectForTest(Head, Warnings, Errors);
	TestEqual(TEXT("Invalid hit bounds part is invalid"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Invalid hit bounds error"),
		WacomBattleSceneEnemyPresentationSpec::IssuesContain(Errors, TEXT("HitBoundsExtent")));

	Head->HitBoundsExtent = FVector(42.0f, 42.0f, 42.0f);
	Body->PartSlotId = TEXT("Head");
	SceneHost.Host->RefreshBattleEnemyPartAuthoringState();
	Result = WacomBattleSceneEnemyPresentationSpec::ValidateObjectForTest(SceneHost.Host, Warnings, Errors);
	TestEqual(TEXT("Duplicate part slot host is invalid"), Result, EDataValidationResult::Invalid);
	TestTrue(TEXT("Duplicate slot error"),
		WacomBattleSceneEnemyPresentationSpec::IssuesContain(Errors, TEXT("重复 PartSlotId")));

	FWacomBattleEnemyPartVisualLayer ValidLayer;
	ValidLayer.LayerId = TEXT("Body.Main");
	ValidLayer.Sprite = NewObject<UPaperSprite>(Body);
	ValidLayer.RelativeScale3D = FVector(1.0f, 1.0f, 1.0f);
	Body->PartSlotId = TEXT("Body");
	Body->VisualLayers = { ValidLayer };
	SceneHost.Host->RefreshBattleEnemyPartAuthoringState();
	Result = WacomBattleSceneEnemyPresentationSpec::ValidateObjectForTest(Body, Warnings, Errors);
	TestEqual(TEXT("Legal per-part visual validates"), Result, EDataValidationResult::Valid);
	TestEqual(TEXT("Legal per-part visual has no errors"), Errors.Num(), 0);
	return true;
}
