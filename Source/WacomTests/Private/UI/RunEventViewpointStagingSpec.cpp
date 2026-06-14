// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomFirstPersonViewpointActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Camera/WacomFirstPersonViewStageRequest.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

namespace WacomRunEventViewpointStagingSpec
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
	FWacomUIRunEventViewpointTriggerStageRequestSpec,
	"Wacom.UI.Event.ViewpointStaging.TriggerBuildsOptionalStageRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventViewpointTriggerStageRequestSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomRunEventViewpointStagingSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunEventTriggerActor* EventTrigger = World->SpawnActor<AWacomRunEventTriggerActor>();
	AWacomFirstPersonViewpointActor* Viewpoint =
		World->SpawnActor<AWacomFirstPersonViewpointActor>();
	if (!TestNotNull(TEXT("RunEvent trigger"), EventTrigger)
		|| !TestNotNull(TEXT("Viewpoint"), Viewpoint))
	{
		if (Viewpoint)
		{
			Viewpoint->Destroy();
		}
		if (EventTrigger)
		{
			EventTrigger->Destroy();
		}
		return false;
	}

	FWacomFirstPersonViewStageRequest StageRequest;
	TestFalse(TEXT("Unconfigured RunEvent trigger has no entry viewpoint"),
		EventTrigger->TryBuildRunEventEntryViewStageRequest(StageRequest));
	TestFalse(TEXT("Unconfigured request has no view transform"),
		StageRequest.bHasViewTransform);

	const FVector ViewLocation(-130.0f, 95.0f, 144.0f);
	const FRotator ViewRotation(12.0f, -35.0f, 0.0f);
	Viewpoint->SetActorLocationAndRotation(ViewLocation, ViewRotation);
	Viewpoint->StageBlendTimeSeconds = 0.6f;
	Viewpoint->StageBlendCurve = EWacomFirstPersonViewStageBlendCurve::EaseOut;
	Viewpoint->StageBlendEasePower = 3.5f;
	EventTrigger->PersistentId = TEXT("RunEvent.Stage.DebugSource");
	EventTrigger->RunEventEntryViewpoint = Viewpoint;

	TestTrue(TEXT("Configured RunEvent trigger builds entry stage request"),
		EventTrigger->TryBuildRunEventEntryViewStageRequest(StageRequest));
	TestTrue(TEXT("Stage request has view transform"),
		StageRequest.bHasViewTransform);
	TestEqual(TEXT("Stage request reason is RunEvent entry"),
		StageRequest.Reason,
		FName(TEXT("RunEventEntry")));
	TestEqual(TEXT("Stage request debug source prefers PersistentId"),
		StageRequest.DebugSource,
		FName(TEXT("RunEvent.Stage.DebugSource")));
	TestEqual(TEXT("Stage request view transform matches viewpoint"),
		StageRequest.ViewTransform,
		Viewpoint->GetActorTransform());
	TestEqual(TEXT("Stage request copies viewpoint blend time"),
		StageRequest.BlendTimeSeconds,
		0.6f);
	TestEqual(TEXT("Stage request copies viewpoint blend curve"),
		StageRequest.BlendCurve,
		EWacomFirstPersonViewStageBlendCurve::EaseOut);
	TestEqual(TEXT("Stage request copies viewpoint blend ease power"),
		StageRequest.BlendEasePower,
		3.5f);

	EventTrigger->PersistentId = NAME_None;
	TestTrue(TEXT("Configured RunEvent trigger rebuilds entry stage request"),
		EventTrigger->TryBuildRunEventEntryViewStageRequest(StageRequest));
	TestEqual(TEXT("Stage request debug source falls back to actor name"),
		StageRequest.DebugSource,
		FName(*EventTrigger->GetName()));

	Viewpoint->Destroy();
	EventTrigger->Destroy();
	return true;
}
