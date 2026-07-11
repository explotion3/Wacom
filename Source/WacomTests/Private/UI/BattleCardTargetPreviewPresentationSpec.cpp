// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Actors/WacomBattleEnemyActor.h"
#include "Actors/WacomBattleEnemyPartActor.h"
#include "BattleHUDTestHarness.h"
#include "BattleSceneTargetClickTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Cards/CardEffect.h"
#include "Characters/CharacterDefinition.h"
#include "Commands/BattleCommand.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/PrimitiveComponent.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Events/BattleEvent.h"
#include "Fixtures/BattleTestFixtures.h"
#include "FirstPersonCardLayerSpecReceiver.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "Snapshots/EnemySnapshot.h"
#include "Tags/WacomGameplayTags.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Battle/BattlePresentationStackEntryWidget.h"
#include "UI/Battle/PlayerStatusBar.h"
#include "UI/BattleWidgetSpecReceiver.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "Misc/ScopeExit.h"

namespace
{
	struct FPreviewSceneEnemyHostActors
	{
		AWacomBattleEnemyActor* Host = nullptr;
		TArray<AWacomBattleEnemyPartActor*> Parts;
	};

	UWorld* FindAutomationWorldForTargetPreviewPresentation()
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

	void SettleBattlePresentationQueueForTargetPreviewPresentation(
		UWacomBattleHUDDetailTest& HUD,
		int32 MaxSteps = 32)
	{
		for (int32 Iteration = 0; HUD.IsBattlePresentationBusy() && Iteration < MaxSteps; ++Iteration)
		{
			HUD.AdvanceBattlePresentationQueueForTest();
		}
	}

	FName ResolvePartSlotIdForTargetPreviewPresentation(
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

	FName ResolveFirstPartIdForTargetPreviewPresentation(const UEnemyDefinition* EnemyDefinition)
	{
		if (EnemyDefinition
			&& EnemyDefinition->Parts.IsValidIndex(0)
			&& EnemyDefinition->Parts[0].PartDef)
		{
			return EnemyDefinition->Parts[0].PartDef->PartId;
		}
		return NAME_None;
	}

	FPreviewSceneEnemyHostActors SpawnSceneEnemyHostForTargetPreviewPresentation(
		UWorld& World,
		UEnemyDefinition* EnemyDefinition)
	{
		FPreviewSceneEnemyHostActors Actors;

		FActorSpawnParameters SpawnParams;
		SpawnParams.ObjectFlags |= RF_Transient;
		Actors.Host = World.SpawnActor<AWacomBattleEnemyActor>(
			AWacomBattleEnemyActor::StaticClass(),
			FTransform::Identity,
			SpawnParams);
		if (!Actors.Host)
		{
			return Actors;
		}

		const FName PartId = ResolveFirstPartIdForTargetPreviewPresentation(EnemyDefinition);
		Actors.Host->EnemyDefinition = EnemyDefinition;
		AWacomBattleEnemyPartActor* PartActor =
			World.SpawnActor<AWacomBattleEnemyPartActor>(
				AWacomBattleEnemyPartActor::StaticClass(),
				FTransform(FVector(100.0f, 0.0f, 0.0f)),
				SpawnParams);
		if (PartActor)
		{
			PartActor->PartId = PartId;
			PartActor->PartSlotId =
				ResolvePartSlotIdForTargetPreviewPresentation(EnemyDefinition, PartId);
			PartActor->AttachToActor(Actors.Host, FAttachmentTransformRules::KeepWorldTransform);
			Actors.Parts.Add(PartActor);
		}

		Actors.Host->RefreshBattleEnemyPartAuthoringState();
		return Actors;
	}

	void DestroySceneEnemyHostForTargetPreviewPresentation(FPreviewSceneEnemyHostActors& Actors)
	{
		for (AWacomBattleEnemyPartActor* PartActor : Actors.Parts)
		{
			if (IsValid(PartActor))
			{
				PartActor->Destroy();
			}
		}
		Actors.Parts.Reset();

		if (IsValid(Actors.Host))
		{
			Actors.Host->Destroy();
		}
		Actors.Host = nullptr;
	}

	FGuid FindFirstHandCardByTargetModeForTargetPreviewPresentation(
		const FBattleSnapshot& Snapshot,
		ECardTargetMode TargetMode)
	{
		for (const FHandCardSnapshot& Card : Snapshot.Hand.Cards)
		{
			if (Card.Definition && Card.Definition->TargetMode == TargetMode)
			{
				return Card.InstanceId;
			}
		}
		return FGuid();
	}

	FWacomFirstPersonCardLayerSlotView MakeProjectedSlotForTargetPreviewPresentation(
		const FGuid& CardInstanceId,
		const FVector2D& ScreenPosition = FVector2D(500.0f, 600.0f))
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.bIsPlayable = true;
		Slot.ScreenPosition = ScreenPosition;
		Slot.WidgetPosition = ScreenPosition;
		Slot.InputHitCenter = ScreenPosition;
		Slot.RenderScale = 0.55f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardDragView MakeTargetedDragViewForTargetPreviewPresentation(
		const FGuid& CardInstanceId,
		const FVector2D& SourceSlotPosition = FVector2D(500.0f, 600.0f))
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
		DragView.SourceSlotView =
			MakeProjectedSlotForTargetPreviewPresentation(CardInstanceId, SourceSlotPosition);
		DragView.PressScreenPosition = FVector2D(500.0f, 600.0f);
		DragView.CurrentScreenPosition = FVector2D(540.0f, 590.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D::ZeroVector;
		DragView.bHasPointerViewportPosition = true;
		return DragView;
	}

	FWacomFirstPersonCardDragView MakeNoTargetCommitDragViewForTargetPreviewPresentation(
		const FGuid& CardInstanceId,
		const FVector2D& SourceSlotPosition = FVector2D(500.0f, 600.0f))
	{
		FWacomFirstPersonCardDragView DragView;
		DragView.CardInstanceId = CardInstanceId;
		DragView.GestureState = EWacomFirstPersonCardGestureState::AimingTargetedCard;
		DragView.bCommitArmed = true;
		DragView.SourceSlotView =
			MakeProjectedSlotForTargetPreviewPresentation(CardInstanceId, SourceSlotPosition);
		DragView.PressScreenPosition = SourceSlotPosition;
		DragView.CurrentScreenPosition = FVector2D(SourceSlotPosition.X, SourceSlotPosition.Y - 140.0f);
		DragView.PointerViewportPosition = DragView.CurrentScreenPosition;
		DragView.PointerNormalizedViewportPosition = FVector2D::ZeroVector;
		DragView.bHasPointerViewportPosition = false;
		return DragView;
	}

	UTextBlock* FindTextBlockForTargetPreviewPresentation(UWidgetTree* WidgetTree, FName WidgetName)
	{
		return WidgetTree ? Cast<UTextBlock>(WidgetTree->FindWidget(WidgetName)) : nullptr;
	}

	FCardEffect MakePreviewPresentationEffect(
		const FGameplayTag& EffectType,
		int32 Magnitude)
	{
		FCardEffect Effect;
		Effect.EffectType = EffectType;
		Effect.Magnitude = Magnitude;
		Effect.Target = WacomTags::Target_SingleEnemyPart;
		return Effect;
	}

	UCardDefinition* MakeEnemyPartCardForPreviewPresentation(
		FWacomBattleFixture& Fixture,
		FName CardId,
		const TArray<FCardEffect>& Effects)
	{
		UCardDefinition* Card = Fixture.MakeSimpleDamageCard(/*Cost*/0, /*Damage*/0);
		Card->CardId = CardId;
		Card->Effects = Effects;
		return Card;
	}

	FWacomInteractionTargetHandle MakeWorldTargetForPreviewPresentation(
		const FBattleSnapshot& Snapshot)
	{
		const FEnemyPartSnapshot* Part = FWacomBattleFixture::GetEnemyPartSnapshot(Snapshot, 0);
		if (!Part)
		{
			return FWacomInteractionTargetHandle();
		}

		return FWacomInteractionTargetHandle::ForWorldTarget(
			Part->InstanceId,
			nullptr,
			FVector::ZeroVector,
			FVector2D::ZeroVector,
			FGameplayTag(),
			NAME_None,
			Part->EncounterId,
			Part->EnemySlotId,
			Part->PartSlotId);
	}

	const FWacomCardViewEffectBadge* FindBadge(
		const FWacomCardViewData& CardViewData,
		EWacomCardViewEffectBadgeKind BadgeKind)
	{
		for (const FWacomCardViewEffectBadge& Badge : CardViewData.EffectBadges)
		{
			if (Badge.Kind == BadgeKind)
			{
				return &Badge;
			}
		}
		return nullptr;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattlePresentationStackUsesCardTargetPreviewSpec,
	"Wacom.UI.Battle.PresentationStackUsesCardTargetPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattlePresentationStackUsesCardTargetPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindAutomationWorldForTargetPreviewPresentation();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	FCardEffect PoisonEffect =
		MakePreviewPresentationEffect(WacomTags::Effect_ApplyStatus_Poison, 1);
	UCardDefinition* PoisonCard = MakeEnemyPartCardForPreviewPresentation(
		Fixture,
		TEXT("UI.TargetPreview.PoisonSeed"),
		{ PoisonEffect });

	FCardEffect DamageEffect =
		MakePreviewPresentationEffect(WacomTags::Effect_Damage, 2);
	FMagnitudeModifier AddMod;
	AddMod.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
	AddMod.Condition.ParamTag = WacomTags::Status_Poison;
	AddMod.Op = EMagnitudeModOp::Add;
	AddMod.Value = 3;
	FMagnitudeModifier MultiplyMod;
	MultiplyMod.Condition.ConditionType = WacomTags::Condition_Target_HasStatus;
	MultiplyMod.Condition.ParamTag = WacomTags::Status_Poison;
	MultiplyMod.Op = EMagnitudeModOp::Multiply;
	MultiplyMod.Value = 2;
	DamageEffect.MagnitudeModifiers = { AddMod, MultiplyMod };
	UCardDefinition* PreviewDamageCard = MakeEnemyPartCardForPreviewPresentation(
		Fixture,
		TEXT("UI.TargetPreview.ModifiedDamage"),
		{ DamageEffect });

	UCharacterDefinition* Character = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ PoisonCard, PreviewDamageCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UEnemyDefinition* Enemy =
		Fixture.MakeSinglePartEnemyWithIntentDamage(/*Hp*/100, /*Initiative*/50, /*IntentResist*/0, /*Damage*/0);
	UBattleSession* Session = Fixture.CreateSession(Character, Enemy, /*Seed*/1);

	FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid PoisonCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, PoisonCard->CardId);
	if (!TestTrue(TEXT("Poison seed card is in hand"), PoisonCardId.IsValid()))
	{
		return false;
	}

	TestTrue(TEXT("Seed poison before HUD submit"),
		Session->ResolveCommand(FBattleCommand::MakePlayCardOnEnemyPartKey(
			PoisonCardId,
			FWacomBattleFixture::FindPartKey(Snapshot, 0))).IsOk());
	Snapshot = Session->BuildSnapshot();

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), Harness->HUD()))
	{
		return false;
	}

	Harness->AttachPresentationStack();
	Harness->SetSession(Session);
	UWacomBattleHUDDetailTest* HUD = Harness->HUD();
	if (!TestNotNull(TEXT("HUD after SetSession"), HUD))
	{
		return false;
	}

	const FGuid PreviewDamageCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Session->BuildSnapshot(), PreviewDamageCard->CardId);
	if (!TestTrue(TEXT("Preview damage card is in hand"), PreviewDamageCardId.IsValid()))
	{
		return false;
	}

	HUD->SetTargetSelectionStateForTest(PreviewDamageCardId);
	TestEqual(TEXT("Target select test state is active"), HUD->GetUIState(), EBattleUIState::TargetSelect);
	HUD->OnEnemyPartClickedByUser(MakeWorldTargetForPreviewPresentation(Session->BuildSnapshot()));

	TestEqual(TEXT("Target submit appends one stack entry"), HUD->GetPresentationStackEntryCountForTest(), 1);
	const TArray<FWacomBattlePresentationStackEntryView>& Entries =
		HUD->GetPresentationStackEntriesForTest();
	if (!TestTrue(TEXT("Presentation stack entry exists"), Entries.IsValidIndex(0)))
	{
		return false;
	}

	const FWacomCardViewEffectBadge* DamageBadge =
		FindBadge(Entries[0].CardViewData, EWacomCardViewEffectBadgeKind::Damage);
	if (!TestNotNull(TEXT("Stack entry has damage badge"), DamageBadge))
	{
		return false;
	}

	TestEqual(TEXT("Stack mini card uses target preview magnitude"), DamageBadge->Value, 10);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFirstPersonEnemyPreviewReusesStableDetailSpec,
	"Wacom.UI.Battle.FirstPersonTargetPreview.EnemyPartRepeatMoveDoesNotRestartDetailDelay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFirstPersonEnemyPreviewReusesStableDetailSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindAutomationWorldForTargetPreviewPresentation();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = MakeEnemyPartCardForPreviewPresentation(
		Fixture,
		TEXT("UI.TargetPreview.RepeatMoveDamage"),
		{ MakePreviewPresentationEffect(WacomTags::Effect_Damage, 4) });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0);
	UCharacterDefinition* CharacterDefinition = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ DamageCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UBattleSession* Session = Fixture.CreateSession(CharacterDefinition, Enemy, /*Seed*/1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId =
		FindFirstHandCardByTargetModeForTargetPreviewPresentation(Snapshot, ECardTargetMode::SingleEnemyPart);

	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity);
	AWacomPlayerCharacter* PlayerCharacter =
		World->SpawnActor<AWacomPlayerCharacter>(
			AWacomPlayerCharacter::StaticClass(),
			FTransform::Identity);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	FPreviewSceneEnemyHostActors SceneEnemy =
		SpawnSceneEnemyHostForTargetPreviewPresentation(*World, Enemy);
	ON_SCOPE_EXIT
	{
		DestroySceneEnemyHostForTargetPreviewPresentation(SceneEnemy);
		if (IsValid(PlayerCharacter))
		{
			PlayerCharacter->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Player character"), PlayerCharacter)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy part exists"), SceneEnemy.Parts.Num() > 0)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid()))
	{
		return false;
	}

	PC->Possess(PlayerCharacter);
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->TakeWidget();
	HUD->SetCardDetailReadabilityPolishForTest(true);
	HUD->CardDetailHoverDelaySeconds = 0.10f;
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	SettleBattlePresentationQueueForTargetPreviewPresentation(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHit(
		PC,
		SceneEnemy.Parts[0],
		SceneEnemy.Parts[0]->GetHitBounds());

	FWacomFirstPersonCardDragView FirstDragView =
		MakeTargetedDragViewForTargetPreviewPresentation(SourceCardId, FVector2D(500.0f, 600.0f));
	FWacomFirstPersonCardDragView SecondDragView =
		MakeTargetedDragViewForTargetPreviewPresentation(SourceCardId, FVector2D(520.0f, 600.0f));
	FirstDragView.bHasPointerViewportPosition = false;
	SecondDragView.bHasPointerViewportPosition = false;

	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, FirstDragView);
	TestEqual(TEXT("Enemy part drag resolves world target preview"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardWorldTarget);
	TestTrue(TEXT("Enemy part drag preview can submit"), DropResult.bCanSubmit);

	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, FirstDragView);
	TestFalse(TEXT("Detail is initially pending hover delay"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	HUD->TickCardDetailMotionForTest(0.05f);
	TestFalse(TEXT("Detail is still hidden before full hover delay"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, SecondDragView);
	HUD->TickCardDetailMotionForTest(0.06f);
	TestTrue(TEXT("Equivalent repeat preview keeps accumulated hover delay"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestTrue(TEXT("Detail uses section document data"),
		HUD->GetFirstPersonCardDetailDataForTest().Sections.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFirstPersonSceneHoverPreviewReusesStableDetailSpec,
	"Wacom.UI.Battle.FirstPersonTargetPreview.SceneEnemyHoverRepeatDoesNotRestartDetailDelay",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFirstPersonSceneHoverPreviewReusesStableDetailSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindAutomationWorldForTargetPreviewPresentation();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* DamageCard = MakeEnemyPartCardForPreviewPresentation(
		Fixture,
		TEXT("UI.TargetPreview.SceneHoverDamage"),
		{ MakePreviewPresentationEffect(WacomTags::Effect_Damage, 4) });
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0);
	UCharacterDefinition* CharacterDefinition = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ DamageCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UBattleSession* Session = Fixture.CreateSession(CharacterDefinition, Enemy, /*Seed*/1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId =
		FindFirstHandCardByTargetModeForTargetPreviewPresentation(Snapshot, ECardTargetMode::SingleEnemyPart);

	FActorSpawnParameters SpawnParams;
	SpawnParams.ObjectFlags |= RF_Transient;
	AWacomBattleSceneClickRouterPlayerControllerTest* PC =
		World->SpawnActor<AWacomBattleSceneClickRouterPlayerControllerTest>(
			AWacomBattleSceneClickRouterPlayerControllerTest::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	AWacomPlayerCharacter* PlayerCharacter =
		World->SpawnActor<AWacomPlayerCharacter>(
			AWacomPlayerCharacter::StaticClass(),
			FTransform::Identity,
			SpawnParams);
	UWacomBattleHUDDetailTest* HUD = NewObject<UWacomBattleHUDDetailTest>(PC);
	FPreviewSceneEnemyHostActors SceneEnemy =
		SpawnSceneEnemyHostForTargetPreviewPresentation(*World, Enemy);
	ON_SCOPE_EXIT
	{
		DestroySceneEnemyHostForTargetPreviewPresentation(SceneEnemy);
		if (IsValid(PlayerCharacter))
		{
			PlayerCharacter->Destroy();
		}
		if (IsValid(PC))
		{
			PC->Destroy();
		}
	};

	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestNotNull(TEXT("Player character"), PlayerCharacter)
		|| !TestNotNull(TEXT("Scene enemy host"), SceneEnemy.Host)
		|| !TestTrue(TEXT("Scene enemy part exists"), SceneEnemy.Parts.Num() > 0)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid()))
	{
		return false;
	}

	PC->Possess(PlayerCharacter);
	HUD->SetOwningPlayerForTest(PC);
	HUD->SetWorldForTest(World);
	HUD->TakeWidget();
	HUD->SetCardDetailReadabilityPolishForTest(true);
	HUD->CardDetailHoverDelaySeconds = 0.10f;
	HUD->SetSession(Session);
	HUD->SetBattleSceneEnemyHostsForTest({ SceneEnemy.Host });
	SettleBattlePresentationQueueForTargetPreviewPresentation(*HUD);
	HUD->SetTargetSelectionStateForTest(SourceCardId);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UWacomFirstPersonCardAnchorComponent* RuntimeAnchor =
		PlayerCharacter->GetFirstPersonCardAnchorComponent();
	UWacomFirstPersonCardAnchorSpecProbeComponent* ProbeAnchor =
		NewObject<UWacomFirstPersonCardAnchorSpecProbeComponent>(PlayerCharacter);
	if (!TestNotNull(TEXT("Runtime first-person anchor"), RuntimeAnchor)
		|| !TestNotNull(TEXT("Probe first-person anchor"), ProbeAnchor))
	{
		return false;
	}
	if (!TestTrue(TEXT("Runtime first-person anchor has battle hand entries"),
		RuntimeAnchor->HasRuntimeCardLayerData()))
	{
		return false;
	}
	ProbeAnchor->RegisterComponent();
	ProbeAnchor->FollowInterpSpeed = 0.0f;
	FWacomFirstPersonCardLayerTestAccess::SetRuntimeCardLayerEntries(*ProbeAnchor,
		RuntimeAnchor->GetRuntimeCardLayerSourceId(),
		RuntimeAnchor->GetRuntimeCardLayerEntries());
	HUD->SetFirstPersonCardAnchorForTest(ProbeAnchor);
	ProbeAnchor->RefreshAnchor(0.0f);

	bool bSourceSlotProjected = false;
	for (const FWacomFirstPersonCardLayerSlotView& SlotView :
		ProbeAnchor->BuildActiveCardLayerSlotViewsForTest())
	{
		if (SlotView.Entry.CardInstanceId == SourceCardId)
		{
			bSourceSlotProjected = SlotView.bProjected;
			break;
		}
	}
	if (!TestTrue(TEXT("Pending source card has projected first-person slot"), bSourceSlotProjected))
	{
		return false;
	}

	FWacomBattleSceneTargetClickTestAccess::SetHUD(PC, HUD);
	FWacomBattleSceneTargetClickTestAccess::SetHit(
		PC,
		SceneEnemy.Parts[0],
		SceneEnemy.Parts[0]->GetHitBounds());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest(0.03f);
	TestFalse(TEXT("Detail is initially pending hover delay after scene hover"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	HUD->TickBattleSceneEnemyPartHoverProbeForTest(0.05f);
	TestFalse(TEXT("Detail remains hidden before repeat hover"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());

	HUD->TickBattleSceneEnemyPartHoverProbeForTest(0.05f);
	TestTrue(TEXT("Equivalent repeat scene hover keeps accumulated hover delay"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestTrue(TEXT("Scene hover detail uses section document data"),
		HUD->GetFirstPersonCardDetailDataForTest().Sections.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFirstPersonHandCardPreviewShowsOnFirstUpdateSpec,
	"Wacom.UI.Battle.FirstPersonTargetPreview.HandCardPreviewShowsOnFirstUpdate",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFirstPersonHandCardPreviewShowsOnFirstUpdateSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindAutomationWorldForTargetPreviewPresentation();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* SourceCard = Fixture.MakeHandCardCostModifierCard(/*Cost*/0, /*Magnitude*/2, /*bReduceCost*/false);
	UCardDefinition* TargetCard = Fixture.MakeNoopCard(3);
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0);
	UCharacterDefinition* CharacterDefinition = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ SourceCard, TargetCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UBattleSession* Session = Fixture.CreateSession(CharacterDefinition, Enemy, /*Seed*/1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId =
		FindFirstHandCardByTargetModeForTargetPreviewPresentation(Snapshot, ECardTargetMode::HandCard);
	const FGuid TargetCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, TargetCard->CardId);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	UWacomBattleHUDDetailTest* HUD = Harness ? Harness->HUD() : nullptr;
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid())
		|| !TestTrue(TEXT("Target card exists"), TargetCardId.IsValid()))
	{
		return false;
	}

	HUD->TakeWidget();
	HUD->SetCardDetailReadabilityPolishForTest(false);
	Harness->SetSession(Session);
	SettleBattlePresentationQueueForTargetPreviewPresentation(*HUD);

	FWacomFirstPersonCardDragView DragView =
		MakeTargetedDragViewForTargetPreviewPresentation(SourceCardId, FVector2D(500.0f, 600.0f));
	DragView.CurrentTarget = FWacomInteractionTargetHandle::ForCardTarget(
		TargetCardId,
		HUD,
		FVector2D(650.0f, 600.0f));

	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, DragView);
	TestTrue(TEXT("Detail shows immediately for first hand-card target update"),
		HUD->IsFirstPersonCardDetailPanelVisibleForTest());
	TestTrue(TEXT("Source detail uses section document data"),
		HUD->GetFirstPersonCardDetailDataForTest().Sections.Num() > 0);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleFirstPersonNoTargetCommitShowsPlayerActionPreviewSpec,
	"Wacom.UI.Battle.FirstPersonTargetPreview.NoTargetCommitShowsPlayerActionPreview",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleFirstPersonNoTargetCommitShowsPlayerActionPreviewSpec::RunTest(const FString& /*Parameters*/)
{
	UWorld* World = FindAutomationWorldForTargetPreviewPresentation();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	FWacomBattleFixture Fixture;
	UCardDefinition* ShieldCard = Fixture.MakeNoopCard(/*Cost*/0);
	ShieldCard->CardId = TEXT("UI.TargetPreview.NoTargetShield");
	ShieldCard->TargetMode = ECardTargetMode::None;
	FCardEffect ShieldEffect;
	ShieldEffect.EffectType = WacomTags::Status_Shield;
	ShieldEffect.Magnitude = 10;
	ShieldEffect.Target = WacomTags::Target_Player;
	ShieldCard->Effects = { ShieldEffect };

	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(/*Hp*/50, /*Initiative*/50, /*IntentResist*/0);
	UCharacterDefinition* CharacterDefinition = Fixture.MakeCharacter(
		Fixture.MakeNoopCard(0),
		Fixture.MakeNoopCard(0),
		{ ShieldCard, Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0), Fixture.MakeNoopCard(0) });
	UBattleSession* Session = Fixture.CreateSession(CharacterDefinition, Enemy, /*Seed*/1);
	const FBattleSnapshot Snapshot = Session->BuildSnapshot();
	const FGuid SourceCardId =
		FWacomBattleFixture::FindHandInstanceByCardId(Snapshot, ShieldCard->CardId);

	TUniquePtr<FWacomBattleHUDTestHarness> Harness =
		FWacomBattleHUDTestHarness::CreateHUDWithPlayer(World);
	UWacomBattleHUDDetailTest* HUD = Harness ? Harness->HUD() : nullptr;
	if (!TestNotNull(TEXT("HUD harness"), Harness.Get())
		|| !TestNotNull(TEXT("HUD"), HUD)
		|| !TestTrue(TEXT("Source card exists"), SourceCardId.IsValid()))
	{
		return false;
	}
	if (!TestNotNull(TEXT("First-person character"), Harness->AttachFirstPersonCharacter()))
	{
		return false;
	}

	HUD->TakeWidget();
	UPlayerStatusBar* PlayerStatusBar = Harness->AttachPlayerStatusBar();
	if (!TestNotNull(TEXT("PlayerStatusBar"), PlayerStatusBar))
	{
		return false;
	}
	Harness->SetSession(Session);
	SettleBattlePresentationQueueForTargetPreviewPresentation(*HUD);
	HUD->SyncFirstPersonBattleHandLayerForTest(Snapshot);

	UTextBlock* ShieldText =
		FindTextBlockForTargetPreviewPresentation(PlayerStatusBar->WidgetTree, TEXT("ShieldText"));
	if (!TestNotNull(TEXT("ShieldText"), ShieldText))
	{
		return false;
	}

	FWacomFirstPersonCardDragView DragView =
		MakeNoTargetCommitDragViewForTargetPreviewPresentation(SourceCardId);
	const FWacomBattleCardDropResolveResult DropResult =
		HUD->ResolveFirstPersonCardDropIntentForTest(SourceCardId, DragView);
	TestEqual(TEXT("No-target commit resolves play intent"),
		DropResult.IntentKind,
		EWacomBattleCardDropIntentKind::PlayCardNoTarget);
	TestTrue(TEXT("No-target commit can submit"), DropResult.bCanSubmit);

	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, DragView);

	TestEqual(TEXT("No-target action preview applies projected shield"),
		ShieldText->GetText().ToString(),
		FString(TEXT("护盾 10")));
	TestEqual(TEXT("No-target action preview makes shield text visible"),
		ShieldText->GetVisibility(),
		ESlateVisibility::HitTestInvisible);
	TestTrue(TEXT("No-target action preview applies preview opacity"),
		PlayerStatusBar->GetRenderOpacity() < 1.0f);

	PlayerStatusBar->SetRenderOpacity(0.33f);
	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, DragView);
	TestEqual(TEXT("Equivalent no-target preview does not reapply player preview"),
		PlayerStatusBar->GetRenderOpacity(),
		0.33f);

	DragView.GestureState = EWacomFirstPersonCardGestureState::DraggingNoTargetCard;
	DragView.bCommitArmed = false;
	HUD->HandleFirstPersonCardDragUpdatedForTest(SourceCardId, DragView);
	TestEqual(TEXT("Leaving commit-ready clears player preview opacity"),
		PlayerStatusBar->GetRenderOpacity(),
		1.0f);
	return true;
}
