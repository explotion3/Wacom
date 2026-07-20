// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceSelectionSpec,
	"Wacom.UI.Backpack.Workspace.SelectionContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceSelectionSpec::RunTest(const FString& Parameters)
{
	const FGuid First(1, 1, 1, 1);
	const FGuid Second(2, 2, 2, 2);
	const FGuid ReadOnly(3, 3, 3, 3);
	const FGuid Fourth(4, 4, 4, 4);
	TArray<FWacomBackpackWorkspaceCardHitRecord> Cards = {
		{ First, FVector2D(100.0f, 100.0f), 0, true },
		{ Second, FVector2D(200.0f, 100.0f), 1, true },
		{ ReadOnly, FVector2D(300.0f, 100.0f), 2, false },
		{ Fourth, FVector2D(400.0f, 100.0f), 3, true },
	};

	FWacomBackpackWorkspaceInteractionModel Model;
	Model.ReconcileCards(FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck), Cards);
	Model.ClickCard(First, false);
	TestEqual(TEXT("Plain click replaces selection"), Model.GetSelection().OrderedSelectedInstanceIds, TArray<FGuid>{ First });
	Model.ClickCard(Second, true);
	TestEqual(TEXT("Ctrl click adds movable card"), Model.GetSelection().OrderedSelectedInstanceIds, TArray<FGuid>({ First, Second }));
	Model.ClickCard(First, true);
	TestEqual(TEXT("Ctrl click toggles selected card off"), Model.GetSelection().OrderedSelectedInstanceIds, TArray<FGuid>{ Second });
	Model.ClickCard(ReadOnly, false);
	TestEqual(TEXT("Read-only click cannot enter selection"), Model.GetSelection().OrderedSelectedInstanceIds, TArray<FGuid>{ Second });
	Model.ClickBlank();
	TestTrue(TEXT("Blank click clears selection"), Model.GetSelection().OrderedSelectedInstanceIds.IsEmpty());

	// Marquee selection follows the card body that the player can see. Touching
	// the visible edge counts even when the card center remains outside the box.
	FWacomBackpackWorkspaceCardHitRecord BodyHitCard(
		First,
		FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck),
		FVector2D(500.0f, 500.0f),
		0,
		true);
	BodyHitCard.CardSize = FVector2D(100.0f, 160.0f);
	FWacomBackpackWorkspaceInteractionModel BodyHitModel;
	BodyHitModel.ReconcileCards(MakeArrayView(&BodyHitCard, 1));
	BodyHitModel.BeginMarquee(
		FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck),
		FVector2D(400.0f, 430.0f),
		false);
	BodyHitModel.UpdateMarquee(FVector2D(450.0f, 470.0f));
	BodyHitModel.CompleteMarquee();
	TestEqual(TEXT("Marquee touching the visible card edge selects the card"),
		BodyHitModel.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>{ First });

	BodyHitModel.CancelTransientState();
	FWacomBackpackWorkspaceCardHitRecord RotatedBody = BodyHitCard;
	RotatedBody.CardSize = FVector2D(100.0f, 200.0f);
	RotatedBody.AngleDegrees = 90.0f;
	BodyHitModel.UpdateCardHitLayouts(MakeArrayView(&RotatedBody, 1));
	BodyHitModel.BeginMarquee(
		FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck),
		FVector2D(590.0f, 490.0f),
		false);
	BodyHitModel.UpdateMarquee(FVector2D(600.0f, 510.0f));
	BodyHitModel.CompleteMarquee();
	TestEqual(TEXT("Marquee intersects the card's rotated visible body"),
		BodyHitModel.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>{ First });

	BodyHitModel.BeginMarquee(
		FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck),
		FVector2D(601.0f, 490.0f),
		false);
	BodyHitModel.UpdateMarquee(FVector2D(620.0f, 510.0f));
	BodyHitModel.CompleteMarquee();
	TestTrue(TEXT("Marquee outside the rotated card body does not select it"),
		BodyHitModel.GetSelection().OrderedSelectedInstanceIds.IsEmpty());

	const FWacomBackpackZoneKey BattleZone = FWacomBackpackZoneKey::Make(EZoneKind::BattleDeck);
	Model.BeginMarquee(BattleZone, FVector2D(50.0f, 50.0f), false);
	Model.UpdateMarquee(FVector2D(350.0f, 150.0f));
	Model.CompleteMarquee();
	TestEqual(TEXT("Marquee uses card centers and excludes read-only cards"),
		Model.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>({ First, Second }));
	TArray<FWacomBackpackWorkspaceCardHitRecord> RefreshedLayouts = {
		{ First, BattleZone, FVector2D(600.0f, 100.0f), 10, true },
		{ Second, BattleZone, FVector2D(700.0f, 100.0f), 11, true },
	};
	Model.UpdateCardHitLayouts(RefreshedLayouts);
	Model.BeginMarquee(BattleZone, FVector2D(550.0f, 50.0f), false);
	Model.UpdateMarquee(FVector2D(750.0f, 150.0f));
	Model.CompleteMarquee();
	TestEqual(TEXT("Marquee consumes refreshed actual card positions"),
		Model.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>({ First, Second }));
	RefreshedLayouts = {
		{ First, BattleZone, FVector2D(100.0f, 100.0f), 0, true },
		{ Second, BattleZone, FVector2D(200.0f, 100.0f), 1, true },
	};
	Model.UpdateCardHitLayouts(RefreshedLayouts);

	Model.BeginMarquee(BattleZone, FVector2D(150.0f, 50.0f), true);
	Model.UpdateMarquee(FVector2D(450.0f, 150.0f));
	Model.CompleteMarquee();
	TestEqual(TEXT("Ctrl marquee toggles against drag-start selection"),
		Model.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>({ First, Fourth }));

	Model.SelectAllMovable();
	TestEqual(TEXT("Ctrl+A selects all and only movable physical cards"),
		Model.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>({ First, Second, Fourth }));

	const FWacomBackpackZoneKey FluxZone = FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	TArray<FWacomBackpackWorkspaceCardHitRecord> MultiZoneCards = {
		{ First, FluxZone, FVector2D(100.0f, 100.0f), 0, true },
		{ Second, BattleZone, FVector2D(200.0f, 100.0f), 1, true },
		{ Fourth, BattleZone, FVector2D(300.0f, 100.0f), 2, true },
	};
	Model.CancelTransientState();
	Model.ReconcileCards(MultiZoneCards);
	Model.ClickCard(First, false);
	Model.ClickCard(Second, true);
	TestEqual(TEXT("Selection switches atomically to the clicked source zone"),
		Model.GetSelection().OrderedSelectedInstanceIds, TArray<FGuid>{ Second });
	TestEqual(TEXT("Selection records its source zone"), Model.GetSelection().SourceZone, BattleZone);

	// A special-zone physical card can also have a battle-deck projection with the
	// same InstanceId. Browse-only projection order must never shadow the entity.
	const FGuid SharedPhysicalIdentity(9, 9, 9, 9);
	const FWacomBackpackZoneKey SpecialZone =
		FWacomBackpackZoneKey::Make(EZoneKind::SpecialZone, FGuid(7, 7, 7, 7));
	Model.CancelTransientState();
	const TArray<FWacomBackpackWorkspaceCardHitRecord> SameIdentityCards = {
		{ SharedPhysicalIdentity, BattleZone, FVector2D(100.0f), 10, false },
		{ SharedPhysicalIdentity, SpecialZone, FVector2D(200.0f), 20, true },
	};
	Model.ReconcileCards(SameIdentityCards);
	Model.ClickCard(SharedPhysicalIdentity, false);
	TestEqual(TEXT("Read-only projection cannot shadow same-ID physical entity"),
		Model.GetSelection().OrderedSelectedInstanceIds,
		TArray<FGuid>{ SharedPhysicalIdentity });
	TestEqual(TEXT("Same-ID physical entity keeps its movable source zone"),
		Model.GetSelection().SourceZone,
		SpecialZone);

	Model.CancelTransientState();
	Model.ReconcileCards(MultiZoneCards);
	Model.BeginMarquee(BattleZone, FVector2D(150.0f, 50.0f), false);
	Model.UpdateMarquee(FVector2D(350.0f, 150.0f));
	Model.CompleteMarquee();
	TestEqual(TEXT("Marquee only selects cards from its originating pile"),
		Model.GetSelection().OrderedSelectedInstanceIds, TArray<FGuid>({ Second, Fourth }));
	TestTrue(TEXT("Pile title drag enters the mutually-exclusive pile move mode"),
		Model.BeginPileMove(BattleZone, FVector2D(20.0f), FVector2D(80.0f, 90.0f)));
	Model.UpdatePileMove(FVector2D(52.0f, 36.0f));
	const FWacomBackpackWorkspacePileMoveState CompletedPileMove = Model.CompletePileMove();
	TestEqual(TEXT("Pile follows the pointer delta directly"),
		CompletedPileMove.CurrentPosition, FVector2D(112.0f, 106.0f));
	TestEqual(TEXT("Completed pile move returns to idle"),
		Model.GetMode(), EWacomBackpackWorkspaceInteractionMode::Idle);
	return true;
}

#endif
