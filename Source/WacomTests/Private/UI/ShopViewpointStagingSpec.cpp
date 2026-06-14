// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace WacomShopViewpointStagingSpec
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
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopViewpointTriggerStageRequestSpec,
	"Wacom.UI.Shop.ViewpointStaging.TriggerBuildsOptionalStageRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopViewpointTriggerStageRequestSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomShopViewpointStagingSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomShopTriggerActor* Shop = World->SpawnActor<AWacomShopTriggerActor>();
	AWacomFirstPersonViewpointActor* Viewpoint =
		World->SpawnActor<AWacomFirstPersonViewpointActor>();
	if (!TestNotNull(TEXT("Shop trigger"), Shop)
		|| !TestNotNull(TEXT("Viewpoint"), Viewpoint))
	{
		if (Viewpoint)
		{
			Viewpoint->Destroy();
		}
		if (Shop)
		{
			Shop->Destroy();
		}
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	TestFalse(TEXT("Unconfigured shop trigger has no entry viewpoint"),
		Shop->TryBuildShopEntryViewStageRequest(StageRequest));
	TestFalse(TEXT("Unconfigured request has no view transform"),
		StageRequest.bHasViewTransform);

	const FVector ViewLocation(210.0f, -75.0f, 156.0f);
	const FRotator ViewRotation(8.0f, 42.0f, 0.0f);
	Viewpoint->SetActorLocationAndRotation(ViewLocation, ViewRotation);
	Viewpoint->StageBlendTimeSeconds = 0.45f;
	Viewpoint->StageBlendCurve = EWacomFirstPersonViewStageBlendCurve::EaseInOut;
	Viewpoint->StageBlendEasePower = 2.75f;
	Shop->PersistentId = TEXT("Shop.Stage.DebugSource");
	Shop->ShopEntryViewpoint = Viewpoint;

	TestTrue(TEXT("Configured shop trigger builds entry stage request"),
		Shop->TryBuildShopEntryViewStageRequest(StageRequest));
	TestTrue(TEXT("Stage request has view transform"),
		StageRequest.bHasViewTransform);
	TestEqual(TEXT("Stage request reason is shop entry"),
		StageRequest.Reason,
		FName(TEXT("ShopEntry")));
	TestEqual(TEXT("Stage request debug source prefers PersistentId"),
		StageRequest.DebugSource,
		FName(TEXT("Shop.Stage.DebugSource")));
	TestEqual(TEXT("Stage request view transform matches viewpoint"),
		StageRequest.ViewTransform,
		Viewpoint->GetActorTransform());
	TestEqual(TEXT("Stage request copies viewpoint blend time"),
		StageRequest.BlendTimeSeconds,
		0.45f);
	TestEqual(TEXT("Stage request copies viewpoint blend curve"),
		StageRequest.BlendCurve,
		EWacomFirstPersonViewStageBlendCurve::EaseInOut);
	TestEqual(TEXT("Stage request copies viewpoint blend ease power"),
		StageRequest.BlendEasePower,
		2.75f);

	Shop->PersistentId = NAME_None;
	TestTrue(TEXT("Configured shop trigger rebuilds entry stage request"),
		Shop->TryBuildShopEntryViewStageRequest(StageRequest));
	TestEqual(TEXT("Stage request debug source falls back to actor name"),
		StageRequest.DebugSource,
		FName(*Shop->GetName()));

	Viewpoint->Destroy();
	Shop->Destroy();
	return true;
}
