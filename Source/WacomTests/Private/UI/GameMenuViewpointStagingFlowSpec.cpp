// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunTunnelSegmentActor.h"
#include "Camera/WacomFirstPersonViewStageCoordinator.h"
#include "Camera/WacomFirstPersonViewpointPlacement.h"
#include "Characters/CharacterDefinition.h"
#include "Components/SplineComponent.h"
#include "Components/WacomFirstPersonViewStageBlendComponent.h"
#include "Components/WacomRunTunnelMovementComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/RunEventDefinition.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Foundation/WacomGameUIManagerSubsystem.h"
#include "UI/Foundation/WacomMenuWidgetBase.h"
#include "UI/Foundation/WacomUIDeveloperSettings.h"
#include "UI/Foundation/WacomUITags.h"
#include "UI/PlayerControllerRunInteractionTestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UI/WacomUISettingsTestProbes.h"
#include "UI/WacomUITestAccess.h"
#include "UObject/StrongObjectPtr.h"

struct FWacomGameMenuViewpointStageReturnFlowTestAccess
{
	static void Tick(UWacomFirstPersonViewStageBlendComponent& Component, float DeltaTime)
	{
		Component.TickComponent(DeltaTime, LEVELTICK_All, nullptr);
	}
};

namespace WacomGameMenuViewpointStagingFlowSpec
{
	class FScopedUISettingsOverride
	{
	public:
		FScopedUISettingsOverride()
			: Settings(GetMutableDefault<UWacomUIDeveloperSettings>())
			, SavedPrimaryLayoutClass(Settings->PrimaryLayoutClass)
			, SavedWidgetClasses(Settings->WidgetClasses)
			, SavedAppToastWidgetClass(Settings->AppToastWidgetClass)
		{
			Settings->PrimaryLayoutClass.Reset();
			Settings->WidgetClasses.Reset();
			Settings->AppToastWidgetClass.Reset();
		}

		~FScopedUISettingsOverride()
		{
			Settings->PrimaryLayoutClass = SavedPrimaryLayoutClass;
			Settings->WidgetClasses = SavedWidgetClasses;
			Settings->AppToastWidgetClass = SavedAppToastWidgetClass;
		}

	private:
		UWacomUIDeveloperSettings* Settings = nullptr;
		TSoftClassPtr<UWacomPrimaryGameLayout> SavedPrimaryLayoutClass;
		TArray<FWacomUIWidgetClassEntry> SavedWidgetClasses;
		TSoftClassPtr<UWacomAppToastWidget> SavedAppToastWidgetClass;
	};

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

	UWacomRunEventDefinition* MakeRunEvent(UObject* Outer)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.GameMenu.Viewpoint");
		Event->DisplayName = FText::FromString(TEXT("Viewpoint Event"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Close;
		Close.ChoiceId = TEXT("Close");
		Close.LabelText = FText::FromString(TEXT("Close"));
		Close.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.TitleText = FText::FromString(TEXT("Viewpoint Event"));
		Start.BodyText = FText::FromString(TEXT("Body"));
		Start.Choices = { Close };
		Event->Nodes = { Start };
		return Event;
	}

	UCharacterDefinition* MakeRunCharacter(UObject* Outer)
	{
		UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
		Character->CharacterId = TEXT("Character.GameMenu.Viewpoint");
		Character->DisplayName = FText::FromString(TEXT("Viewpoint Character"));
		Character->FingerCount = 3;
		Character->HpPerFinger = 10;
		return Character;
	}

	FWacomFirstPersonViewStageRequest MakeStageRequest(float BlendTimeSeconds = 1.0f)
	{
		FWacomFirstPersonViewStageRequest Request;
		Request.bHasViewTransform = true;
		Request.ViewTransform = FTransform(
			FRotator(8.0f, 72.0f, 0.0f),
			FVector(650.0f, 180.0f, 210.0f));
		Request.Reason = TEXT("RunEventEntry");
		Request.DebugSource = TEXT("Event.GameMenu.Viewpoint");
		Request.BlendTimeSeconds = BlendTimeSeconds;
		return Request;
	}

	AWacomRunTunnelSegmentActor* SpawnTestSegment(
		UWorld& World,
		const FVector& Start,
		const FVector& End)
	{
		AWacomRunTunnelSegmentActor* Segment =
			World.SpawnActor<AWacomRunTunnelSegmentActor>(
				AWacomRunTunnelSegmentActor::StaticClass(),
				FTransform::Identity);
		if (!Segment || !Segment->GetPathSpline())
		{
			return Segment;
		}

		USplineComponent* Spline = Segment->GetPathSpline();
		Spline->ClearSplinePoints(false);
		Spline->AddSplinePoint(Start, ESplineCoordinateSpace::World, false);
		Spline->AddSplinePoint(End, ESplineCoordinateSpace::World, false);
		Spline->SetSplinePointType(0, ESplinePointType::Linear, false);
		Spline->SetSplinePointType(1, ESplinePointType::Linear, false);
		Spline->UpdateSpline();
		return Segment;
	}

	void PushRunEventScreenLikeRouter(
		AWacomPlayerController& PC,
		UWacomUISettingsGameUIManagerProbe& UIManager,
		URunSession& Run,
		FName PersistentId,
		UWacomRunEventDefinition& Event,
		bool bReturnToRunTunnelAfterClose,
		int32& BeginRunEventCount,
		FWacomAsyncWidgetPushResult& OutResult)
	{
		FWacomAsyncWidgetPushRequest Request;
		Request.LayerTag = WacomUITags::UI_Layer_GameMenu.GetTag();
		Request.WidgetTag = WacomUITags::UI_Widget_RunEventScreen.GetTag();
		Request.FallbackClass = UWacomRunEventScreenProbe::StaticClass();
		Request.bLogMissingEntry = false;
		Request.BeforePush = [&Run, PersistentId, &Event, &BeginRunEventCount](FName& OutFailureReason)
		{
			++BeginRunEventCount;
			if (!Run.BeginRunEvent(PersistentId, &Event))
			{
				OutFailureReason = TEXT("BeginRunEventFailed");
				return false;
			}
			return true;
		};
		Request.Rollback = [&Run](FName /*FailureReason*/)
		{
			Run.EndRunEvent();
		};
		Request.OnComplete = [
			&PC,
			bReturnToRunTunnelAfterClose,
			&OutResult](const FWacomAsyncWidgetPushResult& Result)
		{
			OutResult = Result;
			if (!bReturnToRunTunnelAfterClose)
			{
				if (!Result.bSucceeded)
				{
					PC.SetRunFirstPersonCardLayerTransitionSuppressedByGameMenu(false);
				}
				return;
			}

			if (Result.bSucceeded)
			{
				PC.ArmGameMenuViewpointReturnForMenu(
					Cast<UWacomMenuWidgetBase>(Result.PushedWidget));
			}
			else
			{
				PC.ReturnFromGameMenuViewpointStageAfterFailedOpen();
			}
		};
		UIManager.PushRegisteredWidgetToLayerAsync(MoveTemp(Request));
	}

	bool StartStagedRunEventOpenLikeRouter(
		AWacomPlayerController& PC,
		AWacomPlayerCharacter& Character,
		UWacomUISettingsGameUIManagerProbe& UIManager,
		URunSession& Run,
		FName PersistentId,
		UWacomRunEventDefinition& Event,
		const FWacomFirstPersonViewStageRequest& StageRequest,
		int32& BeginRunEventCount,
		FWacomAsyncWidgetPushResult& OutResult)
	{
		PC.BeginGameMenuViewpointStageTransition(FName(TEXT("RunEventEntry")));

		auto OpenAfterStage =
			[&PC, &UIManager, &Run, PersistentId, &Event, &BeginRunEventCount, &OutResult]()
			{
				PushRunEventScreenLikeRouter(
					PC,
					UIManager,
					Run,
					PersistentId,
					Event,
					/*bReturnToRunTunnelAfterClose*/true,
					BeginRunEventCount,
					OutResult);
			};

		TFunction<void()> DeferredOpenAfterStage = OpenAfterStage;
		const bool bDeferred =
			FWacomFirstPersonViewStageCoordinator::StageFirstPersonView(
				Character,
				PC,
				StageRequest,
				MoveTemp(DeferredOpenAfterStage));
		if (!bDeferred)
		{
			OpenAfterStage();
		}
		return bDeferred;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuViewpointStagingDefersRunEventOpenSpec,
	"Wacom.UI.GameMenu.ViewpointStaging.DefersRunEventOpenUntilStageCompletes",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuViewpointStagingDefersRunEventOpenSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomGameMenuViewpointStagingFlowSpec;

	FScopedUISettingsOverride SettingsOverride;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerControllerProbe* PC = World->SpawnActor<AWacomPlayerControllerProbe>();
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character))
	{
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	PC->Possess(Character);
	PC->SetPawn(Character);
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	if (!TestNotNull(TEXT("Stage blend"), StageBlend))
	{
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(
		NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(
		NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, Layout.Get());

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UCharacterDefinition> RunCharacter(MakeRunCharacter(Run.Get()));
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeRunEvent(Run.Get()));
	TestTrue(TEXT("Run initializes"), Run->Initialize(RunCharacter.Get()));
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC, Run.Get());

	const FName PersistentId(TEXT("Event.GameMenu.Viewpoint.Deferred"));
	int32 BeginRunEventCount = 0;
	FWacomAsyncWidgetPushResult PushResult;
	TestTrue(TEXT("Staged RunEvent open defers while entry view blends"),
		StartStagedRunEventOpenLikeRouter(
			*PC,
			*Character,
			*UIManager,
			*Run,
			PersistentId,
			*Event,
			MakeStageRequest(/*BlendTimeSeconds*/1.0f),
			BeginRunEventCount,
			PushResult));

	TestTrue(TEXT("Stage transition is active before blend completes"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestFalse(TEXT("Return is not armed before menu push"),
		PC->IsGameMenuViewpointReturnArmed());
	TestFalse(TEXT("RunEvent has not begun before blend completes"),
		Run->IsRunEventActive());
	TestEqual(TEXT("BeforePush has not run before blend completes"),
		BeginRunEventCount,
		0);

	FWacomGameMenuViewpointStageReturnFlowTestAccess::Tick(*StageBlend, 0.5f);
	TestTrue(TEXT("Blend remains active at half time"),
		StageBlend->IsStageBlendActive());
	TestFalse(TEXT("RunEvent still has not begun at half time"),
		Run->IsRunEventActive());
	TestEqual(TEXT("BeforePush still has not run at half time"),
		BeginRunEventCount,
		0);

	FWacomGameMenuViewpointStageReturnFlowTestAccess::Tick(*StageBlend, 0.5f);
	TestFalse(TEXT("Entry blend completes"), StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Async push succeeds after stage completion"),
		PushResult.bSucceeded);
	TestEqual(TEXT("BeforePush runs once after stage completion"),
		BeginRunEventCount,
		1);
	TestTrue(TEXT("RunEvent begins after stage completion"),
		Run->IsRunEventActive());
	TestEqual(TEXT("Active RunEvent id matches request"),
		Run->BuildCurrentRunEventSnapshot().PersistentId,
		PersistentId);
	TestFalse(TEXT("Stage transition clears after successful push"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestTrue(TEXT("Successful staged menu arms return"),
		PC->IsGameMenuViewpointReturnArmed());

	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuViewpointStagingMenuCloseReturnsToRunTunnelSpec,
	"Wacom.UI.GameMenu.ViewpointStaging.MenuCloseReturnsToRunTunnel",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuViewpointStagingMenuCloseReturnsToRunTunnelSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomGameMenuViewpointStagingFlowSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerControllerProbe* PC = World->SpawnActor<AWacomPlayerControllerProbe>();
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment =
		SpawnTestSegment(*World, FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f));
	TStrongObjectPtr<UWacomRunEventScreenProbe> Menu(NewObject<UWacomRunEventScreenProbe>());
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("RunTunnel segment"), Segment)
		|| !TestNotNull(TEXT("Menu"), Menu.Get()))
	{
		if (Segment)
		{
			Segment->Destroy();
		}
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	PC->Possess(Character);
	PC->SetPawn(Character);
	UWacomRunTunnelMovementComponent* Tunnel =
		Character->GetRunTunnelMovementComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	if (!TestNotNull(TEXT("Run tunnel"), Tunnel)
		|| !TestNotNull(TEXT("Stage blend"), StageBlend))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Tunnel->ReturnStageBlendTimeSeconds = 1.0f;
	TestTrue(TEXT("Run tunnel activates"),
		Tunnel->ActivateRunTunnel(Segment, 300.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run tunnel is suspended before menu return"),
		Tunnel->IsRunTunnelSuspended());
	TestTrue(TEXT("Temporary menu viewpoint applies"),
		WacomFirstPersonViewpointPlacement::ApplyViewTransform(
			*Character,
			*PC,
			FTransform(
				FRotator(10.0f, 90.0f, 0.0f),
				FVector(720.0f, 220.0f, 200.0f))));

	PC->RegisterActiveGameMenuWidget(Menu.Get());
	PC->ArmGameMenuViewpointReturnForMenu(Menu.Get());
	TestTrue(TEXT("Return is armed for staged menu"),
		PC->IsGameMenuViewpointReturnArmed());

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	TestTrue(TEXT("Closing staged menu starts return transition"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestFalse(TEXT("Return arm is consumed when return starts"),
		PC->IsGameMenuViewpointReturnArmed());
	TestTrue(TEXT("Return blend starts"),
		StageBlend->IsStageBlendActive());
	TestTrue(TEXT("Run tunnel remains suspended during return"),
		Tunnel->IsRunTunnelSuspended());

	FWacomGameMenuViewpointStageReturnFlowTestAccess::Tick(*StageBlend, 1.0f);
	TestFalse(TEXT("Return blend completes"),
		StageBlend->IsStageBlendActive());
	TestFalse(TEXT("Stage transition clears after return"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestFalse(TEXT("Run tunnel resumes after return"),
		Tunnel->IsRunTunnelSuspended());

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuViewpointStagingFailedPushRollsBackAndReturnsSpec,
	"Wacom.UI.GameMenu.ViewpointStaging.FailedPushRollsBackAndReturns",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuViewpointStagingFailedPushRollsBackAndReturnsSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomGameMenuViewpointStagingFlowSpec;

	FScopedUISettingsOverride SettingsOverride;
	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomPlayerControllerProbe* PC = World->SpawnActor<AWacomPlayerControllerProbe>();
	AWacomPlayerCharacter* Character =
		World->SpawnActor<AWacomPlayerCharacter>(AWacomPlayerCharacter::StaticClass(), FTransform::Identity);
	AWacomRunTunnelSegmentActor* Segment =
		SpawnTestSegment(*World, FVector::ZeroVector, FVector(1000.0f, 0.0f, 0.0f));
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Character"), Character)
		|| !TestNotNull(TEXT("RunTunnel segment"), Segment))
	{
		if (Segment)
		{
			Segment->Destroy();
		}
		if (Character)
		{
			Character->Destroy();
		}
		if (PC)
		{
			PC->Destroy();
		}
		return false;
	}

	PC->Possess(Character);
	PC->SetPawn(Character);
	UWacomRunTunnelMovementComponent* Tunnel =
		Character->GetRunTunnelMovementComponent();
	UWacomFirstPersonViewStageBlendComponent* StageBlend =
		Character->GetFirstPersonViewStageBlendComponent();
	if (!TestNotNull(TEXT("Run tunnel"), Tunnel)
		|| !TestNotNull(TEXT("Stage blend"), StageBlend))
	{
		Segment->Destroy();
		Character->Destroy();
		PC->Destroy();
		return false;
	}

	Tunnel->ReturnStageBlendTimeSeconds = 1.0f;
	TestTrue(TEXT("Run tunnel activates"),
		Tunnel->ActivateRunTunnel(Segment, 250.0f));
	Character->SetExplorationInputEnabled(false);
	TestTrue(TEXT("Run tunnel is suspended before staged open"),
		Tunnel->IsRunTunnelSuspended());

	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomUISettingsGameUIManagerProbe> UIManager(
		NewObject<UWacomUISettingsGameUIManagerProbe>(GameInstance.Get()));
	TStrongObjectPtr<UWacomUISettingsPrimaryLayoutProbe> Layout(
		NewObject<UWacomUISettingsPrimaryLayoutProbe>(GameInstance.Get()));
	FWacomUITestAccess::SetPrimaryLayout(*UIManager, Layout.Get());
	UIManager->bFailNextPush = true;

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UCharacterDefinition> RunCharacter(MakeRunCharacter(Run.Get()));
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeRunEvent(Run.Get()));
	TestTrue(TEXT("Run initializes"), Run->Initialize(RunCharacter.Get()));
	FWacomPlayerControllerRunInteractionTestAccess::SetRunSession(PC, Run.Get());

	int32 BeginRunEventCount = 0;
	FWacomAsyncWidgetPushResult PushResult;
	TestTrue(TEXT("Failed staged RunEvent open still defers entry stage"),
		StartStagedRunEventOpenLikeRouter(
			*PC,
			*Character,
			*UIManager,
			*Run,
			TEXT("Event.GameMenu.Viewpoint.FailedPush"),
			*Event,
			MakeStageRequest(/*BlendTimeSeconds*/1.0f),
			BeginRunEventCount,
			PushResult));

	FWacomGameMenuViewpointStageReturnFlowTestAccess::Tick(*StageBlend, 1.0f);
	TestFalse(TEXT("Push fails after entry stage"),
		PushResult.bSucceeded);
	TestEqual(TEXT("Failure reason comes from UI push"),
		PushResult.FailureReason,
		FName(TEXT("PushFailed")));
	TestEqual(TEXT("BeginRunEvent ran once before failed push rollback"),
		BeginRunEventCount,
		1);
	TestFalse(TEXT("Failed push rollback ends RunEvent"),
		Run->IsRunEventActive());
	TestTrue(TEXT("Failed staged open starts return transition"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestFalse(TEXT("Failed staged open does not arm menu return"),
		PC->IsGameMenuViewpointReturnArmed());
	TestTrue(TEXT("Return blend is active after failed push"),
		StageBlend->IsStageBlendActive());

	FWacomGameMenuViewpointStageReturnFlowTestAccess::Tick(*StageBlend, 1.0f);
	TestFalse(TEXT("Return blend completes after failed push"),
		StageBlend->IsStageBlendActive());
	TestFalse(TEXT("Stage transition clears after failed push return"),
		PC->IsGameMenuViewpointStageTransitionActive());
	TestFalse(TEXT("Run tunnel resumes after failed push return"),
		Tunnel->IsRunTunnelSuspended());

	Segment->Destroy();
	Character->Destroy();
	PC->Destroy();
	return true;
}
