// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "Components/ChildActorComponent.h"
#include "Components/WacomBattleEnemyPartPresentationComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"

#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h"
#include "PaperFlipbook.h"
#include "PaperFlipbookComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"

namespace WacomBattleSceneEnemyActorSpec
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

	UPaperFlipbook* MakeOneFrameFlipbookForTest(UObject* Outer)
	{
		UPaperSprite* FrameSprite = NewObject<UPaperSprite>(Outer);
		UPaperFlipbook* Flipbook = NewObject<UPaperFlipbook>(Outer);
		FScopedFlipbookMutator Mutator(Flipbook);
		Mutator.FramesPerSecond = 12.0f;
		FPaperFlipbookKeyFrame KeyFrame;
		KeyFrame.Sprite = FrameSprite;
		KeyFrame.FrameRun = 1;
		Mutator.KeyFrames.Add(KeyFrame);
		return Flipbook;
	}

	UEnemyDefinition* MakeThreePartIdentityEnemyDefinition(UObject* Outer)
	{
		UEnemyDefinition* Enemy = NewObject<UEnemyDefinition>(Outer ? Outer : GetTransientPackage(), NAME_None, RF_Transient);
		if (!Enemy)
		{
			return nullptr;
		}

		Enemy->EnemyId = TEXT("Test.Enemy.SceneIdentityOrder");
		const TArray<TPair<FName, FName>> PartSpecs = {
			{ TEXT("Snake.Head"), TEXT("Head") },
			{ TEXT("Snake.Body"), TEXT("Body") },
			{ TEXT("Snake.Tail"), TEXT("Tail") }
		};
		for (const TPair<FName, FName>& PartSpec : PartSpecs)
		{
			UEnemyPartDefinition* Part = NewObject<UEnemyPartDefinition>(Enemy, NAME_None, RF_Transient);
			Part->PartId = PartSpec.Key;
			Part->MaxHp = 20;

			FEnemyPartSlot Slot;
			Slot.PartSlotId = PartSpec.Value;
			Slot.PartDef = Part;
			Enemy->Parts.Add(Slot);
		}
		return Enemy;
	}

	UChildActorComponent* AddPartChildActorComponent(AWacomBattleEnemyActor& Host, FName ComponentName)
	{
		UChildActorComponent* ChildComponent = NewObject<UChildActorComponent>(&Host, ComponentName);
		if (!ChildComponent)
		{
			return nullptr;
		}

		ChildComponent->SetupAttachment(Host.GetRootComponent());
		Host.AddInstanceComponent(ChildComponent);
		ChildComponent->RegisterComponent();
		ChildComponent->SetChildActorClass(AWacomBattleEnemyPartActor::StaticClass());
		return ChildComponent;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartActorVisibilityAndVisualSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartActorConfiguresVisibilityHitBoundsAndVisual",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartActorVisibilityAndVisualSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	PartActor->PartId = TEXT("Test.Part.Head");
	PartActor->PartSlotId = TEXT("Head");
	PartActor->HitBoundsExtent = FVector(80.f, 45.f, 35.f);
	PartActor->RefreshAuthoringState();

	TestEqual(TEXT("Hit bounds extent is facade controlled"),
		PartActor->GetHitBounds()->GetUnscaledBoxExtent(),
		FVector(80.f, 45.f, 35.f));
	TestEqual(TEXT("Hit bounds uses query-only collision"),
		PartActor->GetHitBounds()->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Hit bounds ignores camera"),
		PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Camera),
		ECR_Ignore);
	TestEqual(TEXT("Hit bounds blocks visibility trace"),
		PartActor->GetHitBounds()->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);
	TestFalse(TEXT("Hit bounds does not generate overlaps"),
		PartActor->GetHitBounds()->GetGenerateOverlapEvents());

	TestNotNull(TEXT("Visual layers root exists"), PartActor->GetVisualLayersRoot());
	TestTrue(TEXT("Presentation feedback target is visual layers root"),
		PartActor->GetPresentationComponent()->FeedbackTargetComponent == PartActor->GetVisualLayersRoot());
	TestNull(TEXT("Presentation no longer targets legacy primitive"),
		PartActor->GetPresentationComponent()->VisualTargetComponent.Get());
	TestEqual(TEXT("No visual resources reports none"),
		PartActor->GetBattleSceneEnemyPartDebugView().VisualAuthoringMode,
		FName(TEXT("None")));
	TestEqual(TEXT("No visual resources reports missing visual resource"),
		PartActor->GetBattleSceneEnemyPartDebugView().AuthoringState,
		FName(TEXT("MissingVisualResource")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyPartVisualLayersSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyPartVisualLayersGeneratePaperSpritesAndFlipbooks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyPartVisualLayersSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyPartActor* PartActor =
		World->SpawnActor<AWacomBattleEnemyPartActor>(
			AWacomBattleEnemyPartActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	if (!TestNotNull(TEXT("Part actor"), PartActor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(PartActor))
		{
			PartActor->Destroy();
		}
	};

	UPaperSprite* BackSprite = NewObject<UPaperSprite>(PartActor);
	UPaperSprite* FrontSprite = NewObject<UPaperSprite>(PartActor);
	UPaperFlipbook* IdleFlipbook =
		WacomBattleSceneEnemyActorSpec::MakeOneFrameFlipbookForTest(PartActor);
	FWacomBattleEnemyPartVisualLayer BackLayer;
	BackLayer.LayerId = TEXT("Back");
	BackLayer.Sprite = BackSprite;
	BackLayer.RelativeLocation = FVector(1.0f, 2.0f, 3.0f);
	BackLayer.RelativeRotation = FRotator(0.0f, 15.0f, 0.0f);
	BackLayer.RelativeScale3D = FVector(1.1f, 1.2f, 1.3f);
	BackLayer.SortOrder = -3;
	BackLayer.Tint = FLinearColor(0.25f, 0.5f, 0.75f, 0.65f);
	BackLayer.bVisible = true;
	FWacomBattleEnemyPartVisualLayer FrontLayer;
	FrontLayer.LayerId = TEXT("Front");
	FrontLayer.Sprite = FrontSprite;
	FrontLayer.RelativeLocation = FVector(4.0f, 5.0f, 6.0f);
	FrontLayer.RelativeScale3D = FVector(0.9f, 0.8f, 0.7f);
	FrontLayer.SortOrder = 8;
	FrontLayer.Tint = FLinearColor(1.0f, 0.2f, 0.1f, 1.0f);
	FrontLayer.bVisible = false;
	FWacomBattleEnemyPartVisualLayer MissingSpriteLayer;
	MissingSpriteLayer.LayerId = TEXT("MissingSprite");
	FWacomBattleEnemyPartVisualLayer IdleLayer;
	IdleLayer.LayerId = TEXT("Idle");
	IdleLayer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
	IdleLayer.Flipbook = IdleFlipbook;
	IdleLayer.RelativeLocation = FVector(7.0f, 8.0f, 9.0f);
	IdleLayer.RelativeRotation = FRotator(0.0f, -20.0f, 0.0f);
	IdleLayer.RelativeScale3D = FVector(1.4f, 1.5f, 1.6f);
	IdleLayer.SortOrder = 4;
	IdleLayer.Tint = FLinearColor(0.4f, 1.0f, 0.6f, 0.8f);
	IdleLayer.bVisible = true;
	IdleLayer.FlipbookPlayRate = 1.5f;
	IdleLayer.bLoopFlipbook = false;
	IdleLayer.FlipbookStartTimeSeconds = 0.05f;
	IdleLayer.bAutoPlayFlipbook = true;
	FWacomBattleEnemyPartVisualLayer MissingFlipbookLayer;
	MissingFlipbookLayer.LayerId = TEXT("MissingFlipbook");
	MissingFlipbookLayer.LayerMode = EWacomBattleEnemyPartVisualLayerMode::Flipbook;
	PartActor->VisualLayers = { BackLayer, MissingSpriteLayer, FrontLayer, IdleLayer, MissingFlipbookLayer };
	PartActor->RefreshAuthoringState();

	TArray<UPaperSpriteComponent*> SpriteComponents;
	PartActor->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	TArray<UPaperFlipbookComponent*> FlipbookComponents;
	PartActor->GetComponents<UPaperFlipbookComponent>(FlipbookComponents);
	TestEqual(TEXT("Only static layers with sprites generate sprite components"), SpriteComponents.Num(), 2);
	TestEqual(TEXT("Only flipbook layers with flipbooks generate flipbook components"), FlipbookComponents.Num(), 1);
	TestTrue(TEXT("Visual layers root exists"), PartActor->GetVisualLayersRoot() != nullptr);
	TestTrue(TEXT("Presentation feedback target is visual layers root"),
		PartActor->GetPresentationComponent()->FeedbackTargetComponent == PartActor->GetVisualLayersRoot());

	UPaperSpriteComponent* BackComponent = nullptr;
	UPaperSpriteComponent* FrontComponent = nullptr;
	UPaperFlipbookComponent* IdleComponent = nullptr;
	for (UPaperSpriteComponent* Component : SpriteComponents)
	{
		if (Component && Component->GetSprite() == BackSprite)
		{
			BackComponent = Component;
		}
		else if (Component && Component->GetSprite() == FrontSprite)
		{
			FrontComponent = Component;
		}
	}
	for (UPaperFlipbookComponent* Component : FlipbookComponents)
	{
		if (Component && Component->GetFlipbook() == IdleFlipbook)
		{
			IdleComponent = Component;
		}
	}

	if (!TestNotNull(TEXT("Back sprite component"), BackComponent)
		|| !TestNotNull(TEXT("Front sprite component"), FrontComponent)
		|| !TestNotNull(TEXT("Idle flipbook component"), IdleComponent))
	{
		return false;
	}

	TestEqual(TEXT("Back component location"), BackComponent->GetRelativeLocation(), BackLayer.RelativeLocation);
	TestEqual(TEXT("Back component rotation"), BackComponent->GetRelativeRotation(), BackLayer.RelativeRotation);
	TestEqual(TEXT("Back component scale"), BackComponent->GetRelativeScale3D(), BackLayer.RelativeScale3D);
	TestEqual(TEXT("Back sort priority"), BackComponent->TranslucencySortPriority, BackLayer.SortOrder);
	TestEqual(TEXT("Back tint"), BackComponent->GetSpriteColor(), BackLayer.Tint);
	TestTrue(TEXT("Back visible"), BackComponent->IsVisible());
	TestEqual(TEXT("Back no collision"), BackComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Back no overlaps"), BackComponent->GetGenerateOverlapEvents());
	TestEqual(TEXT("Front sort priority"), FrontComponent->TranslucencySortPriority, FrontLayer.SortOrder);
	TestFalse(TEXT("Front visibility follows layer"), FrontComponent->IsVisible());
	TestEqual(TEXT("Idle flipbook location"), IdleComponent->GetRelativeLocation(), IdleLayer.RelativeLocation);
	TestEqual(TEXT("Idle flipbook rotation"), IdleComponent->GetRelativeRotation(), IdleLayer.RelativeRotation);
	TestEqual(TEXT("Idle flipbook scale"), IdleComponent->GetRelativeScale3D(), IdleLayer.RelativeScale3D);
	TestEqual(TEXT("Idle flipbook sort priority"), IdleComponent->TranslucencySortPriority, IdleLayer.SortOrder);
	TestEqual(TEXT("Idle flipbook tint"), IdleComponent->GetSpriteColor(), IdleLayer.Tint);
	TestTrue(TEXT("Idle flipbook visible"), IdleComponent->IsVisible());
	TestEqual(TEXT("Idle flipbook no collision"), IdleComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Idle flipbook no overlaps"), IdleComponent->GetGenerateOverlapEvents());
	TestEqual(TEXT("Idle flipbook play rate"), IdleComponent->GetPlayRate(), IdleLayer.FlipbookPlayRate);
	TestFalse(TEXT("Idle flipbook looping follows layer"), IdleComponent->IsLooping());
	TestTrue(TEXT("Idle flipbook starts playing"), IdleComponent->IsPlaying());

	const FWacomBattleSceneEnemyPartDebugView View = PartActor->GetBattleSceneEnemyPartDebugView();
	TestTrue(TEXT("Debug reports visual layers"), View.bUsingVisualLayers);
	TestEqual(TEXT("Debug layer count"), View.VisualLayerCount, 5);
	TestEqual(TEXT("Debug generated component count"), View.GeneratedVisualLayerComponentCount, 3);
	TestEqual(TEXT("Debug generated static component count"), View.GeneratedStaticVisualLayerComponentCount, 2);
	TestEqual(TEXT("Debug generated flipbook component count"), View.GeneratedFlipbookVisualLayerComponentCount, 1);
	TestEqual(TEXT("Debug registered component count"), View.RegisteredVisualLayerComponentCount, 3);
	TestEqual(TEXT("Debug registered static component count"), View.RegisteredStaticVisualLayerComponentCount, 2);
	TestEqual(TEXT("Debug registered flipbook component count"), View.RegisteredFlipbookVisualLayerComponentCount, 1);
	TestEqual(TEXT("Debug visible component count"), View.VisibleVisualLayerComponentCount, 2);
	TestEqual(TEXT("Debug visible static component count"), View.VisibleStaticVisualLayerComponentCount, 1);
	TestEqual(TEXT("Debug visible flipbook component count"), View.VisibleFlipbookVisualLayerComponentCount, 1);
	TestEqual(TEXT("Debug missing asset count"), View.MissingVisualLayerAssetCount, 2);
	TestEqual(TEXT("Debug missing sprite count"), View.MissingVisualLayerSpriteCount, 1);
	TestEqual(TEXT("Debug missing flipbook count"), View.MissingVisualLayerFlipbookCount, 1);
	TestTrue(TEXT("Debug layer ids contain back"), View.VisualLayerIds.Contains(TEXT("Back")));
	TestTrue(TEXT("Debug layer ids contain idle"), View.VisualLayerIds.Contains(TEXT("Idle")));
	TestTrue(TEXT("Debug visual asset names contain idle flipbook"),
		View.VisualLayerAssetNames.Contains(FName(*IdleFlipbook->GetName())));
	TestEqual(TEXT("Debug feedback target"), View.FeedbackTargetName, FName(TEXT("VisualLayersRoot")));
	TestEqual(TEXT("Details visual mode mirrors debug view"),
		PartActor->VisualAuthoringMode,
		FName(TEXT("VisualLayers")));
	TestEqual(TEXT("Details layer count mirrors debug view"),
		PartActor->AuthoringVisualLayerCount,
		View.VisualLayerCount);
	TestEqual(TEXT("Details generated layer count mirrors debug view"),
		PartActor->AuthoringGeneratedVisualLayerComponentCount,
		View.GeneratedVisualLayerComponentCount);
	TestEqual(TEXT("Details registered layer count mirrors debug view"),
		PartActor->AuthoringRegisteredVisualLayerComponentCount,
		View.RegisteredVisualLayerComponentCount);
	TestEqual(TEXT("Details visible layer count mirrors debug view"),
		PartActor->AuthoringVisibleVisualLayerComponentCount,
		View.VisibleVisualLayerComponentCount);
	TestEqual(TEXT("Details missing asset count mirrors debug view"),
		PartActor->AuthoringMissingVisualLayerAssetCount,
		View.MissingVisualLayerAssetCount);
	TestEqual(TEXT("Details feedback target mirrors debug view"),
		PartActor->AuthoringFeedbackTargetName,
		View.FeedbackTargetName);
	TestTrue(TEXT("Summary reports visual layer count"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("VisualLayerCount=5")));
	TestTrue(TEXT("Summary reports flipbook component count"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("GeneratedFlipbookVisualLayerComponents=1")));
	TestTrue(TEXT("Summary reports registered component count"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("RegisteredVisualLayerComponents=3")));
	TestTrue(TEXT("Summary reports visible component count"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(TEXT("VisibleVisualLayerComponents=2")));
	TestTrue(TEXT("Summary reports visual layer ids"),
		PartActor->GetBattleSceneEnemyPartDebugSummary().Contains(
			TEXT("VisualLayerIds=Back|MissingSprite|Front|Idle|MissingFlipbook")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostVisualGeneratesPaperComponentsSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostVisualGeneratesPaperSpriteAndFlipbook",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostVisualGeneratesPaperComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
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

	UPaperSprite* HostSprite = NewObject<UPaperSprite>(Host);
	Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::StaticSprite;
	Host->HostSprite = HostSprite;
	Host->HostVisualRelativeLocation = FVector(11.f, 12.f, 13.f);
	Host->HostVisualRelativeRotation = FRotator(0.f, 23.f, 0.f);
	Host->HostVisualRelativeScale3D = FVector(1.2f, 1.3f, 1.4f);
	Host->HostVisualSortOrder = 17;
	Host->HostVisualTint = FLinearColor(0.2f, 0.4f, 0.6f, 0.75f);
	Host->RefreshBattleEnemyPartAuthoringState();

	UPaperSpriteComponent* SpriteComponent = Host->GetGeneratedHostSpriteVisualComponent();
	TestNotNull(TEXT("Host sprite visual component generated"), SpriteComponent);
	TestNull(TEXT("Host flipbook visual component absent"), Host->GetGeneratedHostFlipbookVisualComponent());
	if (!SpriteComponent)
	{
		return false;
	}
	TestEqual(TEXT("Host sprite asset"), SpriteComponent->GetSprite(), HostSprite);
	TestEqual(TEXT("Host sprite location"), SpriteComponent->GetRelativeLocation(), Host->HostVisualRelativeLocation);
	TestEqual(TEXT("Host sprite rotation"), SpriteComponent->GetRelativeRotation(), Host->HostVisualRelativeRotation);
	TestEqual(TEXT("Host sprite scale"), SpriteComponent->GetRelativeScale3D(), Host->HostVisualRelativeScale3D);
	TestEqual(TEXT("Host sprite sort priority"), SpriteComponent->TranslucencySortPriority, Host->HostVisualSortOrder);
	TestEqual(TEXT("Host sprite tint"), SpriteComponent->GetSpriteColor(), Host->HostVisualTint);
	TestEqual(TEXT("Host sprite no collision"), SpriteComponent->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestFalse(TEXT("Host sprite no overlaps"), SpriteComponent->GetGenerateOverlapEvents());
	TestTrue(TEXT("Host visual active"), Host->IsHostVisualActive());
	TestEqual(TEXT("Host debug visual mode"),
		Host->GetBattleSceneEnemyDebugView().HostVisualMode,
		FName(TEXT("StaticSprite")));
	TestEqual(TEXT("Host debug generated component count"),
		Host->GetBattleSceneEnemyDebugView().GeneratedHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host debug registered component count"),
		Host->GetBattleSceneEnemyDebugView().RegisteredHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host debug visible component count"),
		Host->GetBattleSceneEnemyDebugView().VisibleHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host details registered component count"),
		Host->AuthoringRegisteredHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host details visible component count"),
		Host->AuthoringVisibleHostVisualComponentCount,
		1);

	UPaperFlipbook* HostFlipbook = WacomBattleSceneEnemyActorSpec::MakeOneFrameFlipbookForTest(Host);
	Host->HostVisualMode = EWacomBattleEnemyHostVisualMode::Flipbook;
	Host->HostFlipbook = HostFlipbook;
	Host->HostFlipbookPlayRate = 1.75f;
	Host->bLoopHostFlipbook = false;
	Host->HostFlipbookStartTimeSeconds = 0.02f;
	Host->RefreshBattleEnemyPartAuthoringState();

	TArray<UPaperSpriteComponent*> SpriteComponents;
	Host->GetComponents<UPaperSpriteComponent>(SpriteComponents);
	TArray<UPaperFlipbookComponent*> FlipbookComponents;
	Host->GetComponents<UPaperFlipbookComponent>(FlipbookComponents);
	TestEqual(TEXT("Switching to flipbook removes stale host sprite component"), SpriteComponents.Num(), 0);
	TestEqual(TEXT("One host flipbook component generated"), FlipbookComponents.Num(), 1);
	UPaperFlipbookComponent* FlipbookComponent = Host->GetGeneratedHostFlipbookVisualComponent();
	TestNotNull(TEXT("Host flipbook visual component generated"), FlipbookComponent);
	TestNull(TEXT("Host sprite visual component absent after switch"), Host->GetGeneratedHostSpriteVisualComponent());
	if (!FlipbookComponent)
	{
		return false;
	}
	TestEqual(TEXT("Host flipbook asset"), FlipbookComponent->GetFlipbook(), HostFlipbook);
	TestEqual(TEXT("Host flipbook play rate"), FlipbookComponent->GetPlayRate(), Host->HostFlipbookPlayRate);
	TestFalse(TEXT("Host flipbook looping follows config"), FlipbookComponent->IsLooping());
	TestEqual(TEXT("Host debug flipbook mode"),
		Host->GetBattleSceneEnemyDebugView().HostVisualMode,
		FName(TEXT("Flipbook")));
	TestEqual(TEXT("Host debug registered flipbook count"),
		Host->GetBattleSceneEnemyDebugView().RegisteredHostVisualComponentCount,
		1);
	TestEqual(TEXT("Host debug visible flipbook count"),
		Host->GetBattleSceneEnemyDebugView().VisibleHostVisualComponentCount,
		1);
	TestTrue(TEXT("Host summary reports host visual"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("UsingHostVisual=true")));
	TestTrue(TEXT("Host summary reports registered visual"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("RegisteredHostVisualComponents=1")));
	TestTrue(TEXT("Host summary reports visible visual"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("VisibleHostVisualComponents=1")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleSceneEnemyHostDoesNotAutofillIdentityFromChildNamesSpec,
	"Wacom.UI.Battle.BattleSceneEnemyActor.BattleSceneEnemyHostDoesNotAutofillIdentityFromChildActorNames",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleSceneEnemyHostDoesNotAutofillIdentityFromChildNamesSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleSceneEnemyActorSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleEnemyActor* Host =
		World->SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
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

	Host->EnemyDefinition = WacomBattleSceneEnemyActorSpec::MakeThreePartIdentityEnemyDefinition(Host);

	UChildActorComponent* FirstComponent =
		WacomBattleSceneEnemyActorSpec::AddPartChildActorComponent(*Host, TEXT("MisleadingTailName"));
	UChildActorComponent* SecondComponent =
		WacomBattleSceneEnemyActorSpec::AddPartChildActorComponent(*Host, TEXT("MisleadingHeadName"));
	UChildActorComponent* ThirdComponent =
		WacomBattleSceneEnemyActorSpec::AddPartChildActorComponent(*Host, TEXT("MisleadingBodyName"));
	if (!TestNotNull(TEXT("First child component"), FirstComponent)
		|| !TestNotNull(TEXT("Second child component"), SecondComponent)
		|| !TestNotNull(TEXT("Third child component"), ThirdComponent))
	{
		return false;
	}

	AWacomBattleEnemyPartActor* FirstPart =
		Cast<AWacomBattleEnemyPartActor>(FirstComponent->GetChildActor());
	AWacomBattleEnemyPartActor* SecondPart =
		Cast<AWacomBattleEnemyPartActor>(SecondComponent->GetChildActor());
	AWacomBattleEnemyPartActor* ThirdPart =
		Cast<AWacomBattleEnemyPartActor>(ThirdComponent->GetChildActor());
	if (!TestNotNull(TEXT("First child actor"), FirstPart)
		|| !TestNotNull(TEXT("Second child actor"), SecondPart)
		|| !TestNotNull(TEXT("Third child actor"), ThirdPart))
	{
		return false;
	}

	TestTrue(TEXT("First part starts blank"), FirstPart->PartId.IsNone() && FirstPart->PartSlotId.IsNone());
	TestTrue(TEXT("Second part starts blank"), SecondPart->PartId.IsNone() && SecondPart->PartSlotId.IsNone());
	TestTrue(TEXT("Third part starts blank"), ThirdPart->PartId.IsNone() && ThirdPart->PartSlotId.IsNone());

	Host->RefreshBattleEnemyPartAuthoringState();

	TestTrue(TEXT("First scanned part remains blank"), FirstPart->PartId.IsNone() && FirstPart->PartSlotId.IsNone());
	TestTrue(TEXT("Second scanned part remains blank"), SecondPart->PartId.IsNone() && SecondPart->PartSlotId.IsNone());
	TestTrue(TEXT("Third scanned part remains blank"), ThirdPart->PartId.IsNone() && ThirdPart->PartSlotId.IsNone());
	TestEqual(TEXT("Host still injects enemy slot for debug/bridge context"), FirstPart->EnemySlotId, FName(TEXT("Enemy")));
	TestEqual(TEXT("First part reports missing identity"), FirstPart->AuthoringState, FName(TEXT("MissingIdentity")));
	TestEqual(TEXT("Second part reports missing identity"), SecondPart->AuthoringState, FName(TEXT("MissingIdentity")));
	TestEqual(TEXT("Third part reports missing identity"), ThirdPart->AuthoringState, FName(TEXT("MissingIdentity")));

	const FWacomBattleSceneEnemyDebugView View = Host->GetBattleSceneEnemyDebugView();
	TestEqual(TEXT("Host remains not ready without explicit part slots"), View.AuthoringState, FName(TEXT("PartSlotMismatch")));
	TestEqual(TEXT("Host missing definition part slots are reported"), View.MissingDefinitionPartSlotIds.Num(), 3);
	TestTrue(TEXT("Host summary reports missing definition part slots"),
		Host->GetBattleSceneEnemyDebugSummary().Contains(TEXT("MissingDefinitionPartSlotIds=[Head,Body,Tail]")));
	return true;
}
