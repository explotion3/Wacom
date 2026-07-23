// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "../../../WacomApp/Private/GameFramework/WacomBattleEnemyPartInteractionQueryPolicy.h"
#include "../../../WacomApp/Private/Components/WacomBattleEnemyPartOutlineProxyGeometry.h"
#include "../../../WacomApp/Private/Interaction/WacomInteractionTargetHitResolver.h"
#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartTargetPreviewStyle.h"
#include "Components/BoxComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SceneComponent.h"
#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomBattleEnemyPartSpriteLayerComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Misc/ScopeExit.h"
#include "PaperSprite.h"
#include "PhysicsEngine/BodySetup.h"
#include "StaticMeshResources.h"
#include "Snapshots/BattleSnapshot.h"
#include "Testing/WacomEnemySceneRuntimeAutomationTestView.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "Interaction/WacomInteractionCollisionChannels.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleEnemyInteractionVisualSpec
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

	template <typename TComponent>
	TComponent* AddSceneComponent(
		AWacomBattleEnemyActor& Host,
		USceneComponent& Parent,
		FName Name)
	{
		TComponent* Component = NewObject<TComponent>(
			&Host,
			Name,
			RF_Transient | RF_Transactional);
		Host.AddInstanceComponent(Component);
		Component->CreationMethod = EComponentCreationMethod::Instance;
		Component->SetupAttachment(&Parent);
		Component->RegisterComponent();
		return Component;
	}

	UPaperSprite* MakeCollisionSprite(UObject& Outer)
	{
		UPaperSprite* Sprite = NewObject<UPaperSprite>(&Outer, NAME_None, RF_Transient);
		UBodySetup* BodySetup = NewObject<UBodySetup>(Sprite, NAME_None, RF_Transient);
		BodySetup->AggGeom.BoxElems.Add(FKBoxElem(96.0f, 12.0f, 112.0f));
		Sprite->BodySetup = BodySetup;
		return Sprite;
	}

	UPaperSprite* MakeRenderBoundsOnlySprite(UObject& Outer)
	{
		UPaperSprite* Source = LoadObject<UPaperSprite>(
			nullptr,
			TEXT("/Game/Wacom/Art/Enemies/TrainingWarrior/Sprites/"
				"SPR_Enemy_TrainingWarrior_Idle_00."
				"SPR_Enemy_TrainingWarrior_Idle_00"));
		UPaperSprite* Sprite = Source
			? DuplicateObject<UPaperSprite>(Source, &Outer)
			: nullptr;
		if (Sprite)
		{
			Sprite->BodySetup = nullptr;
		}
		return Sprite;
	}

	enum class ECollisionFixtureMode : uint8
	{
		Stable,
		InteractionVisualBoundsFallback,
		DirectVisualUnionFallback,
		DefaultSafetyFallback,
	};

	struct FFixture
	{
		TStrongObjectPtr<UEnemyDefinition> EnemyDefinition;
		TStrongObjectPtr<UEnemyPartDefinition> PartDefinition;
		AWacomBattleEnemyActor* Host = nullptr;
		UWacomBattleEnemyPartComponent* Part = nullptr;
		UWacomBattleEnemyPartSpriteLayerComponent* InteractionVisual = nullptr;
		UWacomBattleEnemyPartSpriteLayerComponent* DecorationVisual = nullptr;
		UPaperSprite* StableSprite = nullptr;
		FBattleSnapshot Snapshot;

		bool Initialize(
			UWorld& World,
			ECollisionFixtureMode Mode = ECollisionFixtureMode::Stable)
		{
			PartDefinition.Reset(NewObject<UEnemyPartDefinition>(
				GetTransientPackage(), NAME_None, RF_Transient));
			PartDefinition->PartId = TEXT("Interaction.Body");
			PartDefinition->MaxHp = 10;
			EnemyDefinition.Reset(NewObject<UEnemyDefinition>(
				GetTransientPackage(), NAME_None, RF_Transient));
			EnemyDefinition->EnemyId = TEXT("Enemy.Interaction");
			FEnemyPartSlot& Slot = EnemyDefinition->Parts.AddDefaulted_GetRef();
			Slot.PartSlotId = TEXT("Body");
			Slot.PartDef = PartDefinition.Get();

			FActorSpawnParameters SpawnParams;
			SpawnParams.ObjectFlags |= RF_Transient;
			Host = World.SpawnActor<AWacomBattleEnemyActor>(
				FVector::ZeroVector,
				FRotator::ZeroRotator,
				SpawnParams);
			if (!Host)
			{
				return false;
			}
			Host->EnemyDefinition = EnemyDefinition.Get();
			Host->EnemySlotId = TEXT("Enemy");
			Part = AddSceneComponent<UWacomBattleEnemyPartComponent>(
				*Host, *Host->GetRootComponent(), TEXT("Part_Body"));
			Part->PartSlotId = TEXT("Body");
			Part->SetDerivedPartId(TEXT("Interaction.Body"));
			Part->InteractionVisualLayerId = TEXT("Body.Main");

			if (Mode != ECollisionFixtureMode::DefaultSafetyFallback)
			{
				StableSprite = Mode == ECollisionFixtureMode::Stable
					? MakeCollisionSprite(*Host)
					: MakeRenderBoundsOnlySprite(*Host);
				if (!StableSprite)
				{
					return false;
				}
				InteractionVisual =
					AddSceneComponent<UWacomBattleEnemyPartSpriteLayerComponent>(
						*Host, *Part, TEXT("Visual_Body_Main"));
				InteractionVisual->LayerId = TEXT("Body.Main");
				InteractionVisual->SetSprite(StableSprite);
				DecorationVisual =
					AddSceneComponent<UWacomBattleEnemyPartSpriteLayerComponent>(
						*Host, *Part, TEXT("Visual_Body_Decoration"));
				DecorationVisual->LayerId = Mode
					== ECollisionFixtureMode::DirectVisualUnionFallback
						? FName(TEXT("Body.Main"))
						: FName(TEXT("Body.Decoration"));
				DecorationVisual->SetSprite(
					Mode == ECollisionFixtureMode::DirectVisualUnionFallback
						? MakeRenderBoundsOnlySprite(*Host)
						: NewObject<UPaperSprite>(
							Host, NAME_None, RF_Transient));
				if (Mode == ECollisionFixtureMode::DirectVisualUnionFallback)
				{
					DecorationVisual->SetRelativeLocation(FVector(32.0f, 0.0f, 0.0f));
				}
			}
			Host->NotifyEnemySceneComponentTopologyChanged();

			Snapshot.EncounterId = TEXT("Encounter.Interaction");
			Snapshot.Phase = EBattlePhase::PlayerAction;
			FEnemySnapshot& Enemy = Snapshot.Enemies.AddDefaulted_GetRef();
			Enemy.Definition = EnemyDefinition.Get();
			Enemy.EncounterId = Snapshot.EncounterId;
			Enemy.EnemySlotId = TEXT("Enemy");
			FEnemyPartSnapshot& PartSnapshot = Enemy.Parts.AddDefaulted_GetRef();
			PartSnapshot.InstanceId = FGuid::NewGuid();
			PartSnapshot.Definition = PartDefinition.Get();
			PartSnapshot.EncounterId = Snapshot.EncounterId;
			PartSnapshot.EnemySlotId = Enemy.EnemySlotId;
			PartSnapshot.PartSlotId = TEXT("Body");
			PartSnapshot.Identity = FBattlePartSlotIdentity::Make(
				PartSnapshot.EncounterId,
				PartSnapshot.EnemySlotId,
				PartSnapshot.PartSlotId);
			PartSnapshot.CurrentHp = 10;
			PartSnapshot.MaxHp = 10;
			FWacomEnemySceneRuntimeAutomationTestView::InitializeBinding(
				*Host, Snapshot.EncounterId, TEXT("Enemy"));
			return FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
				*Host, *Part, Snapshot);
		}

		UBoxComponent* FindFallbackCollision() const
		{
			if (!Host || !Part)
			{
				return nullptr;
			}
			TArray<UBoxComponent*> Boxes;
			Host->GetComponents(Boxes);
			UBoxComponent** Match = Boxes.FindByPredicate([this](const UBoxComponent* Box)
			{
				return Box
					&& Box->GetAttachParent() == Part
					&& Box->HasAnyFlags(RF_Transient);
			});
			return Match ? *Match : nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyInteractionVisualCollisionSpec,
	"Wacom.UI.Battle.EnemyScene.InteractionVisual.StableCollisionAndProviderIdentity",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyInteractionVisualCollisionSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInteractionVisualSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FFixture Fixture;
	if (!TestTrue(TEXT("Fixture binds"), Fixture.Initialize(*World)))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Fixture.Host))
		{
			Fixture.Host->Destroy();
		}
	};

	FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
		*Fixture.Host, *Fixture.Part, true, false);
	const FWacomBattleEnemyPartRuntimeDebugView Debug = Fixture.Part->GetRuntimeDebugView();
	TestTrue(TEXT("Interaction layer resolves exactly"), Debug.bInteractionVisualResolved);
	TestTrue(TEXT("Stable collision is ready"), Debug.bInteractionCollisionReady);
	TestFalse(TEXT("Box fallback is disabled"), Debug.bUsingBoxCollisionFallback);
	TestEqual(TEXT("Stable collision source is explicit"),
		Debug.InteractionCollisionSource, FName(TEXT("StableSpriteBodySetup")));
	TestFalse(TEXT("Part is identity-only, not a PrimitiveComponent"),
		Fixture.Part->IsA<UPrimitiveComponent>());
	TestEqual(TEXT("Interaction visual owns query collision"),
		Fixture.InteractionVisual->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Interaction visual blocks only the dedicated channel"),
		Fixture.InteractionVisual->GetCollisionResponseToChannel(
			Wacom::Interaction::BattleEnemyPartTraceChannel),
		ECR_Block);
	TestEqual(TEXT("Interaction visual does not block Visibility"),
		Fixture.InteractionVisual->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Ignore);
	TestEqual(TEXT("Decoration remains non-colliding"),
		Fixture.DecorationVisual->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
	TestNull(TEXT("Collision-ready Part never creates a transient fallback"),
		Fixture.FindFallbackCollision());

	const FWacomInteractionTargetHandle InteractionHandle =
		Fixture.InteractionVisual->BuildWorldTargetHandle();
	TestTrue(TEXT("Interaction visual forwards a valid Part handle"),
		InteractionHandle.IsValid());
	TestTrue(TEXT("Forwarded SourceObject remains the Part"),
		InteractionHandle.SourceObject.Get() == Fixture.Part);
	TestFalse(TEXT("Decoration cannot forward target identity"),
		Fixture.DecorationVisual->BuildWorldTargetHandle().IsValid());
	FHitResult DecorationHit;
	DecorationHit.Component = Fixture.DecorationVisual;
	DecorationHit.HitObjectHandle = FActorInstanceHandle(Fixture.Host);
	TestFalse(TEXT("Actor fallback never converts a decoration hit into a battle Part"),
		WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(
			DecorationHit).IsValid());

	UBodySetup* StableBodySetup = Fixture.InteractionVisual->GetBodySetup();
	Fixture.InteractionVisual->SetSprite(NewObject<UPaperSprite>(
		Fixture.Host, NAME_None, RF_Transient));
	TestTrue(TEXT("Runtime sprite swap keeps authored Idle collision BodySetup"),
		Fixture.InteractionVisual->GetBodySetup() == StableBodySetup);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyInteractionFallbackCollisionSpec,
	"Wacom.UI.Battle.EnemyScene.InteractionVisual.TransientFallbackSourcesAndLifecycle",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyInteractionFallbackCollisionSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInteractionVisualSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	const TArray<TPair<ECollisionFixtureMode, FName>> Cases =
	{
		{ ECollisionFixtureMode::InteractionVisualBoundsFallback,
			TEXT("InteractionVisualBoundsFallback") },
		{ ECollisionFixtureMode::DirectVisualUnionFallback,
			TEXT("DirectVisualUnionFallback") },
		{ ECollisionFixtureMode::DefaultSafetyFallback,
			TEXT("DefaultSafetyFallback") },
	};
	for (const TPair<ECollisionFixtureMode, FName>& Case : Cases)
	{
		FFixture Fixture;
		if (!TestTrue(TEXT("Fallback fixture binds"),
			Fixture.Initialize(*World, Case.Key)))
		{
			continue;
		}

		TestNull(TEXT("Fallback is not created before HUD registration"),
			Fixture.FindFallbackCollision());
		FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
			*Fixture.Host, *Fixture.Part, true, false);
		UBoxComponent* Fallback = Fixture.FindFallbackCollision();
		if (TestNotNull(TEXT("Fallback is lazily created"), Fallback))
		{
			TestEqual(TEXT("Fallback source is diagnosed"),
				Fixture.Part->GetRuntimeDebugView().InteractionCollisionSource,
				Case.Value);
			TestTrue(TEXT("Fallback compatibility flag remains available"),
				Fixture.Part->GetRuntimeDebugView().bUsingBoxCollisionFallback);
			TestEqual(TEXT("Fallback becomes query-only while registered"),
				Fallback->GetCollisionEnabled(), ECollisionEnabled::QueryOnly);
			TestEqual(TEXT("Fallback blocks only the dedicated channel"),
				Fallback->GetCollisionResponseToChannel(
					Wacom::Interaction::BattleEnemyPartTraceChannel),
				ECR_Block);
			TestEqual(TEXT("Fallback ignores Visibility"),
				Fallback->GetCollisionResponseToChannel(ECC_Visibility),
				ECR_Ignore);
			TestFalse(TEXT("Fallback does not generate overlap events"),
				Fallback->GetGenerateOverlapEvents());
			TestTrue(TEXT("Fallback half extents respect the 6cm minimum"),
				Fallback->GetUnscaledBoxExtent().GetMin() >= 6.0f);
			if (Case.Key == ECollisionFixtureMode::DefaultSafetyFallback)
			{
				TestTrue(TEXT("No-visual fallback uses the fixed safety bounds"),
					Fallback->GetUnscaledBoxExtent().Equals(
						FVector(55.0f, 45.0f, 55.0f)));
			}

			FHitResult FallbackHit;
			FallbackHit.Component = Fallback;
			FallbackHit.HitObjectHandle = FActorInstanceHandle(Fixture.Host);
			const FWacomInteractionTargetHandle Handle =
				WacomInteractionTargetHitResolver::BuildWorldTargetHandleFromHit(
					FallbackHit);
			TestTrue(TEXT("Fallback forwards a valid Part handle"),
				Handle.IsValid());
			TestTrue(TEXT("Fallback SourceObject remains the identity Part"),
				Handle.SourceObject.Get() == Fixture.Part);

			TestTrue(TEXT("Repeated sync succeeds"),
				FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
					*Fixture.Host, *Fixture.Part, Fixture.Snapshot));
			TestTrue(TEXT("Repeated refresh reuses one fallback component"),
				Fixture.FindFallbackCollision() == Fallback);
			FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
				*Fixture.Host, *Fixture.Part, false, false);
			TestEqual(TEXT("Registry removal immediately disables fallback"),
				Fallback->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
			FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
				*Fixture.Host, *Fixture.Part, true, false);
			Fixture.Snapshot.Enemies[0].Parts[0].bDestroyed = true;
			TestFalse(TEXT("Destroyed snapshot removes the interactive binding"),
				FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
					*Fixture.Host, *Fixture.Part, Fixture.Snapshot));
			TestEqual(TEXT("Destroyed immediately disables fallback"),
				Fallback->GetCollisionEnabled(), ECollisionEnabled::NoCollision);
		}

		Fixture.Host->RetireRuntimeEncounterPresentation();
		if (UBoxComponent* RetiredFallback = Fixture.FindFallbackCollision())
		{
			TestEqual(TEXT("Retire keeps fallback disabled"),
				RetiredFallback->GetCollisionEnabled(),
				ECollisionEnabled::NoCollision);
		}
		Fixture.Host->Destroy();
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyInteractionQueryPolicySpec,
	"Wacom.UI.Battle.EnemyScene.InteractionQuery.OcclusionRegistryAndStableOrdering",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyInteractionQueryPolicySpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleEnemyPartInteractionQueryCandidate Candidate;
	Candidate.bInCurrentRegistry = true;
	Candidate.TraceDepth = 90.0f;
	TestTrue(TEXT("Candidate before occluder is eligible"),
		FWacomBattleEnemyPartInteractionQueryPolicy::IsEligible(Candidate, 100.0f));
	Candidate.TraceDepth = 110.0f;
	TestFalse(TEXT("Candidate behind strict occluder is rejected"),
		FWacomBattleEnemyPartInteractionQueryPolicy::IsEligible(Candidate, 100.0f));
	Candidate.TraceDepth = 90.0f;
	Candidate.bInCurrentRegistry = false;
	TestFalse(TEXT("Candidate outside current HUD registry is rejected"),
		FWacomBattleEnemyPartInteractionQueryPolicy::IsEligible(
			Candidate, TNumericLimits<float>::Max()));

	FWacomBattleEnemyPartInteractionQueryCandidate Left;
	Left.bInCurrentRegistry = true;
	Left.ScreenDistanceSquared = 9.0f;
	Left.TraceDepth = 120.0f;
	Left.StableIdentity = TEXT("Encounter|Enemy|Left");
	FWacomBattleEnemyPartInteractionQueryCandidate Right = Left;
	Right.ScreenDistanceSquared = 16.0f;
	Right.TraceDepth = 80.0f;
	Right.StableIdentity = TEXT("Encounter|Enemy|Right");
	TestTrue(TEXT("Screen-space distance wins before trace depth"),
		FWacomBattleEnemyPartInteractionQueryPolicy::IsPreferred(Left, Right));
	Right.ScreenDistanceSquared = Left.ScreenDistanceSquared;
	TestFalse(TEXT("Nearer trace depth wins when screen distance ties"),
		FWacomBattleEnemyPartInteractionQueryPolicy::IsPreferred(Left, Right));
	Right.TraceDepth = Left.TraceDepth;
	TestTrue(TEXT("Stable identity is final deterministic tie-break"),
		FWacomBattleEnemyPartInteractionQueryPolicy::IsPreferred(Left, Right));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyInteractionOutlinePlaneUVOrientationSpec,
	"Wacom.UI.Battle.EnemyScene.InteractionVisual.OutlinePlaneUVOrientation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyInteractionOutlinePlaneUVOrientationSpec::RunTest(
	const FString& /*Parameters*/)
{
	UStaticMesh* Plane = LoadObject<UStaticMesh>(
		nullptr, TEXT("/Engine/BasicShapes/Plane.Plane"));
	if (!TestNotNull(TEXT("Formal outline proxy plane loads"), Plane)
		|| !Plane->GetRenderData()
		|| Plane->GetRenderData()->LODResources.IsEmpty())
	{
		return false;
	}

	const FStaticMeshLODResources& LOD = Plane->GetRenderData()->LODResources[0];
	const uint32 VertexCount = LOD.VertexBuffers.PositionVertexBuffer.GetNumVertices();
	if (!TestTrue(TEXT("Proxy plane exposes render vertices"), VertexCount > 2))
	{
		return false;
	}
	double MeanLocalY = 0.0;
	double MeanV = 0.0;
	for (uint32 Index = 0; Index < VertexCount; ++Index)
	{
		MeanLocalY += LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(Index).Y;
		MeanV += LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index, 0).Y;
	}
	MeanLocalY /= VertexCount;
	MeanV /= VertexCount;
	double YVCovariance = 0.0;
	for (uint32 Index = 0; Index < VertexCount; ++Index)
	{
		const double LocalY = LOD.VertexBuffers.PositionVertexBuffer.VertexPosition(Index).Y;
		const double V = LOD.VertexBuffers.StaticMeshVertexBuffer.GetVertexUV(Index, 0).Y;
		YVCovariance += (LocalY - MeanLocalY) * (V - MeanV);
	}
	TestTrue(TEXT("Basic Plane UV.V runs with its local +Y axis"),
		YVCovariance > UE_DOUBLE_SMALL_NUMBER);
	const FVector RotatedVAxis =
		WacomBattleEnemyPartOutlineProxyGeometry::ResolvePlaneToSpriteRotation()
		.RotateVector(FVector::YAxisVector);
	TestTrue(TEXT("Formal proxy rotation maps increasing Plane V to Sprite local +Z"),
		RotatedVAxis.Z > 1.0 - UE_KINDA_SMALL_NUMBER);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyInteractionOutlineProxyGeometrySpec,
	"Wacom.UI.Battle.EnemyScene.InteractionVisual.OutlineProxyPreservesSilhouetteAndAtlasRect",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyInteractionOutlineProxyGeometrySpec::RunTest(
	const FString& /*Parameters*/)
{
	const TArray<FVector4> BakedRenderData =
	{
		FVector4(-16.0, -16.0, 0.250, 0.750),
		FVector4(16.0, -16.0, 0.375, 0.750),
		FVector4(-16.0, 16.0, 0.250, 0.500),
	};
	FWacomBattleEnemyPartOutlineProxyFrame Frame;
	if (!TestTrue(TEXT("Proxy frame resolves from a Paper2D atlas triangle"),
		WacomBattleEnemyPartOutlineProxyGeometry::BuildFrame(
			BakedRenderData,
			FVector2D(0.250, 0.500),
			FVector2D(0.125, 0.250),
			FVector2D(256.0, 128.0),
			1.0f,
			2.0f,
			Frame)))
	{
		return false;
	}

	TestTrue(TEXT("Source center remains at the authored pivot"),
		Frame.LocalCenter.Equals(FVector2D::ZeroVector));
	TestTrue(TEXT("Source world size is recovered from baked UV mapping"),
		Frame.SourceSizeUnrealUnits.Equals(FVector2D(32.0, 32.0)));
	TestTrue(TEXT("Only the proxy canvas gains two source pixels per side"),
		Frame.PaddedSizeUnrealUnits.Equals(FVector2D(36.0, 36.0)));
	TestTrue(TEXT("Source pixel dimensions follow the local sprite axes"),
		Frame.SourcePixelSize.Equals(FVector2D(32.0, 32.0)));
	TestTrue(TEXT("Atlas origin maps the local lower-left corner"),
		Frame.AtlasUVOrigin.Equals(FVector2D(0.250, 0.750)));
	TestTrue(TEXT("Atlas X axis remains horizontal"),
		Frame.AtlasUVAxisX.Equals(FVector2D(0.125, 0.0)));
	TestTrue(TEXT("Atlas Y axis preserves Paper2D's inverted V"),
		Frame.AtlasUVAxisY.Equals(FVector2D(0.0, -0.250)));
	TestTrue(TEXT("Proxy-to-source remap accounts for transparent padding"),
		Frame.CanvasToSourceScale.Equals(FVector2D(1.125, 1.125)));

	const FVector2D SourceLeftProxyUV(
		0.5 - 0.5 / Frame.CanvasToSourceScale.X,
		0.5);
	TestTrue(TEXT("Expanded proxy maps its inset edge back to source edge"),
		WacomBattleEnemyPartOutlineProxyGeometry::MapProxyUVToSourceLocal(
			Frame, SourceLeftProxyUV).Equals(FVector2D(0.0, 0.5)));
	const float ReconstructedSourceLeft =
		(SourceLeftProxyUV.X - 0.5) * Frame.PaddedSizeUnrealUnits.X
		+ Frame.LocalCenter.X;
	TestTrue(TEXT("Source silhouette keeps its original world-space left edge"),
		FMath::IsNearlyEqual(ReconstructedSourceLeft, -16.0f));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomBattleEnemyInteractionOutlineStateSpec,
	"Wacom.UI.Battle.EnemyScene.InteractionVisual.OutlineStateReuseAndCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomBattleEnemyInteractionOutlineStateSpec::RunTest(const FString& /*Parameters*/)
{
	using namespace WacomBattleEnemyInteractionVisualSpec;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	FFixture Fixture;
	if (!TestTrue(TEXT("Fixture binds"), Fixture.Initialize(*World)))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Fixture.Host))
		{
			Fixture.Host->Destroy();
		}
	};

	UWacomBattleEnemyPartTargetPreviewStyle* Style =
		NewObject<UWacomBattleEnemyPartTargetPreviewStyle>(
			Fixture.Host, NAME_None, RF_Transient);
	Style->OutlineMaterial = NewObject<UMaterial>(Fixture.Host, NAME_None, RF_Transient);
	Fixture.Host->DefaultTargetPreviewStyle = Style;
	FWacomEnemySceneRuntimeAutomationTestView::SetRegisteredAndTargetable(
		*Fixture.Host, *Fixture.Part, true, false);
	TestTrue(TEXT("Target-selection sync succeeds"),
		FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
			*Fixture.Host, *Fixture.Part, Fixture.Snapshot, true, true));
	FWacomBattleEnemyPartRuntimeDebugView Debug = Fixture.Part->GetRuntimeDebugView();
	TestEqual(TEXT("Legal target receives low outline"),
		Debug.OutlineState, FName(TEXT("Selectable")));
	TestTrue(TEXT("Outline proxy is created lazily"), Debug.bOutlineComponentCreated);
	TestEqual(TEXT("Exactly one proxy was created"), Debug.OutlineComponentCreateCount, 1);

	FWacomEnemySceneRuntimeAutomationTestView::SetHoverPrediction(
		*Fixture.Host, *Fixture.Part);
	Debug = Fixture.Part->GetRuntimeDebugView();
	TestEqual(TEXT("Legal hover receives high outline"),
		Debug.OutlineState, FName(TEXT("Hovered")));
	TestEqual(TEXT("Hover reuses proxy"), Debug.OutlineComponentCreateCount, 1);

	FWacomEnemySceneRuntimeAutomationTestView::SetDragTargetPreview(
		*Fixture.Host,
		*Fixture.Part,
		EWacomFirstPersonCardDragTargetFeedbackState::Invalid);
	Debug = Fixture.Part->GetRuntimeDebugView();
	TestEqual(TEXT("Invalid target suppresses outline"),
		Debug.OutlineState, FName(TEXT("None")));
	FWacomEnemySceneRuntimeAutomationTestView::ClearDragTargetPreview(
		*Fixture.Host, *Fixture.Part);
	TestEqual(TEXT("Clearing invalid preview restores legal hover"),
		Fixture.Part->GetRuntimeDebugView().OutlineState,
		FName(TEXT("Hovered")));

	FWacomEnemySceneRuntimeAutomationTestView::ClearHoverPrediction(
		*Fixture.Host, *Fixture.Part);
	TestEqual(TEXT("Clearing hover restores selectable outline"),
		Fixture.Part->GetRuntimeDebugView().OutlineState,
		FName(TEXT("Selectable")));
	Style->OutlineMaterial = nullptr;
	FWacomEnemySceneRuntimeAutomationTestView::SyncPart(
		*Fixture.Host, *Fixture.Part, Fixture.Snapshot, true, true);
	TestEqual(TEXT("Missing material safely disables outline"),
		Fixture.Part->GetRuntimeDebugView().OutlineState,
		FName(TEXT("None")));

	Fixture.Host->RetireRuntimeEncounterPresentation();
	Debug = Fixture.Part->GetRuntimeDebugView();
	TestEqual(TEXT("Retire clears outline state"),
		Debug.OutlineState, FName(TEXT("None")));
	TestFalse(TEXT("Retire destroys transient outline proxy"),
		Debug.bOutlineComponentCreated);
	return true;
}
