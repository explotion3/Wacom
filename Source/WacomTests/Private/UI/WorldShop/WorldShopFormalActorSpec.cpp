// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Actors/WacomWorldShopActor.h"
#include "Actors/WacomWorldShopHostActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Components/BoxComponent.h"
#include "Components/ChildActorComponent.h"
#include "Components/SceneComponent.h"
#include "Components/SphereComponent.h"
#include "Components/WacomRunMapNodeBindingComponent.h"
#include "Engine/World.h"
#include "Map/WacomMapTypes.h"

namespace WacomWorldShopFormalActorSpec
{
	struct FTransientWorldFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

		~FTransientWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}

		template <typename TActor>
		TActor* Spawn() const
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			return World
				? World->SpawnActor<TActor>(
					TActor::StaticClass(),
					FTransform::Identity,
					Params)
				: nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopFormalActorCompositeContractSpec,
	"Wacom.UI.WorldShop.FormalActor.CompositeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopFormalActorCompositeContractSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopFormalActorSpec;

	FTransientWorldFixture Fixture;
	AWacomWorldShopActor* Shop = Fixture.Spawn<AWacomWorldShopActor>();
	if (!TestNotNull(TEXT("Formal shop spawns"), Shop))
	{
		return false;
	}

	TestTrue(TEXT("Formal shop remains a legacy-compatible trigger"),
		Shop->IsA<AWacomShopTriggerActor>());
	TestEqual(TEXT("Default trigger radius"), Shop->TriggerRadius, 350.0f);
	TestEqual(TEXT("Trigger sphere follows authored radius"),
		Shop->GetTriggerSphere()->GetUnscaledSphereRadius(), 350.0f);

	USceneComponent* PresentationRoot = Shop->GetPresentationRootComponent();
	UChildActorComponent* HostComponent = Shop->GetWorldShopHostComponent();
	UChildActorComponent* ViewpointComponent =
		Shop->GetShopEntryViewpointComponent();
	TestNotNull(TEXT("Presentation root exists"), PresentationRoot);
	TestNotNull(TEXT("Internal host component exists"), HostComponent);
	TestNotNull(TEXT("Internal viewpoint component exists"), ViewpointComponent);
	TestTrue(TEXT("Host component is parented to presentation root"),
		HostComponent && HostComponent->GetAttachParent() == PresentationRoot);
	TestTrue(TEXT("Click bounds follow presentation root"),
		Shop->GetClickBounds()->GetAttachParent() == PresentationRoot);
	TestEqual(TEXT("Click bounds depth extent"),
		Shop->GetClickBounds()->GetUnscaledBoxExtent().X, 60.0);
	TestEqual(TEXT("Click bounds width extent"),
		Shop->GetClickBounds()->GetUnscaledBoxExtent().Y, 210.0);
	TestEqual(TEXT("Click bounds height extent"),
		Shop->GetClickBounds()->GetUnscaledBoxExtent().Z, 140.0);

	TestTrue(TEXT("Host child class is exact native host"),
		HostComponent
			&& HostComponent->GetChildActorClass()
				== AWacomWorldShopHostActor::StaticClass());
	TestTrue(TEXT("Viewpoint child class is exact native viewpoint"),
		ViewpointComponent
			&& ViewpointComponent->GetChildActorClass()
				== AWacomFirstPersonViewpointActor::StaticClass());

	AWacomWorldShopHostActor* InternalHost =
		Shop->GetInternalWorldShopHost();
	AWacomFirstPersonViewpointActor* InternalViewpoint =
		Shop->GetInternalShopEntryViewpoint();
	if (!TestNotNull(TEXT("Internal host is created"), InternalHost)
		|| !TestNotNull(TEXT("Internal viewpoint is created"), InternalViewpoint))
	{
		return false;
	}
	TestTrue(TEXT("Internal host belongs to fixture world"),
		InternalHost->GetWorld() == Fixture.World);
	TestEqual(TEXT("Internal host exposes eight offer anchors"),
		InternalHost->GetEnabledOfferAnchorsSorted().Num(), 8);
	TestTrue(TEXT("Internal host default profile validates for eight offers"),
		InternalHost->ValidateForOfferCount(8).bValid);
	TestEqual(TEXT("Viewpoint default blend time"),
		InternalViewpoint->StageBlendTimeSeconds, 0.25f);
	TestEqual(TEXT("Viewpoint default blend curve"),
		InternalViewpoint->StageBlendCurve,
		EWacomFirstPersonViewStageBlendCurve::SmoothStep);
	TestTrue(TEXT("Viewpoint is 320 cm in front of presentation"),
		ViewpointComponent->GetRelativeLocation().Equals(
			FVector(320.0f, 0.0f, -20.0f)));
	TestTrue(TEXT("Viewpoint faces the presentation"),
		ViewpointComponent->GetRelativeRotation().Equals(
			FRotator(0.0f, 180.0f, 0.0f)));

	UWacomRunMapNodeBindingComponent* Binding =
		Shop->GetRunMapNodeBindingComponent();
	TestNotNull(TEXT("Run map binding exists"), Binding);
	TestTrue(TEXT("Run map binding defaults to Shop"),
		Binding && Binding->NodeType == EWacomMapNodeType::Shop);
	TestTrue(TEXT("Run map binding leaves per-instance NodeId unset"),
		Binding && Binding->NodeId.IsNone());

	AWacomWorldShopHostActor* ExternalHost =
		Fixture.Spawn<AWacomWorldShopHostActor>();
	AWacomFirstPersonViewpointActor* ExternalViewpoint =
		Fixture.Spawn<AWacomFirstPersonViewpointActor>();
	Shop->WorldShopHost = ExternalHost;
	Shop->ShopEntryViewpoint = ExternalViewpoint;
	Shop->PersistentId = TEXT("Shop.Formal.CompositeTest");

	FWacomFirstPersonViewStageRequest FormalStageRequest;
	TestTrue(TEXT("Formal shop resolves its internal viewpoint"),
		Shop->TryBuildShopEntryViewStageRequest(FormalStageRequest));
	TestTrue(TEXT("Formal stage transform ignores legacy external viewpoint"),
		FormalStageRequest.ViewTransform.Equals(
			InternalViewpoint->GetActorTransform()));
	TestEqual(TEXT("Formal debug resolves its internal host"),
		Shop->GetShopTriggerDebugView(nullptr).WorldShopHostName,
		InternalHost->GetName());

	AWacomShopTriggerActor* LegacyShop =
		Fixture.Spawn<AWacomShopTriggerActor>();
	LegacyShop->PersistentId = TEXT("Shop.Legacy.CompositeTest");
	LegacyShop->WorldShopHost = ExternalHost;
	LegacyShop->ShopEntryViewpoint = ExternalViewpoint;
	FWacomFirstPersonViewStageRequest LegacyStageRequest;
	TestTrue(TEXT("Legacy trigger still resolves external viewpoint"),
		LegacyShop->TryBuildShopEntryViewStageRequest(LegacyStageRequest));
	TestTrue(TEXT("Legacy stage transform remains external"),
		LegacyStageRequest.ViewTransform.Equals(
			ExternalViewpoint->GetActorTransform()));
	TestEqual(TEXT("Legacy debug still resolves external host"),
		LegacyShop->GetShopTriggerDebugView(nullptr).WorldShopHostName,
		ExternalHost->GetName());
	return true;
}

#endif
