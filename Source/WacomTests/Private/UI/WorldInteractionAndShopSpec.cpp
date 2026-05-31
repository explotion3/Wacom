// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Cards/CardDefinition.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomWorldInteractable.h"
#include "RunSession.h"
#include "RunState.h"
#include "Shops/ShopDefinition.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Events/WacomRunEventPresentationBuilder.h"
#include "UI/Events/WacomRunEventChoiceButton.h"
#include "UI/Events/WacomRunEventScreen.h"
#include "UI/Foundation/WacomAppToastSubsystem.h"
#include "UI/Foundation/WacomAppToastWidget.h"
#include "UI/Shop/WacomShopOfferRowWidget.h"
#include "UI/Shop/WacomShopPresentationBuilder.h"
#include "UI/Shop/WacomShopScreen.h"
#include "UI/WacomUITestAccess.h"
#include "UI/WacomShopRunEventTestProbes.h"

#include "Engine/GameInstance.h"
#include "Engine/Engine.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
	UWorld* FindWorldInteractionAutomationWorld()
	{
		if (!GEngine)
		{
			return nullptr;
		}

		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.WorldType == EWorldType::Game
				|| Context.WorldType == EWorldType::PIE
				|| Context.WorldType == EWorldType::Editor)
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	void InjectRunSession(AWacomPlayerController* PC, URunSession* Run)
	{
		FObjectProperty* RunSessionProperty = FindFProperty<FObjectProperty>(PC->GetClass(), TEXT("RunSession"));
		if (RunSessionProperty)
		{
			RunSessionProperty->SetObjectPropertyValue_InContainer(PC, Run);
		}
	}

	UWacomRunEventDefinition* MakeUiRunEvent(UObject* Outer)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.UI");
		Event->DisplayName = FText::FromString(TEXT("UI事件"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Close;
		Close.ChoiceId = TEXT("Close");
		Close.LabelText = FText::FromString(TEXT("关闭事件"));
		Close.bCloseEventAfterResolve = true;
		Close.bMarkEventCompleted = true;

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.TitleText = FText::FromString(TEXT("UI标题"));
		Start.BodyText = FText::FromString(TEXT("UI正文"));
		Start.Choices = { Close };
		Event->Nodes = { Start };
		return Event;
	}

	UWacomRunEventDefinition* MakeUiRunEventCardPaymentEvent(
		UObject* Outer,
		UCardDefinition* PaidCard)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.UI.Payment");
		Event->DisplayName = FText::FromString(TEXT("支付事件"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Pay;
		Pay.ChoiceId = TEXT("PayFang");
		Pay.LabelText = FText::FromString(TEXT("交出毒牙"));
		Pay.CardPayment.bRequiresOwnedCardPayment = true;
		Pay.CardPayment.PaymentZoneId = TEXT("RunEvent.Pay.Fang");
		Pay.CardPayment.AllowedCardDefinitions.Add(PaidCard);
		FWacomRunEventEffectDefinition Gold;
		Gold.Type = EWacomRunEventEffectType::AddGold;
		Gold.Value = 1;
		Pay.Effects.Add(Gold);
		Pay.bCloseEventAfterResolve = true;
		Pay.bMarkEventCompleted = true;

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices = { Pay };
		Event->Nodes = { Start };
		return Event;
	}

	FGuid FindUiStorageInstanceIdByDefinition(const FRunBackpackStorageSnapshot& Snapshot, const UCardDefinition* Card)
	{
		for (const FRunStorageCardView& View : Snapshot.Flux.ContentCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return View.Instance.InstanceId;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BattleDeckPhysicalCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return View.Instance.InstanceId;
			}
		}
		for (const FRunStorageCardView& View : Snapshot.BurdenCards)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				return View.Instance.InstanceId;
			}
		}
		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			if (Special.OwnerCard.Instance.Definition.Get() == Card)
			{
				return Special.OwnerCard.Instance.InstanceId;
			}
			for (const FRunStorageCardView& View : Special.ContentCards)
			{
				if (View.Instance.Definition.Get() == Card)
				{
					return View.Instance.InstanceId;
				}
			}
		}
		return FGuid();
	}

	bool UiStorageContainsDefinition(const FRunBackpackStorageSnapshot& Snapshot, const UCardDefinition* Card)
	{
		return FindUiStorageInstanceIdByDefinition(Snapshot, Card).IsValid();
	}

	AActor* SpawnTransientActor(UWorld& World)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		return World.SpawnActor<AActor>(AActor::StaticClass(), FTransform::Identity, SpawnParams);
	}

	UWacomInteractionTargetComponent* AddInteractionTargetComponent(AActor& Owner)
	{
		UWacomInteractionTargetComponent* Component = NewObject<UWacomInteractionTargetComponent>(&Owner);
		Owner.AddInstanceComponent(Component);
		Component->RegisterComponent();
		return Component;
	}

	UWacomRunWorldInteractionTargetBridgeComponent* AddRunWorldBridgeComponent(
		AActor& Owner,
		FName StableId = TEXT("Run.Target.Test"))
	{
		UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
			NewObject<UWacomRunWorldInteractionTargetBridgeComponent>(&Owner);
		Owner.AddInstanceComponent(Bridge);
		Bridge->RunTargetStableId = StableId;
		Bridge->RegisterComponent();
		return Bridge;
	}

	UStaticMeshComponent* AddVisualComponent(AActor& Owner, const FVector& Scale = FVector::OneVector)
	{
		UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(&Owner);
		Owner.AddInstanceComponent(Component);
		Component->SetRelativeScale3D(Scale);
		Component->RegisterComponent();
		return Component;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionClosestPromptSpec,
	"Wacom.UI.WorldInteraction.ClosestPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionClosestPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerActor> Near(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomShopTriggerActor> Far(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomShopTriggerActor> Disabled(NewObject<AWacomShopTriggerActor>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());

	Near->SetActorLocation(FVector(100.f, 0.f, 0.f));
	Near->PersistentId = TEXT("Shop.Near");
	Near->InteractPromptText = FText::FromString(TEXT("按 E 交易"));
	Far->SetActorLocation(FVector(300.f, 0.f, 0.f));
	Far->PersistentId = TEXT("Shop.Far");
	Far->InteractPromptText = FText::FromString(TEXT("按 E 战斗"));
	Disabled->SetActorLocation(FVector(10.f, 0.f, 0.f));
	Disabled->InteractPromptText = FText::FromString(TEXT("不可用"));

	PC->RegisterCandidateInteractable(Far.Get());
	PC->RegisterCandidateInteractable(Near.Get());
	PC->RegisterCandidateInteractable(Disabled.Get());

	TestTrue(TEXT("Closest available interactable wins"), PC->ReadClosestInteractable() == Near.Get());
	TestEqual(TEXT("Prompt comes from closest available interactable"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 交易")));

	PC->UnregisterCandidateInteractable(Near.Get());
	TestTrue(TEXT("After unregister, far candidate wins"), PC->ReadClosestInteractable() == Far.Get());
	TestEqual(TEXT("Disabled candidate still ignored"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 战斗")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionShopTriggerNativeContractSpec,
	"Wacom.UI.WorldInteraction.ShopTriggerNativeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionShopTriggerNativeContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());

	TestFalse(TEXT("Shop with None id is not interactable"),
		Shop->CanInteract_Implementation(PC.Get()));

	Shop->PersistentId = TEXT("Shop.Test");
	Shop->InteractPromptText = FText::FromString(TEXT("按 E 商店"));

	TestTrue(TEXT("Shop with id is interactable"), Shop->CanInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Shop prompt uses configured text"),
		Shop->GetInteractPromptText_Implementation(PC.Get()).ToString(),
		FString(TEXT("按 E 商店")));
	TestEqual(TEXT("Shop interact location is actor location"),
		Shop->GetInteractLocation_Implementation(PC.Get()),
		Shop->GetActorLocation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionRunEventTriggerNativeContractSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerNativeContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionRunEventTriggerNativeContractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Trigger.Get()));
	InjectRunSession(PC.Get(), Run.Get());

	TestFalse(TEXT("Event trigger without id/definition is not interactable"),
		Trigger->CanInteract_Implementation(PC.Get()));

	Trigger->PersistentId = TEXT("Event.UI.Trigger");
	Trigger->EventDefinition = Event.Get();
	Trigger->InteractPromptText = FText::FromString(TEXT("按 E 事件"));
	Trigger->CompletedPromptText = FText::FromString(TEXT("事件已完成"));
	Trigger->CompletedToastText = FText::FromString(TEXT("该事件已完成"));

	TestTrue(TEXT("Event trigger with id and definition is interactable"),
		Trigger->CanInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Event prompt uses configured text"),
		Trigger->GetInteractPromptText_Implementation(PC.Get()).ToString(),
		FString(TEXT("按 E 事件")));
	TestEqual(TEXT("Event interact location is actor location"),
		Trigger->GetInteractLocation_Implementation(PC.Get()),
		Trigger->GetActorLocation());

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(Trigger->PersistentId, Event.Get()));
	TestTrue(TEXT("Complete event via option"), Run->ChooseRunEventOption(TEXT("Close")));
	TestTrue(TEXT("Completed event trigger remains weakly interactable"),
		Trigger->CanInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Completed prompt uses weak text"),
		Trigger->GetInteractPromptText_Implementation(PC.Get()).ToString(),
		FString(TEXT("事件已完成")));
	TestFalse(TEXT("Completed event does not reopen"), Trigger->TryInteract_Implementation(PC.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionCompletedRunEventWeakPromptSpec,
	"Wacom.UI.WorldInteraction.CompletedRunEventWeakPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionCompletedRunEventWeakPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Trigger.Get()));
	InjectRunSession(PC.Get(), Run.Get());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Trigger->SetActorLocation(FVector(80.f, 0.f, 0.f));
	Trigger->PersistentId = TEXT("Event.UI.CompletedPrompt");
	Trigger->EventDefinition = Event.Get();
	Trigger->CompletedPromptText = FText::FromString(TEXT("事件已完成"));

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(Trigger->PersistentId, Event.Get()));
	TestTrue(TEXT("Complete event via option"), Run->ChooseRunEventOption(TEXT("Close")));

	PC->RegisterCandidateInteractable(Trigger.Get());
	TestTrue(TEXT("Completed event remains closest candidate"), PC->ReadClosestInteractable() == Trigger.Get());
	TestEqual(TEXT("Completed event prompt shown"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("事件已完成")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIWorldInteractionBattleTriggerCompatibilitySpec,
	"Wacom.UI.WorldInteraction.BattleTriggerCompatibility",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIWorldInteractionBattleTriggerCompatibilitySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<ABattleTriggerActor> Trigger(NewObject<ABattleTriggerActor>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Trigger->SetActorLocation(FVector(50.f, 0.f, 0.f));

	TestFalse(TEXT("Battle trigger without enemy is not interactable"),
		Trigger->CanInteract_Implementation(PC.Get()));

	PC->RegisterCandidateTrigger(Trigger.Get());
	TestNull(TEXT("Compatibility registration ignores unavailable trigger"), PC->ReadClosestInteractable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgeConfiguresInteractionTargetSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgeConfiguresInteractionTargetComponent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgeConfiguresInteractionTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Target = AddInteractionTargetComponent(*Owner);
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.Config"));

	TestTrue(TEXT("Refresh binding succeeds"), Bridge->RefreshRunWorldTargetBinding());
	TestTrue(TEXT("Runtime target id generated"), Target->GetTargetId().IsValid());
	TestTrue(TEXT("Run target tag written"),
		Target->GetInteractionTargetTag().MatchesTagExact(WacomTags::Interaction_Target_Run_Object));
	TestEqual(TEXT("Stable id written"),
		Target->GetStableTargetId(), FName(TEXT("Run.Target.Config")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgeAutoGeneratesRuntimeIdSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgeAutoGeneratesRuntimeTargetId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgeAutoGeneratesRuntimeIdSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Target = AddInteractionTargetComponent(*Owner);
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge = AddRunWorldBridgeComponent(*Owner);

	TestFalse(TEXT("Target starts without runtime id"), Target->GetTargetId().IsValid());
	TestTrue(TEXT("Refresh binding succeeds"), Bridge->RefreshRunWorldTargetBinding());
	TestTrue(TEXT("Bridge generated runtime id"), Bridge->GetRuntimeTargetId().IsValid());
	TestEqual(TEXT("Generated id written to target component"),
		Target->GetTargetId(), Bridge->GetRuntimeTargetId());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgePreservesStableIdSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgePreservesStableTargetId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgePreservesStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Target = AddInteractionTargetComponent(*Owner);
	const FGuid ExistingId = FGuid::NewGuid();
	Target->SetTargetId(ExistingId);
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.Stable"));

	TestTrue(TEXT("Refresh binding succeeds"), Bridge->RefreshRunWorldTargetBinding());
	TestEqual(TEXT("Existing runtime id preserved"), Target->GetTargetId(), ExistingId);
	TestEqual(TEXT("Stable id preserved on bridge"),
		Bridge->GetRunWorldTargetDebugView().RunTargetStableId,
		FName(TEXT("Run.Target.Stable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgePreviewScaleSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgePreviewScalesAndClearsVisualTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgePreviewScaleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UStaticMeshComponent* Visual = AddVisualComponent(*Owner, FVector(2.0f, 3.0f, 4.0f));
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge = AddRunWorldBridgeComponent(*Owner);
	Bridge->VisualTargetComponent = Visual;
	Bridge->ProbePreviewScale = 1.10f;

	Bridge->SetProbePreviewActive(true);
	TestTrue(TEXT("Preview active"), Bridge->IsProbePreviewActive());
	TestEqual(TEXT("Visual scaled for preview"),
		Visual->GetRelativeScale3D(), FVector(2.2f, 3.3f, 4.4f));

	Bridge->ClearProbePreview();
	TestFalse(TEXT("Preview cleared"), Bridge->IsProbePreviewActive());
	TestEqual(TEXT("Visual scale restored"),
		Visual->GetRelativeScale3D(), FVector(2.0f, 3.0f, 4.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgeDebugSummarySpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgeDebugSummaryReportsBindingAndPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgeDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	AddInteractionTargetComponent(*Owner);
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge =
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.Debug"));
	Bridge->RefreshRunWorldTargetBinding();
	Bridge->SetProbePreviewActive(true);

	const FString Summary = Bridge->GetRunWorldTargetDebugSummary();
	TestTrue(TEXT("Summary includes owner"), Summary.Contains(TEXT("RunWorldInteractionTarget")));
	TestTrue(TEXT("Summary includes stable id"), Summary.Contains(TEXT("Run.Target.Debug")));
	TestTrue(TEXT("Summary reports configured"), Summary.Contains(TEXT("Configured=true")));
	TestTrue(TEXT("Summary reports preview active"), Summary.Contains(TEXT("PreviewActive=true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetProbeBuildsHandleSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunProbeBuildsHandleForRunObject",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetProbeBuildsHandleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Target = AddInteractionTargetComponent(*Owner);
	const FGuid TargetId = FGuid::NewGuid();
	Target->SetTargetId(TargetId);
	Target->SetStableTargetId(TEXT("Run.Target.Probe"));
	Target->SetInteractionTargetTag(WacomTags::Interaction_Target_Run_Object);

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);

	FWacomInteractionTargetHandle Handle;
	TestTrue(TEXT("Run probe finds run object"), PC->ProbeRunSceneTargetForTest(Handle));
	TestEqual(TEXT("Run probe preserves id"), Handle.WorldTargetId, TargetId);
	TestTrue(TEXT("Run probe preserves tag"),
		Handle.TargetTag.MatchesTagExact(WacomTags::Interaction_Target_Run_Object));
	TestEqual(TEXT("Run probe preserves stable id"),
		Handle.StableTargetId, FName(TEXT("Run.Target.Probe")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetProbeRejectsBattleTargetSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunProbeRejectsBattleEnemyPartTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetProbeRejectsBattleTargetSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Target = AddInteractionTargetComponent(*Owner);
	Target->SetTargetId(FGuid::NewGuid());
	Target->SetStableTargetId(TEXT("Snake.Body"));
	Target->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);

	FWacomInteractionTargetHandle Handle;
	TestFalse(TEXT("Run probe rejects battle target"), PC->ProbeRunSceneTargetForTest(Handle));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetProbeWidgetPositionSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunProbeAtWidgetPositionWritesScreenPosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetProbeWidgetPositionSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* Owner = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("Owner actor spawned"), Owner))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Owner))
		{
			Owner->Destroy();
		}
	};

	UWacomInteractionTargetComponent* Target = AddInteractionTargetComponent(*Owner);
	Target->SetTargetId(FGuid::NewGuid());
	Target->SetStableTargetId(TEXT("Run.Target.WidgetProbe"));
	Target->SetInteractionTargetTag(WacomTags::Interaction_Target_Run_Object);

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);

	FWacomInteractionTargetHandle Handle;
	const FVector2D WidgetPosition(321.0f, 654.0f);
	TestTrue(TEXT("Run widget-position probe finds target"),
		PC->ProbeRunSceneTargetAtWidgetPositionForTest(WidgetPosition, Handle));
	TestEqual(TEXT("Run widget-position probe writes screen position"),
		Handle.ScreenPosition, WidgetPosition);
	TestEqual(TEXT("Run widget-position probe writes hit world location"),
		Handle.WorldLocation, FVector(321.0f, 654.0f, 0.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetProbePreviewSwitchSpec,
	"Wacom.UI.RunWorldInteractionTarget.ProbePreviewSwitchesAndClearsPreviousTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetProbePreviewSwitchSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AActor* First = SpawnTransientActor(*World);
	AActor* Second = SpawnTransientActor(*World);
	if (!TestNotNull(TEXT("First actor spawned"), First)
		|| !TestNotNull(TEXT("Second actor spawned"), Second))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(First))
		{
			First->Destroy();
		}
		if (IsValid(Second))
		{
			Second->Destroy();
		}
	};

	AddInteractionTargetComponent(*First);
	UWacomRunWorldInteractionTargetBridgeComponent* FirstBridge =
		AddRunWorldBridgeComponent(*First, TEXT("Run.Target.First"));
	AddVisualComponent(*First);
	FirstBridge->RefreshRunWorldTargetBinding();

	AddInteractionTargetComponent(*Second);
	UWacomRunWorldInteractionTargetBridgeComponent* SecondBridge =
		AddRunWorldBridgeComponent(*Second, TEXT("Run.Target.Second"));
	AddVisualComponent(*Second);
	SecondBridge->RefreshRunWorldTargetBinding();

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->bEnableRunWorldTargetProbePreview = true;

	PC->SetRunSceneHitForTest(First);
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("First target preview active"), FirstBridge->IsProbePreviewActive());
	TestFalse(TEXT("Second target preview inactive"), SecondBridge->IsProbePreviewActive());

	PC->SetRunSceneHitForTest(Second);
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestFalse(TEXT("First target preview cleared after switch"), FirstBridge->IsProbePreviewActive());
	TestTrue(TEXT("Second target preview active after switch"), SecondBridge->IsProbePreviewActive());

	PC->ClearRunSceneHitForTest();
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestFalse(TEXT("Second target preview cleared after no hit"), SecondBridge->IsProbePreviewActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopOfferPresentationBuilderSpec,
	"Wacom.UI.Shop.OfferPresentationBuilder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopOfferPresentationBuilderSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DamageCard = Fx.MakeSimpleDamageCard(/*Cost*/2, /*Damage*/5);
	DamageCard->CardId = TEXT("Shop.Damage");
	DamageCard->DisplayName = FText::FromString(TEXT("伤害商品"));

	FRunShopOffer Offer;
	Offer.OfferId = FGuid::NewGuid();
	Offer.CardDefinition = DamageCard;
	Offer.Price = 3;

	FWacomShopOfferPresentationView View =
		UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 5);
	TestTrue(TEXT("Affordable offer can be purchased"), View.bCanPurchase);
	TestEqual(TEXT("Affordable view keeps card definition"), View.CardDefinition.Get(), DamageCard);
	TestEqual(TEXT("Affordable action text"), View.ActionText.ToString(), FString(TEXT("购买")));
	TestEqual(TEXT("Affordable disabled reason"), View.DisabledReason, NAME_None);
	TestEqual(TEXT("Price text uses gold"), View.PriceText.ToString(), FString(TEXT("3 金币")));
	TestEqual(TEXT("Card view data name from card builder"), View.CardViewData.Name.ToString(), FString(TEXT("伤害商品")));
	TestEqual(TEXT("Card view data cost from card builder"), View.CardViewData.Cost, 2);
	TestEqual(TEXT("Card view data has one badge"), View.CardViewData.EffectBadges.Num(), 1);
	if (View.CardViewData.EffectBadges.IsValidIndex(0))
	{
		TestTrue(TEXT("Badge is damage"),
			View.CardViewData.EffectBadges[0].Kind == EWacomCardViewEffectBadgeKind::Damage);
		TestEqual(TEXT("Damage badge value"), View.CardViewData.EffectBadges[0].Value, 5);
	}

	Offer.bPurchased = true;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 5);
	TestFalse(TEXT("Purchased offer disabled"), View.bCanPurchase);
	TestEqual(TEXT("Purchased action text"), View.ActionText.ToString(), FString(TEXT("已购买")));
	TestEqual(TEXT("Purchased reason"), View.DisabledReason, FName(TEXT("Purchased")));

	Offer.bPurchased = false;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 1);
	TestFalse(TEXT("Insufficient gold disabled"), View.bCanPurchase);
	TestEqual(TEXT("Insufficient action text"), View.ActionText.ToString(), FString(TEXT("金币不足")));
	TestEqual(TEXT("Insufficient reason"), View.DisabledReason, FName(TEXT("InsufficientGold")));

	Offer.Price = 0;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 0);
	TestTrue(TEXT("Free offer can be purchased"), View.bCanPurchase);
	TestEqual(TEXT("Free price text"), View.PriceText.ToString(), FString(TEXT("免费")));

	Offer.CardDefinition = nullptr;
	View = UWacomShopPresentationBuilder::BuildOfferPresentationView(Offer, /*CurrentGold*/ 99);
	TestFalse(TEXT("Missing card disabled"), View.bCanPurchase);
	TestEqual(TEXT("Missing card name"), View.CardNameText.ToString(), FString(TEXT("未知卡牌")));
	TestEqual(TEXT("Missing card action text"), View.ActionText.ToString(), FString(TEXT("不可购买")));
	TestEqual(TEXT("Missing card reason"), View.DisabledReason, FName(TEXT("MissingCard")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventPresentationBuilderSpec,
	"Wacom.UI.Event.PresentationBuilder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventPresentationBuilderSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Card = Fx.MakeNoopCard(0);
	Card->CardId = TEXT("Event.Toast.Card");
	Card->DisplayName = FText::FromString(TEXT("事件提示卡"));

	TestEqual(TEXT("Gold reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("InsufficientGold")).ToString(),
		FString(TEXT("金币不足")));
	TestEqual(TEXT("Node reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("InsufficientNode")).ToString(),
		FString(TEXT("行动点不足")));
	TestEqual(TEXT("Pressure reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("PressureTooHigh")).ToString(),
		FString(TEXT("压力过高")));
	TestEqual(TEXT("Missing required card reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("MissingRequiredCard")).ToString(),
		FString(TEXT("缺少所需卡牌")));
	TestEqual(TEXT("Protected card reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("ProtectedCard")).ToString(),
		FString(TEXT("该卡牌不能被移除")));
	TestEqual(TEXT("Last capacity reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("LastCapacityProvider")).ToString(),
		FString(TEXT("这是最后一张背包容量卡")));
	TestEqual(TEXT("Event dependency reason text"),
		UWacomRunEventPresentationBuilder::FormatDisabledReason(TEXT("RequiredEventNotCompleted")).ToString(),
		FString(TEXT("前置事件未完成")));
	TestEqual(TEXT("Pressure name text"),
		UWacomRunEventPresentationBuilder::FormatPressureName(EWacomPressureType::Misdeed).ToString(),
		FString(TEXT("恶行")));

	FRunEventChoiceResult Result;
	Result.bSucceeded = true;
	Result.PaidCardDefinition = Card;
	Result.bEventClosedAfterResolve = true;
	FRunEventChoiceEffectResult GainCard;
	GainCard.EffectType = EWacomRunEventEffectType::GainCard;
	GainCard.CardDefinition = Card;
	GainCard.bApplied = true;
	FRunEventChoiceEffectResult Gold;
	Gold.EffectType = EWacomRunEventEffectType::AddGold;
	Gold.ActualDelta = 2;
	Gold.bApplied = true;
	FRunEventChoiceEffectResult Pressure;
	Pressure.EffectType = EWacomRunEventEffectType::AddPressure;
	Pressure.PressureType = EWacomPressureType::Misdeed;
	Pressure.ActualDelta = 3;
	Pressure.bApplied = true;
	FRunEventChoiceEffectResult Node;
	Node.EffectType = EWacomRunEventEffectType::ConsumeNode;
	Node.ActualDelta = -1;
	Node.bApplied = true;
	FRunEventChoiceEffectResult RemoveCard;
	RemoveCard.EffectType = EWacomRunEventEffectType::RemoveCard;
	RemoveCard.CardDefinition = Card;
	RemoveCard.ActualDelta = -1;
	RemoveCard.bApplied = true;
	FRunEventChoiceEffectResult MarkEvent;
	MarkEvent.EffectType = EWacomRunEventEffectType::MarkEventCompleted;
	MarkEvent.ActualDelta = 1;
	MarkEvent.bApplied = true;
	Result.EffectResults = { GainCard, Gold, Pressure, Node, RemoveCard, MarkEvent };

	const TArray<FWacomAppToastView> Toasts =
		UWacomRunEventPresentationBuilder::BuildToastViewsFromChoiceResult(Result);
	TestEqual(TEXT("Seven visible toasts including paid card and outcome"), Toasts.Num(), 7);
	if (Toasts.Num() == 7)
	{
		TestEqual(TEXT("Card toast"), Toasts[0].MessageText.ToString(), FString(TEXT("获得卡牌：事件提示卡")));
		TestEqual(TEXT("Gold toast"), Toasts[1].MessageText.ToString(), FString(TEXT("获得 2 金币")));
		TestEqual(TEXT("Pressure toast"), Toasts[2].MessageText.ToString(), FString(TEXT("恶行 +3")));
		TestEqual(TEXT("Node toast"), Toasts[3].MessageText.ToString(), FString(TEXT("消耗 1 行动点")));
		TestEqual(TEXT("Remove card toast still works"), Toasts[4].MessageText.ToString(), FString(TEXT("交出卡牌：事件提示卡")));
		TestEqual(TEXT("Paid card toast appended"), Toasts[5].MessageText.ToString(), FString(TEXT("交出卡牌：事件提示卡")));
		TestEqual(TEXT("Outcome toast appended"), Toasts[6].MessageText.ToString(), FString(TEXT("事件已结束")));
	}

	FRunEventChoiceResult NodeChanged;
	NodeChanged.bSucceeded = true;
	NodeChanged.bNodeChanged = true;
	NodeChanged.ResolvedNodeId = TEXT("After");
	NodeChanged.ResolvedNodeTitleText = FText::FromString(TEXT("后续节点"));
	const TArray<FWacomAppToastView> NodeToasts =
		UWacomRunEventPresentationBuilder::BuildToastViewsFromChoiceResult(NodeChanged);
	TestEqual(TEXT("Node change emits one outcome toast"), NodeToasts.Num(), 1);
	if (NodeToasts.Num() == 1)
	{
		TestEqual(TEXT("Node change toast text"), NodeToasts[0].MessageText.ToString(), FString(TEXT("进入：后续节点")));
	}

	FRunEventChoiceResult NodeChangedWithoutTitle;
	NodeChangedWithoutTitle.bSucceeded = true;
	NodeChangedWithoutTitle.bNodeChanged = true;
	NodeChangedWithoutTitle.ResolvedNodeId = TEXT("AfterIdOnly");
	const TArray<FWacomAppToastView> NodeIdToasts =
		UWacomRunEventPresentationBuilder::BuildToastViewsFromChoiceResult(NodeChangedWithoutTitle);
	TestEqual(TEXT("Node id fallback emits one outcome toast"), NodeIdToasts.Num(), 1);
	if (NodeIdToasts.Num() == 1)
	{
		TestEqual(TEXT("Node id fallback toast text"), NodeIdToasts[0].MessageText.ToString(), FString(TEXT("进入：AfterIdOnly")));
	}

	FRunEventChoiceResult Blocked;
	Blocked.DisabledReason = TEXT("InsufficientGold");
	Blocked.PaidCardDefinition = Card;
	const TArray<FWacomAppToastView> BlockedToasts =
		UWacomRunEventPresentationBuilder::BuildToastViewsFromChoiceResult(Blocked);
	TestEqual(TEXT("Blocked emits one toast"), BlockedToasts.Num(), 1);
	if (BlockedToasts.Num() == 1)
	{
		TestEqual(TEXT("Blocked toast text"), BlockedToasts[0].MessageText.ToString(), FString(TEXT("金币不足")));
		TestTrue(TEXT("Blocked toast warning"), BlockedToasts[0].Tone == EWacomAppToastTone::Warning);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventChoiceButtonPaymentStatusSpec,
	"Wacom.UI.Event.PaymentChoiceRowStatus",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventChoiceButtonPaymentStatusSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunEventChoiceButton> Button(NewObject<UWacomRunEventChoiceButton>());
	Button->TakeWidget();

	FRunEventChoiceSnapshot PaymentChoice;
	PaymentChoice.ChoiceId = TEXT("Pay");
	PaymentChoice.LabelText = FText::FromString(TEXT("交出毒牙"));
	PaymentChoice.bAvailable = true;
	PaymentChoice.bRequiresOwnedCardPayment = true;
	PaymentChoice.PaymentCandidateCount = 2;
	Button->SetChoiceSnapshot(PaymentChoice);
	TestEqual(TEXT("Payment row shows candidate count"),
		Button->GetDisplayedPaymentStatusTextForTest().ToString(),
		FString(TEXT("拖入卡牌支付：2 张可用")));
	TestEqual(TEXT("Payment status visible for payment choice"),
		Button->GetPaymentStatusVisibilityForTest(),
		ESlateVisibility::HitTestInvisible);

	PaymentChoice.bAvailable = false;
	PaymentChoice.PaymentCandidateCount = 0;
	PaymentChoice.PaymentDisabledReason = TEXT("MissingRequiredCard");
	Button->SetChoiceSnapshot(PaymentChoice);
	TestEqual(TEXT("Payment row shows missing candidate reason"),
		Button->GetDisplayedPaymentStatusTextForTest().ToString(),
		FString(TEXT("缺少可支付卡牌：缺少所需卡牌")));

	FRunEventChoiceSnapshot NonPaymentChoice;
	NonPaymentChoice.ChoiceId = TEXT("Leave");
	NonPaymentChoice.LabelText = FText::FromString(TEXT("离开"));
	NonPaymentChoice.bAvailable = true;
	Button->SetChoiceSnapshot(NonPaymentChoice);
	TestEqual(TEXT("Non-payment row hides payment status"),
		Button->GetPaymentStatusVisibilityForTest(),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventScreenSnapshotAndChoiceSpec,
	"Wacom.UI.Event.ScreenSnapshotAndChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventScreenSnapshotAndChoiceSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Run.Get()));
	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(NewObject<UWacomRunEventScreenProbe>());
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWacomAppToastWidget> ToastWidget(NewObject<UWacomAppToastWidget>());
	ToastWidget->TakeWidget();
	FWacomUITestAccess::SetToastWidget(*ToastSubsystem, ToastWidget.Get());

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Event.UI.Screen"), Event.Get()));
	Screen->SetRunSession(Run.Get());
	Screen->SetToastSubsystem(ToastSubsystem.Get());
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshEvent();
	TestTrue(TEXT("Event screen activates without world stack"), Screen->IsActivated());

	TestEqual(TEXT("Screen title from snapshot"), Screen->ReadTitleText().ToString(), FString(TEXT("UI标题")));
	TestEqual(TEXT("Screen body from snapshot"), Screen->ReadBodyText().ToString(), FString(TEXT("UI正文")));
	TestEqual(TEXT("One choice row"), Screen->ReadChoiceCount(), 1);
	TestEqual(TEXT("Choice label stored"),
		Screen->ReadChoiceSnapshot(0).LabelText.ToString(),
		FString(TEXT("关闭事件")));
	TestTrue(TEXT("Choose close succeeds"), Screen->ChooseChoiceAt(0));
	TestTrue(TEXT("Event completed after choice"), Run->IsRunEventCompleted(TEXT("Event.UI.Screen")));
	TestFalse(TEXT("Event no longer active after close choice"), Run->IsRunEventActive());
	TestFalse(TEXT("Close choice deactivates event screen once flow ends"), Screen->IsActivated());
	TestEqual(TEXT("Close choice emits event ended toast"), ToastWidget->GetVisibleToastCount(), 1);
	{
		const TArray<FWacomAppToastView> Toasts = FWacomUITestAccess::GetCurrentToasts(*ToastWidget);
		if (Toasts.IsValidIndex(0))
		{
			TestEqual(TEXT("Close outcome toast text"),
				Toasts[0].MessageText.ToString(),
				FString(TEXT("事件已结束")));
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventScreenCardPaymentSpec,
	"Wacom.UI.Event.CardPaymentChoice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventScreenCardPaymentSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = Fx.MakeNoopCard(0);
	Fang->CardId = TEXT("PoisonFang");
	UCardDefinition* Other = Fx.MakeNoopCard(0);
	Other->CardId = TEXT("OtherCard");
	UCardDefinition* Bag = Fx.MakeNoopCard(0);
	Bag->Physique.Capacity = 8;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(1),
		Fx.MakeNoopCard(1),
		{ Bag, Fang, Other });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	const FGuid FangId = FindUiStorageInstanceIdByDefinition(Run->BuildBackpackStorageSnapshot(), Fang);
	const FGuid OtherId = FindUiStorageInstanceIdByDefinition(Run->BuildBackpackStorageSnapshot(), Other);
	TestTrue(TEXT("Fang instance found"), FangId.IsValid());
	TestTrue(TEXT("Other instance found"), OtherId.IsValid());

	TStrongObjectPtr<UWacomRunEventDefinition> Event(
		MakeUiRunEventCardPaymentEvent(Run.Get(), Fang));
	TestTrue(TEXT("Begin payment UI event"),
		Run->BeginRunEvent(TEXT("Event.UI.Payment"), Event.Get()));

	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(NewObject<UWacomRunEventScreenProbe>());
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWacomAppToastWidget> ToastWidget(NewObject<UWacomAppToastWidget>());
	ToastWidget->TakeWidget();
	FWacomUITestAccess::SetToastWidget(*ToastSubsystem, ToastWidget.Get());
	Screen->SetRunSession(Run.Get());
	Screen->SetToastSubsystem(ToastSubsystem.Get());
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshEvent();

	TestEqual(TEXT("One payment choice"), Screen->ReadChoiceCount(), 1);
	const FRunEventChoiceSnapshot Choice = Screen->ReadChoiceSnapshot(0);
	TestTrue(TEXT("Choice requires payment"), Choice.bRequiresOwnedCardPayment);
	TestEqual(TEXT("Payment zone stored"), Choice.PaymentZoneId, FName(TEXT("RunEvent.Pay.Fang")));
	TestEqual(TEXT("One candidate exposed"), Choice.PaymentCandidateCount, 1);
	if (Choice.PaymentCandidateInstanceIds.IsValidIndex(0))
	{
		TestEqual(TEXT("Candidate is fang instance"), Choice.PaymentCandidateInstanceIds[0], FangId);
	}

	const FWacomRunEventScreenDebugView InitialDebug = Screen->ReadRunEventScreenDebugView();
	TestTrue(TEXT("Debug reports run session"), InitialDebug.bHasRunSession);
	TestTrue(TEXT("Debug reports active event"), InitialDebug.bIsEventActive);
	TestEqual(TEXT("Debug reports active node"), InitialDebug.CurrentNodeId, FName(TEXT("Start")));
	TestEqual(TEXT("Debug reports cached choice count"), InitialDebug.CachedChoiceCount, 1);
	TestEqual(TEXT("Debug reports payment choice count"), InitialDebug.PaymentChoiceCount, 1);
	TestEqual(TEXT("Debug reports candidate count"), InitialDebug.PaymentCandidateInstanceCount, 1);
	TestEqual(TEXT("Debug reports zone mapping count"), InitialDebug.PaymentZoneMappingCount, 1);
	TestTrue(TEXT("Debug reports payment zone mapping"),
		InitialDebug.PaymentZoneMappingSummary.Contains(TEXT("RunEvent.Pay.Fang->PayFang")));
	const FString InitialDebugSummary = Screen->ReadRunEventScreenDebugSummary();
	TestTrue(TEXT("Debug summary includes active node"),
		InitialDebugSummary.Contains(TEXT("Node=Start")));
	TestTrue(TEXT("Debug summary includes choice counts"),
		InitialDebugSummary.Contains(TEXT("Choices=1"))
		&& InitialDebugSummary.Contains(TEXT("PaymentChoices=1"))
		&& InitialDebugSummary.Contains(TEXT("Candidates=1")));
	TestTrue(TEXT("Debug summary includes zone map"),
		InitialDebugSummary.Contains(TEXT("RunEvent.Pay.Fang->PayFang")));

	TestFalse(TEXT("Clicking payment choice without drag is blocked"),
		Screen->ChooseChoiceAt(0));
	TestTrue(TEXT("Event remains active after blocked payment click"), Run->IsRunEventActive());
	TestEqual(TEXT("Blocked payment click emits toast"), ToastWidget->GetVisibleToastCount(), 1);
	{
		const TArray<FWacomAppToastView> Toasts = FWacomUITestAccess::GetCurrentToasts(*ToastWidget);
		if (Toasts.IsValidIndex(0))
		{
			TestEqual(TEXT("Blocked payment toast text"),
				Toasts[0].MessageText.ToString(),
				FString(TEXT("需要拖入卡牌支付")));
		}
	}

	FWacomRunMenuCardDropResolveResult WrongDrop;
	WrongDrop.SourceCardInstanceId = OtherId;
	WrongDrop.ZoneId = TEXT("RunEvent.Pay.Fang");
	const FWacomRunMenuCardDropResolveResult WrongResult =
		Screen->ResolveDropForTest(WrongDrop);
	TestEqual(TEXT("Wrong candidate rejected by screen"),
		WrongResult.IntentKind,
		EWacomRunMenuCardDropIntentKind::Reject);
	TestEqual(TEXT("Wrong candidate rejection keeps validation reason"),
		WrongResult.RunValidationReason,
		FName(TEXT("PaymentCardNotAllowed")));
	const FString RejectedDebugSummary = Screen->ReadRunEventScreenDebugSummary();
	TestTrue(TEXT("Debug records rejected payment resolve"),
		RejectedDebugSummary.Contains(TEXT("LastResolve=Resolve"))
		&& RejectedDebugSummary.Contains(TEXT("Intent=Reject"))
		&& RejectedDebugSummary.Contains(TEXT("RunValidation=PaymentCardNotAllowed")));

	FWacomRunMenuCardDropResolveResult ValidDrop;
	ValidDrop.SourceCardInstanceId = FangId;
	ValidDrop.ZoneId = TEXT("RunEvent.Pay.Fang");
	const FWacomRunMenuCardDropResolveResult ValidResult =
		Screen->ResolveDropForTest(ValidDrop);
	TestEqual(TEXT("Valid candidate resolves submit intent"),
		ValidResult.IntentKind,
		EWacomRunMenuCardDropIntentKind::SubmitZoneTarget);
	TestEqual(TEXT("RunEventScreen handles submit"),
		ValidResult.SubmitPolicy,
		EWacomRunMenuCardDropSubmitPolicy::MenuHandled);
	TestEqual(TEXT("RunEventScreen records submit reason"),
		ValidResult.SubmitReason,
		FName(TEXT("PayFang")));
	const FString AcceptedDebugSummary = Screen->ReadRunEventScreenDebugSummary();
	TestTrue(TEXT("Debug records accepted payment resolve"),
		AcceptedDebugSummary.Contains(TEXT("Intent=SubmitZoneTarget"))
		&& AcceptedDebugSummary.Contains(TEXT("SubmitPolicy=MenuHandled"))
		&& AcceptedDebugSummary.Contains(TEXT("SubmitReason=PayFang"))
		&& AcceptedDebugSummary.Contains(TEXT("CanSubmit=true")));

	FWacomRunMenuCardDropResolveResult SubmittedResult;
	TestTrue(TEXT("RunEventScreen submit succeeds"),
		Screen->SubmitDropForTest(ValidResult, SubmittedResult));
	TestTrue(TEXT("Submit result records success"), SubmittedResult.bSubmitted);
	const FString SubmittedDebugSummary = Screen->ReadRunEventScreenDebugSummary();
	TestTrue(TEXT("Debug records submit result"),
		SubmittedDebugSummary.Contains(TEXT("LastSubmit=Submit"))
		&& SubmittedDebugSummary.Contains(TEXT("Submitted=true"))
		&& SubmittedDebugSummary.Contains(TEXT("SubmitPolicy=MenuHandled")));
	TestFalse(TEXT("Paid card removed by RunEvent option"),
		UiStorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), Fang));
	TestTrue(TEXT("Other card remains"),
		UiStorageContainsDefinition(Run->BuildBackpackStorageSnapshot(), Other));
	TestEqual(TEXT("Choice effect applied"), Run->GetGold(), 1);
	TestFalse(TEXT("Payment event closes after submit"), Run->IsRunEventActive());
	{
		const TArray<FWacomAppToastView> Toasts = FWacomUITestAccess::GetCurrentToasts(*ToastWidget);
		const bool bHasPaidToast = Toasts.ContainsByPredicate(
			[](const FWacomAppToastView& Toast)
			{
				return Toast.MessageText.ToString() == TEXT("交出卡牌：PoisonFang");
			});
		TestTrue(TEXT("Successful payment emits paid card toast"), bHasPaidToast);
		const bool bHasOutcomeToast = Toasts.ContainsByPredicate(
			[](const FWacomAppToastView& Toast)
			{
				return Toast.MessageText.ToString() == TEXT("事件已结束");
			});
		TestTrue(TEXT("Successful payment emits outcome toast"), bHasOutcomeToast);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventScreenBlockedChoiceToastSpec,
	"Wacom.UI.Event.ScreenBlockedChoiceToast",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventScreenBlockedChoiceToastSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(NewObject<UWacomRunEventDefinition>(Run.Get()));
	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(NewObject<UWacomRunEventScreenProbe>());
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWacomAppToastWidget> ToastWidget(NewObject<UWacomAppToastWidget>());
	ToastWidget->TakeWidget();
	FWacomUITestAccess::SetToastWidget(*ToastSubsystem, ToastWidget.Get());

	Event->EventId = TEXT("Event.UI.Blocked");
	Event->DisplayName = FText::FromString(TEXT("禁用事件"));
	Event->StartNodeId = TEXT("Start");
	FWacomRunEventChoiceDefinition Locked;
	Locked.ChoiceId = TEXT("Locked");
	Locked.LabelText = FText::FromString(TEXT("金币选项"));
	FWacomRunEventConditionDefinition GoldCondition;
	GoldCondition.Type = EWacomRunEventConditionType::MinGold;
	GoldCondition.Value = 1;
	Locked.Conditions.Add(GoldCondition);
	FWacomRunEventNodeDefinition Start;
	Start.NodeId = TEXT("Start");
	Start.Choices = { Locked };
	Event->Nodes = { Start };

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(TEXT("Event.UI.Blocked.Actor"), Event.Get()));
	Screen->SetRunSession(Run.Get());
	Screen->SetToastSubsystem(ToastSubsystem.Get());
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshEvent();
	TestTrue(TEXT("Blocked event screen remains active before choice"), Screen->IsActivated());

	TestEqual(TEXT("One blocked choice"), Screen->ReadChoiceCount(), 1);
	TestFalse(TEXT("Choice unavailable"), Screen->ReadChoiceSnapshot(0).bAvailable);
	TestFalse(TEXT("Choosing blocked option fails"), Screen->ChooseChoiceAt(0));
	TestTrue(TEXT("Event remains active"), Run->IsRunEventActive());
	TestTrue(TEXT("Blocked choice keeps screen active"), Screen->IsActivated());
	TestEqual(TEXT("Blocked choice emits toast"), ToastWidget->GetVisibleToastCount(), 1);
	const TArray<FWacomAppToastView> Toasts = FWacomUITestAccess::GetCurrentToasts(*ToastWidget);
	if (Toasts.IsValidIndex(0))
	{
		TestEqual(TEXT("Blocked toast text"), Toasts[0].MessageText.ToString(), FString(TEXT("金币不足")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopPurchaseFailureToastTextSpec,
	"Wacom.UI.Shop.PurchaseFailureToastText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopPurchaseFailureToastTextSpec::RunTest(const FString& /*Parameters*/)
{
	TestEqual(TEXT("Insufficient gold toast"),
		UWacomShopScreenProbe::FormatPurchaseFailureToast(TEXT("InsufficientGold")).ToString(),
		FString(TEXT("金币不足")));
	TestEqual(TEXT("Purchased toast"),
		UWacomShopScreenProbe::FormatPurchaseFailureToast(TEXT("Purchased")).ToString(),
		FString(TEXT("该商品已购买")));
	TestEqual(TEXT("Missing card toast"),
		UWacomShopScreenProbe::FormatPurchaseFailureToast(TEXT("MissingCard")).ToString(),
		FString(TEXT("商品不可购买")));
	TestEqual(TEXT("Fallback purchase failure toast"),
		UWacomShopScreenProbe::FormatPurchaseFailureToast(NAME_None).ToString(),
		FString(TEXT("购买失败")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopOfferRowPresentationSpec,
	"Wacom.UI.Shop.OfferRowPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopOfferRowPresentationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomShopOfferPresentationView View;
	View.OfferId = FGuid::NewGuid();
	View.CardNameText = FText::FromString(TEXT("行测试卡"));
	View.PriceText = FText::FromString(TEXT("免费"));
	View.ActionText = FText::FromString(TEXT("购买"));
	View.bCanPurchase = true;

	TStrongObjectPtr<UWacomShopOfferRowWidget> Row(NewObject<UWacomShopOfferRowWidget>());
	Row->TakeWidget();
	Row->SetOfferPresentationView(View);
	TestEqual(TEXT("Row stores presentation view"), Row->GetOfferPresentationView().OfferId, View.OfferId);
	TestTrue(TEXT("Row stored offer remains purchasable"), Row->GetOfferPresentationView().bCanPurchase);

	View.bCanPurchase = false;
	View.ActionText = FText::FromString(TEXT("金币不足"));
	View.DisabledReason = TEXT("InsufficientGold");
	Row->SetOfferPresentationView(View);
	TestFalse(TEXT("Row stores disabled view"), Row->GetOfferPresentationView().bCanPurchase);
	TestEqual(TEXT("Row disabled reason stored"),
		Row->GetOfferPresentationView().DisabledReason,
		FName(TEXT("InsufficientGold")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopScreenSnapshotAndPurchaseSpec,
	"Wacom.UI.Shop.ScreenSnapshotAndPurchase",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopScreenSnapshotAndPurchaseSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* ShopCard = Fx.MakeNoopCard(0);
	ShopCard->CardId = TEXT("Shop.Test.Card");
	ShopCard->DisplayName = FText::FromString(TEXT("商店测试卡"));
	UCardDefinition* SecondCard = Fx.MakeNoopCard(0);
	SecondCard->CardId = TEXT("Shop.Test.Second");
	SecondCard->DisplayName = FText::FromString(TEXT("第二张商店卡"));

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UWacomShopScreenProbe> Screen(NewObject<UWacomShopScreenProbe>());
	TStrongObjectPtr<UGameInstance> GameInstance(NewObject<UGameInstance>());
	TStrongObjectPtr<UWacomAppToastSubsystem> ToastSubsystem(NewObject<UWacomAppToastSubsystem>(GameInstance.Get()));
	TStrongObjectPtr<UWacomAppToastWidget> ToastWidget(NewObject<UWacomAppToastWidget>());

	InjectRunSession(PC.Get(), Run.Get());
	if (!TestNotNull(TEXT("Injected run session"), PC->GetRunSession()))
	{
		return false;
	}
	ToastWidget->TakeWidget();
	FWacomUITestAccess::SetToastWidget(*ToastSubsystem, ToastWidget.Get());

	Run->AddGold(5);
	TArray<FRunShopOfferInput> Offers;
	Offers.Add({ ShopCard, 3 });
	Offers.Add({ SecondCard, 3 });
	TestTrue(TEXT("Begin shop succeeds"), Run->BeginShopVisit(TEXT("Shop.Screen"), Offers));

	Screen->SetRunSession(Run.Get());
	Screen->SetToastSubsystem(ToastSubsystem.Get());
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshShop();
	TestTrue(TEXT("Shop screen activates without world stack"), Screen->IsActivated());

	TestEqual(TEXT("Two offer rows"), Screen->ReadOfferCount(), 2);
	TestTrue(TEXT("First offer starts purchasable"), Screen->ReadOfferPresentationView(0).bCanPurchase);
	TestTrue(TEXT("Second offer starts purchasable"), Screen->ReadOfferPresentationView(1).bCanPurchase);
	TestTrue(TEXT("Gold text reflects run gold"), Screen->ReadGoldText().ToString().Contains(TEXT("5")));
	TestTrue(TEXT("Purchase first offer succeeds"), Screen->PurchaseOfferAt(0));
	TestEqual(TEXT("Gold after purchase"), Run->GetGold(), 2);
	TestEqual(TEXT("Purchase success emits one app toast"), ToastWidget->GetVisibleToastCount(), 1);
	const TArray<FWacomAppToastView> PurchaseToasts = FWacomUITestAccess::GetCurrentToasts(*ToastWidget);
	if (PurchaseToasts.IsValidIndex(0))
	{
		const FWacomAppToastView& Toast = PurchaseToasts[0];
		TestEqual(TEXT("Purchase success toast text"),
			Toast.MessageText.ToString(),
			FString(TEXT("获得卡牌：商店测试卡")));
		TestTrue(TEXT("Purchase success toast is positive"), Toast.Tone == EWacomAppToastTone::Positive);
		TestEqual(TEXT("Purchase success toast icon"), Toast.IconKey, FName(TEXT("CardGained")));
	}
	TestFalse(TEXT("Purchased offer disabled after refresh"), Screen->ReadOfferPresentationView(0).bCanPurchase);
	TestEqual(TEXT("Purchased offer action after refresh"),
		Screen->ReadOfferPresentationView(0).ActionText.ToString(),
		FString(TEXT("已购买")));
	TestFalse(TEXT("Second offer disabled after gold drops"), Screen->ReadOfferPresentationView(1).bCanPurchase);
	TestEqual(TEXT("Second offer becomes insufficient"),
		Screen->ReadOfferPresentationView(1).DisabledReason,
		FName(TEXT("InsufficientGold")));
	const FRunBackpackStorageSnapshot StorageSnapshot = Run->BuildBackpackStorageSnapshot();
	TestTrue(TEXT("Purchased card enters run storage"),
		StorageSnapshot.Flux.ContentCards.ContainsByPredicate([ShopCard](const FRunStorageCardView& CardView)
		{
			return CardView.Instance.Definition.Get() == ShopCard;
		})
		|| StorageSnapshot.BurdenCards.ContainsByPredicate([ShopCard](const FRunStorageCardView& CardView)
		{
			return CardView.Instance.Definition.Get() == ShopCard;
		}));
	TestTrue(TEXT("Shop visit has purchase"), Run->BuildCurrentShopSnapshot().bHasPurchaseThisVisit);

	const int32 NodesBeforeClose = Run->GetRemainingNodeCount();
	Screen->DeactivateWidget();
	TestFalse(TEXT("Shop visit closed"), Run->IsShopVisitActive());
	TestFalse(TEXT("Shop screen deactivates after close flow"), Screen->IsActivated());
	TestEqual(TEXT("Close after purchase consumes one node"), Run->GetRemainingNodeCount(), NodesBeforeClose - 1);
	Screen->DeactivateWidget();
	TestEqual(TEXT("Duplicate deactivation does not consume another node"), Run->GetRemainingNodeCount(), NodesBeforeClose - 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopTriggerDefinitionOffersSpec,
	"Wacom.UI.Shop.TriggerDefinitionOffers",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopTriggerDefinitionOffersSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* DefinitionCard = Fx.MakeNoopCard(0);
	DefinitionCard->CardId = TEXT("Shop.Definition.Card");
	UCardDefinition* ManualCard = Fx.MakeNoopCard(0);
	ManualCard->CardId = TEXT("Shop.Manual.Card");

	TStrongObjectPtr<UShopDefinition> ShopDefinition(NewObject<UShopDefinition>());
	ShopDefinition->ShopId = TEXT("Shop.Definition");
	ShopDefinition->DisplayName = FText::FromString(TEXT("测试商店定义"));
	FShopOfferDefinition DefinitionOffer;
	DefinitionOffer.CardDefinition = DefinitionCard;
	DefinitionOffer.Price = 2;
	ShopDefinition->Offers.Add(DefinitionOffer);

	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());
	Shop->PersistentId = TEXT("Shop.TriggerDefinition");
	Shop->ShopDefinition = ShopDefinition.Get();
	Shop->Offers.Add({ ManualCard, 9 });

	TArray<FRunShopOfferInput> ResolvedOffers = Shop->BuildResolvedOffers();
	TestEqual(TEXT("Definition offer wins count"), ResolvedOffers.Num(), 1);
	if (ResolvedOffers.IsValidIndex(0))
	{
		TestEqual(TEXT("Definition offer wins card"), ResolvedOffers[0].CardDefinition.Get(), DefinitionCard);
		TestEqual(TEXT("Definition offer wins price"), ResolvedOffers[0].Price, 2);
	}

	Shop->ShopDefinition = nullptr;
	ResolvedOffers = Shop->BuildResolvedOffers();
	TestEqual(TEXT("Manual offers fallback count"), ResolvedOffers.Num(), 1);
	if (ResolvedOffers.IsValidIndex(0))
	{
		TestEqual(TEXT("Manual fallback card"), ResolvedOffers[0].CardDefinition.Get(), ManualCard);
		TestEqual(TEXT("Manual fallback price"), ResolvedOffers[0].Price, 9);
	}

	return true;
}

// Lifecycle contract/regression harness:
// FWacomExplorationScreenRouter is a WacomApp Private helper, and the public
// RequestOpenShop/RequestOpenRunEvent path needs a real exploration World,
// AWacomGameMode, GameInstance UIManager, LocalPlayer, and PrimaryLayout stack.
// Without changing Build.cs/runtime or introducing a brittle PIE/UI-asset test,
// this locks the domain invariant the Router must preserve: old GameMenu screen
// deactivation must complete before the new RunSession Begin*, otherwise the
// old screen's NativeOnDeactivated End* can clear the newly active visit/event.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuSwitchClosesOldShopBeforeBeginNewShopSpec,
	"Wacom.UI.GameMenu.SwitchClosesOldShopBeforeBeginNewShop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuSwitchClosesOldShopBeforeBeginNewShopSpec::RunTest(const FString& /*Parameters*/)
{
	const FName OldShopId(TEXT("Shop.GameMenu.Old"));
	const FName NewShopId(TEXT("Shop.GameMenu.New"));
	const TArray<FRunShopOfferInput> Offers;

	{
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TStrongObjectPtr<UWacomShopScreenProbe> OldScreen(NewObject<UWacomShopScreenProbe>());

		TestTrue(TEXT("Hazard: old shop begins before switch"), Run->BeginShopVisit(OldShopId, Offers));
		OldScreen->SetRunSession(Run.Get());
		OldScreen->TakeWidget();
		OldScreen->ActivateWidget();

		TestTrue(TEXT("Hazard: new Begin can replace active shop before old screen closes"),
			Run->BeginShopVisit(NewShopId, Offers));
		TestEqual(TEXT("Hazard: active shop is new before old deactivation"),
			Run->BuildCurrentShopSnapshot().ShopId,
			NewShopId);

		OldScreen->DeactivateWidget();
		TestFalse(TEXT("Hazard: old NativeOnDeactivated End clears the new active shop"),
			Run->IsShopVisitActive());
	}

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomShopScreenProbe> OldScreen(NewObject<UWacomShopScreenProbe>());
	int32 OldScreenDeactivatedCount = 0;

	TestTrue(TEXT("Old shop begins before switch"), Run->BeginShopVisit(OldShopId, Offers));
	OldScreen->SetRunSession(Run.Get());
	OldScreen->OnDeactivated().AddLambda([&OldScreenDeactivatedCount]()
	{
		++OldScreenDeactivatedCount;
	});
	OldScreen->TakeWidget();
	OldScreen->ActivateWidget();

	OldScreen->DeactivateWidget();
	TestEqual(TEXT("Old shop screen deactivates before new Begin"), OldScreenDeactivatedCount, 1);
	TestFalse(TEXT("Old shop End cleared active visit before switch"), Run->IsShopVisitActive());

	TestTrue(TEXT("New shop begins after old screen close"), Run->BeginShopVisit(NewShopId, Offers));
	const FRunShopSnapshot NewSnapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("New shop remains active after switch"), NewSnapshot.bIsActive);
	TestEqual(TEXT("Active shop id belongs to new Begin"), NewSnapshot.ShopId, NewShopId);

	OldScreen->DeactivateWidget();
	const FRunShopSnapshot AfterDuplicateDeactivate = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("Duplicate old deactivation does not clear new shop"), AfterDuplicateDeactivate.bIsActive);
	TestEqual(TEXT("New shop id survives duplicate old deactivation"),
		AfterDuplicateDeactivate.ShopId,
		NewShopId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuSwitchClosesOldRunEventBeforeBeginNewRunEventSpec,
	"Wacom.UI.GameMenu.SwitchClosesOldRunEventBeforeBeginNewRunEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuSwitchClosesOldRunEventBeforeBeginNewRunEventSpec::RunTest(const FString& /*Parameters*/)
{
	const FName OldEventId(TEXT("Event.GameMenu.Old"));
	const FName NewEventId(TEXT("Event.GameMenu.New"));

	{
		TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
		TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Run.Get()));
		TStrongObjectPtr<UWacomRunEventScreenProbe> OldScreen(NewObject<UWacomRunEventScreenProbe>());

		TestTrue(TEXT("Hazard: old event begins before switch"), Run->BeginRunEvent(OldEventId, Event.Get()));
		OldScreen->SetRunSession(Run.Get());
		OldScreen->TakeWidget();
		OldScreen->ActivateWidget();

		TestTrue(TEXT("Hazard: new Begin can replace active event before old screen closes"),
			Run->BeginRunEvent(NewEventId, Event.Get()));
		TestEqual(TEXT("Hazard: active event is new before old deactivation"),
			Run->BuildCurrentRunEventSnapshot().PersistentId,
			NewEventId);

		OldScreen->DeactivateWidget();
		TestFalse(TEXT("Hazard: old NativeOnDeactivated End clears the new active event"),
			Run->IsRunEventActive());
	}

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Run.Get()));
	TStrongObjectPtr<UWacomRunEventScreenProbe> OldScreen(NewObject<UWacomRunEventScreenProbe>());
	int32 OldScreenDeactivatedCount = 0;

	TestTrue(TEXT("Old event begins before switch"), Run->BeginRunEvent(OldEventId, Event.Get()));
	OldScreen->SetRunSession(Run.Get());
	OldScreen->OnDeactivated().AddLambda([&OldScreenDeactivatedCount]()
	{
		++OldScreenDeactivatedCount;
	});
	OldScreen->TakeWidget();
	OldScreen->ActivateWidget();

	OldScreen->DeactivateWidget();
	TestEqual(TEXT("Old event screen deactivates before new Begin"), OldScreenDeactivatedCount, 1);
	TestFalse(TEXT("Old event End cleared active event before switch"), Run->IsRunEventActive());

	TestTrue(TEXT("New event begins after old screen close"), Run->BeginRunEvent(NewEventId, Event.Get()));
	const FRunEventSnapshot NewSnapshot = Run->BuildCurrentRunEventSnapshot();
	TestTrue(TEXT("New event remains active after switch"), NewSnapshot.bIsActive);
	TestEqual(TEXT("Active event id belongs to new Begin"), NewSnapshot.PersistentId, NewEventId);

	OldScreen->DeactivateWidget();
	const FRunEventSnapshot AfterDuplicateDeactivate = Run->BuildCurrentRunEventSnapshot();
	TestTrue(TEXT("Duplicate old deactivation does not clear new event"), AfterDuplicateDeactivate.bIsActive);
	TestEqual(TEXT("New event id survives duplicate old deactivation"),
		AfterDuplicateDeactivate.PersistentId,
		NewEventId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuFailedShopPushDeactivateDoesNotEndNewShopSpec,
	"Wacom.UI.GameMenu.FailedShopPushDeactivateDoesNotEndNewShop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuFailedShopPushDeactivateDoesNotEndNewShopSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomShopScreenProbe> FailedScreen(NewObject<UWacomShopScreenProbe>());
	const TArray<FRunShopOfferInput> Offers;

	TestTrue(TEXT("Failed async shop Begin happens before push failure"),
		Run->BeginShopVisit(TEXT("Shop.Async.Failed"), Offers));
	FailedScreen->SetRunSession(Run.Get());
	FailedScreen->SuppressEndOnNextDeactivateForTest();
	Run->EndShopVisit();
	TestFalse(TEXT("Rollback clears failed active shop"), Run->IsShopVisitActive());

	TestTrue(TEXT("New shop begins after failed push rollback"),
		Run->BeginShopVisit(TEXT("Shop.Async.New"), Offers));
	FailedScreen->DeactivateWidget();

	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("Failed screen deactivation does not clear new shop"), Snapshot.bIsActive);
	TestEqual(TEXT("New shop id survives failed screen deactivation"),
		Snapshot.ShopId,
		FName(TEXT("Shop.Async.New")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIGameMenuFailedRunEventPushDeactivateDoesNotEndNewEventSpec,
	"Wacom.UI.GameMenu.FailedRunEventPushDeactivateDoesNotEndNewEvent",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIGameMenuFailedRunEventPushDeactivateDoesNotEndNewEventSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Run.Get()));
	TStrongObjectPtr<UWacomRunEventScreenProbe> FailedScreen(NewObject<UWacomRunEventScreenProbe>());

	TestTrue(TEXT("Failed async event Begin happens before push failure"),
		Run->BeginRunEvent(TEXT("Event.Async.Failed"), Event.Get()));
	FailedScreen->SetRunSession(Run.Get());
	FailedScreen->SuppressEndOnNextDeactivateForTest();
	Run->EndRunEvent();
	TestFalse(TEXT("Rollback clears failed active event"), Run->IsRunEventActive());

	TestTrue(TEXT("New event begins after failed push rollback"),
		Run->BeginRunEvent(TEXT("Event.Async.New"), Event.Get()));
	FailedScreen->DeactivateWidget();

	const FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
	TestTrue(TEXT("Failed screen deactivation does not clear new event"), Snapshot.bIsActive);
	TestEqual(TEXT("New event id survives failed screen deactivation"),
		Snapshot.PersistentId,
		FName(TEXT("Event.Async.New")));

	return true;
}
