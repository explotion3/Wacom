// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/BattleTriggerActor.h"
#include "Actors/WacomRunCardPickupActor.h"
#include "Actors/WacomRunPickupActor.h"
#include "Actors/WacomRunEventTriggerActor.h"
#include "Actors/WacomShopTriggerActor.h"
#include "Cards/CardDefinition.h"
#include "Components/SceneComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/WacomBattleEnemyPartWorldTargetBridgeComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Components/WacomRunWorldInteractionTargetBridgeComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Events/RunEventDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "GameFramework/WacomPlayerController.h"
#include "Interaction/WacomRunWorldClickableInteractable.h"
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

	UWacomRunEventDefinition* MakeUiRunEventFlagRewardPreviewEvent(
		UObject* Outer,
		UCardDefinition* RewardCard)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = TEXT("Event.UI.FlagReward");
		Event->DisplayName = FText::FromString(TEXT("标记奖励 UI 事件"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Claim;
		Claim.ChoiceId = TEXT("ClaimGoldReward");
		Claim.LabelText = FText::FromString(TEXT("支付 3 金币领取毒牙"));
		FWacomRunEventConditionDefinition RequiresInspect;
		RequiresInspect.Type = EWacomRunEventConditionType::RunFlagSet;
		RequiresInspect.FlagId = TEXT("DebugFlagReward.Inspected");
		FWacomRunEventConditionDefinition RequiresUnclaimed;
		RequiresUnclaimed.Type = EWacomRunEventConditionType::RunFlagNotSet;
		RequiresUnclaimed.FlagId = TEXT("DebugFlagReward.RewardClaimed");
		FWacomRunEventConditionDefinition RequiresGold;
		RequiresGold.Type = EWacomRunEventConditionType::MinGold;
		RequiresGold.Value = 3;
		Claim.Conditions = { RequiresInspect, RequiresUnclaimed, RequiresGold };

		FWacomRunEventEffectDefinition LoseGold;
		LoseGold.Type = EWacomRunEventEffectType::AddGold;
		LoseGold.Value = -3;
		FWacomRunEventEffectDefinition GainCard;
		GainCard.Type = EWacomRunEventEffectType::GainCard;
		GainCard.CardDefinition = RewardCard;
		FWacomRunEventEffectDefinition SetRewardFlag;
		SetRewardFlag.Type = EWacomRunEventEffectType::SetRunFlag;
		SetRewardFlag.FlagId = TEXT("DebugFlagReward.RewardClaimed");
		Claim.Effects = { LoseGold, GainCard, SetRewardFlag };
		Claim.NextNodeId = TEXT("Rewarded");

		FWacomRunEventNodeDefinition Start;
		Start.NodeId = TEXT("Start");
		Start.Choices = { Claim };
		FWacomRunEventNodeDefinition Rewarded;
		Rewarded.NodeId = TEXT("Rewarded");
		Event->Nodes = { Start, Rewarded };
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

	int32 CountUiStorageDefinitions(const FRunBackpackStorageSnapshot& Snapshot, const UCardDefinition* Card)
	{
		if (!Card)
		{
			return 0;
		}

		int32 Count = 0;
		const auto CountView = [&Count, Card](const FRunStorageCardView& View)
		{
			if (View.Instance.Definition.Get() == Card)
			{
				++Count;
			}
		};

		for (const FRunStorageCardView& View : Snapshot.Flux.ContentCards)
		{
			CountView(View);
		}
		for (const FRunStorageCardView& View : Snapshot.BattleDeckPhysicalCards)
		{
			CountView(View);
		}
		for (const FRunStorageCardView& View : Snapshot.BurdenCards)
		{
			CountView(View);
		}
		for (const FRunSpecialStorageView& Special : Snapshot.SpecialZones)
		{
			CountView(Special.OwnerCard);
			for (const FRunStorageCardView& View : Special.ContentCards)
			{
				CountView(View);
			}
		}

		return Count;
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
	Shop->HoverPromptText = FText::FromString(TEXT("点击商店"));

	TestTrue(TEXT("Shop with id is interactable"), Shop->CanInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Shop prompt uses configured text"),
		Shop->GetInteractPromptText_Implementation(PC.Get()).ToString(),
		FString(TEXT("按 E 商店")));
	TestEqual(TEXT("Shop hover prompt uses configured text"),
		Shop->GetHoverPromptText(PC.Get()).ToString(),
		FString(TEXT("点击商店")));
	TestEqual(TEXT("Shop interact location is actor location"),
		Shop->GetInteractLocation_Implementation(PC.Get()),
		Shop->GetActorLocation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeOwnsTargetComponentsSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.ShopTriggerOwnsClickInteractionTargetComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeOwnsTargetComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());

	TestNotNull(TEXT("Shop trigger owns click bounds"), Shop->GetClickBounds());
	TestNotNull(TEXT("Shop trigger owns interaction target"),
		Shop->GetClickInteractionTargetComponent());
	TestNotNull(TEXT("Shop trigger owns run target bridge"),
		Shop->GetClickTargetBridgeComponent());
	if (Shop->GetClickBounds())
	{
		TestEqual(TEXT("Click bounds blocks visibility"),
			Shop->GetClickBounds()->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
		TestFalse(TEXT("Click bounds does not generate overlap"),
			Shop->GetClickBounds()->GetGenerateOverlapEvents());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeStableIdSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.ShopClickTargetUsesPersistentIdAsStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.ClickStable");
	Shop->SyncClickTargetForTest();

	TestEqual(TEXT("Bridge stable id mirrors PersistentId"),
		Shop->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Shop.UI.ClickStable")));
	TestEqual(TEXT("Interaction target stable id mirrors PersistentId"),
		Shop->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Shop.UI.ClickStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeOwnsTargetComponentsSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.BattleTriggerOwnsClickInteractionTargetComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeOwnsTargetComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Battle(NewObject<ABattleTriggerActor>());

	TestNotNull(TEXT("Battle trigger owns click bounds"), Battle->GetClickBounds());
	TestNotNull(TEXT("Battle trigger owns interaction target"),
		Battle->GetClickInteractionTargetComponent());
	TestNotNull(TEXT("Battle trigger owns run target bridge"),
		Battle->GetClickTargetBridgeComponent());
	if (Battle->GetClickBounds())
	{
		TestEqual(TEXT("Click bounds blocks visibility"),
			Battle->GetClickBounds()->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
		TestFalse(TEXT("Click bounds does not generate overlap"),
			Battle->GetClickBounds()->GetGenerateOverlapEvents());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeStableIdSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.BattleClickTargetUsesPersistentIdAsStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ClickStable");
	Battle->SyncClickTargetForTest();

	TestEqual(TEXT("Bridge stable id mirrors PersistentId"),
		Battle->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Battle.UI.ClickStable")));
	TestEqual(TEXT("Interaction target stable id mirrors PersistentId"),
		Battle->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Battle.UI.ClickStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupOwnsTargetComponentsSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupOwnsClickInteractionTargetComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupOwnsTargetComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunPickupActor> Pickup(NewObject<AWacomRunPickupActor>());

	TestNotNull(TEXT("Pickup owns trigger sphere"), Pickup->GetTriggerSphere());
	TestNotNull(TEXT("Pickup owns click bounds"), Pickup->GetClickBounds());
	TestNotNull(TEXT("Pickup owns visible placeholder"), Pickup->GetPickupVisual());
	TestNotNull(TEXT("Pickup owns interaction target"),
		Pickup->GetClickInteractionTargetComponent());
	TestNotNull(TEXT("Pickup owns run target bridge"),
		Pickup->GetClickTargetBridgeComponent());
	if (Pickup->GetClickBounds())
	{
		TestEqual(TEXT("Click bounds blocks visibility"),
			Pickup->GetClickBounds()->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
		TestFalse(TEXT("Click bounds does not generate overlap"),
			Pickup->GetClickBounds()->GetGenerateOverlapEvents());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupStableIdSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupClickTargetUsesPersistentIdAsStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	Pickup->PersistentId = TEXT("Pickup.UI.ClickStable");
	Pickup->SyncClickTargetForTest();

	TestEqual(TEXT("Bridge stable id mirrors PersistentId"),
		Pickup->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Pickup.UI.ClickStable")));
	TestEqual(TEXT("Interaction target stable id mirrors PersistentId"),
		Pickup->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Pickup.UI.ClickStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupOwnsTargetComponentsSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.RunCardPickupOwnsClickInteractionTargetComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupOwnsTargetComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunCardPickupActor> Pickup(NewObject<AWacomRunCardPickupActor>());

	TestNotNull(TEXT("Card pickup owns trigger sphere"), Pickup->GetTriggerSphere());
	TestNotNull(TEXT("Card pickup owns click bounds"), Pickup->GetClickBounds());
	TestNotNull(TEXT("Card pickup owns visible placeholder"), Pickup->GetPickupVisual());
	TestNotNull(TEXT("Card pickup owns interaction target"),
		Pickup->GetClickInteractionTargetComponent());
	TestNotNull(TEXT("Card pickup owns run target bridge"),
		Pickup->GetClickTargetBridgeComponent());
	if (Pickup->GetClickBounds())
	{
		TestEqual(TEXT("Click bounds blocks visibility"),
			Pickup->GetClickBounds()->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
		TestFalse(TEXT("Click bounds does not generate overlap"),
			Pickup->GetClickBounds()->GetGenerateOverlapEvents());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupStableIdSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.RunCardPickupClickTargetUsesPersistentIdAsStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>());
	Pickup->PersistentId = TEXT("Pickup.Card.UI.ClickStable");
	Pickup->SyncClickTargetForTest();

	TestEqual(TEXT("Bridge stable id mirrors PersistentId"),
		Pickup->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Pickup.Card.UI.ClickStable")));
	TestEqual(TEXT("Interaction target stable id mirrors PersistentId"),
		Pickup->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Pickup.Card.UI.ClickStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupBaseComponentsSpec,
	"Wacom.UI.WorldInteraction.RunPickupBase.PickupBaseConfiguresSharedComponentsForGoldAndCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupBaseComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunPickupActor> GoldPickup(NewObject<AWacomRunPickupActor>());
	TStrongObjectPtr<AWacomRunCardPickupActor> CardPickup(NewObject<AWacomRunCardPickupActor>());

	auto CheckPickup = [this](const TCHAR* Label, const AWacomRunPickupActorBase* Pickup)
	{
		if (!TestNotNull(FString::Printf(TEXT("%s pickup"), Label), Pickup))
		{
			return;
		}
		TestNotNull(FString::Printf(TEXT("%s trigger sphere"), Label), Pickup->GetTriggerSphere());
		TestNotNull(FString::Printf(TEXT("%s click bounds"), Label), Pickup->GetClickBounds());
		TestNotNull(FString::Printf(TEXT("%s visual"), Label), Pickup->GetPickupVisual());
		TestNotNull(FString::Printf(TEXT("%s interaction target"), Label),
			Pickup->GetClickInteractionTargetComponent());
		TestNotNull(FString::Printf(TEXT("%s bridge"), Label),
			Pickup->GetClickTargetBridgeComponent());
		if (Pickup->GetClickBounds())
		{
			TestEqual(FString::Printf(TEXT("%s click bounds blocks visibility"), Label),
				Pickup->GetClickBounds()->GetCollisionResponseToChannel(ECC_Visibility),
				ECR_Block);
			TestFalse(FString::Printf(TEXT("%s click bounds does not overlap"), Label),
				Pickup->GetClickBounds()->GetGenerateOverlapEvents());
		}
	};

	CheckPickup(TEXT("Gold"), GoldPickup.Get());
	CheckPickup(TEXT("Card"), CardPickup.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupBaseStableIdSpec,
	"Wacom.UI.WorldInteraction.RunPickupBase.PickupBaseSyncsStableIdForGoldAndCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupBaseStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunPickupClickProbe> GoldPickup(NewObject<AWacomRunPickupClickProbe>());
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> CardPickup(
		NewObject<AWacomRunCardPickupClickProbe>());

	GoldPickup->PersistentId = TEXT("Pickup.Base.GoldStable");
	CardPickup->PersistentId = TEXT("Pickup.Base.CardStable");
	GoldPickup->SyncClickTargetForTest();
	CardPickup->SyncClickTargetForTest();

	TestEqual(TEXT("Gold bridge stable id"),
		GoldPickup->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Pickup.Base.GoldStable")));
	TestEqual(TEXT("Gold interaction stable id"),
		GoldPickup->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Pickup.Base.GoldStable")));
	TestEqual(TEXT("Card bridge stable id"),
		CardPickup->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Pickup.Base.CardStable")));
	TestEqual(TEXT("Card interaction stable id"),
		CardPickup->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Pickup.Base.CardStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupBaseCrossTypeDuplicateSpec,
	"Wacom.UI.WorldInteraction.RunPickupBase.PickupBaseDetectsCrossTypeDuplicatePersistentId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupBaseCrossTypeDuplicateSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomRunPickupClickProbe* GoldPickup = World->SpawnActor<AWacomRunPickupClickProbe>(
		AWacomRunPickupClickProbe::StaticClass(), FTransform::Identity, SpawnParams);
	AWacomRunCardPickupClickProbe* CardPickup = World->SpawnActor<AWacomRunCardPickupClickProbe>(
		AWacomRunCardPickupClickProbe::StaticClass(), FTransform::Identity, SpawnParams);
	ON_SCOPE_EXIT
	{
		if (IsValid(GoldPickup))
		{
			GoldPickup->Destroy();
		}
		if (IsValid(CardPickup))
		{
			CardPickup->Destroy();
		}
	};

	if (!TestNotNull(TEXT("Gold pickup spawned"), GoldPickup)
		|| !TestNotNull(TEXT("Card pickup spawned"), CardPickup))
	{
		return false;
	}

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	InjectRunSession(PC.Get(), Run.Get());

	GoldPickup->PersistentId = TEXT("Pickup.Base.CrossDuplicate");
	GoldPickup->GoldAmount = 2;
	GoldPickup->bDestroyWhenCollected = false;
	GoldPickup->SyncClickTargetForTest();
	CardPickup->PersistentId = TEXT("Pickup.Base.CrossDuplicate");
	CardPickup->CardDefinition = Card.Get();
	CardPickup->bDestroyWhenCollected = false;
	CardPickup->SyncClickTargetForTest();

	TestTrue(TEXT("Gold sees card duplicate"),
		GoldPickup->GetRunPickupBaseDebugView(PC.Get()).bDuplicatePersistentIdDetected);
	TestTrue(TEXT("Card sees gold duplicate"),
		CardPickup->GetRunPickupBaseDebugView(PC.Get()).bDuplicatePersistentIdDetected);

	TestTrue(TEXT("Gold duplicate pickup can collect"),
		GoldPickup->TryInteract_Implementation(PC.Get()));
	TestFalse(TEXT("Card duplicate shares collected state"),
		CardPickup->CanInteract_Implementation(PC.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupBaseSharedDebugSpec,
	"Wacom.UI.WorldInteraction.RunPickupBase.PickupBaseSharedDebugReportsPromptVisualClickTargetAndCollected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupBaseSharedDebugSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.Base.Debug");
	Pickup->GoldAmount = 1;
	Pickup->HoverPromptText = FText::FromString(TEXT("点击共享调试"));
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	FWacomRunPickupBaseDebugView View = Pickup->GetRunPickupBaseDebugView(PC.Get());
	TestTrue(TEXT("Base debug config valid"), View.bConfigValid);
	TestTrue(TEXT("Base debug can interact"), View.bCanInteract);
	TestFalse(TEXT("Base debug not collected"), View.bIsCollected);
	TestTrue(TEXT("Base debug reports visual"), View.bHasRenderableVisual);
	TestTrue(TEXT("Base debug reports click target"), View.bClickTargetConfigured);
	TestEqual(TEXT("Base debug prompt"),
		View.HoverPrompt, FString(TEXT("点击共享调试")));
	TestEqual(TEXT("Base debug stable id"),
		View.ClickTargetStableId, FName(TEXT("Pickup.Base.Debug")));

	TestTrue(TEXT("Collect pickup"), Pickup->TryInteract_Implementation(PC.Get()));
	View = Pickup->GetRunPickupBaseDebugView(PC.Get());
	TestTrue(TEXT("Base debug collected"), View.bIsCollected);
	TestEqual(TEXT("Base debug collected result"), View.LastDebugResult, FName(TEXT("Collected")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupBaseHideLifecycleSpec,
	"Wacom.UI.WorldInteraction.RunPickupBase.PickupBaseHideLifecycleDisablesCollisionForGoldAndCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupBaseHideLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<AWacomRunPickupClickProbe> GoldPickup(NewObject<AWacomRunPickupClickProbe>());
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> CardPickup(
		NewObject<AWacomRunCardPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	GoldPickup->PersistentId = TEXT("Pickup.Base.HideGold");
	GoldPickup->GoldAmount = 1;
	GoldPickup->bDestroyWhenCollected = false;
	CardPickup->PersistentId = TEXT("Pickup.Base.HideCard");
	CardPickup->CardDefinition = Card.Get();
	CardPickup->bDestroyWhenCollected = false;

	TestTrue(TEXT("Gold collect"), GoldPickup->TryInteract_Implementation(PC.Get()));
	TestTrue(TEXT("Gold hidden"), GoldPickup->IsHidden());
	TestFalse(TEXT("Gold collision disabled"), GoldPickup->GetActorEnableCollision());
	TestTrue(TEXT("Card collect"), CardPickup->TryInteract_Implementation(PC.Get()));
	TestTrue(TEXT("Card hidden"), CardPickup->IsHidden());
	TestFalse(TEXT("Card collision disabled"), CardPickup->GetActorEnableCollision());

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
	Trigger->HoverPromptText = FText::FromString(TEXT("点击事件"));
	Trigger->CompletedHoverPromptText = FText::FromString(TEXT("事件已完成（点击）"));
	Trigger->CompletedToastText = FText::FromString(TEXT("该事件已完成"));

	TestTrue(TEXT("Event trigger with id and definition is interactable"),
		Trigger->CanInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Event prompt uses configured text"),
		Trigger->GetInteractPromptText_Implementation(PC.Get()).ToString(),
		FString(TEXT("按 E 事件")));
	TestEqual(TEXT("Event hover prompt uses configured text"),
		Trigger->GetHoverPromptText(PC.Get()).ToString(),
		FString(TEXT("点击事件")));
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
	TestEqual(TEXT("Completed hover prompt uses weak text"),
		Trigger->GetHoverPromptText(PC.Get()).ToString(),
		FString(TEXT("事件已完成（点击）")));
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
	FWacomUIRunEventTriggerConfigureSnakeGiftSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerAuthoring.ConfigureDebugSnakeGiftSampleSetsIdAndDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerConfigureSnakeGiftSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	Trigger->PersistentId = TEXT("Event.Old");
	Trigger->InteractPromptText = FText::FromString(TEXT("旧提示"));
	Trigger->CompletedPromptText = FText::FromString(TEXT("旧完成"));
	Trigger->HoverPromptText = FText::FromString(TEXT("旧点击"));
	Trigger->CompletedHoverPromptText = FText::FromString(TEXT("旧完成点击"));
	Trigger->CompletedToastText = FText::FromString(TEXT("旧 Toast"));

	Trigger->ConfigureDebugSnakeGiftSample();

	TestEqual(TEXT("SnakeGift sample id"),
		Trigger->PersistentId,
		FName(TEXT("Event.DebugSnakeGift.Actor")));
	TestNotNull(TEXT("SnakeGift sample definition"), Trigger->EventDefinition.Get());
	if (Trigger->EventDefinition)
	{
		TestEqual(TEXT("SnakeGift sample event id"),
			Trigger->EventDefinition->EventId,
			FName(TEXT("Event.DebugSnakeGift")));
		TestEqual(TEXT("SnakeGift sample start node"),
			Trigger->EventDefinition->StartNodeId,
			FName(TEXT("Start")));
	}
	TestEqual(TEXT("Default interact prompt restored"),
		Trigger->InteractPromptText.ToString(),
		FString(TEXT("按 E 查看事件")));
	TestEqual(TEXT("Default completed prompt restored"),
		Trigger->CompletedPromptText.ToString(),
		FString(TEXT("事件已完成")));
	TestEqual(TEXT("Default hover prompt restored"),
		Trigger->HoverPromptText.ToString(),
		FString(TEXT("点击查看事件")));
	TestEqual(TEXT("Default completed hover prompt restored"),
		Trigger->CompletedHoverPromptText.ToString(),
		FString(TEXT("事件已完成")));
	TestEqual(TEXT("Default completed toast restored"),
		Trigger->CompletedToastText.ToString(),
		FString(TEXT("该事件已完成")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerConfigureFlagRewardSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerAuthoring.ConfigureDebugFlagRewardSampleSetsIdAndDefinition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerConfigureFlagRewardSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());

	Trigger->ConfigureDebugFlagRewardSample();

	TestEqual(TEXT("FlagReward sample id"),
		Trigger->PersistentId,
		FName(TEXT("Event.DebugFlagReward.Actor")));
	TestNotNull(TEXT("FlagReward sample definition"), Trigger->EventDefinition.Get());
	if (Trigger->EventDefinition)
	{
		TestEqual(TEXT("FlagReward sample event id"),
			Trigger->EventDefinition->EventId,
			FName(TEXT("Event.DebugFlagReward")));
		TestEqual(TEXT("FlagReward sample start node"),
			Trigger->EventDefinition->StartNodeId,
			FName(TEXT("Start")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerConfigureDoesNotMutateRunStateSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerAuthoring.ConfigureSampleDoesNotMutateRunState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerConfigureDoesNotMutateRunStateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	InjectRunSession(PC.Get(), Run.Get());

	Trigger->ConfigureDebugFlagRewardSample();

	TestFalse(TEXT("Configure does not activate event"), Run->IsRunEventActive());
	TestFalse(TEXT("Configure does not complete event"),
		Run->IsRunEventCompleted(Trigger->PersistentId));
	TestEqual(TEXT("Configure does not create event state"),
		Run->GetRunState().RunEventStates.Num(),
		0);
	TestEqual(TEXT("Configure does not change gold"), Run->GetGold(), 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerDebugMissingConfigSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerDebug.RunEventTriggerDebugReportsMissingConfig",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerDebugMissingConfigSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	InjectRunSession(PC.Get(), Run.Get());

	FWacomRunEventTriggerDebugView Debug = Trigger->GetRunEventTriggerDebugView(PC.Get());
	TestEqual(TEXT("Missing config reports actor"), Debug.ActorName, Trigger->GetName());
	TestFalse(TEXT("Missing config cannot interact"), Debug.bCanInteract);
	TestTrue(TEXT("Missing config has run session"), Debug.bHasRunSession);
	TestEqual(TEXT("Missing config result"),
		Debug.LastDebugResult,
		FName(TEXT("MissingPersistentId")));

	Trigger->PersistentId = TEXT("Event.MissingDefinition");
	Debug = Trigger->GetRunEventTriggerDebugView(PC.Get());
	TestEqual(TEXT("Missing definition result"),
		Debug.LastDebugResult,
		FName(TEXT("MissingEventDefinition")));
	TestTrue(TEXT("Missing definition summary is stable"),
		Trigger->GetRunEventTriggerDebugSummary(PC.Get()).Contains(TEXT("Last=MissingEventDefinition")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerDebugConfiguredSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerDebug.RunEventTriggerDebugReportsConfiguredEventIdAndStartNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerDebugConfiguredSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	InjectRunSession(PC.Get(), Run.Get());
	Trigger->ConfigureDebugFlagRewardSample();

	const FWacomRunEventTriggerDebugView Debug = Trigger->GetRunEventTriggerDebugView(PC.Get());
	TestTrue(TEXT("Configured trigger can interact"), Debug.bCanInteract);
	TestEqual(TEXT("Configured event id"), Debug.EventId, FName(TEXT("Event.DebugFlagReward")));
	TestEqual(TEXT("Configured start node"), Debug.StartNodeId, FName(TEXT("Start")));
	TestEqual(TEXT("Configured current node falls back to start"),
		Debug.CurrentNodeId,
		FName(TEXT("Start")));
	TestEqual(TEXT("Configured result"), Debug.LastDebugResult, FName(TEXT("Ok")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerDebugActiveNodeSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerDebug.RunEventTriggerDebugReportsActiveCurrentNode",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerDebugActiveNodeSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	InjectRunSession(PC.Get(), Run.Get());
	Trigger->ConfigureDebugFlagRewardSample();

	TestTrue(TEXT("Begin FlagReward event"),
		Run->BeginRunEvent(Trigger->PersistentId, Trigger->EventDefinition));
	TestTrue(TEXT("InspectMark succeeds"),
		Run->ChooseRunEventOptionWithResult(TEXT("InspectMark")).bSucceeded);

	const FWacomRunEventTriggerDebugView Debug = Trigger->GetRunEventTriggerDebugView(PC.Get());
	TestTrue(TEXT("Debug reports active event"), Debug.bIsActiveEvent);
	TestFalse(TEXT("Debug reports not completed"), Debug.bIsCompleted);
	TestEqual(TEXT("Debug reports current start node"),
		Debug.CurrentNodeId,
		FName(TEXT("Start")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerDebugCompletedSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerDebug.RunEventTriggerDebugReportsCompletedState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerDebugCompletedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Trigger.Get()));
	InjectRunSession(PC.Get(), Run.Get());
	Trigger->PersistentId = TEXT("Event.UI.DebugCompleted");
	Trigger->EventDefinition = Event.Get();

	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(Trigger->PersistentId, Event.Get()));
	TestTrue(TEXT("Complete event succeeds"), Run->ChooseRunEventOption(TEXT("Close")));

	const FWacomRunEventTriggerDebugView Debug = Trigger->GetRunEventTriggerDebugView(PC.Get());
	TestFalse(TEXT("Completed event is not active"), Debug.bIsActiveEvent);
	TestTrue(TEXT("Completed event reports completed"), Debug.bIsCompleted);
	TestEqual(TEXT("Completed event current node"),
		Debug.CurrentNodeId,
		FName(TEXT("Start")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventTriggerDebugSummaryStableSpec,
	"Wacom.UI.WorldInteraction.RunEventTriggerDebug.RunEventTriggerDebugSummaryIsStableForPieLogs",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventTriggerDebugSummaryStableSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());
	InjectRunSession(PC.Get(), Run.Get());
	Trigger->ConfigureDebugFlagRewardSample();

	const FString Summary = Trigger->GetRunEventTriggerDebugSummary(PC.Get());
	TestTrue(TEXT("Summary reports id"),
		Summary.Contains(TEXT("PersistentId=Event.DebugFlagReward.Actor")));
	TestTrue(TEXT("Summary reports event id"),
		Summary.Contains(TEXT("EventId=Event.DebugFlagReward")));
	TestTrue(TEXT("Summary reports start node"),
		Summary.Contains(TEXT("StartNode=Start")));
	TestTrue(TEXT("Summary reports run session"),
		Summary.Contains(TEXT("HasRun=true")));
	TestTrue(TEXT("Summary reports interactable"),
		Summary.Contains(TEXT("CanInteract=true")));
	TestTrue(TEXT("Summary reports hover prompt"),
		Summary.Contains(TEXT("HoverPrompt=点击查看事件")));
	TestTrue(TEXT("Summary reports completed hover prompt"),
		Summary.Contains(TEXT("CompletedHoverPrompt=事件已完成")));
	TestTrue(TEXT("Summary reports result"),
		Summary.Contains(TEXT("Last=Ok")));

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
	FWacomUIRunWorldTargetBridgeCustomDepthSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgePreviewAppliesAndRestoresCustomDepthSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgeCustomDepthSpec::RunTest(const FString& /*Parameters*/)
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

	UStaticMeshComponent* Visual = AddVisualComponent(*Owner);
	Visual->SetRenderCustomDepth(false);
	Visual->SetCustomDepthStencilValue(7);
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge = AddRunWorldBridgeComponent(*Owner);
	Bridge->VisualTargetComponent = Visual;
	Bridge->bEnableProbeScaleSignal = false;
	Bridge->bEnableProbeCustomDepthSignal = true;
	Bridge->ProbeCustomDepthStencilValue = 250;

	Bridge->SetProbePreviewActive(true);
	TestTrue(TEXT("Preview active"), Bridge->IsProbePreviewActive());
	TestTrue(TEXT("CustomDepth enabled for preview"), Visual->bRenderCustomDepth);
	TestEqual(TEXT("Stencil value applied"), Visual->CustomDepthStencilValue, 250);

	Bridge->ClearProbePreview();
	TestFalse(TEXT("Preview cleared"), Bridge->IsProbePreviewActive());
	TestFalse(TEXT("CustomDepth restored"), Visual->bRenderCustomDepth);
	TestEqual(TEXT("Stencil value restored"), Visual->CustomDepthStencilValue, 7);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgeVisualSwitchRestoreSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgePreviewRestoresPreviousTargetWhenVisualTargetChanges",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgeVisualSwitchRestoreSpec::RunTest(const FString& /*Parameters*/)
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

	UStaticMeshComponent* First = AddVisualComponent(*Owner, FVector(2.0f, 2.0f, 2.0f));
	UStaticMeshComponent* Second = AddVisualComponent(*Owner, FVector(3.0f, 3.0f, 3.0f));
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge = AddRunWorldBridgeComponent(*Owner);
	Bridge->ProbePreviewScale = 1.2f;
	Bridge->VisualTargetComponent = First;

	Bridge->SetProbePreviewActive(true);
	TestEqual(TEXT("First target scaled"), First->GetRelativeScale3D(), FVector(2.4f, 2.4f, 2.4f));

	Bridge->VisualTargetComponent = Second;
	Bridge->SetProbePreviewActive(true);
	TestEqual(TEXT("First target restored after switch"), First->GetRelativeScale3D(), FVector(2.0f, 2.0f, 2.0f));
	TestEqual(TEXT("Second target scaled"), Second->GetRelativeScale3D(), FVector(3.6f, 3.6f, 3.6f));

	Bridge->ClearProbePreview();
	TestEqual(TEXT("Second target restored after clear"), Second->GetRelativeScale3D(), FVector(3.0f, 3.0f, 3.0f));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldTargetBridgePrefersRenderableSpec,
	"Wacom.UI.RunWorldInteractionTarget.RunBridgePrefersRenderableOwnerPrimitiveOverClickBounds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldTargetBridgePrefersRenderableSpec::RunTest(const FString& /*Parameters*/)
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

	UBoxComponent* Bounds = NewObject<UBoxComponent>(Owner);
	Owner->AddInstanceComponent(Bounds);
	Bounds->SetRelativeScale3D(FVector(5.0f, 5.0f, 5.0f));
	Bounds->RegisterComponent();

	UStaticMeshComponent* Visual = AddVisualComponent(*Owner, FVector(2.0f, 2.0f, 2.0f));
	UWacomRunWorldInteractionTargetBridgeComponent* Bridge = AddRunWorldBridgeComponent(*Owner);
	Bridge->VisualTargetComponent = Bounds;
	Bridge->ProbePreviewScale = 1.1f;

	Bridge->SetProbePreviewActive(true);
	TestEqual(TEXT("Click bounds scale is unchanged"),
		Bounds->GetRelativeScale3D(), FVector(5.0f, 5.0f, 5.0f));
	TestEqual(TEXT("Renderable visual is scaled"),
		Visual->GetRelativeScale3D(), FVector(2.2f, 2.2f, 2.2f));

	const FWacomRunWorldInteractionTargetDebugView View = Bridge->GetRunWorldTargetDebugView();
	TestTrue(TEXT("Debug reports renderable visual"), View.bHasRenderableVisualTarget);
	TestEqual(TEXT("Debug reports visual target name"), View.VisualTargetName, Visual->GetFName());

	Bridge->ClearProbePreview();
	TestEqual(TEXT("Renderable visual restored"),
		Visual->GetRelativeScale3D(), FVector(2.0f, 2.0f, 2.0f));

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
	TestTrue(TEXT("Summary reports scale signal"), Summary.Contains(TEXT("ScaleSignal=true")));
	TestTrue(TEXT("Summary reports custom depth signal"), Summary.Contains(TEXT("CustomDepthSignal=true")));
	TestTrue(TEXT("Summary reports stencil value"), Summary.Contains(TEXT("Stencil=250")));
	TestTrue(TEXT("Summary reports preview result"), Summary.Contains(TEXT("LastPreview=Applied")));

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
	FWacomUIRunWorldClickContractHelperBoundsSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.ClickHelperConfiguresBoundsCollision",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractHelperBoundsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBoxComponent> Bounds(NewObject<UBoxComponent>());

	FWacomRunWorldClickableInteractableHelper::ConfigureClickBounds(Bounds.Get());

	TestEqual(TEXT("Click helper enables query-only collision"),
		Bounds->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(TEXT("Click helper sets world dynamic object type"),
		Bounds->GetCollisionObjectType(),
		ECC_WorldDynamic);
	TestEqual(TEXT("Click helper blocks visibility"),
		Bounds->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);
	TestEqual(TEXT("Click helper ignores pawn"),
		Bounds->GetCollisionResponseToChannel(ECC_Pawn),
		ECR_Ignore);
	TestFalse(TEXT("Click helper disables overlap generation"),
		Bounds->GetGenerateOverlapEvents());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractHelperStableIdSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.ClickHelperBindsStableIdToBridgeAndInteractionTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractHelperStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AActor> Owner(NewObject<AActor>());
	TStrongObjectPtr<UBoxComponent> Bounds(NewObject<UBoxComponent>(Owner.Get()));
	TStrongObjectPtr<UWacomInteractionTargetComponent> Target(
		NewObject<UWacomInteractionTargetComponent>(Owner.Get()));
	TStrongObjectPtr<UWacomRunWorldInteractionTargetBridgeComponent> Bridge(
		NewObject<UWacomRunWorldInteractionTargetBridgeComponent>(Owner.Get()));

	FWacomRunWorldClickableInteractableHelper::BindClickTarget(
		TEXT("Run.Click.Contract"),
		Bounds.Get(),
		Target.Get(),
		Bridge.Get());

	TestEqual(TEXT("Bridge stable id mirrors helper stable id"),
		Bridge->RunTargetStableId,
		FName(TEXT("Run.Click.Contract")));
	TestEqual(TEXT("Interaction target stable id mirrors helper stable id"),
		Target->GetStableTargetId(),
		FName(TEXT("Run.Click.Contract")));
	TestTrue(TEXT("Bridge visual target is click bounds"),
		Bridge->VisualTargetComponent == Bounds.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractRunEventImplementsSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.RunEventTriggerImplementsClickableContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractRunEventImplementsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());

	TestTrue(TEXT("RunEvent trigger implements clickable contract"),
		Trigger->GetClass()->ImplementsInterface(UWacomRunWorldClickableInteractable::StaticClass()));
	TestTrue(TEXT("RunEvent trigger still implements world interactable"),
		Trigger->GetClass()->ImplementsInterface(UWacomWorldInteractable::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractShopImplementsSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.ShopTriggerImplementsClickableContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractShopImplementsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());

	TestTrue(TEXT("Shop trigger implements clickable contract"),
		Shop->GetClass()->ImplementsInterface(UWacomRunWorldClickableInteractable::StaticClass()));
	TestTrue(TEXT("Shop trigger still implements world interactable"),
		Shop->GetClass()->ImplementsInterface(UWacomWorldInteractable::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractBattleImplementsSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.BattleTriggerImplementsClickableContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractBattleImplementsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<ABattleTriggerActor> Battle(NewObject<ABattleTriggerActor>());

	TestTrue(TEXT("Battle trigger implements clickable contract"),
		Battle->GetClass()->ImplementsInterface(UWacomRunWorldClickableInteractable::StaticClass()));
	TestTrue(TEXT("Battle trigger still implements world interactable"),
		Battle->GetClass()->ImplementsInterface(UWacomWorldInteractable::StaticClass()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractBattleStableIdSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.ClickHelperBindsBattleTriggerStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractBattleStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ContractStable");
	Battle->SyncClickTargetForTest();

	TestEqual(TEXT("Battle bridge stable id mirrors PersistentId"),
		Battle->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Battle.UI.ContractStable")));
	TestEqual(TEXT("Battle target stable id mirrors PersistentId"),
		Battle->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Battle.UI.ContractStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeOwnsTargetComponentsSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.RunEventTriggerOwnsClickInteractionTargetComponents",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeOwnsTargetComponentsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());

	TestNotNull(TEXT("RunEvent trigger owns click bounds"), Trigger->GetClickBounds());
	TestNotNull(TEXT("RunEvent trigger owns interaction target"),
		Trigger->GetClickInteractionTargetComponent());
	TestNotNull(TEXT("RunEvent trigger owns run target bridge"),
		Trigger->GetClickTargetBridgeComponent());
	if (Trigger->GetClickBounds())
	{
		TestEqual(TEXT("Click bounds blocks visibility"),
			Trigger->GetClickBounds()->GetCollisionResponseToChannel(ECC_Visibility),
			ECR_Block);
		TestFalse(TEXT("Click bounds does not generate overlap"),
			Trigger->GetClickBounds()->GetGenerateOverlapEvents());
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeStableIdSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.RunEventClickTargetUsesPersistentIdAsStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.ClickStable");
	Trigger->SyncClickTargetForTest();

	TestEqual(TEXT("Bridge stable id mirrors PersistentId"),
		Trigger->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Event.UI.ClickStable")));
	TestEqual(TEXT("Interaction target stable id mirrors PersistentId"),
		Trigger->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Event.UI.ClickStable")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeConfigureRefreshesStableIdSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.ConfigureDebugSampleRefreshesClickTargetStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeConfigureRefreshesStableIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunEventTriggerActor> Trigger(NewObject<AWacomRunEventTriggerActor>());

	Trigger->ConfigureDebugFlagRewardSample();

	TestEqual(TEXT("FlagReward sample stable id written to bridge"),
		Trigger->GetClickTargetBridgeComponent()->RunTargetStableId,
		FName(TEXT("Event.DebugFlagReward.Actor")));
	TestEqual(TEXT("FlagReward sample stable id written to target"),
		Trigger->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Event.DebugFlagReward.Actor")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeRoutesToInteractSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.LeftClickRunEventTargetRoutesToTryInteract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeRoutesToInteractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.ClickRoute");
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	TestTrue(TEXT("Click route succeeds"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract called once"), Trigger->TryInteractCountForTest, 1);
	TestTrue(TEXT("TryInteract receives PC"),
		Trigger->GetLastInteractingPlayerControllerForTest() == PC.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeWithoutOverlapSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.LeftClickRunEventTargetCanOpenWithoutOverlapCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeWithoutOverlapSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.ClickFar");
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	TestNull(TEXT("No E-key candidate registered"), PC->ReadClosestInteractable());
	TestTrue(TEXT("Far click still routes"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract called by far click"), Trigger->TryInteractCountForTest, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupEKeyCollectsSpec,
	"Wacom.UI.WorldInteraction.RunPickup.EKeyRunPickupAddsGoldAndMarksCollected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupEKeyCollectsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());
	PC->SetPawn(Pawn.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.EKey");
	Pickup->GoldAmount = 4;
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	PC->RegisterCandidateInteractable(Pickup.Get());

	TestTrue(TEXT("E key pickup is closest candidate"), PC->ReadClosestInteractable() == Pickup.Get());
	PC->TryInteractFromConsole();

	TestEqual(TEXT("Pickup adds gold"), Run->GetGold(), 4);
	TestTrue(TEXT("Pickup marked collected"), Run->IsPickupCollected(Pickup->PersistentId));
	TestFalse(TEXT("Collected pickup can no longer interact"), Pickup->CanInteract_Implementation(PC.Get()));
	TestNull(TEXT("Collected pickup unregisters E candidate"), PC->ReadClosestInteractable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupClickCollectsWithoutOverlapSpec,
	"Wacom.UI.WorldInteraction.RunPickup.LeftClickRunPickupAddsGoldWithoutOverlapCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupClickCollectsWithoutOverlapSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.ClickFar");
	Pickup->GoldAmount = 5;
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	TestNull(TEXT("No E-key candidate registered"), PC->ReadClosestInteractable());
	TestTrue(TEXT("Far pickup click routes"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Pickup click adds gold"), Run->GetGold(), 5);
	TestTrue(TEXT("Pickup click marks collected"), Run->IsPickupCollected(Pickup->PersistentId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupCannotCollectTwiceSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupCannotBeCollectedTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupCannotCollectTwiceSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.NoRepeat");
	Pickup->GoldAmount = 2;
	Pickup->bDestroyWhenCollected = false;

	TestTrue(TEXT("First pickup succeeds"), Pickup->TryInteract_Implementation(PC.Get()));
	TestFalse(TEXT("Second pickup rejected"), Pickup->TryInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Gold added once"), Run->GetGold(), 2);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupEKeyCollectsSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.EKeyRunCardPickupAddsCardAndMarksCollected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupEKeyCollectsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());
	PC->SetPawn(Pawn.Get());
	Card->CardId = TEXT("Pickup.Card.UI.EKeyReward");

	Pickup->PersistentId = TEXT("Pickup.Card.UI.EKey");
	Pickup->CardDefinition = Card.Get();
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	PC->RegisterCandidateInteractable(Pickup.Get());

	TestTrue(TEXT("E key card pickup is closest candidate"), PC->ReadClosestInteractable() == Pickup.Get());
	PC->TryInteractFromConsole();

	TestEqual(TEXT("Card pickup adds expected card"),
		CountUiStorageDefinitions(Run->BuildBackpackStorageSnapshot(), Card.Get()), 1);
	TestTrue(TEXT("Card pickup marked collected"), Run->IsPickupCollected(Pickup->PersistentId));
	TestFalse(TEXT("Collected card pickup can no longer interact"),
		Pickup->CanInteract_Implementation(PC.Get()));
	TestNull(TEXT("Collected card pickup unregisters E candidate"), PC->ReadClosestInteractable());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupClickCollectsWithoutOverlapSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.LeftClickRunCardPickupAddsCardWithoutOverlapCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupClickCollectsWithoutOverlapSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());
	Card->CardId = TEXT("Pickup.Card.UI.ClickReward");

	Pickup->PersistentId = TEXT("Pickup.Card.UI.ClickFar");
	Pickup->CardDefinition = Card.Get();
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	TestNull(TEXT("No E-key candidate registered"), PC->ReadClosestInteractable());
	TestTrue(TEXT("Far card pickup click routes"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Card pickup click adds card"),
		CountUiStorageDefinitions(Run->BuildBackpackStorageSnapshot(), Card.Get()), 1);
	TestTrue(TEXT("Card pickup click marks collected"), Run->IsPickupCollected(Pickup->PersistentId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupCannotCollectTwiceSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.RunCardPickupCannotBeCollectedTwice",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupCannotCollectTwiceSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());
	Card->CardId = TEXT("Pickup.Card.UI.NoRepeatReward");

	Pickup->PersistentId = TEXT("Pickup.Card.UI.NoRepeat");
	Pickup->CardDefinition = Card.Get();
	Pickup->bDestroyWhenCollected = false;

	TestTrue(TEXT("First card pickup succeeds"), Pickup->TryInteract_Implementation(PC.Get()));
	TestFalse(TEXT("Second card pickup rejected"), Pickup->TryInteract_Implementation(PC.Get()));
	TestEqual(TEXT("Card added once"),
		CountUiStorageDefinitions(Run->BuildBackpackStorageSnapshot(), Card.Get()), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeRoutesToInteractSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.LeftClickShopTargetRoutesToTryInteract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeRoutesToInteractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.ClickRoute");
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());

	TestTrue(TEXT("Click route succeeds"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract called once"), Shop->TryInteractCountForTest, 1);
	TestTrue(TEXT("TryInteract receives PC"),
		Shop->GetLastInteractingPlayerControllerForTest() == PC.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeWithoutOverlapSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.LeftClickShopTargetCanOpenWithoutOverlapCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeWithoutOverlapSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.ClickFar");
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());

	TestNull(TEXT("No E-key candidate registered"), PC->ReadClosestInteractable());
	TestTrue(TEXT("Far click still routes"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract called by far click"), Shop->TryInteractCountForTest, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeRoutesToInteractSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.LeftClickBattleTargetRoutesToTryInteract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeRoutesToInteractSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ClickRoute");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());

	TestTrue(TEXT("Battle click route succeeds"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract called once"), Battle->TryInteractCountForTest, 1);
	TestTrue(TEXT("TryInteract receives PC"),
		Battle->GetLastInteractingPlayerControllerForTest() == PC.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeWithoutOverlapSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.LeftClickBattleTargetCanOpenWithoutOverlapCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeWithoutOverlapSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ClickFar");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());

	TestNull(TEXT("No E-key candidate registered"), PC->ReadClosestInteractable());
	TestTrue(TEXT("Far battle click still routes"), PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract called by far click"), Battle->TryInteractCountForTest, 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractRoutesClickableSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.GenericClickableActorRoutesThroughSharedResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractRoutesClickableSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>());
	Target->StableIdForTest = TEXT("Run.Generic.ClickRoute");
	Target->SyncClickTargetForTest();
	Target->ClickTargetBridge->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Target.Get(), Target->ClickBounds);

	TestTrue(TEXT("Clickable world interactable routes"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Clickable route calls TryInteract"),
		Target->TryInteractCountForTest,
		1);
	TestTrue(TEXT("Clickable route passes PC"),
		Target->GetLastInteractingPlayerControllerForTest() == PC.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldGenericClickableHoverSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.GenericClickableHoverUsesSharedResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldGenericClickableHoverSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>());
	Target->StableIdForTest = TEXT("Run.Generic.Hover");
	Target->HoverPromptForTest = FText::FromString(TEXT("点击测试通用目标"));
	Target->SyncClickTargetForTest();
	Target->ClickTargetBridge->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Target.Get(), Target->ClickBounds);

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestEqual(TEXT("Generic hover prompt uses clickable contract"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击测试通用目标")));
	const FString Summary = PC->ReadRunWorldInteractableHoverDebugSummaryForTest();
	TestTrue(TEXT("Generic hover debug reports stable id"),
		Summary.Contains(TEXT("StableId=Run.Generic.Hover")));
	TestTrue(TEXT("Generic hover debug reports shared debug"),
		Summary.Contains(TEXT("Debug=RunWorldClickable")));
	TestTrue(TEXT("Generic hover activates bridge preview"),
		Target->ClickTargetBridge->IsProbePreviewActive());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractRoutesPickupSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.ControllerRoutesPickupThroughSharedResolver",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractRoutesPickupSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.SharedResolver");
	Pickup->GoldAmount = 1;
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	TestTrue(TEXT("Pickup clickable world interactable routes"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Pickup route applies gold"), Run->GetGold(), 1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractRoutesBattleSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.ControllerRoutesBattleTriggerThroughClickableContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractRoutesBattleSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ClickableContract");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());

	TestTrue(TEXT("Battle clickable world interactable routes"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Battle clickable route calls TryInteract"),
		Battle->TryInteractCountForTest,
		1);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractRejectsNonClickableSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.ResolverRejectsRunObjectWithoutClickableContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractRejectsNonClickableSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunWorldNonClickableInteractableProbe* Owner =
		World->SpawnActor<AWacomRunWorldNonClickableInteractableProbe>(
			AWacomRunWorldNonClickableInteractableProbe::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Non-clickable interactable spawned"), Owner))
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
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.WorldInteractableOnly"));
	TestTrue(TEXT("World interactable run target configured"),
		Bridge->RefreshRunWorldTargetBinding());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);

	TestFalse(TEXT("World interactable without clickable contract does not route"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Rejected target does not receive TryInteract"),
		Owner->TryInteractCountForTest,
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldGenericRejectsNoWorldInteractableSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.ResolverRejectsRunObjectWithoutWorldInteractable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldGenericRejectsNoWorldInteractableSpec::RunTest(const FString& /*Parameters*/)
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
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.NoWorldContract"));
	TestTrue(TEXT("Run target configured"), Bridge->RefreshRunWorldTargetBinding());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);
	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestFalse(TEXT("Run target without world interactable does not route"),
		PC->RouteRunWorldInteractableClickForTest());
	TestTrue(TEXT("Hover debug records missing world interactable contract"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(
			TEXT("Reason=MissingWorldInteractableContract")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldGenericRejectsWrongTagSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.ResolverRejectsWrongTargetTag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldGenericRejectsWrongTagSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>());
	Target->StableIdForTest = TEXT("Run.Generic.WrongTag");
	Target->SyncClickTargetForTest();
	Target->ClickTargetBridge->RefreshRunWorldTargetBinding();
	Target->ClickInteractionTarget->SetInteractionTargetTag(WacomTags::Interaction_Target_Battle_EnemyPart);
	PC->SetRunSceneHitForTest(Target.Get(), Target->ClickBounds);

	FWacomInteractionTargetHandle Handle;
	TestFalse(TEXT("Wrong tag is rejected by Run scene probe"),
		PC->ProbeRunSceneTargetForTest(Handle));
	TestFalse(TEXT("Wrong tag target does not click route"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Wrong tag target is not interacted with"),
		Target->TryInteractCountForTest,
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickBridgeRejectsUnsupportedSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickBridge.LeftClickRejectsUnsupportedRunTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickBridgeRejectsUnsupportedSpec::RunTest(const FString& /*Parameters*/)
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
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.NotRunEvent"));
	TestTrue(TEXT("Non-RunEvent run target configured"), Bridge->RefreshRunWorldTargetBinding());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);

	TestFalse(TEXT("Non-RunEvent run target does not route"),
		PC->RouteRunWorldInteractableClickForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeIgnoredOutsideExplorationSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.LeftClickIgnoredOutsideExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeIgnoredOutsideExplorationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.ClickNotExploration");
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());
	PC->SetRunProbeExplorationFlowForTest(false);

	TestFalse(TEXT("Click route ignored outside exploration"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract not called"), Shop->TryInteractCountForTest, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeIgnoredOutsideExplorationSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.LeftClickIgnoredOutsideExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeIgnoredOutsideExplorationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ClickNotExploration");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());
	PC->SetRunProbeExplorationFlowForTest(false);

	TestFalse(TEXT("Battle click route ignored outside exploration"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract not called"), Battle->TryInteractCountForTest, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeIgnoredWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.LeftClickIgnoredWhenGameMenuActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeIgnoredWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Shop->PersistentId = TEXT("Shop.UI.ClickMenuBlocked");
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());

	TestFalse(TEXT("Click route ignored while menu active"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract not called while menu active"),
		Shop->TryInteractCountForTest,
		0);

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeIgnoredWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.LeftClickIgnoredWhenGameMenuActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeIgnoredWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Battle->PersistentId = TEXT("Battle.UI.ClickMenuBlocked");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());

	TestFalse(TEXT("Battle click route ignored while menu active"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract not called while menu active"),
		Battle->TryInteractCountForTest,
		0);

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeIgnoredOutsideExplorationSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.LeftClickIgnoredOutsideExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeIgnoredOutsideExplorationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.ClickNotExploration");
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->SetRunProbeExplorationFlowForTest(false);

	TestFalse(TEXT("Click route ignored outside exploration"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract not called"), Trigger->TryInteractCountForTest, 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeIgnoredWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.LeftClickIgnoredWhenGameMenuActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeIgnoredWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Trigger->PersistentId = TEXT("Event.UI.ClickMenuBlocked");
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());

	TestFalse(TEXT("Click route ignored while menu active"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("TryInteract not called while menu active"),
		Trigger->TryInteractCountForTest,
		0);

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventClickBridgeEKeyStillUsesClosestCandidateSpec,
	"Wacom.UI.WorldInteraction.RunEventClickBridge.EKeyInteractStillUsesClosestCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventClickBridgeEKeyStillUsesClosestCandidateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Near(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Far(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(PC.Get()));
	InjectRunSession(PC.Get(), Run.Get());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Near->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Near->PersistentId = TEXT("Event.UI.EKeyNear");
	Near->EventDefinition = Event.Get();
	Far->SetActorLocation(FVector(500.f, 0.f, 0.f));
	Far->PersistentId = TEXT("Event.UI.EKeyFar");
	Far->EventDefinition = Event.Get();

	PC->RegisterCandidateInteractable(Far.Get());
	PC->RegisterCandidateInteractable(Near.Get());
	TestTrue(TEXT("E-key nearest candidate remains the closest registered RunEvent"),
		PC->ReadClosestInteractable() == Near.Get());
	PC->TryInteractFromConsole();

	TestEqual(TEXT("Near candidate receives E-key interact"),
		Near->TryInteractCountForTest,
		1);
	TestEqual(TEXT("Far candidate not used by E-key"),
		Far->TryInteractCountForTest,
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopClickBridgeEKeyStillUsesClosestCandidateSpec,
	"Wacom.UI.WorldInteraction.ShopClickBridge.EKeyInteractStillUsesClosestCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopClickBridgeEKeyStillUsesClosestCandidateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Near(
		NewObject<AWacomShopTriggerClickProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Far(
		NewObject<AWacomShopTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Near->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Near->PersistentId = TEXT("Shop.UI.EKeyNear");
	Far->SetActorLocation(FVector(500.f, 0.f, 0.f));
	Far->PersistentId = TEXT("Shop.UI.EKeyFar");

	PC->RegisterCandidateInteractable(Far.Get());
	PC->RegisterCandidateInteractable(Near.Get());
	TestTrue(TEXT("E-key nearest candidate remains the closest registered Shop"),
		PC->ReadClosestInteractable() == Near.Get());
	PC->TryInteractFromConsole();

	TestEqual(TEXT("Near shop receives E-key interact"),
		Near->TryInteractCountForTest,
		1);
	TestEqual(TEXT("Far shop not used by E-key"),
		Far->TryInteractCountForTest,
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerClickBridgeEKeyStillUsesClosestCandidateSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerClickBridge.EKeyInteractStillUsesClosestCandidate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerClickBridgeEKeyStillUsesClosestCandidateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Near(
		NewObject<AWacomBattleTriggerClickProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Far(
		NewObject<AWacomBattleTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Near->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Near->PersistentId = TEXT("Battle.UI.EKeyNear");
	Near->EnemyDef = NewObject<UEnemyDefinition>(Near.Get());
	Far->SetActorLocation(FVector(500.f, 0.f, 0.f));
	Far->PersistentId = TEXT("Battle.UI.EKeyFar");
	Far->EnemyDef = NewObject<UEnemyDefinition>(Far.Get());

	PC->RegisterCandidateInteractable(Far.Get());
	PC->RegisterCandidateInteractable(Near.Get());
	TestTrue(TEXT("E-key nearest candidate remains the closest registered Battle trigger"),
		PC->ReadClosestInteractable() == Near.Get());
	PC->TryInteractFromConsole();

	TestEqual(TEXT("Near battle receives E-key interact"),
		Near->TryInteractCountForTest,
		1);
	TestEqual(TEXT("Far battle not used by E-key"),
		Far->TryInteractCountForTest,
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractHoverPromptSpec,
	"Wacom.UI.WorldInteraction.RunWorldClickContract.HoverPromptUsesClickableContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractHoverPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.ContractHover");
	Trigger->EventDefinition = MakeUiRunEvent(Trigger.Get());
	Trigger->HoverPromptText = FText::FromString(TEXT("接口点击提示"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestEqual(TEXT("Hover prompt comes from clickable contract"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("接口点击提示")));
	TestTrue(TEXT("Hover debug uses ok reason"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=Ok")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldClickContractSharedDebugSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.GenericClickableDebugReportsStableIdVisualAndReason",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldClickContractSharedDebugSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>());
	Target->StableIdForTest = TEXT("Run.Generic.Debug");
	Target->HoverPromptForTest = FText::FromString(TEXT("共享调试通用目标"));
	Target->LastDebugResultForTest = TEXT("BlockedForTest");
	Target->SyncClickTargetForTest();
	Target->ClickTargetBridge->RefreshRunWorldTargetBinding();

	const FWacomRunWorldClickableInteractableDebugView Debug =
		Target->GetRunWorldClickableDebugView_Implementation(nullptr);

	TestEqual(TEXT("Shared debug reports actor name"),
		Debug.ActorName,
		Target->GetName());
	TestEqual(TEXT("Shared debug reports stable id"),
		Debug.StableId,
		FName(TEXT("Run.Generic.Debug")));
	TestTrue(TEXT("Shared debug reports stable id present"),
		Debug.bHasStableId);
	TestTrue(TEXT("Shared debug reports world interactable contract"),
		Debug.bImplementsWorldInteractable);
	TestTrue(TEXT("Shared debug reports clickable contract"),
		Debug.bImplementsClickableContract);
	TestEqual(TEXT("Shared debug reports bridge stable id"),
		Debug.ClickTargetStableId,
		FName(TEXT("Run.Generic.Debug")));
	TestEqual(TEXT("Shared debug reports prompt"),
		Debug.HoverPrompt,
		FString(TEXT("共享调试通用目标")));
	TestTrue(TEXT("Shared debug reports configured click target"),
		Debug.bClickTargetConfigured);
	TestTrue(TEXT("Shared debug reports configured click bounds"),
		Debug.bClickBoundsConfigured);
	TestTrue(TEXT("Shared debug reports interaction target"),
		Debug.bHasInteractionTargetComponent);
	TestTrue(TEXT("Shared debug reports bridge"),
		Debug.bHasBridgeComponent);
	TestTrue(TEXT("Shared debug reports visual target"),
		Debug.bHasVisualTarget);
	TestTrue(TEXT("Shared debug reports renderable visual target"),
		Debug.bHasRenderableVisualTarget);
	TestEqual(TEXT("Shared debug reports visual target name"),
		Debug.VisualTargetName,
		FName(TEXT("Visual")));
	TestEqual(TEXT("Shared debug reports reject reason"),
		Debug.RejectReason,
		FName(TEXT("BlockedForTest")));
	TestTrue(TEXT("Shared debug summary includes renderable visual"),
		FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(Debug).Contains(
			TEXT("HasRenderableVisual=true")));
	TestTrue(TEXT("Shared debug summary includes click bounds"),
		FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(Debug).Contains(
			TEXT("ClickBounds=true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldGenericMissingRenderableSpec,
	"Wacom.UI.WorldInteraction.RunWorldGenericClickableContract.MissingRenderableVisualReportsDebugButDoesNotBlockClick",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldGenericMissingRenderableSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomGenericRunWorldClickableInteractableProbe> Target(
		NewObject<AWacomGenericRunWorldClickableInteractableProbe>());
	Target->StableIdForTest = TEXT("Run.Generic.NoVisual");
	Target->Visual->SetVisibility(false);
	Target->SyncClickTargetForTest();
	Target->ClickTargetBridge->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Target.Get(), Target->ClickBounds);

	TestTrue(TEXT("Missing renderable visual does not block click route"),
		PC->RouteRunWorldInteractableClickForTest());
	TestEqual(TEXT("Missing renderable visual still calls TryInteract"),
		Target->TryInteractCountForTest,
		1);

	const FWacomRunWorldClickableInteractableDebugView Debug =
		Target->GetRunWorldClickableDebugView_Implementation(PC.Get());
	TestFalse(TEXT("Debug reports no renderable visual"),
		Debug.bHasRenderableVisualTarget);
	TestTrue(TEXT("Debug summary reports no renderable visual"),
		FWacomRunWorldClickableInteractableHelper::BuildDebugSummary(Debug).Contains(
			TEXT("HasRenderableVisual=false")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptShowsClickPromptSpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.RunEventHoverShowsClickPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptShowsClickPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.Hover");
	Trigger->HoverPromptText = FText::FromString(TEXT("点击测试事件"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Hover prompt wins current interaction prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击测试事件")));
	TestTrue(TEXT("Hover debug reports actor"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(Trigger->GetName()));
	TestTrue(TEXT("Hover debug reports stable id"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("StableId=Event.UI.Hover")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverActivatesSharedProbeVisualSpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.RunEventHoverActivatesSharedProbeVisualSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverActivatesSharedProbeVisualSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.VisualSignal");
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(Trigger.Get());
	Trigger->AddInstanceComponent(Visual);
	Visual->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));
	Visual->SetRenderCustomDepth(false);
	Trigger->GetClickTargetBridgeComponent()->ProbePreviewScale = 1.1f;
	Trigger->GetClickTargetBridgeComponent()->ProbeCustomDepthStencilValue = 250;
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestTrue(TEXT("RunEvent bridge preview active"),
		Trigger->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestEqual(TEXT("RunEvent visual scaled by shared probe"),
		Visual->GetRelativeScale3D(), FVector(2.2f, 2.2f, 2.2f));
	TestTrue(TEXT("RunEvent visual custom depth enabled"), Visual->bRenderCustomDepth);
	TestEqual(TEXT("RunEvent visual stencil applied"), Visual->CustomDepthStencilValue, 250);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptCompletedSpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.CompletedRunEventHoverShowsWeakPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptCompletedSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(MakeUiRunEvent(Trigger.Get()));
	InjectRunSession(PC.Get(), Run.Get());

	Trigger->PersistentId = TEXT("Event.UI.HoverCompleted");
	Trigger->EventDefinition = Event.Get();
	Trigger->CompletedHoverPromptText = FText::FromString(TEXT("完成后点击提示"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	TestTrue(TEXT("Begin event succeeds"), Run->BeginRunEvent(Trigger->PersistentId, Event.Get()));
	TestTrue(TEXT("Complete event via option"), Run->ChooseRunEventOption(TEXT("Close")));

	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestEqual(TEXT("Completed hover prompt shown"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("完成后点击提示")));
	TestTrue(TEXT("Hover debug reports completed"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Completed=true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverPromptShowsClickPromptSpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.ShopHoverShowsClickPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverPromptShowsClickPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.Hover");
	Shop->HoverPromptText = FText::FromString(TEXT("点击测试商店"));
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Shop hover prompt wins current interaction prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击测试商店")));
	TestTrue(TEXT("Hover debug reports shop actor"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(Shop->GetName()));
	TestTrue(TEXT("Hover debug reports shop stable id"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("StableId=Shop.UI.Hover")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverActivatesSharedProbeVisualSpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.ShopHoverActivatesSharedProbeVisualSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverActivatesSharedProbeVisualSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.VisualSignal");
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(Shop.Get());
	Shop->AddInstanceComponent(Visual);
	Visual->SetRelativeScale3D(FVector(3.0f, 3.0f, 3.0f));
	Visual->SetRenderCustomDepth(false);
	Shop->GetClickTargetBridgeComponent()->ProbePreviewScale = 1.1f;
	Shop->GetClickTargetBridgeComponent()->ProbeCustomDepthStencilValue = 251;
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestTrue(TEXT("Shop bridge preview active"),
		Shop->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestEqual(TEXT("Shop visual scaled by shared probe"),
		Visual->GetRelativeScale3D(), FVector(3.3f, 3.3f, 3.3f));
	TestTrue(TEXT("Shop visual custom depth enabled"), Visual->bRenderCustomDepth);
	TestEqual(TEXT("Shop visual stencil applied"), Visual->CustomDepthStencilValue, 251);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupHoverPromptShowsClickPromptSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupHoverShowsClickPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupHoverPromptShowsClickPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.Hover");
	Pickup->HoverPromptText = FText::FromString(TEXT("点击测试拾取"));
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Pickup hover prompt wins current interaction prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击测试拾取")));
	TestTrue(TEXT("Hover debug reports pickup actor"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(Pickup->GetName()));
	TestTrue(TEXT("Hover debug reports pickup stable id"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("StableId=Pickup.UI.Hover")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupHoverActivatesSharedProbeVisualSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupHoverActivatesSharedProbeVisualSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupHoverActivatesSharedProbeVisualSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.VisualSignal");
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	Pickup->GetPickupVisual()->SetRelativeScale3D(FVector(1.5f, 1.5f, 1.5f));
	Pickup->GetPickupVisual()->SetRenderCustomDepth(false);
	Pickup->GetClickTargetBridgeComponent()->ProbePreviewScale = 1.1f;
	Pickup->GetClickTargetBridgeComponent()->ProbeCustomDepthStencilValue = 253;
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestTrue(TEXT("Pickup bridge preview active"),
		Pickup->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestEqual(TEXT("Pickup visual scaled by shared probe"),
		Pickup->GetPickupVisual()->GetRelativeScale3D(), FVector(1.65f, 1.65f, 1.65f));
	TestTrue(TEXT("Pickup visual custom depth enabled"), Pickup->GetPickupVisual()->bRenderCustomDepth);
	TestEqual(TEXT("Pickup visual stencil applied"), Pickup->GetPickupVisual()->CustomDepthStencilValue, 253);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerHoverPromptShowsClickPromptSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.BattleHoverShowsClickPrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerHoverPromptShowsClickPromptSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.Hover");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->HoverPromptText = FText::FromString(TEXT("点击测试战斗"));
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Battle hover prompt wins current interaction prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击测试战斗")));
	TestTrue(TEXT("Hover debug reports battle actor"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(Battle->GetName()));
	TestTrue(TEXT("Hover debug reports battle stable id"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("StableId=Battle.UI.Hover")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHoverActivatesSharedProbeVisualSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.BattleHoverActivatesSharedProbeVisualSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHoverActivatesSharedProbeVisualSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.VisualSignal");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(Battle.Get());
	Battle->AddInstanceComponent(Visual);
	Visual->SetRelativeScale3D(FVector(4.0f, 4.0f, 4.0f));
	Visual->SetRenderCustomDepth(false);
	Battle->GetClickTargetBridgeComponent()->ProbePreviewScale = 1.1f;
	Battle->GetClickTargetBridgeComponent()->ProbeCustomDepthStencilValue = 252;
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestTrue(TEXT("Battle bridge preview active"),
		Battle->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestEqual(TEXT("Battle visual scaled by shared probe"),
		Visual->GetRelativeScale3D(), FVector(4.4f, 4.4f, 4.4f));
	TestTrue(TEXT("Battle visual custom depth enabled"), Visual->bRenderCustomDepth);
	TestEqual(TEXT("Battle visual stencil applied"), Visual->CustomDepthStencilValue, 252);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptOverridesEKeySpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.HoverPromptOverridesEKeyCandidatePrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptOverridesEKeySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Shop->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Shop->PersistentId = TEXT("Shop.HoverOverride");
	Shop->InteractPromptText = FText::FromString(TEXT("按 E 商店"));
	Trigger->PersistentId = TEXT("Event.UI.HoverOverride");
	Trigger->HoverPromptText = FText::FromString(TEXT("点击事件优先"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	PC->RegisterCandidateInteractable(Shop.Get());
	TestEqual(TEXT("E-key prompt starts active"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 商店")));

	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Hover prompt overrides E-key prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击事件优先")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptClearingRestoresEKeySpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.ClearingHoverRestoresEKeyCandidatePrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptClearingRestoresEKeySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Shop->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Shop->PersistentId = TEXT("Shop.HoverRestore");
	Shop->InteractPromptText = FText::FromString(TEXT("按 E 商店"));
	Trigger->PersistentId = TEXT("Event.UI.HoverRestore");
	Trigger->HoverPromptText = FText::FromString(TEXT("点击事件"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	PC->RegisterCandidateInteractable(Shop.Get());
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Hover prompt active"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击事件")));

	PC->ClearRunSceneHitForTest();
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("E-key prompt restored after hover clears"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 商店")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverPromptOverridesEKeySpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.ShopHoverOverridesEKeyCandidatePrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverPromptOverridesEKeySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> EventTrigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	EventTrigger->SetActorLocation(FVector(50.f, 0.f, 0.f));
	EventTrigger->PersistentId = TEXT("Event.UI.ShopHoverOverride");
	EventTrigger->EventDefinition = MakeUiRunEvent(EventTrigger.Get());
	EventTrigger->InteractPromptText = FText::FromString(TEXT("按 E 事件"));
	Shop->PersistentId = TEXT("Shop.UI.HoverOverride");
	Shop->HoverPromptText = FText::FromString(TEXT("点击商店优先"));
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	PC->RegisterCandidateInteractable(EventTrigger.Get());
	TestEqual(TEXT("E-key prompt starts active"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 事件")));

	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Shop hover prompt overrides E-key prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击商店优先")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerHoverPromptOverridesEKeySpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.BattleHoverOverridesEKeyCandidatePrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerHoverPromptOverridesEKeySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Shop->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Shop->PersistentId = TEXT("Shop.BattleHoverOverride");
	Shop->InteractPromptText = FText::FromString(TEXT("按 E 商店"));
	Battle->PersistentId = TEXT("Battle.UI.HoverOverride");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->HoverPromptText = FText::FromString(TEXT("点击战斗优先"));
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	PC->RegisterCandidateInteractable(Shop.Get());
	TestEqual(TEXT("E-key prompt starts active"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 商店")));

	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Battle hover prompt overrides E-key prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击战斗优先")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverPromptClearingRestoresEKeySpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.ClearingShopHoverRestoresEKeyCandidatePrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverPromptClearingRestoresEKeySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> EventTrigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	EventTrigger->SetActorLocation(FVector(50.f, 0.f, 0.f));
	EventTrigger->PersistentId = TEXT("Event.UI.ShopHoverRestore");
	EventTrigger->EventDefinition = MakeUiRunEvent(EventTrigger.Get());
	EventTrigger->InteractPromptText = FText::FromString(TEXT("按 E 事件"));
	Shop->PersistentId = TEXT("Shop.UI.HoverRestore");
	Shop->HoverPromptText = FText::FromString(TEXT("点击商店"));
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	PC->RegisterCandidateInteractable(EventTrigger.Get());
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Shop hover prompt active"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击商店")));

	PC->ClearRunSceneHitForTest();
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("E-key prompt restored after shop hover clears"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 事件")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerHoverPromptClearingRestoresEKeySpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.ClearingBattleHoverRestoresEKeyCandidatePrompt",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerHoverPromptClearingRestoresEKeySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<APawn> Pawn(NewObject<APawn>());
	TStrongObjectPtr<AWacomShopTriggerActor> Shop(NewObject<AWacomShopTriggerActor>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());

	Pawn->SetActorLocation(FVector::ZeroVector);
	PC->SetPawn(Pawn.Get());
	Shop->SetActorLocation(FVector(50.f, 0.f, 0.f));
	Shop->PersistentId = TEXT("Shop.BattleHoverRestore");
	Shop->InteractPromptText = FText::FromString(TEXT("按 E 商店"));
	Battle->PersistentId = TEXT("Battle.UI.HoverRestore");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->HoverPromptText = FText::FromString(TEXT("点击战斗"));
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	PC->RegisterCandidateInteractable(Shop.Get());
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("Battle hover prompt active"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击战斗")));

	PC->ClearRunSceneHitForTest();
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestEqual(TEXT("E-key prompt restored after battle hover clears"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("按 E 商店")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptIgnoredOutsideExplorationSpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.HoverPromptIgnoredOutsideExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptIgnoredOutsideExplorationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.HoverNotExploration");
	Trigger->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->SetRunProbeExplorationFlowForTest(false);

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Hover prompt ignored outside exploration"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records not exploration"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=NotInExploration")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverPromptIgnoredOutsideExplorationSpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.ShopHoverIgnoredOutsideExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverPromptIgnoredOutsideExplorationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.HoverNotExploration");
	Shop->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());
	PC->SetRunProbeExplorationFlowForTest(false);

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Shop hover prompt ignored outside exploration"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records not exploration"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=NotInExploration")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerHoverPromptIgnoredOutsideExplorationSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.BattleHoverIgnoredOutsideExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerHoverPromptIgnoredOutsideExplorationSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.HoverNotExploration");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());
	PC->SetRunProbeExplorationFlowForTest(false);

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Battle hover prompt ignored outside exploration"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records not exploration"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=NotInExploration")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptIgnoredWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.HoverPromptIgnoredWhenGameMenuActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptIgnoredWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Trigger->PersistentId = TEXT("Event.UI.HoverMenuBlocked");
	Trigger->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Hover prompt ignored while menu active"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records menu block"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=BlockedByMenuOrDrag"))
		|| PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=GameMenuActive")));

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldProbeVisualClearsWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.RunWorldHoverPrompt.GameMenuClearsRunWorldProbeVisualSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldProbeVisualClearsWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Trigger->PersistentId = TEXT("Event.UI.VisualMenuBlocked");
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	UStaticMeshComponent* Visual = NewObject<UStaticMeshComponent>(Trigger.Get());
	Trigger->AddInstanceComponent(Visual);
	Visual->SetRelativeScale3D(FVector(2.0f, 2.0f, 2.0f));
	Visual->SetRenderCustomDepth(false);
	Trigger->GetClickTargetBridgeComponent()->ProbePreviewScale = 1.1f;
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Preview activates before menu"),
		Trigger->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestTrue(TEXT("Visual custom depth active before menu"), Visual->bRenderCustomDepth);

	PC->RegisterActiveGameMenuWidget(Menu.Get());
	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestFalse(TEXT("Preview clears while menu active"),
		Trigger->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestEqual(TEXT("Visual scale restored while menu active"),
		Visual->GetRelativeScale3D(), FVector(2.0f, 2.0f, 2.0f));
	TestFalse(TEXT("Visual custom depth restored while menu active"), Visual->bRenderCustomDepth);

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerHoverPromptIgnoredWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.BattleHoverIgnoredWhenGameMenuActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerHoverPromptIgnoredWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Battle->PersistentId = TEXT("Battle.UI.HoverMenuBlocked");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Battle hover prompt ignored while menu active"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records menu block"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=BlockedByMenuOrDrag"))
		|| PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=GameMenuActive")));

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptIgnoredDuringRunMenuDragSpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.HoverPromptIgnoredDuringRunMenuCardDrag",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptIgnoredDuringRunMenuDragSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Trigger->PersistentId = TEXT("Event.UI.HoverDragBlocked");
	Trigger->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());
	PC->SetRunFirstPersonMenuLeaseForTest();

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Hover prompt ignored while menu lease drag path is active"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records drag/menu block"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=BlockedByMenuOrDrag"))
		|| PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=GameMenuActive")));

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldHoverPromptRejectsUnsupportedSpec,
	"Wacom.UI.WorldInteraction.RunWorldHoverPrompt.HoverPromptRejectsUnsupportedRunTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldHoverPromptRejectsUnsupportedSpec::RunTest(const FString& /*Parameters*/)
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
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.HoverNotRunEvent"));
	TestTrue(TEXT("Non-RunEvent run target configured"), Bridge->RefreshRunWorldTargetBinding());

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);
	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestTrue(TEXT("Non-RunEvent hover prompt rejected"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records missing world interactable contract"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(
			TEXT("Reason=MissingWorldInteractableContract")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunWorldUnsupportedTargetNoVisualSignalSpec,
	"Wacom.UI.WorldInteraction.RunWorldHoverPrompt.UnsupportedRunTargetDoesNotLeaveProbeVisualSignalActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunWorldUnsupportedTargetNoVisualSignalSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	AWacomRunWorldNonClickableInteractableProbe* Owner =
		World->SpawnActor<AWacomRunWorldNonClickableInteractableProbe>(
			AWacomRunWorldNonClickableInteractableProbe::StaticClass(),
			FTransform::Identity);
	if (!TestNotNull(TEXT("Non-clickable interactable spawned"), Owner))
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
		AddRunWorldBridgeComponent(*Owner, TEXT("Run.Target.UnsupportedVisual"));
	UStaticMeshComponent* Visual = AddVisualComponent(*Owner, FVector(2.0f, 2.0f, 2.0f));
	Visual->SetRenderCustomDepth(false);
	Bridge->RefreshRunWorldTargetBinding();

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	PC->SetRunSceneHitForTest(Owner);
	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestFalse(TEXT("Unsupported target does not keep preview active"),
		Bridge->IsProbePreviewActive());
	TestEqual(TEXT("Unsupported target scale is unchanged"),
		Visual->GetRelativeScale3D(), FVector(2.0f, 2.0f, 2.0f));
	TestFalse(TEXT("Unsupported target custom depth is unchanged"), Visual->bRenderCustomDepth);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverPromptIgnoredWhenGameMenuActiveSpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.ShopHoverIgnoredWhenGameMenuActive",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverPromptIgnoredWhenGameMenuActiveSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	TStrongObjectPtr<UWacomMenuWidgetBaseProbe> Menu(NewObject<UWacomMenuWidgetBaseProbe>());
	Shop->PersistentId = TEXT("Shop.UI.HoverMenuBlocked");
	Shop->HoverPromptText = FText::FromString(TEXT("不应显示"));
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());
	PC->RegisterActiveGameMenuWidget(Menu.Get());

	PC->UpdateRunWorldTargetProbePreviewForTest();
	TestTrue(TEXT("Shop hover prompt ignored while menu active"),
		PC->ReadCurrentInteractPrompt().IsEmpty());
	TestTrue(TEXT("Hover debug records menu block"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=BlockedByMenuOrDrag"))
		|| PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(TEXT("Reason=GameMenuActive")));

	PC->UnregisterActiveGameMenuWidget(Menu.Get());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIShopHoverPromptDebugSummarySpec,
	"Wacom.UI.WorldInteraction.ShopHoverPrompt.HoverDebugSummaryReportsShopActorPromptAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIShopHoverPromptDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomShopTriggerClickProbe> Shop(
		NewObject<AWacomShopTriggerClickProbe>());
	Shop->PersistentId = TEXT("Shop.UI.HoverDebug");
	Shop->HoverPromptText = FText::FromString(TEXT("点击调试商店"));
	Shop->SyncClickTargetForTest();
	Shop->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Shop.Get(), Shop->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	const FString Summary = PC->ReadRunWorldInteractableHoverDebugSummaryForTest();
	TestTrue(TEXT("Summary reports shop actor"), Summary.Contains(Shop->GetName()));
	TestTrue(TEXT("Summary reports shop prompt"), Summary.Contains(TEXT("Prompt=点击调试商店")));
	TestTrue(TEXT("Summary reports shop stable id"), Summary.Contains(TEXT("StableId=Shop.UI.HoverDebug")));
	TestTrue(TEXT("Summary reports target handle"), Summary.Contains(TEXT("Target=")));

	const FString ShopSummary = Shop->GetShopTriggerDebugSummary(PC.Get());
	TestTrue(TEXT("Shop debug reports click target"), ShopSummary.Contains(TEXT("ClickTarget=true")));
	TestTrue(TEXT("Shop debug reports hover prompt"),
		ShopSummary.Contains(TEXT("HoverPrompt=点击调试商店")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleTriggerHoverPromptDebugSummarySpec,
	"Wacom.UI.WorldInteraction.BattleTriggerHoverPrompt.HoverDebugSummaryReportsBattleActorPromptAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleTriggerHoverPromptDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomBattleTriggerClickProbe> Battle(
		NewObject<AWacomBattleTriggerClickProbe>());
	Battle->PersistentId = TEXT("Battle.UI.HoverDebug");
	Battle->EnemyDef = NewObject<UEnemyDefinition>(Battle.Get());
	Battle->HoverPromptText = FText::FromString(TEXT("点击调试战斗"));
	Battle->SyncClickTargetForTest();
	Battle->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Battle.Get(), Battle->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	const FString Summary = PC->ReadRunWorldInteractableHoverDebugSummaryForTest();
	TestTrue(TEXT("Summary reports battle actor"), Summary.Contains(Battle->GetName()));
	TestTrue(TEXT("Summary reports battle prompt"), Summary.Contains(TEXT("Prompt=点击调试战斗")));
	TestTrue(TEXT("Summary reports battle stable id"), Summary.Contains(TEXT("StableId=Battle.UI.HoverDebug")));
	TestTrue(TEXT("Summary reports target handle"), Summary.Contains(TEXT("Target=")));

	const FString BattleSummary = Battle->GetBattleTriggerDebugSummary(PC.Get());
	TestTrue(TEXT("Battle debug reports click target"), BattleSummary.Contains(TEXT("ClickTarget=true")));
	TestTrue(TEXT("Battle debug reports hover prompt"),
		BattleSummary.Contains(TEXT("HoverPrompt=点击调试战斗")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupDebugSummarySpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupDebugSummaryReportsGoldCollectedAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.Debug");
	Pickup->GoldAmount = 7;
	Pickup->HoverPromptText = FText::FromString(TEXT("点击调试拾取"));
	Pickup->bDestroyWhenCollected = false;
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	const FString HoverSummary = PC->ReadRunWorldInteractableHoverDebugSummaryForTest();
	TestTrue(TEXT("Hover summary reports pickup actor"), HoverSummary.Contains(Pickup->GetName()));
	TestTrue(TEXT("Hover summary reports pickup prompt"), HoverSummary.Contains(TEXT("Prompt=点击调试拾取")));
	TestTrue(TEXT("Hover summary reports pickup stable id"), HoverSummary.Contains(TEXT("StableId=Pickup.UI.Debug")));

	FString PickupSummary = Pickup->GetRunPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Pickup summary reports gold amount"), PickupSummary.Contains(TEXT("Gold=7")));
	TestTrue(TEXT("Pickup summary reports click target"), PickupSummary.Contains(TEXT("ClickTarget=true")));
	TestTrue(TEXT("Pickup summary reports stable id"),
		PickupSummary.Contains(TEXT("ClickStableId=Pickup.UI.Debug")));
	TestTrue(TEXT("Pickup starts uncollected"), PickupSummary.Contains(TEXT("Collected=false")));

	TestTrue(TEXT("Pickup interaction succeeds"), Pickup->TryInteract_Implementation(PC.Get()));
	PickupSummary = Pickup->GetRunPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Pickup summary reports collected"), PickupSummary.Contains(TEXT("Collected=true")));
	TestTrue(TEXT("Pickup summary reports collected result"), PickupSummary.Contains(TEXT("Last=Collected")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupDebugMissingPersistentIdSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupDebugReportsMissingPersistentId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupDebugMissingPersistentIdSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = NAME_None;
	Pickup->GoldAmount = 3;
	Pickup->SyncClickTargetForTest();

	const FWacomRunPickupDebugView View = Pickup->GetRunPickupDebugView(PC.Get());
	TestFalse(TEXT("Missing id config is invalid"), View.bConfigValid);
	TestEqual(TEXT("Missing id config reason"),
		View.ConfigWarningReason, FName(TEXT("MissingPersistentId")));
	TestFalse(TEXT("Missing id cannot interact"), View.bCanInteract);

	const FString Summary = Pickup->GetRunPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Summary reports invalid config"), Summary.Contains(TEXT("ConfigValid=false")));
	TestTrue(TEXT("Summary reports missing id"),
		Summary.Contains(TEXT("ConfigReason=MissingPersistentId")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupDebugInvalidGoldSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupDebugReportsInvalidGoldAmount",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupDebugInvalidGoldSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.InvalidGold");
	Pickup->GoldAmount = 0;
	Pickup->SyncClickTargetForTest();

	const FWacomRunPickupDebugView View = Pickup->GetRunPickupDebugView(PC.Get());
	TestFalse(TEXT("Invalid gold config is invalid"), View.bConfigValid);
	TestEqual(TEXT("Invalid gold config reason"),
		View.ConfigWarningReason, FName(TEXT("InvalidGoldAmount")));
	TestFalse(TEXT("Invalid gold cannot interact"), View.bCanInteract);

	const FString Summary = Pickup->GetRunPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Summary reports invalid gold"),
		Summary.Contains(TEXT("ConfigReason=InvalidGoldAmount")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupDuplicatePersistentIdSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupDuplicatePersistentIdReportsWarningButDoesNotBlock",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupDuplicatePersistentIdSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomRunPickupClickProbe* First = World->SpawnActor<AWacomRunPickupClickProbe>(
		AWacomRunPickupClickProbe::StaticClass(), FTransform::Identity, SpawnParams);
	AWacomRunPickupClickProbe* Second = World->SpawnActor<AWacomRunPickupClickProbe>(
		AWacomRunPickupClickProbe::StaticClass(), FTransform::Identity, SpawnParams);
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

	if (!TestNotNull(TEXT("First pickup spawned"), First)
		|| !TestNotNull(TEXT("Second pickup spawned"), Second))
	{
		return false;
	}

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	InjectRunSession(PC.Get(), Run.Get());

	First->PersistentId = TEXT("Pickup.UI.Duplicate");
	First->GoldAmount = 2;
	First->bDestroyWhenCollected = false;
	First->SyncClickTargetForTest();
	Second->PersistentId = TEXT("Pickup.UI.Duplicate");
	Second->GoldAmount = 2;
	Second->bDestroyWhenCollected = false;
	Second->SyncClickTargetForTest();

	const FWacomRunPickupDebugView FirstView = First->GetRunPickupDebugView(PC.Get());
	TestTrue(TEXT("Duplicate id detected"), FirstView.bDuplicatePersistentIdDetected);
	TestTrue(TEXT("Duplicate id does not make config invalid"), FirstView.bConfigValid);
	TestTrue(TEXT("Duplicate id remains interactable before collect"),
		First->CanInteract_Implementation(PC.Get()));

	TestTrue(TEXT("First duplicate pickup can collect"),
		First->TryInteract_Implementation(PC.Get()));
	TestFalse(TEXT("Second duplicate pickup shares collected state"),
		Second->CanInteract_Implementation(PC.Get()));

	const FString Summary = Second->GetRunPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Summary reports duplicate"), Summary.Contains(TEXT("Duplicate=true")));
	TestTrue(TEXT("Summary reports collected shared state"), Summary.Contains(TEXT("Collected=true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupDebugVisualAndClickTargetSpec,
	"Wacom.UI.WorldInteraction.RunPickup.RunPickupDebugReportsRenderableVisualAndClickTarget",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupDebugVisualAndClickTargetSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(NewObject<AWacomRunPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.UI.VisualDebug");
	Pickup->GoldAmount = 1;
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();

	const FWacomRunPickupDebugView View = Pickup->GetRunPickupDebugView(PC.Get());
	TestTrue(TEXT("Pickup config valid"), View.bConfigValid);
	TestTrue(TEXT("Pickup reports click target"), View.bClickTargetConfigured);
	TestTrue(TEXT("Pickup reports renderable visual"), View.bHasRenderableVisual);
	TestEqual(TEXT("Pickup click target stable id"),
		View.ClickTargetStableId, FName(TEXT("Pickup.UI.VisualDebug")));

	const FString Summary = Pickup->GetRunPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Summary reports visual"), Summary.Contains(TEXT("HasVisual=true")));
	TestTrue(TEXT("Summary reports click target"), Summary.Contains(TEXT("ClickTarget=true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupConfigureDebugSampleSpec,
	"Wacom.UI.WorldInteraction.RunPickup.ConfigureDebugGoldPickupSampleSetsDefaultsAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupConfigureDebugSampleSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(
		NewObject<AWacomRunPickupClickProbe>(
			GetTransientPackage(),
			FName(TEXT("PickupConfigureProbe"))));

	Pickup->PersistentId = TEXT("Old.Id");
	Pickup->GoldAmount = 99;
	Pickup->TriggerRadius = 500.f;
	Pickup->bDestroyWhenCollected = false;
	Pickup->InteractPromptText = FText::FromString(TEXT("old interact"));
	Pickup->HoverPromptText = FText::FromString(TEXT("old hover"));
	Pickup->CollectedHoverPromptText = FText::FromString(TEXT("old collected"));

	Pickup->ConfigureDebugGoldPickupSample();

	TestEqual(TEXT("Sample persistent id uses actor name"),
		Pickup->PersistentId, FName(TEXT("Pickup.Debug.PickupConfigureProbe")));
	TestEqual(TEXT("Sample gold"), Pickup->GoldAmount, 3);
	TestEqual(TEXT("Sample trigger radius"), Pickup->TriggerRadius, 160.f);
	TestTrue(TEXT("Sample destroys when collected"), Pickup->bDestroyWhenCollected);
	TestEqual(TEXT("Sample interact prompt"),
		Pickup->GetInteractPromptText_Implementation(nullptr).ToString(),
		FString(TEXT("按 E 拾取")));
	TestEqual(TEXT("Sample hover prompt"),
		Pickup->GetHoverPromptText(nullptr).ToString(),
		FString(TEXT("点击拾取")));
	TestEqual(TEXT("Sample click stable id"),
		Pickup->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Pickup.Debug.PickupConfigureProbe")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunPickupConfigureDebugSampleRunStateSpec,
	"Wacom.UI.WorldInteraction.RunPickup.ConfigureDebugGoldPickupSampleDoesNotMutateRunState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunPickupConfigureDebugSampleRunStateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunPickupClickProbe> Pickup(
		NewObject<AWacomRunPickupClickProbe>(
			GetTransientPackage(),
			FName(TEXT("PickupConfigureRunStateProbe"))));
	InjectRunSession(PC.Get(), Run.Get());

	Run->CollectGoldPickup(TEXT("Pickup.Existing"), 5);
	const int32 GoldBefore = Run->GetGold();
	const bool bExistingCollectedBefore = Run->IsPickupCollected(TEXT("Pickup.Existing"));

	Pickup->ConfigureDebugGoldPickupSample();

	TestEqual(TEXT("Configure sample does not change gold"), Run->GetGold(), GoldBefore);
	TestEqual(TEXT("Configure sample preserves existing collected state"),
		Run->IsPickupCollected(TEXT("Pickup.Existing")), bExistingCollectedBefore);
	TestFalse(TEXT("Configure sample does not mark new id collected"),
		Run->IsPickupCollected(Pickup->PersistentId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupHoverPromptAndVisualSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.RunCardPickupHoverShowsClickPromptAndVisualSignal",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupHoverPromptAndVisualSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>());
	InjectRunSession(PC.Get(), Run.Get());

	Pickup->PersistentId = TEXT("Pickup.Card.UI.Hover");
	Pickup->CardDefinition = Card.Get();
	Pickup->HoverPromptText = FText::FromString(TEXT("点击测试卡牌拾取"));
	Pickup->SyncClickTargetForTest();
	Pickup->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	Pickup->GetPickupVisual()->SetRelativeScale3D(FVector(1.25f, 1.25f, 1.25f));
	Pickup->GetPickupVisual()->SetRenderCustomDepth(false);
	Pickup->GetClickTargetBridgeComponent()->ProbePreviewScale = 1.2f;
	Pickup->GetClickTargetBridgeComponent()->ProbeCustomDepthStencilValue = 252;
	PC->SetRunSceneHitForTest(Pickup.Get(), Pickup->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	TestEqual(TEXT("Card pickup hover prompt wins current interaction prompt"),
		PC->ReadCurrentInteractPrompt().ToString(),
		FString(TEXT("点击测试卡牌拾取")));
	TestTrue(TEXT("Card pickup bridge preview active"),
		Pickup->GetClickTargetBridgeComponent()->IsProbePreviewActive());
	TestEqual(TEXT("Card pickup visual scaled by shared probe"),
		Pickup->GetPickupVisual()->GetRelativeScale3D(), FVector(1.5f, 1.5f, 1.5f));
	TestTrue(TEXT("Card pickup visual custom depth enabled"),
		Pickup->GetPickupVisual()->bRenderCustomDepth);
	TestEqual(TEXT("Card pickup visual stencil applied"),
		Pickup->GetPickupVisual()->CustomDepthStencilValue, 252);
	TestTrue(TEXT("Hover debug reports card pickup stable id"),
		PC->ReadRunWorldInteractableHoverDebugSummaryForTest().Contains(
			TEXT("StableId=Pickup.Card.UI.Hover")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupDebugSummarySpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.RunCardPickupDebugReportsCardConfigDuplicateVisualAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindWorldInteractionAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomRunCardPickupClickProbe* First = World->SpawnActor<AWacomRunCardPickupClickProbe>(
		AWacomRunCardPickupClickProbe::StaticClass(), FTransform::Identity, SpawnParams);
	AWacomRunCardPickupClickProbe* Second = World->SpawnActor<AWacomRunCardPickupClickProbe>(
		AWacomRunCardPickupClickProbe::StaticClass(), FTransform::Identity, SpawnParams);
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

	if (!TestNotNull(TEXT("First card pickup spawned"), First)
		|| !TestNotNull(TEXT("Second card pickup spawned"), Second))
	{
		return false;
	}

	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	InjectRunSession(PC.Get(), Run.Get());
	Card->CardId = TEXT("Pickup.Card.UI.DebugReward");

	First->PersistentId = TEXT("Pickup.Card.UI.Debug");
	First->CardDefinition = Card.Get();
	First->bDestroyWhenCollected = false;
	First->SyncClickTargetForTest();
	First->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	Second->PersistentId = TEXT("Pickup.Card.UI.Debug");
	Second->CardDefinition = Card.Get();
	Second->bDestroyWhenCollected = false;
	Second->SyncClickTargetForTest();

	const FWacomRunCardPickupDebugView View = First->GetRunCardPickupDebugView(PC.Get());
	TestTrue(TEXT("Card pickup config valid"), View.bConfigValid);
	TestTrue(TEXT("Card pickup duplicate detected"), View.bDuplicatePersistentIdDetected);
	TestTrue(TEXT("Card pickup reports visual"), View.bHasRenderableVisual);
	TestTrue(TEXT("Card pickup reports click target"), View.bClickTargetConfigured);
	TestEqual(TEXT("Card pickup debug card id"),
		View.CardId, FName(TEXT("Pickup.Card.UI.DebugReward")));
	TestEqual(TEXT("Card pickup click stable id"),
		View.ClickTargetStableId, FName(TEXT("Pickup.Card.UI.Debug")));

	const FString Summary = First->GetRunCardPickupDebugSummary(PC.Get());
	TestTrue(TEXT("Summary reports card id"),
		Summary.Contains(TEXT("CardId=Pickup.Card.UI.DebugReward")));
	TestTrue(TEXT("Summary reports duplicate"), Summary.Contains(TEXT("Duplicate=true")));
	TestTrue(TEXT("Summary reports visual"), Summary.Contains(TEXT("HasVisual=true")));
	TestTrue(TEXT("Summary reports click target"), Summary.Contains(TEXT("ClickTarget=true")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupConfigureDebugSampleSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.ConfigureDebugCardPickupSampleSetsDefaultsAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupConfigureDebugSampleSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>(
			GetTransientPackage(),
			FName(TEXT("CardPickupConfigureProbe"))));
	TStrongObjectPtr<UCardDefinition> FallbackCard(NewObject<UCardDefinition>());

	Pickup->PersistentId = TEXT("Old.Card.Id");
	Pickup->CardDefinition = FallbackCard.Get();
	Pickup->TriggerRadius = 500.f;
	Pickup->bDestroyWhenCollected = false;
	Pickup->InteractPromptText = FText::FromString(TEXT("old interact"));
	Pickup->HoverPromptText = FText::FromString(TEXT("old hover"));
	Pickup->CollectedHoverPromptText = FText::FromString(TEXT("old collected"));

	Pickup->ConfigureDebugCardPickupSample();

	TestEqual(TEXT("Sample persistent id uses actor name"),
		Pickup->PersistentId, FName(TEXT("Pickup.Debug.Card.CardPickupConfigureProbe")));
	TestEqual(TEXT("Sample trigger radius"), Pickup->TriggerRadius, 160.f);
	TestTrue(TEXT("Sample destroys when collected"), Pickup->bDestroyWhenCollected);
	TestEqual(TEXT("Sample interact prompt"),
		Pickup->GetInteractPromptText_Implementation(nullptr).ToString(),
		FString(TEXT("按 E 拾取卡牌")));
	TestEqual(TEXT("Sample hover prompt"),
		Pickup->GetHoverPromptText(nullptr).ToString(),
		FString(TEXT("点击拾取卡牌")));
	TestNotNull(TEXT("Sample keeps or loads a card definition"), Pickup->CardDefinition.Get());
	TestEqual(TEXT("Sample click stable id"),
		Pickup->GetClickInteractionTargetComponent()->GetStableTargetId(),
		FName(TEXT("Pickup.Debug.Card.CardPickupConfigureProbe")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunCardPickupConfigureDebugSampleRunStateSpec,
	"Wacom.UI.WorldInteraction.RunCardPickup.ConfigureDebugCardPickupSampleDoesNotMutateRunState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunCardPickupConfigureDebugSampleRunStateSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PC.Get()));
	TStrongObjectPtr<AWacomRunCardPickupClickProbe> Pickup(
		NewObject<AWacomRunCardPickupClickProbe>(
			GetTransientPackage(),
			FName(TEXT("CardPickupConfigureRunStateProbe"))));
	InjectRunSession(PC.Get(), Run.Get());

	Run->CollectGoldPickup(TEXT("Pickup.Existing.CardTest"), 5);
	const int32 GoldBefore = Run->GetGold();
	const int32 BackpackBefore = Run->GetBackpack().Num();
	const bool bExistingCollectedBefore =
		Run->IsPickupCollected(TEXT("Pickup.Existing.CardTest"));

	Pickup->ConfigureDebugCardPickupSample();

	TestEqual(TEXT("Configure card sample does not change gold"), Run->GetGold(), GoldBefore);
	TestEqual(TEXT("Configure card sample does not add card"),
		Run->GetBackpack().Num(), BackpackBefore);
	TestEqual(TEXT("Configure card sample preserves existing collected state"),
		Run->IsPickupCollected(TEXT("Pickup.Existing.CardTest")), bExistingCollectedBefore);
	TestFalse(TEXT("Configure card sample does not mark new id collected"),
		Run->IsPickupCollected(Pickup->PersistentId));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventHoverPromptDebugSummarySpec,
	"Wacom.UI.WorldInteraction.RunEventHoverPrompt.HoverDebugSummaryReportsActorPromptAndStableId",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventHoverPromptDebugSummarySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PC(NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<AWacomRunEventTriggerClickProbe> Trigger(
		NewObject<AWacomRunEventTriggerClickProbe>());
	Trigger->PersistentId = TEXT("Event.UI.HoverDebug");
	Trigger->HoverPromptText = FText::FromString(TEXT("点击调试事件"));
	Trigger->SyncClickTargetForTest();
	Trigger->GetClickTargetBridgeComponent()->RefreshRunWorldTargetBinding();
	PC->SetRunSceneHitForTest(Trigger.Get(), Trigger->GetClickBounds());

	PC->UpdateRunWorldTargetProbePreviewForTest();

	const FString Summary = PC->ReadRunWorldInteractableHoverDebugSummaryForTest();
	TestTrue(TEXT("Summary reports actor"), Summary.Contains(Trigger->GetName()));
	TestTrue(TEXT("Summary reports prompt"), Summary.Contains(TEXT("Prompt=点击调试事件")));
	TestTrue(TEXT("Summary reports stable id"), Summary.Contains(TEXT("StableId=Event.UI.HoverDebug")));
	TestTrue(TEXT("Summary reports target handle"), Summary.Contains(TEXT("Target=")));

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

	FRunEventChoiceSnapshot PaymentReadyChoice;
	PaymentReadyChoice.ChoiceId = TEXT("PayReady");
	PaymentReadyChoice.bAvailable = true;
	PaymentReadyChoice.bRequiresOwnedCardPayment = true;
	PaymentReadyChoice.PaymentCandidateCount = 3;
	const FWacomRunEventChoiceRequirementView PaymentReadyView =
		UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(PaymentReadyChoice);
	TestEqual(TEXT("Payment ready requirement text"),
		PaymentReadyView.RequirementText.ToString(),
		FString(TEXT("拖入卡牌支付：3 张可用")));
	TestEqual(TEXT("Payment ready tone"),
		PaymentReadyView.Tone,
		EWacomRunEventChoiceAvailabilityTone::Requirement);

	FRunEventChoiceSnapshot PaymentMissingChoice;
	PaymentMissingChoice.ChoiceId = TEXT("PayMissing");
	PaymentMissingChoice.bAvailable = false;
	PaymentMissingChoice.bRequiresOwnedCardPayment = true;
	PaymentMissingChoice.PaymentDisabledReason = TEXT("MissingRequiredCard");
	const FWacomRunEventChoiceRequirementView PaymentMissingView =
		UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(PaymentMissingChoice);
	TestEqual(TEXT("Payment missing requirement text"),
		PaymentMissingView.RequirementText.ToString(),
		FString(TEXT("缺少可支付卡牌：缺少所需卡牌")));
	TestTrue(TEXT("Payment missing blocked text is empty to avoid duplicate row"),
		PaymentMissingView.BlockedReasonText.IsEmpty());
	TestEqual(TEXT("Payment missing primary reason"),
		PaymentMissingView.PrimaryReason,
		FName(TEXT("MissingRequiredCard")));
	TestEqual(TEXT("Payment missing tone"),
		PaymentMissingView.Tone,
		EWacomRunEventChoiceAvailabilityTone::Blocked);

	FRunEventChoiceSnapshot BlockedChoice;
	BlockedChoice.ChoiceId = TEXT("GoldLocked");
	BlockedChoice.bAvailable = false;
	BlockedChoice.DisabledReason = TEXT("InsufficientGold");
	const FWacomRunEventChoiceRequirementView BlockedView =
		UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(BlockedChoice);
	TestTrue(TEXT("Blocked non-payment has empty requirement"),
		BlockedView.RequirementText.IsEmpty());
	TestEqual(TEXT("Blocked reason text"),
		BlockedView.BlockedReasonText.ToString(),
		FString(TEXT("不可选：金币不足")));
	TestEqual(TEXT("Blocked primary reason"),
		BlockedView.PrimaryReason,
		FName(TEXT("InsufficientGold")));

	FRunEventChoiceSnapshot AvailableChoice;
	AvailableChoice.ChoiceId = TEXT("Leave");
	AvailableChoice.bAvailable = true;
	const FWacomRunEventChoiceRequirementView AvailableView =
		UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(AvailableChoice);
	TestTrue(TEXT("Available non-payment has no requirement text"),
		AvailableView.RequirementText.IsEmpty());
	TestTrue(TEXT("Available non-payment has no blocked reason text"),
		AvailableView.BlockedReasonText.IsEmpty());
	TestEqual(TEXT("Available non-payment tone"),
		AvailableView.Tone,
		EWacomRunEventChoiceAvailabilityTone::Ready);

	FRunEventChoiceSnapshot RequirementChoice;
	RequirementChoice.ChoiceId = TEXT("RequirementPreview");
	RequirementChoice.bAvailable = false;
	RequirementChoice.DisabledReason = TEXT("InsufficientGold");
	FRunEventChoiceRequirementSnapshot GoldRequirement;
	GoldRequirement.Kind = ERunEventChoiceRequirementKind::MinGold;
	GoldRequirement.bSatisfied = false;
	GoldRequirement.DisabledReason = TEXT("InsufficientGold");
	GoldRequirement.RequiredValue = 3;
	GoldRequirement.CurrentValue = 1;
	FRunEventChoiceRequirementSnapshot NodeRequirement;
	NodeRequirement.Kind = ERunEventChoiceRequirementKind::MinNodeCount;
	NodeRequirement.bSatisfied = true;
	NodeRequirement.RequiredValue = 1;
	NodeRequirement.CurrentValue = 2;
	FRunEventChoiceRequirementSnapshot PressureRequirement;
	PressureRequirement.Kind = ERunEventChoiceRequirementKind::MaxPressure;
	PressureRequirement.bSatisfied = false;
	PressureRequirement.DisabledReason = TEXT("PressureTooHigh");
	PressureRequirement.RequiredValue = 2;
	PressureRequirement.CurrentValue = 5;
	PressureRequirement.PressureType = EWacomPressureType::Misdeed;
	FRunEventChoiceRequirementSnapshot HasCardRequirement;
	HasCardRequirement.Kind = ERunEventChoiceRequirementKind::HasCard;
	HasCardRequirement.bSatisfied = true;
	HasCardRequirement.CardDefinition = Card;
	FRunEventChoiceRequirementSnapshot EventRequirement;
	EventRequirement.Kind = ERunEventChoiceRequirementKind::EventCompleted;
	EventRequirement.bSatisfied = false;
	EventRequirement.DisabledReason = TEXT("RequiredEventNotCompleted");
	EventRequirement.TargetPersistentId = TEXT("Event.Target");
	FRunEventChoiceRequirementSnapshot RunFlagSetRequirement;
	RunFlagSetRequirement.Kind = ERunEventChoiceRequirementKind::RunFlagSet;
	RunFlagSetRequirement.bSatisfied = false;
	RunFlagSetRequirement.DisabledReason = TEXT("RequiredRunFlagMissing");
	RunFlagSetRequirement.FlagId = TEXT("SnakeGift.HasFang");
	FRunEventChoiceRequirementSnapshot RunFlagNotSetRequirement;
	RunFlagNotSetRequirement.Kind = ERunEventChoiceRequirementKind::RunFlagNotSet;
	RunFlagNotSetRequirement.bSatisfied = true;
	RunFlagNotSetRequirement.FlagId = TEXT("SnakeGift.RewardClaimed");
	FRunEventChoiceRequirementSnapshot PaymentRequirement;
	PaymentRequirement.Kind = ERunEventChoiceRequirementKind::CardPayment;
	PaymentRequirement.bSatisfied = true;
	PaymentRequirement.PaymentCandidateCount = 2;
	RequirementChoice.Requirements = {
		GoldRequirement,
		NodeRequirement,
		PressureRequirement,
		HasCardRequirement,
		EventRequirement,
		RunFlagSetRequirement,
		RunFlagNotSetRequirement,
		PaymentRequirement };
	const FWacomRunEventChoiceRequirementView RequirementView =
		UWacomRunEventPresentationBuilder::BuildChoiceRequirementView(RequirementChoice);
	TestEqual(TEXT("Requirement item count"), RequirementView.RequirementItems.Num(), 8);
	TestEqual(TEXT("Unsatisfied requirement count"), RequirementView.UnsatisfiedRequirementCount, 4);
	if (RequirementView.RequirementItems.Num() == 8)
	{
		TestEqual(TEXT("Gold requirement text"),
			RequirementView.RequirementItems[0].Text.ToString(),
			FString(TEXT("需要金币：3 / 当前 1")));
		TestEqual(TEXT("Node requirement text"),
			RequirementView.RequirementItems[1].Text.ToString(),
			FString(TEXT("需要行动点：1 / 当前 2")));
		TestEqual(TEXT("Pressure requirement text"),
			RequirementView.RequirementItems[2].Text.ToString(),
			FString(TEXT("压力不高于：恶行 2 / 当前 5")));
		TestEqual(TEXT("Has card requirement text"),
			RequirementView.RequirementItems[3].Text.ToString(),
			FString(TEXT("需要持有：事件提示卡")));
		TestEqual(TEXT("Event requirement text"),
			RequirementView.RequirementItems[4].Text.ToString(),
			FString(TEXT("需要事件已完成：Event.Target")));
		TestEqual(TEXT("RunFlagSet requirement text"),
			RequirementView.RequirementItems[5].Text.ToString(),
			FString(TEXT("需要标记：SnakeGift.HasFang")));
		TestEqual(TEXT("RunFlagNotSet requirement text"),
			RequirementView.RequirementItems[6].Text.ToString(),
			FString(TEXT("不能有标记：SnakeGift.RewardClaimed")));
		TestEqual(TEXT("Payment requirement item text"),
			RequirementView.RequirementItems[7].Text.ToString(),
			FString(TEXT("需要拖入卡牌支付：2 张可用")));
	}

	FRunEventChoiceSnapshot ConsequenceChoice;
	ConsequenceChoice.ChoiceId = TEXT("ConsequencePreview");
	FRunEventChoiceConsequenceSnapshot GainCardConsequence;
	GainCardConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	GainCardConsequence.EffectType = EWacomRunEventEffectType::GainCard;
	GainCardConsequence.CardDefinition = Card;
	FRunEventChoiceConsequenceSnapshot GoldGainConsequence;
	GoldGainConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	GoldGainConsequence.EffectType = EWacomRunEventEffectType::AddGold;
	GoldGainConsequence.Amount = 3;
	FRunEventChoiceConsequenceSnapshot GoldLossConsequence;
	GoldLossConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	GoldLossConsequence.EffectType = EWacomRunEventEffectType::AddGold;
	GoldLossConsequence.Amount = -2;
	FRunEventChoiceConsequenceSnapshot PressureConsequence;
	PressureConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	PressureConsequence.EffectType = EWacomRunEventEffectType::AddPressure;
	PressureConsequence.Amount = 5;
	PressureConsequence.PressureType = EWacomPressureType::Misdeed;
	FRunEventChoiceConsequenceSnapshot NodeConsequence;
	NodeConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	NodeConsequence.EffectType = EWacomRunEventEffectType::ConsumeNode;
	NodeConsequence.Amount = 1;
	FRunEventChoiceConsequenceSnapshot RemoveCardConsequence;
	RemoveCardConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	RemoveCardConsequence.EffectType = EWacomRunEventEffectType::RemoveCard;
	RemoveCardConsequence.CardDefinition = Card;
	FRunEventChoiceConsequenceSnapshot MarkEventConsequence;
	MarkEventConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	MarkEventConsequence.EffectType = EWacomRunEventEffectType::MarkEventCompleted;
	MarkEventConsequence.TargetPersistentId = TEXT("Event.Target");
	FRunEventChoiceConsequenceSnapshot SetFlagConsequence;
	SetFlagConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	SetFlagConsequence.EffectType = EWacomRunEventEffectType::SetRunFlag;
	SetFlagConsequence.FlagId = TEXT("SnakeGift.HasFang");
	FRunEventChoiceConsequenceSnapshot ClearFlagConsequence;
	ClearFlagConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	ClearFlagConsequence.EffectType = EWacomRunEventEffectType::ClearRunFlag;
	ClearFlagConsequence.FlagId = TEXT("SnakeGift.RewardClaimed");
	FRunEventChoiceConsequenceSnapshot TransitionConsequence;
	TransitionConsequence.Kind = ERunEventChoiceConsequenceKind::NodeTransition;
	TransitionConsequence.ResolvedNodeId = TEXT("After");
	TransitionConsequence.ResolvedNodeTitleText = FText::FromString(TEXT("后续节点"));
	FRunEventChoiceConsequenceSnapshot EventEndsConsequence;
	EventEndsConsequence.Kind = ERunEventChoiceConsequenceKind::EventEnds;
	ConsequenceChoice.Consequences = {
		GainCardConsequence,
		GoldGainConsequence,
		GoldLossConsequence,
		PressureConsequence,
		NodeConsequence,
		RemoveCardConsequence,
		MarkEventConsequence,
		SetFlagConsequence,
		ClearFlagConsequence,
		TransitionConsequence,
		EventEndsConsequence };
	const FWacomRunEventChoiceConsequenceView ConsequenceView =
		UWacomRunEventPresentationBuilder::BuildChoiceConsequenceView(ConsequenceChoice);
	TestEqual(TEXT("Consequence item count"), ConsequenceView.ConsequenceItems.Num(), 11);
	if (ConsequenceView.ConsequenceItems.Num() == 11)
	{
		TestEqual(TEXT("Gain card consequence text"),
			ConsequenceView.ConsequenceItems[0].Text.ToString(),
			FString(TEXT("获得卡牌：事件提示卡")));
		TestEqual(TEXT("Gold gain consequence text"),
			ConsequenceView.ConsequenceItems[1].Text.ToString(),
			FString(TEXT("获得金币：3")));
		TestEqual(TEXT("Gold loss consequence text"),
			ConsequenceView.ConsequenceItems[2].Text.ToString(),
			FString(TEXT("失去金币：2")));
		TestEqual(TEXT("Pressure consequence text"),
			ConsequenceView.ConsequenceItems[3].Text.ToString(),
			FString(TEXT("恶行 +5")));
		TestEqual(TEXT("Node consequence text"),
			ConsequenceView.ConsequenceItems[4].Text.ToString(),
			FString(TEXT("消耗行动点：1")));
		TestEqual(TEXT("Remove card consequence text"),
			ConsequenceView.ConsequenceItems[5].Text.ToString(),
			FString(TEXT("交出卡牌：事件提示卡")));
		TestEqual(TEXT("Mark event consequence text"),
			ConsequenceView.ConsequenceItems[6].Text.ToString(),
			FString(TEXT("完成事件：Event.Target")));
		TestEqual(TEXT("Set flag consequence text"),
			ConsequenceView.ConsequenceItems[7].Text.ToString(),
			FString(TEXT("设置标记：SnakeGift.HasFang")));
		TestEqual(TEXT("Clear flag consequence text"),
			ConsequenceView.ConsequenceItems[8].Text.ToString(),
			FString(TEXT("清除标记：SnakeGift.RewardClaimed")));
		TestEqual(TEXT("Transition consequence text"),
			ConsequenceView.ConsequenceItems[9].Text.ToString(),
			FString(TEXT("进入：后续节点")));
		TestEqual(TEXT("Event ends consequence text"),
			ConsequenceView.ConsequenceItems[10].Text.ToString(),
			FString(TEXT("事件将结束")));
	}

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
	FRunEventChoiceRequirementSnapshot PaymentRequirement;
	PaymentRequirement.Kind = ERunEventChoiceRequirementKind::CardPayment;
	PaymentRequirement.bSatisfied = true;
	PaymentRequirement.PaymentCandidateCount = 2;
	PaymentChoice.Requirements.Add(PaymentRequirement);
	Button->SetChoiceSnapshot(PaymentChoice);
	TestEqual(TEXT("Payment row shows candidate count"),
		Button->GetDisplayedPaymentStatusTextForTest().ToString(),
		FString(TEXT("拖入卡牌支付：2 张可用")));
	TestEqual(TEXT("Payment row stores requirement candidate count"),
		Button->GetChoiceRequirementView().PaymentCandidateCount,
		2);
	TestEqual(TEXT("Payment row stores requirement tone"),
		Button->GetChoiceRequirementView().Tone,
		EWacomRunEventChoiceAvailabilityTone::Requirement);
	TestEqual(TEXT("Payment status visible for payment choice"),
		Button->GetPaymentStatusVisibilityForTest(),
		ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Payment requirement is not duplicated in generic list"),
		Button->GetDisplayedRequirementItemCountForTest(),
		0);
	TestEqual(TEXT("Payment has no consequence by default"),
		Button->GetDisplayedConsequenceItemCountForTest(),
		0);

	PaymentChoice.bAvailable = false;
	PaymentChoice.PaymentCandidateCount = 0;
	PaymentChoice.PaymentDisabledReason = TEXT("MissingRequiredCard");
	PaymentChoice.Requirements[0].bSatisfied = false;
	PaymentChoice.Requirements[0].PaymentCandidateCount = 0;
	PaymentChoice.Requirements[0].DisabledReason = TEXT("MissingRequiredCard");
	Button->SetChoiceSnapshot(PaymentChoice);
	TestEqual(TEXT("Payment row shows missing candidate reason"),
		Button->GetDisplayedPaymentStatusTextForTest().ToString(),
		FString(TEXT("缺少可支付卡牌：缺少所需卡牌")));
	TestEqual(TEXT("Payment missing disabled reason line hidden"),
		Button->GetDisabledReasonVisibilityForTest(),
		ESlateVisibility::Collapsed);

	FRunEventChoiceSnapshot NonPaymentChoice;
	NonPaymentChoice.ChoiceId = TEXT("Leave");
	NonPaymentChoice.LabelText = FText::FromString(TEXT("离开"));
	NonPaymentChoice.bAvailable = true;
	FRunEventChoiceConsequenceSnapshot NodeConsequence;
	NodeConsequence.Kind = ERunEventChoiceConsequenceKind::NodeTransition;
	NodeConsequence.ResolvedNodeId = TEXT("After");
	NodeConsequence.ResolvedNodeTitleText = FText::FromString(TEXT("后续节点"));
	NonPaymentChoice.Consequences.Add(NodeConsequence);
	Button->SetChoiceSnapshot(NonPaymentChoice);
	TestEqual(TEXT("Non-payment row hides payment status"),
		Button->GetPaymentStatusVisibilityForTest(),
		ESlateVisibility::Collapsed);
	TestEqual(TEXT("Non-payment row shows consequence item"),
		Button->GetDisplayedConsequenceItemCountForTest(),
		1);
	TestEqual(TEXT("Non-payment consequence item text"),
		Button->GetDisplayedConsequenceItemTextForTest(0).ToString(),
		FString(TEXT("进入：后续节点")));

	FRunEventChoiceSnapshot BlockedNonPaymentChoice;
	BlockedNonPaymentChoice.ChoiceId = TEXT("GoldLocked");
	BlockedNonPaymentChoice.bAvailable = false;
	BlockedNonPaymentChoice.DisabledReason = TEXT("InsufficientGold");
	FRunEventChoiceRequirementSnapshot GoldRequirement;
	GoldRequirement.Kind = ERunEventChoiceRequirementKind::MinGold;
	GoldRequirement.bSatisfied = false;
	GoldRequirement.DisabledReason = TEXT("InsufficientGold");
	GoldRequirement.RequiredValue = 3;
	GoldRequirement.CurrentValue = 1;
	BlockedNonPaymentChoice.Requirements.Add(GoldRequirement);
	Button->SetChoiceSnapshot(BlockedNonPaymentChoice);
	TestEqual(TEXT("Blocked non-payment row shows disabled reason"),
		Button->GetDisplayedDisabledReasonTextForTest().ToString(),
		FString(TEXT("不可选：金币不足")));
	TestEqual(TEXT("Blocked non-payment disabled reason visible"),
		Button->GetDisabledReasonVisibilityForTest(),
		ESlateVisibility::HitTestInvisible);
	TestEqual(TEXT("Blocked non-payment row shows one requirement item"),
		Button->GetDisplayedRequirementItemCountForTest(),
		1);
	TestEqual(TEXT("Blocked non-payment requirement item text"),
		Button->GetDisplayedRequirementItemTextForTest(0).ToString(),
		FString(TEXT("需要金币：3 / 当前 1")));

	FRunEventChoiceSnapshot RunFlagChoice;
	RunFlagChoice.ChoiceId = TEXT("FlagChoice");
	RunFlagChoice.bAvailable = false;
	RunFlagChoice.DisabledReason = TEXT("RequiredRunFlagMissing");
	FRunEventChoiceRequirementSnapshot RunFlagRequirement;
	RunFlagRequirement.Kind = ERunEventChoiceRequirementKind::RunFlagSet;
	RunFlagRequirement.bSatisfied = false;
	RunFlagRequirement.DisabledReason = TEXT("RequiredRunFlagMissing");
	RunFlagRequirement.FlagId = TEXT("SnakeGift.HasFang");
	FRunEventChoiceConsequenceSnapshot RunFlagConsequence;
	RunFlagConsequence.Kind = ERunEventChoiceConsequenceKind::Effect;
	RunFlagConsequence.EffectType = EWacomRunEventEffectType::SetRunFlag;
	RunFlagConsequence.FlagId = TEXT("SnakeGift.HasFang");
	RunFlagChoice.Requirements.Add(RunFlagRequirement);
	RunFlagChoice.Consequences.Add(RunFlagConsequence);
	Button->SetChoiceSnapshot(RunFlagChoice);
	TestEqual(TEXT("RunFlag blocked reason text"),
		Button->GetDisplayedDisabledReasonTextForTest().ToString(),
		FString(TEXT("不可选：缺少所需标记")));
	TestEqual(TEXT("RunFlag requirement fallback text"),
		Button->GetDisplayedRequirementItemTextForTest(0).ToString(),
		FString(TEXT("需要标记：SnakeGift.HasFang")));
	TestEqual(TEXT("RunFlag consequence fallback text"),
		Button->GetDisplayedConsequenceItemTextForTest(0).ToString(),
		FString(TEXT("设置标记：SnakeGift.HasFang")));

	FRunEventChoiceSnapshot ZeroConsequenceChoice;
	ZeroConsequenceChoice.ChoiceId = TEXT("ZeroConsequences");
	ZeroConsequenceChoice.bAvailable = true;
	FRunEventChoiceConsequenceSnapshot ZeroGold;
	ZeroGold.Kind = ERunEventChoiceConsequenceKind::Effect;
	ZeroGold.EffectType = EWacomRunEventEffectType::AddGold;
	ZeroGold.Amount = 0;
	FRunEventChoiceConsequenceSnapshot ZeroPressure;
	ZeroPressure.Kind = ERunEventChoiceConsequenceKind::Effect;
	ZeroPressure.EffectType = EWacomRunEventEffectType::AddPressure;
	ZeroPressure.Amount = 0;
	FRunEventChoiceConsequenceSnapshot ZeroNode;
	ZeroNode.Kind = ERunEventChoiceConsequenceKind::Effect;
	ZeroNode.EffectType = EWacomRunEventEffectType::ConsumeNode;
	ZeroNode.Amount = 0;
	ZeroConsequenceChoice.Consequences = { ZeroGold, ZeroPressure, ZeroNode };
	Button->SetChoiceSnapshot(ZeroConsequenceChoice);
	TestEqual(TEXT("Zero amount consequences remain hidden in fallback list"),
		Button->GetDisplayedConsequenceItemCountForTest(),
		0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventChoiceButtonSnapshotAppliedEventSpec,
	"Wacom.UI.Event.ChoiceSnapshotAppliedBlueprintNativeEventFires",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventChoiceButtonSnapshotAppliedEventSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunEventChoiceButtonClassProbe> Button(
		NewObject<UWacomRunEventChoiceButtonClassProbe>());

	FRunEventChoiceSnapshot Choice;
	Choice.ChoiceId = TEXT("PayBeforeConstruct");
	Button->SetChoiceSnapshot(Choice);
	TestEqual(TEXT("Event waits until widget is constructed"),
		Button->SnapshotAppliedCountForTest,
		0);

	Button->TakeWidget();
	TestEqual(TEXT("Event fires after construct for pending snapshot"),
		Button->SnapshotAppliedCountForTest,
		1);
	TestEqual(TEXT("Pending snapshot choice id forwarded"),
		Button->LastAppliedChoiceIdForTest,
		FName(TEXT("PayBeforeConstruct")));

	Choice.ChoiceId = TEXT("PayAfterConstruct");
	Button->SetChoiceSnapshot(Choice);
	TestEqual(TEXT("Event fires immediately after construct"),
		Button->SnapshotAppliedCountForTest,
		2);
	TestEqual(TEXT("Updated snapshot choice id forwarded"),
		Button->LastAppliedChoiceIdForTest,
		FName(TEXT("PayAfterConstruct")));

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
	FWacomUIRunEventScreenWbpBindingAuthoringSpec,
	"Wacom.UI.Event.WbpBindingAuthoringSurface",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventScreenWbpBindingAuthoringSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Fang = Fx.MakeNoopCard(0);
	Fang->CardId = TEXT("PoisonFang");
	UCardDefinition* Bag = Fx.MakeNoopCard(0);
	Bag->Physique.Capacity = 8;
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(1),
		Fx.MakeNoopCard(1),
		{ Bag, Fang });
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	TStrongObjectPtr<UWacomRunEventDefinition> Event(
		MakeUiRunEventCardPaymentEvent(Run.Get(), Fang));
	TestTrue(TEXT("Begin payment UI event"),
		Run->BeginRunEvent(TEXT("Event.UI.Payment.Authoring"), Event.Get()));

	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(NewObject<UWacomRunEventScreenProbe>());
	Screen->SetRunSession(Run.Get());
	Screen->SetChoiceButtonClassForTest(UWacomRunEventChoiceButtonClassProbe::StaticClass());
	Screen->SetPaymentDropTargetClassForTest(UWacomRunEventPaymentDropTargetWidgetClassProbe::StaticClass());
	Screen->SetPaymentChoiceMinDesiredWidthForTest(512.0f);
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshEvent();

	TestEqual(TEXT("Configured choice class resolves"),
		Screen->ReadChoiceButtonWidgetClass().Get(),
		UWacomRunEventChoiceButtonClassProbe::StaticClass());
	TestEqual(TEXT("Configured drop target class resolves"),
		Screen->ReadPaymentDropTargetWidgetClass().Get(),
		UWacomRunEventPaymentDropTargetWidgetClassProbe::StaticClass());
	TestEqual(TEXT("Configured payment min width stored"),
		Screen->ReadPaymentChoiceMinDesiredWidth(),
		512.0f);

	UWacomRunEventChoiceButton* ChoiceButton = Screen->ReadChoiceButtonWidget(0);
	if (ChoiceButton)
	{
		ChoiceButton->TakeWidget();
	}
	TestTrue(TEXT("Dynamic choice row uses configured class"),
		ChoiceButton && ChoiceButton->IsA(UWacomRunEventChoiceButtonClassProbe::StaticClass()));
	const UWacomRunEventChoiceButtonClassProbe* ChoiceProbe =
		Cast<UWacomRunEventChoiceButtonClassProbe>(ChoiceButton);
	TestTrue(TEXT("Choice snapshot applied event fired on dynamic row"),
		ChoiceProbe && ChoiceProbe->SnapshotAppliedCountForTest > 0);
	if (ChoiceProbe)
	{
		TestEqual(TEXT("Choice snapshot apply forwards choice id"),
			ChoiceProbe->LastAppliedChoiceIdForTest,
			FName(TEXT("PayFang")));
	}

	const UWacomRunMenuDropTargetWidget* DropTarget = Screen->ReadPaymentDropTarget(0);
	TestTrue(TEXT("Payment choice uses configured drop target class"),
		DropTarget && DropTarget->IsA(UWacomRunEventPaymentDropTargetWidgetClassProbe::StaticClass()));
	if (DropTarget)
	{
		TestEqual(TEXT("Payment zone id assigned by screen"),
			DropTarget->ZoneId,
			FName(TEXT("RunEvent.Pay.Fang")));
		TestEqual(TEXT("Stable target id mirrors zone id"),
			DropTarget->StableTargetId,
			FName(TEXT("RunEvent.Pay.Fang")));
		TestEqual(TEXT("Screen does not override drop target default preview scale"),
			DropTarget->ProbePreviewScale,
			1.111f);
	}

	const FWacomRunEventScreenDebugView Debug = Screen->ReadRunEventScreenDebugView();
	TestEqual(TEXT("Custom drop target still registers one zone mapping"),
		Debug.PaymentZoneMappingCount,
		1);
	TestTrue(TEXT("Custom drop target keeps zone mapping summary"),
		Debug.PaymentZoneMappingSummary.Contains(TEXT("RunEvent.Pay.Fang->PayFang")));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunEventScreenWbpBindingFallbackSpec,
	"Wacom.UI.Event.MissingConfiguredChildClassesFallbackToNative",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventScreenWbpBindingFallbackSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(NewObject<UWacomRunEventScreenProbe>());

	TestEqual(TEXT("Missing choice class falls back to native"),
		Screen->ReadChoiceButtonWidgetClass().Get(),
		UWacomRunEventChoiceButton::StaticClass());
	TestEqual(TEXT("Missing drop target class falls back to native"),
		Screen->ReadPaymentDropTargetWidgetClass().Get(),
		UWacomRunMenuDropTargetWidget::StaticClass());

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
	TestEqual(TEXT("Debug reports available choice count"), InitialDebug.AvailableChoiceCount, 1);
	TestEqual(TEXT("Debug reports unavailable choice count"), InitialDebug.UnavailableChoiceCount, 0);
	TestEqual(TEXT("Debug reports payment choice count"), InitialDebug.PaymentChoiceCount, 1);
	TestEqual(TEXT("Debug reports candidate count"), InitialDebug.PaymentCandidateInstanceCount, 1);
	TestEqual(TEXT("Debug reports zone mapping count"), InitialDebug.PaymentZoneMappingCount, 1);
	TestTrue(TEXT("Debug reports availability summary"),
		InitialDebug.ChoiceAvailabilitySummary.Contains(TEXT("PayFang:Requirement:None")));
	TestTrue(TEXT("Debug reports requirement summary"),
		InitialDebug.ChoiceRequirementSummary.Contains(TEXT("PayFang:1/0")));
	TestTrue(TEXT("Debug reports consequence summary"),
		InitialDebug.ChoiceConsequenceSummary.Contains(TEXT("PayFang:2")));
	TestTrue(TEXT("Debug reports choice preview summary"),
		InitialDebug.ChoicePreviewSummary.Contains(TEXT("PayFang:Available=true:First=None:Req=1/0:Pay=1:Consequences=2:Outcome=EventEnds")));
	TestTrue(TEXT("Debug reports payment zone mapping"),
		InitialDebug.PaymentZoneMappingSummary.Contains(TEXT("RunEvent.Pay.Fang->PayFang")));
	const FString InitialDebugSummary = Screen->ReadRunEventScreenDebugSummary();
	TestTrue(TEXT("Debug summary includes active node"),
		InitialDebugSummary.Contains(TEXT("Node=Start")));
	TestTrue(TEXT("Debug summary includes choice counts"),
		InitialDebugSummary.Contains(TEXT("Choices=1"))
		&& InitialDebugSummary.Contains(TEXT("AvailableChoices=1"))
		&& InitialDebugSummary.Contains(TEXT("UnavailableChoices=0"))
		&& InitialDebugSummary.Contains(TEXT("PaymentChoices=1"))
		&& InitialDebugSummary.Contains(TEXT("Candidates=1")));
	TestTrue(TEXT("Debug summary includes availability"),
		InitialDebugSummary.Contains(TEXT("Availability=[PayFang:Requirement:None]")));
	TestTrue(TEXT("Debug summary includes requirements"),
		InitialDebugSummary.Contains(TEXT("Requirements=[PayFang:1/0]")));
	TestTrue(TEXT("Debug summary includes consequences"),
		InitialDebugSummary.Contains(TEXT("Consequences=[PayFang:2]")));
	TestTrue(TEXT("Debug summary includes preview"),
		InitialDebugSummary.Contains(TEXT("Preview=[PayFang:Available=true:First=None:Req=1/0:Pay=1:Consequences=2:Outcome=EventEnds]")));
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
	{
		const FWacomRunEventScreenDebugView Debug = Screen->ReadRunEventScreenDebugView();
		TestTrue(TEXT("Blocked preview summary records first reason"),
			Debug.ChoicePreviewSummary.Contains(TEXT("Locked:Available=false:First=InsufficientGold:Req=1/1:Pay=0:Consequences=0:Outcome=None")));
		TestTrue(TEXT("Blocked debug summary includes preview"),
			Screen->ReadRunEventScreenDebugSummary().Contains(TEXT("Preview=[Locked:Available=false:First=InsufficientGold:Req=1/1:Pay=0:Consequences=0:Outcome=None]")));
	}
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
	FWacomUIRunEventScreenFlagRewardPreviewSpec,
	"Wacom.UI.Event.DebugFlagRewardPreviewSummary",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunEventScreenFlagRewardPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;
	UCardDefinition* Reward = Fx.MakeNoopCard(0);
	Reward->DisplayName = FText::FromString(TEXT("毒牙"));
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> Event(
		MakeUiRunEventFlagRewardPreviewEvent(Run.Get(), Reward));
	TStrongObjectPtr<UWacomRunEventScreenProbe> Screen(NewObject<UWacomRunEventScreenProbe>());

	TestTrue(TEXT("Begin flag reward UI event"),
		Run->BeginRunEvent(TEXT("Event.UI.FlagReward.Actor"), Event.Get()));
	Screen->SetRunSession(Run.Get());
	Screen->TakeWidget();
	Screen->ActivateWidget();
	Screen->RefreshEvent();

	TestEqual(TEXT("One flag reward choice"), Screen->ReadChoiceCount(), 1);
	const FRunEventChoiceSnapshot Choice = Screen->ReadChoiceSnapshot(0);
	TestEqual(TEXT("Choice id"), Choice.ChoiceId, FName(TEXT("ClaimGoldReward")));
	TestFalse(TEXT("Choice initially blocked"), Choice.bAvailable);
	TestEqual(TEXT("First blocked reason is missing flag"),
		Choice.DisabledReason,
		FName(TEXT("RequiredRunFlagMissing")));
	TestEqual(TEXT("Three requirements"), Choice.Requirements.Num(), 3);
	TestEqual(TEXT("Four consequences including node transition"), Choice.Consequences.Num(), 4);

	const FWacomRunEventScreenDebugView Debug = Screen->ReadRunEventScreenDebugView();
	TestTrue(TEXT("Requirement summary reports three unsatisfied requirements"),
		Debug.ChoiceRequirementSummary.Contains(TEXT("ClaimGoldReward:3/2")));
	TestTrue(TEXT("Consequence summary reports four consequence facts"),
		Debug.ChoiceConsequenceSummary.Contains(TEXT("ClaimGoldReward:4")));
	TestTrue(TEXT("Preview summary reports flag/gold reward counts and transition"),
		Debug.ChoicePreviewSummary.Contains(TEXT("ClaimGoldReward:Available=false:First=RequiredRunFlagMissing:Req=3/2:Pay=0:Consequences=4:Outcome=NodeTransition:Rewarded")));
	TestTrue(TEXT("Debug summary includes flag reward preview"),
		Screen->ReadRunEventScreenDebugSummary().Contains(TEXT("Preview=[ClaimGoldReward:Available=false:First=RequiredRunFlagMissing:Req=3/2:Pay=0:Consequences=4:Outcome=NodeTransition:Rewarded]")));

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
