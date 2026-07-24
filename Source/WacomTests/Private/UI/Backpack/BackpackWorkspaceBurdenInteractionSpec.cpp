// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#if WITH_AUTOMATION_TESTS

#include "../BackpackScreenTestAccess.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceStateSubsystem.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "InputCoreTypes.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomBackpackWorkspaceStyle.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomBackpackZonePileTypes.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FRunStorageCardView MakeBurdenCard(
	UCardDefinition* Definition,
	const FGuid& InstanceId)
{
	FRunStorageCardView Card;
	Card.Instance.InstanceId = InstanceId;
	Card.Instance.Definition = Definition;
	Card.PhysicalZone = EZoneKind::BurdenZone;
	return Card;
}

const FRunStorageCardView* FindCardByDefinition(
	TConstArrayView<FRunStorageCardView> Cards,
	const UCardDefinition* Definition)
{
	return Cards.FindByPredicate(
		[Definition](const FRunStorageCardView& Card)
		{
			return Card.Instance.Definition == Definition;
		});
}

struct FBurdenInteractionFixture
{
	TStrongObjectPtr<URunSession> Run;
	TStrongObjectPtr<UWacomBackpackScreen> Screen;
	FGuid BurdenId;
	FGuid IncomingId;
};

TUniquePtr<FBurdenInteractionFixture> BuildBurdenInteractionFixture(
	UObject* Outer,
	bool bWithDeleteProvider)
{
	UCardDefinition* Bag = NewObject<UCardDefinition>(Outer);
	Bag->CardId = TEXT("Backpack.Burden.FixtureBag");
	Bag->Physique.Capacity = 8;
	UCardDefinition* DeleteProvider = NewObject<UCardDefinition>(Outer);
	DeleteProvider->CardId = TEXT("Backpack.Burden.FixtureDeleteProvider");
	DeleteProvider->Keywords.AddTag(
		WacomTags::Card_Keyword_DeleteProvider);
	UCardDefinition* BurdenCard = NewObject<UCardDefinition>(Outer);
	BurdenCard->CardId = TEXT("Backpack.Burden.FixtureStored");
	BurdenCard->Rarity = WacomTags::Card_Rarity_White;
	UCardDefinition* IncomingCard = NewObject<UCardDefinition>(Outer);
	IncomingCard->CardId = TEXT("Backpack.Burden.FixtureIncoming");

	UCharacterDefinition* Character = NewObject<UCharacterDefinition>(Outer);
	Character->CharacterId = TEXT("Backpack.Burden.FixtureCharacter");
	Character->StarterDeck.Add(Bag);
	if (bWithDeleteProvider)
	{
		Character->StarterDeck.Add(DeleteProvider);
	}

	TUniquePtr<FBurdenInteractionFixture> Fixture =
		MakeUnique<FBurdenInteractionFixture>();
	Fixture->Run.Reset(NewObject<URunSession>(Outer));
	if (!InitializeRunSessionForTest(*Fixture->Run, Character).IsOk())
	{
		return nullptr;
	}
	Fixture->Run->AcquireCardToRun(BurdenCard);
	Fixture->Run->AcquireCardToRun(IncomingCard);
	const FRunBackpackStorageSnapshot Initial =
		Fixture->Run->BuildBackpackStorageSnapshot();
	const FRunStorageCardView* BurdenSource =
		FindCardByDefinition(Initial.Flux.ContentCards, BurdenCard);
	const FRunStorageCardView* IncomingSource =
		FindCardByDefinition(Initial.Flux.ContentCards, IncomingCard);
	if (!BurdenSource || !IncomingSource)
	{
		return nullptr;
	}
	Fixture->BurdenId = BurdenSource->Instance.InstanceId;
	Fixture->IncomingId = IncomingSource->Instance.InstanceId;
	if (!Fixture->Run->MoveInstance(
			Fixture->BurdenId,
			EZoneKind::BurdenZone,
			FGuid()))
	{
		return nullptr;
	}
	const FRunBackpackStorageSnapshot Moved =
		Fixture->Run->BuildBackpackStorageSnapshot();
	if (!Moved.BurdenCards.ContainsByPredicate(
			[&Fixture](const FRunStorageCardView& Card)
			{
				return Card.Instance.InstanceId == Fixture->BurdenId;
			}))
	{
		return nullptr;
	}
	Fixture->Screen.Reset(
		FWacomBackpackScreenTestAccess::Create(
			Outer,
			Fixture->Run.Get()));
	return Fixture;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenPhysicalInteractionSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.PhysicalCardsAndMovablePile",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenPhysicalInteractionSpec::RunTest(
	const FString& Parameters)
{
	TStrongObjectPtr<UCardDefinition> Definition(NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Burden.Physical");
	const FGuid FirstId(101, 102, 103, 104);
	const FGuid SecondId(105, 106, 107, 108);

	FRunBackpackStorageSnapshot Snapshot;
	Snapshot.BurdenCards = {
		MakeBurdenCard(Definition.Get(), FirstId),
		MakeBurdenCard(Definition.Get(), SecondId)
	};
	FWacomBackpackWorkspaceStateStore Store;
	const FWacomBackpackZoneKey BurdenZone =
		FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone);
	Store.SetExpandedPile(BurdenZone);

	const FWacomBackpackWorkspaceScene Scene =
		FWacomBackpackWorkspaceSceneBuilder::Build(
			Snapshot,
			Store,
			FWacomBackpackWorkspaceCarryProjection(),
			*GetDefault<UWacomBackpackWorkspaceStyle>(),
			FVector2D(1600.0f, 900.0f));

	const FWacomBackpackWorkspaceScenePileEntry* BurdenPile =
		Scene.Piles.FindByPredicate(
			[](const FWacomBackpackWorkspaceScenePileEntry& Pile)
			{
				return Pile.Zone.Zone == EZoneKind::BurdenZone;
			});
	if (!TestNotNull(TEXT("Burden has a real pile scene entry"), BurdenPile))
	{
		return false;
	}
	TestTrue(TEXT("Burden keeps warning presentation"), BurdenPile->View.bWarning);
	TestTrue(TEXT("Burden pile can use the normal title-drag flow"), BurdenPile->View.bMovable);
	TestFalse(TEXT("Burden is not a player deposit target"),
		BurdenPile->View.bAcceptsExternalCardDrop);

	TArray<FWacomBackpackWorkspaceCardHitRecord> HitRecords;
	for (int32 Index = 0; Index < Scene.Cards.Num(); ++Index)
	{
		const FWacomBackpackWorkspaceSceneCardEntry& Card = Scene.Cards[Index];
		TestEqual(TEXT("Burden entity has no read-only identity"),
			Card.ReadOnlyKind, EWacomBackpackWorkspaceCardReadOnlyKind::None);
		TestTrue(TEXT("Expanded Burden entity is interactive"), Card.bWorkspaceInteractive);
		HitRecords.Emplace(
			Card.CardView.Instance.InstanceId,
			BurdenZone,
			Scene.CardLayouts[Index].CardCenter,
			Scene.CardLayouts[Index].LayerRank,
			Card.bWorkspaceInteractive);
	}

	FWacomBackpackWorkspaceInteractionModel Interaction;
	Interaction.ReconcileCards(BurdenZone, HitRecords);
	Interaction.SelectAllMovable(BurdenZone);
	TestEqual(TEXT("Ctrl+A selects every movable Burden entity"),
		Interaction.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>({ FirstId, SecondId }));
	TestTrue(TEXT("Selecting a Burden entity can begin multi-card Carry"),
		Interaction.BeginCarry(FirstId, FVector2D(600.0f, 400.0f), 77));
	TestEqual(TEXT("Carry preserves the Burden source identity"),
		Interaction.GetCarry().SourceZone.Zone, EZoneKind::BurdenZone);
	TestEqual(TEXT("Carry contains every selected Burden entity"),
		Interaction.GetCarry().RemainingInstanceIds,
		TArray<FGuid>({ FirstId, SecondId }));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenPileLayoutLifetimeSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.PileLayoutLifetime",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenPileLayoutLifetimeSpec::RunTest(
	const FString& Parameters)
{
	FWacomBackpackWorkspaceStateStore Store;
	const FWacomBackpackZoneKey BurdenZone =
		FWacomBackpackZoneKey::Make(EZoneKind::BurdenZone);
	FWacomBackpackWorkspacePileLayoutEntry Entry;
	Entry.NormalizedPosition = FVector2D(0.31f, 0.67f);
	Entry.LayerRank = 12;
	Entry.bHasManualPlacement = true;
	Store.SetPileLayout(BurdenZone, Entry);
	Store.SetExpandedPile(BurdenZone);

	Store.ReconcilePiles({});
	const FWacomBackpackWorkspacePileLayoutEntry* Preserved =
		Store.FindPileLayout(BurdenZone);
	if (!TestNotNull(TEXT("An empty Burden zone preserves its same-Run pile layout"), Preserved))
	{
		return false;
	}
	TestTrue(TEXT("Burden reappearance restores the exact manual position"),
		Preserved->NormalizedPosition.Equals(Entry.NormalizedPosition));
	TestEqual(TEXT("Burden reappearance restores its layer rank"),
		Preserved->LayerRank, Entry.LayerRank);
	TestFalse(TEXT("An absent Burden pile cannot remain expanded"),
		Store.GetExpandedPile().IsSet());

	Store.ResetPileLayouts();
	TestNull(TEXT("Explicit layout reset clears remembered Burden placement"),
		Store.FindPileLayout(BurdenZone));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenInboundRejectionSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.ExternalInboundRejected",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenInboundRejectionSpec::RunTest(
	const FString& Parameters)
{
	for (const FKey& ReleaseKey :
		{ EKeys::Enter, EKeys::Gamepad_FaceButton_Bottom })
	{
		TUniquePtr<FBurdenInteractionFixture> Fixture =
			BuildBurdenInteractionFixture(
				GetTransientPackage(),
				false);
		if (!TestNotNull(
				TEXT("External inbound fixture initializes"),
				Fixture.Get())
			|| !Fixture->Screen)
		{
			continue;
		}

		TestTrue(TEXT("External Flux card enters Carry"),
			FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
				*Fixture->Screen,
				TArray<FGuid>{ Fixture->IncomingId }));
		const uint64 RevisionBefore =
			Fixture->Run->GetBackpackStorageSnapshotRevision();
		const FWacomBackpackWorkspaceAutomationTestView CarryBefore =
			FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen);
		TestTrue(TEXT("Burden remains a focusable semantic target"),
			FWacomBackpackScreenTestAccess::FocusWorkspacePileTarget(
				*Fixture->Screen,
				EZoneKind::BurdenZone));
		const FWacomBackpackDropFeedbackView Feedback =
			FWacomBackpackScreenTestAccess::WorkspacePileDropFeedback(
				*Fixture->Screen,
				EZoneKind::BurdenZone);
		TestTrue(TEXT("External inbound displays Rejected"),
			Feedback.IsRejected());
		TestEqual(TEXT("External inbound explains the system-only policy"),
			Feedback.Message.ToString(),
			FString(TEXT("负重区只接收容量溢出的卡牌")));
		TestTrue(TEXT("Initial guarded semantic release is handled"),
			FWacomBackpackScreenTestAccess::SendWorkspaceScreenKeyDown(
				*Fixture->Screen,
				ReleaseKey));
		TestTrue(TEXT("Keyboard and gamepad releases share the rejection path"),
			FWacomBackpackScreenTestAccess::SendWorkspaceScreenKeyDown(
				*Fixture->Screen,
				ReleaseKey));

		const FWacomBackpackWorkspaceAutomationTestView CarryAfter =
			FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen);
		TestEqual(TEXT("Rejected inbound preserves exact Carry"),
			CarryAfter.CarriedInstanceIds,
			CarryBefore.CarriedInstanceIds);
		TestTrue(TEXT("Rejected inbound preserves semantic card feedback"),
			FWacomBackpackScreenTestAccess::IsWorkspaceCarryDropRejected(
				*Fixture->Screen));
		TestEqual(TEXT("Rejected inbound never advances storage revision"),
			Fixture->Run->GetBackpackStorageSnapshotRevision(),
			RevisionBefore);
		const FRunBackpackStorageSnapshot After =
			Fixture->Run->BuildBackpackStorageSnapshot();
		TestTrue(TEXT("Rejected card remains in Flux"),
			After.Flux.ContentCards.ContainsByPredicate(
				[&Fixture](const FRunStorageCardView& Card)
				{
					return Card.Instance.InstanceId
						== Fixture->IncomingId;
				}));
		TestEqual(TEXT("Rejected inbound adds no Burden card"),
			After.BurdenCards.Num(),
			1);

		TestTrue(TEXT("Direct pointer-target flow also resolves the target"),
			FWacomBackpackScreenTestAccess::
				ReleaseAllToPileWithSynchronousRefresh(
					*Fixture->Screen,
					EZoneKind::BurdenZone));
		TestEqual(TEXT("Pointer-target rejection also preserves revision"),
			Fixture->Run->GetBackpackStorageSnapshotRevision(),
			RevisionBefore);
		TestEqual(TEXT("Pointer-target rejection also preserves Carry"),
			FWacomBackpackScreenTestAccess::WorkspaceView(
				*Fixture->Screen).CarriedInstanceIds,
			CarryBefore.CarriedInstanceIds);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenOutboundAndPilePositionSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.OutboundAndPilePosition",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenOutboundAndPilePositionSpec::RunTest(
	const FString& Parameters)
{
	TUniquePtr<FBurdenInteractionFixture> Fixture =
		BuildBurdenInteractionFixture(
			GetTransientPackage(),
			false);
	if (!TestNotNull(
			TEXT("Burden outbound fixture initializes"),
			Fixture.Get())
		|| !Fixture->Screen)
	{
		return false;
	}

	const FVector2D CommittedPosition(0.27f, 0.63f);
	FVector2D StoredPosition;
	TestTrue(TEXT("Screen accepts a Burden pile position commit"),
		FWacomBackpackScreenTestAccess::
			CommitWorkspacePilePositionAndReadBack(
				*Fixture->Screen,
				EZoneKind::BurdenZone,
				FGuid(),
				CommittedPosition,
				StoredPosition));
	TestTrue(TEXT("Burden pile commit stores the normalized position"),
		StoredPosition.Equals(CommittedPosition));
	FWacomBackpackScreenTestAccess::DeactivateWorkspaceScreen(
		*Fixture->Screen);
	FWacomBackpackScreenTestAccess::ActivateWorkspaceScreen(
		*Fixture->Screen);
	TestTrue(TEXT("Closing and reopening preserves the same-Run pile position"),
		FWacomBackpackScreenTestAccess::ReadWorkspacePilePosition(
			*Fixture->Screen,
			EZoneKind::BurdenZone,
			FGuid(),
			StoredPosition));
	TestTrue(TEXT("Reopened Burden pile restores the committed position"),
		StoredPosition.Equals(CommittedPosition));

	FWacomBackpackScreenTestAccess::ActivateZone(
		*Fixture->Screen,
		EZoneKind::BurdenZone);
	TestTrue(TEXT("Expanded Burden entity enters Carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			TArray<FGuid>{ Fixture->BurdenId }));
	const uint64 ReturnRevision =
		Fixture->Run->GetBackpackStorageSnapshotRevision();
	TestTrue(TEXT("Burden entity can be returned to its source pile"),
		FWacomBackpackScreenTestAccess::
			ReleaseCurrentToPileWithSynchronousRefresh(
				*Fixture->Screen,
				EZoneKind::BurdenZone));
	TestEqual(TEXT("Same-zone return is presentation-only"),
		Fixture->Run->GetBackpackStorageSnapshotRevision(),
		ReturnRevision);
	TestTrue(TEXT("Same-zone return completes Carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(
			*Fixture->Screen).CarriedInstanceIds.IsEmpty());

	TestTrue(TEXT("Returned Burden entity can be carried again"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			TArray<FGuid>{ Fixture->BurdenId }));
	TestTrue(TEXT("Burden entity can use the existing authoritative move flow"),
		FWacomBackpackScreenTestAccess::
			ReleaseCurrentToPileWithSynchronousRefresh(
				*Fixture->Screen,
				EZoneKind::BattleDeck));
	const FRunBackpackStorageSnapshot Moved =
		Fixture->Run->BuildBackpackStorageSnapshot();
	TestTrue(TEXT("Successful outbound move reaches BattleDeck"),
		Moved.BattleDeckPhysicalCards.ContainsByPredicate(
			[&Fixture](const FRunStorageCardView& Card)
			{
				return Card.Instance.InstanceId == Fixture->BurdenId;
			}));
	TestFalse(TEXT("Successful outbound move removes the Burden identity"),
		Moved.BurdenCards.ContainsByPredicate(
			[&Fixture](const FRunStorageCardView& Card)
			{
				return Card.Instance.InstanceId == Fixture->BurdenId;
			}));

	TUniquePtr<FBurdenInteractionFixture> FullTargetFixture =
		BuildBurdenInteractionFixture(
			GetTransientPackage(),
			false);
	if (!TestNotNull(
			TEXT("Full-target Burden fixture initializes"),
			FullTargetFixture.Get())
		|| !FullTargetFixture->Screen)
	{
		return false;
	}
	for (int32 Index = 0; Index < 8; ++Index)
	{
		UCardDefinition* Filler =
			NewObject<UCardDefinition>(GetTransientPackage());
		Filler->CardId = FName(
			*FString::Printf(
				TEXT("Backpack.Burden.BattleFiller.%d"),
				Index));
		FullTargetFixture->Run->AcquireCardToRun(Filler);
		const FRunBackpackStorageSnapshot BeforeFillerMove =
			FullTargetFixture->Run->BuildBackpackStorageSnapshot();
		const FRunStorageCardView* FillerSource =
			FindCardByDefinition(
				BeforeFillerMove.Flux.ContentCards,
				Filler);
		if (!TestNotNull(
			TEXT("Battle filler starts in Flux"),
			FillerSource))
		{
			return false;
		}
		TestTrue(TEXT("Battle filler reaches the authoritative BattleDeck"),
			FullTargetFixture->Run->MoveInstance(
				FillerSource->Instance.InstanceId,
				EZoneKind::BattleDeck,
				FGuid()));
	}
	TestTrue(TEXT("Full-target fixture restores its source card to Burden"),
		FullTargetFixture->Run->MoveInstance(
			FullTargetFixture->BurdenId,
			EZoneKind::BurdenZone,
			FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*FullTargetFixture->Screen);
	FWacomBackpackScreenTestAccess::ActivateZone(
		*FullTargetFixture->Screen,
		EZoneKind::BurdenZone);
	TestTrue(TEXT("Burden card can be carried toward a full target"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*FullTargetFixture->Screen,
			TArray<FGuid>{ FullTargetFixture->BurdenId }));
	const uint64 FullTargetRevision =
		FullTargetFixture->Run->GetBackpackStorageSnapshotRevision();
	TestTrue(TEXT("A full BattleDeck remains focusable as a release target"),
		FWacomBackpackScreenTestAccess::FocusWorkspacePileTarget(
			*FullTargetFixture->Screen,
			EZoneKind::BattleDeck));
	TestTrue(TEXT("Run capacity failure is presented as Rejected"),
		FWacomBackpackScreenTestAccess::WorkspacePileDropFeedback(
			*FullTargetFixture->Screen,
			EZoneKind::BattleDeck).IsRejected());
	TestTrue(TEXT("Rejected capacity release is handled without mutation"),
		FWacomBackpackScreenTestAccess::
			ReleaseCurrentToPileWithSynchronousRefresh(
				*FullTargetFixture->Screen,
				EZoneKind::BattleDeck));
	TestEqual(TEXT("Capacity rejection preserves storage revision"),
		FullTargetFixture->Run->GetBackpackStorageSnapshotRevision(),
		FullTargetRevision);
	TestEqual(TEXT("Capacity rejection preserves the exact Carry"),
		FWacomBackpackScreenTestAccess::WorkspaceView(
			*FullTargetFixture->Screen).CarriedInstanceIds,
		TArray<FGuid>{ FullTargetFixture->BurdenId });
	TestTrue(TEXT("Capacity rejection keeps the entity in Burden"),
		FullTargetFixture->Run->BuildBackpackStorageSnapshot().
			BurdenCards.ContainsByPredicate(
				[&FullTargetFixture](const FRunStorageCardView& Card)
				{
					return Card.Instance.InstanceId
						== FullTargetFixture->BurdenId;
				}));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceBurdenSaleIntegrationSpec,
	"Wacom.UI.Backpack.Workspace.BurdenInteraction.SaleIntegration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceBurdenSaleIntegrationSpec::RunTest(
	const FString& Parameters)
{
	TUniquePtr<FBurdenInteractionFixture> Fixture =
		BuildBurdenInteractionFixture(
			GetTransientPackage(),
			true);
	if (!TestNotNull(
			TEXT("Burden sale fixture initializes"),
			Fixture.Get())
		|| !Fixture->Screen)
	{
		return false;
	}

	FWacomBackpackScreenTestAccess::ActivateZone(
		*Fixture->Screen,
		EZoneKind::BurdenZone);
	TestTrue(TEXT("Burden sale card enters Carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*Fixture->Screen,
			TArray<FGuid>{ Fixture->BurdenId }));
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*Fixture->Screen,
		TArray<FGuid>{ Fixture->BurdenId });
	const FRunBackpackStorageSnapshot Sold =
		Fixture->Run->BuildBackpackStorageSnapshot();
	TestFalse(TEXT("Successful sale removes the Burden entity"),
		Sold.BurdenCards.ContainsByPredicate(
			[&Fixture](const FRunStorageCardView& Card)
			{
				return Card.Instance.InstanceId == Fixture->BurdenId;
			}));
	TestEqual(TEXT("White Burden card grants the canonical sale reward"),
		Fixture->Run->GetGold(),
		1);
	const FWacomBackpackWorkspaceAutomationTestView Queued =
		FWacomBackpackScreenTestAccess::WorkspaceView(
			*Fixture->Screen);
	TestEqual(TEXT("Burden sale preserves the original Widget for departure"),
		Queued.SaleDepartureQueuedCardCount
			+ Queued.SaleDepartureActiveCardCount,
		1);

	FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
		*Fixture->Screen,
		0.0f);
	FWacomBackpackScreenTestAccess::ForceWorkspaceSaleReadiness(
		*Fixture->Screen);
	FWacomBackpackScreenTestAccess::TickWorkspaceSaleDeparture(
		*Fixture->Screen,
		0.0f);
	const TArray<FWacomBackpackSaleCardSurfaceProbe> Probes =
		FWacomBackpackScreenTestAccess::WorkspaceSaleSurfaceProbes(
			*Fixture->Screen);
	TestTrue(TEXT("Burden sale uses the existing real dissolve surface"),
		Probes.ContainsByPredicate(
			[](const FWacomBackpackSaleCardSurfaceProbe& Probe)
			{
				return Probe.bPlayedDissolveActive
					&& Probe.bUsingSurfaceEffectMaterial;
			}));

	TUniquePtr<FBurdenInteractionFixture> BatchFixture =
		BuildBurdenInteractionFixture(
			GetTransientPackage(),
			true);
	if (!TestNotNull(
			TEXT("Batch Burden sale fixture initializes"),
			BatchFixture.Get())
		|| !BatchFixture->Screen)
	{
		return false;
	}

	UCardDefinition* SecondBurdenCard =
		NewObject<UCardDefinition>(GetTransientPackage());
	SecondBurdenCard->CardId =
		TEXT("Backpack.Burden.FixtureBatchStored");
	SecondBurdenCard->Rarity = WacomTags::Card_Rarity_White;
	BatchFixture->Run->AcquireCardToRun(SecondBurdenCard);
	const FRunBackpackStorageSnapshot BeforeBatchMove =
		BatchFixture->Run->BuildBackpackStorageSnapshot();
	const FRunStorageCardView* SecondBurdenSource =
		FindCardByDefinition(
			BeforeBatchMove.Flux.ContentCards,
			SecondBurdenCard);
	if (!TestNotNull(
			TEXT("Second batch card starts in Flux"),
			SecondBurdenSource))
	{
		return false;
	}
	const FGuid SecondBurdenId =
		SecondBurdenSource->Instance.InstanceId;
	FCardInstance RefilledOriginal;
	EZoneKind RefilledOriginalZone = EZoneKind::Backpack;
	FGuid RefilledOriginalOwner;
	TestTrue(TEXT("Original batch card remains an owned physical entity"),
		BatchFixture->Run->FindInstance(
			BatchFixture->BurdenId,
			RefilledOriginal,
			RefilledOriginalZone,
			RefilledOriginalOwner));
	if (RefilledOriginalZone != EZoneKind::BurdenZone)
	{
		TestTrue(TEXT("Fixture restores the auto-refilled card to Burden"),
			BatchFixture->Run->MoveInstance(
				BatchFixture->BurdenId,
				EZoneKind::BurdenZone,
				FGuid()));
	}
	TestTrue(TEXT("Second batch card enters Burden through the Run authority"),
		BatchFixture->Run->MoveInstance(
			SecondBurdenId,
			EZoneKind::BurdenZone,
			FGuid()));
	FWacomBackpackScreenTestAccess::Refresh(*BatchFixture->Screen);
	FWacomBackpackScreenTestAccess::ActivateZone(
		*BatchFixture->Screen,
		EZoneKind::BurdenZone);
	const TArray<FGuid> BatchIds{
		BatchFixture->BurdenId,
		SecondBurdenId
	};
	TestTrue(TEXT("Multiple Burden entities enter one Carry"),
		FWacomBackpackScreenTestAccess::BeginWorkspaceCarryForIds(
			*BatchFixture->Screen,
			BatchIds));
	FRunDeckBatchDeleteRequest BatchPreviewRequest;
	BatchPreviewRequest.InstanceIds = BatchIds;
	BatchPreviewRequest.ExpectedSource.Zone = EZoneKind::BurdenZone;
	BatchPreviewRequest.ExpectedStorageRevision =
		BatchFixture->Run->GetBackpackStorageSnapshotRevision();
	const FRunDeckBatchDeletePreview BatchPreview =
		BatchFixture->Run->ValidateDeleteCardsForGoldAtomic(
			BatchPreviewRequest);
	TestTrue(
		*FString::Printf(
			TEXT("Batch Burden sale passes Run preflight (Reason=%s)"),
			*BatchPreview.Validation.DisabledReason.ToString()),
		BatchPreview.Validation.bCanExecute);
	FWacomBackpackScreenTestAccess::SubmitWorkspaceDelete(
		*BatchFixture->Screen,
		BatchIds);

	const FRunBackpackStorageSnapshot BatchSold =
		BatchFixture->Run->BuildBackpackStorageSnapshot();
	for (const FGuid& SoldId : BatchIds)
	{
		TestFalse(TEXT("Batch sale removes every Burden identity"),
			BatchSold.BurdenCards.ContainsByPredicate(
				[SoldId](const FRunStorageCardView& Card)
				{
					return Card.Instance.InstanceId == SoldId;
				}));
	}
	TestEqual(TEXT("Batch Burden sale grants both canonical rewards"),
		BatchFixture->Run->GetGold(),
		2);
	const FWacomBackpackWorkspaceAutomationTestView BatchQueued =
		FWacomBackpackScreenTestAccess::WorkspaceView(
			*BatchFixture->Screen);
	TestEqual(TEXT("Every batch-sold Burden Widget enters SaleDeparture"),
		BatchQueued.SaleDepartureQueuedCardCount
			+ BatchQueued.SaleDepartureActiveCardCount,
		2);
	return true;
}

#endif
