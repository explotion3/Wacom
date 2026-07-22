// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/BattleHUD.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"
#include "UI/Battle/WacomBattlePresentationTargetCue.h"
#include "UI/Common/PileCountView.h"
#include "BattleHUDTestHarness.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "Events/BattleEvent.h"

#include "Components/WacomBattleEnemyPartComponent.h"
#include "Components/WacomInteractionTargetComponent.h"
#include "Engine/Engine.h"
#include "Engine/EngineTypes.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomBattleWidgetSpec
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

	struct FSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<UWacomBattleEnemyPartComponent*> Parts;
	};

	FName ResolvePartSlotIdForDefinitionPart(
		const UEnemyDefinition* EnemyDefinition,
		FName PartId)
	{
		if (!EnemyDefinition || PartId.IsNone())
		{
			return NAME_None;
		}

		for (const FEnemyPartSlot& Slot : EnemyDefinition->Parts)
		{
			if (Slot.PartDef && Slot.PartDef->PartId == PartId)
			{
				return Slot.PartSlotId;
			}
		}
		return NAME_None;
	}

	FSceneEnemyHostActors SpawnSceneEnemyHost(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition,
		const TArray<FName>& PartIds)
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;

		FSceneEnemyHostActors Result;
		Result.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Result.Host)
		{
			return Result;
		}

		Result.Host->EnemyDefinition = EnemyDefinition;
		for (int32 Index = 0; Index < PartIds.Num(); ++Index)
		{
			UWacomBattleEnemyPartComponent* PartComponent =
				NewObject<UWacomBattleEnemyPartComponent>(
					Result.Host,
					*FString::Printf(TEXT("OtherTestPart_%d"), Index),
					RF_Transient);
			if (!PartComponent)
			{
				continue;
			}

			Result.Host->AddInstanceComponent(PartComponent);
			PartComponent->SetupAttachment(Result.Host->GetRootComponent());
			PartComponent->SetRelativeLocation(
				FVector(100.f * static_cast<float>(Index + 1), 0.f, 0.f));
			PartComponent->SetBoxExtent(FVector(40.f));
			PartComponent->SetDerivedPartId(PartIds[Index]);
			PartComponent->PartSlotId =
				ResolvePartSlotIdForDefinitionPart(EnemyDefinition, PartIds[Index]);
			PartComponent->RegisterComponent();
			Result.Parts.Add(PartComponent);
		}

		Result.Host->NotifyEnemySceneComponentTopologyChanged();
		return Result;
	}

	void DestroySceneEnemyHost(FSceneEnemyHostActors& Actors)
	{
		Actors.Parts.Reset();

		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}

}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleEventPresentationBuilderChineseTextSpec,
	"Wacom.UI.Battle.EventPresentationChineseText",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleEventPresentationBuilderChineseTextSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> PoisonFang(NewObject<UCardDefinition>());
	PoisonFang->CardId = TEXT("PoisonFang");
	PoisonFang->DisplayName = FText::FromString(TEXT("毒牙"));

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardGained;
		Event.CardDefinition = PoisonFang.Get();
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("CardGained uses display name"),
			View.MessageText.ToString(),
			FString(TEXT("获得卡牌：毒牙")));
		TestTrue(TEXT("CardGained should display"), View.bShouldDisplay);
		TestEqual(TEXT("CardGained tone is positive"), View.VisualTone, EWacomBattleEventVisualTone::Positive);
		TestEqual(TEXT("CardGained icon key"), View.IconKey, FName(TEXT("CardGained")));
		TestEqual(TEXT("FormatEventForPlayer matches view message"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			View.MessageText.ToString());
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::StatusApplied;
		Event.Tag = WacomTags::Status_Poison;
		Event.Amount = 1;
		TestEqual(TEXT("StatusApplied localizes poison"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			FString(TEXT("施加中毒 1 层")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::DamageDealt;
		Event.Tag = WacomTags::Status_Poison;
		Event.Amount = 3;
		TestEqual(TEXT("DamageDealt localizes poison source"),
			UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
			FString(TEXT("中毒造成 3 点伤害")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::BattleEnded;
		Event.Count = 1;
		const FBattleEventPresentationView VictoryView = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("BattleEnded victory is Chinese"),
			VictoryView.MessageText.ToString(),
			FString(TEXT("战斗胜利")));
		TestEqual(TEXT("BattleEnded victory is positive"),
			VictoryView.VisualTone,
			EWacomBattleEventVisualTone::Positive);

		Event.Count = 0;
		const FBattleEventPresentationView DefeatView = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("BattleEnded defeat is Chinese"),
			DefeatView.MessageText.ToString(),
			FString(TEXT("战斗失败")));
		TestEqual(TEXT("BattleEnded defeat is danger"),
			DefeatView.VisualTone,
			EWacomBattleEventVisualTone::Danger);
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandLimitDiscarded;
		Event.HandLimitDiscardSource = EHandLimitDiscardSource::EffectDraw;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestEqual(TEXT("HandLimitDiscarded source is Chinese"),
			View.MessageText.ToString(),
			FString(TEXT("因抽牌效果弃置 1 张牌")));
		TestEqual(TEXT("HandLimitDiscarded tone is warning"),
			View.VisualTone,
			EWacomBattleEventVisualTone::Warning);
		TestEqual(TEXT("HandLimitDiscarded icon key"),
			View.IconKey,
			FName(TEXT("HandLimitDiscarded")));
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::HandZoneChanged;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestTrue(TEXT("HandZoneChanged remains hidden"),
			View.MessageText.IsEmpty());
		TestFalse(TEXT("HandZoneChanged should not display"), View.bShouldDisplay);
		TestEqual(TEXT("Hidden event has no icon"), View.IconKey, NAME_None);
	}

	{
		FBattleEvent Event;
		Event.Type = EBattleEventType::CardPlayed;
		Event.Amount = 2;
		const FBattleEventPresentationView View = UWacomBattleEventPresentationBuilder::BuildEventPresentationView(Event);
		TestTrue(TEXT("CardPlayed should display"), View.bShouldDisplay);
		TestEqual(TEXT("CardPlayed defaults to neutral tone"),
			View.VisualTone,
			EWacomBattleEventVisualTone::Neutral);
	}

	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPileCountDisplaySpec,
	"Wacom.UI.Battle.HUDPileCountDisplaysPlayedPileWithDiscardPile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPileCountDisplaySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());
	HUD->CreatePileViewsForTest();
	UPileCountView* DrawPileView = HUD->GetDrawPileViewForTest();
	UPileCountView* DiscardPileView = HUD->GetDiscardPileViewForTest();
	UPileCountView* ExhaustPileView = HUD->GetExhaustPileViewForTest();
	if (!TestNotNull(TEXT("Draw pile view"), DrawPileView)
		|| !TestNotNull(TEXT("Discard pile view"), DiscardPileView)
		|| !TestNotNull(TEXT("Exhaust pile view"), ExhaustPileView))
	{
		return false;
	}

	FBattleSnapshot Snapshot;
	Snapshot.Phase = EBattlePhase::PlayerAction;
	Snapshot.PileCounts.DrawCount = 4;
	Snapshot.PileCounts.DiscardCount = 2;
	Snapshot.PileCounts.PlayedCount = 3;
	Snapshot.PileCounts.ExhaustCount = 1;

	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestEqual(TEXT("Draw pile display remains numeric"),
		DrawPileView->GetCountDisplayText().ToString(),
		FString(TEXT("4")));
	TestEqual(TEXT("Discard pile keeps numeric discard count"),
		DiscardPileView->GetCount(),
		2);
	TestEqual(TEXT("Discard pile combines discard and played counts"),
		DiscardPileView->GetCountDisplayText().ToString(),
		FString(TEXT("2+3")));
	TestEqual(TEXT("Exhaust pile display remains numeric"),
		ExhaustPileView->GetCountDisplayText().ToString(),
		FString(TEXT("1")));

	Snapshot.PileCounts.DiscardCount = 5;
	Snapshot.PileCounts.PlayedCount = 0;
	HUD->RefreshFromSnapshotForTest(Snapshot);

	TestEqual(TEXT("Discard pile returns to numeric display without played cards"),
		DiscardPileView->GetCountDisplayText().ToString(),
		FString(TEXT("5")));
	TestEqual(TEXT("Discard pile numeric cache refreshes"),
		DiscardPileView->GetCount(),
		5);

	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDCardDetailDefaultsSpec,
	"Wacom.UI.Battle.BattleHUD.FallbackLayout.CardDetailDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDCardDetailDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UBattleHUD> HUD(NewObject<UBattleHUD>());

	TestEqual(TEXT("Default card detail width"), HUD->CardDetailPanelEstimatedSize.X, 360.0);
	TestEqual(TEXT("Default card detail height"), HUD->CardDetailPanelEstimatedSize.Y, 420.0);
	TestEqual(TEXT("Default card detail padding"), HUD->CardDetailPanelPadding, 12.0f);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationTargetRegistryUnknownTargetNoopsSpec,
	"Wacom.UI.Battle.PresentationTargetRegistry.UnknownTargetNoops",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationTargetRegistryUnknownTargetNoopsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomBattleHUDDetailTest> HUD(NewObject<UWacomBattleHUDDetailTest>());

	HUD->PlayBattlePresentationCueForTest(EBattleEventType::DamageDealt, FBattlePartSlotIdentity(), 3);
	HUD->PlayBattlePresentationCueForTest(
		EBattleEventType::DamageDealt,
		FBattlePartSlotIdentity(TEXT("Encounter"), TEXT("UnknownEnemy"), TEXT("UnknownPart")),
		3);

	TestEqual(TEXT("Unknown cues do not create registry entries"), HUD->GetBattlePresentationTargetCountForTest(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDPrivateCoordinatorSurfaceSpec,
	"Wacom.UI.Battle.BattleHUDPrivateCoordinatorsPreservePublicHUDContracts",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDPrivateCoordinatorSurfaceSpec::RunTest(const FString& /*Parameters*/)
{
	static const TCHAR* PrivateHelperHeaders[] = {
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDSceneEnemyTargetCoordinator.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDPresentationCoordinator.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDCombatLogController.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDFirstPersonHandBridge.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattleHUDCardDetailController.h"),
		TEXT("Source/WacomApp/Private/UI/Battle/WacomBattlePresentationTimerOwner.h"),
	};

	for (const TCHAR* RelativeHeaderPath : PrivateHelperHeaders)
	{
		const FString HeaderPath = FPaths::ConvertRelativePathToFull(
			FPaths::Combine(FPaths::ProjectDir(), RelativeHeaderPath));
		FString HeaderText;
		if (!TestTrue(
				FString::Printf(TEXT("Private helper header exists: %s"), RelativeHeaderPath),
				FPaths::FileExists(HeaderPath)))
		{
			continue;
		}
		if (!TestTrue(
				FString::Printf(TEXT("Private helper header can be read: %s"), RelativeHeaderPath),
				FFileHelper::LoadFileToString(HeaderText, *HeaderPath)))
		{
			continue;
		}

		TestFalse(
			FString::Printf(TEXT("%s does not declare a reflected UCLASS"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("UCLASS(")));
		TestFalse(
			FString::Printf(TEXT("%s does not declare a reflected USTRUCT"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("USTRUCT(")));
		TestFalse(
			FString::Printf(TEXT("%s does not use GENERATED_BODY"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("GENERATED_BODY")));
		TestFalse(
			FString::Printf(TEXT("%s does not export a WacomApp public symbol"), RelativeHeaderPath),
			HeaderText.Contains(TEXT("WACOMAPP_API FWacomBattleHUD")));
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleHUDSceneEnemyCoordinatorLifecycleSpec,
	"Wacom.UI.Battle.BattleHUDSceneEnemyCoordinatorLifecycleCleansHostState",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleHUDSceneEnemyCoordinatorLifecycleSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = WacomBattleWidgetSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fx;
	UEnemyDefinition* Enemy = Fx.MakeThreePartEnemy(20, 20, 20, 5, 5, 5);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(0),
		Fx.MakeNoopCard(0),
		{ Fx.MakeNoopCard(0), Fx.MakeNoopCard(0), Fx.MakeNoopCard(0) });
	UBattleSession* Session = Fx.CreateSession(Character, Enemy, 1);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDOnly(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}

	FWacomBattleHUDTestSceneEnemyHost& CurrentHost =
		Harness->AttachSceneEnemyHost(
			Enemy,
			{ TEXT("Test.Part.Head"), TEXT("Test.Part.Body"), TEXT("Test.Part.Tail") });
	WacomBattleWidgetSpec::FSceneEnemyHostActors OtherHost =
		WacomBattleWidgetSpec::SpawnSceneEnemyHost(
			*World,
			Enemy,
			{ TEXT("Other.Part.Head") });
	if (!TestNotNull(TEXT("Current scene enemy host"), CurrentHost.Host)
		|| !TestNotNull(TEXT("Other scene enemy host"), OtherHost.Host)
		|| !TestEqual(TEXT("Current host part count"), CurrentHost.Parts.Num(), 3)
		|| !TestEqual(TEXT("Other host part count"), OtherHost.Parts.Num(), 1))
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
		return false;
	}
	ON_SCOPE_EXIT
	{
		WacomBattleWidgetSpec::DestroySceneEnemyHost(OtherHost);
	};

	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	Harness->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ CurrentHost.Host });
	HUD->RefreshFromSnapshotForTest(Session->BuildSnapshot());
	Harness->SettlePresentationQueue();

	TestTrue(TEXT("HUD registry contains configured scene enemy host"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(CurrentHost.Host));
	TestFalse(TEXT("HUD registry rejects unconfigured scene enemy host"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(OtherHost.Host));
	TestEqual(TEXT("Coordinator exposes only current host bridges through HUD"),
		HUD->GetBattleSceneEnemyPartComponentCountForTest(),
		3);

	UWacomBattleEnemyPartComponent* CurrentPart = CurrentHost.Parts[0];
	UWacomBattleEnemyPartComponent* OtherPart = OtherHost.Parts[0];
	const FWacomInteractionTargetHandle CurrentHandle = CurrentPart->BuildWorldTargetHandle();
	const FWacomInteractionTargetHandle OtherHandle = OtherPart->BuildWorldTargetHandle();

	TestTrue(TEXT("Current host part is accepted by HUD registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(CurrentHandle));
	TestFalse(TEXT("Other host part is filtered by HUD registry"),
		HUD->IsBattleSceneEnemyPartWorldTargetInCurrentRegistry(OtherHandle));
	TestTrue(TEXT("Current part is bound to snapshot"),
		CurrentPart->GetRuntimeDebugView().bBoundToSnapshot);
	TestFalse(TEXT("Other part remains unbound"),
		OtherPart->GetRuntimeDebugView().bBoundToSnapshot);

	HUD->SetSession(nullptr);
	TestFalse(TEXT("Session clear removes scene enemy host"),
		HUD->IsBattleSceneEnemyHostInCurrentRegistry(CurrentHost.Host));
	TestEqual(TEXT("Session clear removes bridge registry"),
		HUD->GetBattleSceneEnemyPartComponentCountForTest(),
		0);
	TestFalse(TEXT("Session clear unbinds current part"),
		CurrentPart->GetRuntimeDebugView().bBoundToSnapshot);

	return true;
}
