// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/DragDropOperation.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomCardDragOperation.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Backpack/WacomZoneDropTarget.h"

#include "Cards/CardDefinition.h"

#include "UObject/StrongObjectPtr.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDragOperationDefaultsSpec,
	"Wacom.UI.Backpack.DragOperationDefaults",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDragOperationDefaultsSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, 4.5.3a DragOperation payload defaults
	TStrongObjectPtr<UWacomCardDragOperation> Op(NewObject<UWacomCardDragOperation>());

	TestFalse(TEXT("Default InstanceId invalid"), Op->InstanceId.IsValid());
	TestTrue(TEXT("Default FromZone is Backpack"), Op->FromZone == EZoneKind::Backpack);
	TestFalse(TEXT("Default FromZoneOwnerInstanceId invalid"), Op->FromZoneOwnerInstanceId.IsValid());
	TestNull(TEXT("Default Definition null"), Op->Definition.Get());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardDragPayloadSpec,
	"Wacom.UI.Backpack.DeckCardDragPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardDragPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.1/R6.2: DeckCardWidget emits a normalized Wacom drag payload.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();

	const FGuid IgnoredOwnerId = FGuid::NewGuid();
	const EZoneKind NonSpecialZones[] =
	{
		EZoneKind::Backpack,
		EZoneKind::BattleDeck,
		EZoneKind::BurdenZone,
	};

	for (const EZoneKind Zone : NonSpecialZones)
	{
		Widget->SetCard(Inst, Zone, IgnoredOwnerId);

		UWacomCardDragOperation* DragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
		TestNotNull(TEXT("DeckCardWidget emits UWacomCardDragOperation"), DragOp);
		if (!DragOp)
		{
			continue;
		}

		TestEqual(TEXT("InstanceId copied"), DragOp->InstanceId, Inst.InstanceId);
		TestTrue(TEXT("FromZone copied"), DragOp->FromZone == Zone);
		TestFalse(TEXT("Non-SpecialZone owner id normalized to invalid"), DragOp->FromZoneOwnerInstanceId.IsValid());
		TestEqual(TEXT("Definition copied"), DragOp->Definition.Get(), Card.Get());
	}

	const FGuid SpecialOwnerId = FGuid::NewGuid();
	Widget->SetCard(Inst, EZoneKind::SpecialZone, SpecialOwnerId);

	UWacomCardDragOperation* SpecialDragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
	TestNotNull(TEXT("SpecialZone drag emits UWacomCardDragOperation"), SpecialDragOp);
	if (SpecialDragOp)
	{
		TestTrue(TEXT("SpecialZone FromZone copied"), SpecialDragOp->FromZone == EZoneKind::SpecialZone);
		TestEqual(TEXT("SpecialZone owner id preserved"), SpecialDragOp->FromZoneOwnerInstanceId, SpecialOwnerId);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardDragRejectsIncompletePayloadSpec,
	"Wacom.UI.Backpack.DeckCardDragRejectsIncompletePayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardDragRejectsIncompletePayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.2: Drag source does not emit payload without both card and instance id.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TestNull(TEXT("Empty widget has no drag operation"), Widget->BuildDragOperation());

	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	FCardInstance Inst;
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::Backpack, FGuid::NewGuid());

	TestNull(TEXT("Invalid instance id has no drag operation"), Widget->BuildDragOperation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDropTargetRejectsForeignOperationSpec,
	"Wacom.UI.Backpack.DropTargetRejectsForeignOperation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDropTargetRejectsForeignOperationSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.3: DropTarget rejects non-Wacom operations before touching RunSession.
	TStrongObjectPtr<UWacomZoneDropTarget> Target(NewObject<UWacomZoneDropTarget>());
	Target->Configure(EZoneKind::BattleDeck, FGuid::NewGuid());

	TStrongObjectPtr<UDragDropOperation> ForeignOperation(NewObject<UDragDropOperation>());

	TestFalse(TEXT("Null operation rejected"), Target->TryHandleDropOperation(nullptr));
	TestFalse(TEXT("Foreign drag operation rejected"), Target->TryHandleDropOperation(ForeignOperation.Get()));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDeckCardMoveClickUnboundSpec,
	"Wacom.UI.Backpack.DeckCardMoveClickUnbound",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDeckCardMoveClickUnboundSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.6: Main card button is a display/drag hotspot, not a click-to-move command.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("MoveButton has no click bindings"), Widget->HasMoveButtonClickBindings());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackDragOperationPayloadSpec,
	"Wacom.UI.Backpack.DragOperationPayload",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackDragOperationPayloadSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, DragOperation carries stable instance source fields.
	TStrongObjectPtr<UWacomCardDragOperation> Op(NewObject<UWacomCardDragOperation>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	const FGuid InstanceId = FGuid::NewGuid();
	const FGuid OwnerId = FGuid::NewGuid();
	Op->InstanceId = InstanceId;
	Op->FromZone = EZoneKind::SpecialZone;
	Op->FromZoneOwnerInstanceId = OwnerId;
	Op->Definition = Card.Get();

	TestEqual(TEXT("InstanceId payload"), Op->InstanceId, InstanceId);
	TestTrue(TEXT("FromZone payload"), Op->FromZone == EZoneKind::SpecialZone);
	TestEqual(TEXT("Owner payload"), Op->FromZoneOwnerInstanceId, OwnerId);
	TestEqual(TEXT("Definition payload"), Op->Definition.Get(), Card.Get());

	Op->FromZone = EZoneKind::Backpack;
	Op->FromZoneOwnerInstanceId = FGuid();
	TestFalse(TEXT("Non-SpecialZone owner invalid by convention"), Op->FromZoneOwnerInstanceId.IsValid());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneBattleEnabledBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneBattleEnabledBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.10: SpecialZone cards expose an identifiable battle-enabled badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Inst.bBattleEnabledInSpecialZone = true;

	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	Widget->TakeWidget();

	TestTrue(TEXT("BattleEnabledBadge visible for selected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	Inst.bBattleEnabledInSpecialZone = false;
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());
	TestFalse(TEXT("BattleEnabledBadge collapsed for unselected SpecialZone card"), Widget->IsBattleEnabledBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackSpecialZoneTitleAndReadyBadgeSpec,
	"Wacom.UI.Backpack.SpecialZoneTitleAndReadyBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackSpecialZoneTitleAndReadyBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.7/R6.13: SpecialZone section title and battle-ready badge are deterministic.
	const FString Title = UWacomBackpackScreen::BuildSpecialZoneTitleText(
		FText::FromString(TEXT("蛛茧绒囊")),
		1,
		2).ToString();

	TestTrue(TEXT("SpecialZone title includes owner name"), Title.Contains(TEXT("蛛茧绒囊")));
	TestTrue(TEXT("SpecialZone title includes count/capacity"), Title.Contains(TEXT("1 / 2")));
	TestTrue(
		TEXT("BattleReady badge visible when owner is in BattleDeck"),
		UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::BattleDeck) != ESlateVisibility::Collapsed);
	TestEqual(
		TEXT("BattleReady badge collapsed when owner is in Backpack"),
		UWacomBackpackScreen::GetSpecialZoneBattleReadyBadgeVisibility(EZoneKind::Backpack),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackProjectedFromBadgeSpec,
	"Wacom.UI.Backpack.ProjectedFromBadge",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackProjectedFromBadgeSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.10: BattleDeck projection keeps a visible source-owner badge.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	Widget->TakeWidget();

	TestFalse(TEXT("ProjectedFromBadge hidden by default"), Widget->IsProjectedFromBadgeVisible());

	const FText SourceText = FText::FromString(TEXT("来自 蛛茧绒囊"));
	Widget->SetProjectedFromBadgeText(SourceText);

	TestTrue(TEXT("ProjectedFromBadge visible when text is set"), Widget->IsProjectedFromBadgeVisible());
	TestEqual(TEXT("ProjectedFromBadge text preserved"), Widget->GetProjectedFromBadgeText().ToString(), SourceText.ToString());

	Widget->SetProjectedFromBadgeText(FText::GetEmpty());
	TestFalse(TEXT("ProjectedFromBadge collapsed when text is cleared"), Widget->IsProjectedFromBadgeVisible());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBattleEnabledToggleRequestSpec,
	"Wacom.UI.Backpack.BattleEnabledToggleRequest",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBattleEnabledToggleRequestSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.11: right-click toggle path emits one request for the card instance.
	TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());

	FCardInstance Inst;
	Inst.InstanceId = FGuid::NewGuid();
	Inst.Definition = Card.Get();
	Widget->SetCard(Inst, EZoneKind::SpecialZone, FGuid::NewGuid());

	int32 ToggleCount = 0;
	FGuid LastToggledId;
	Widget->OnBattleEnabledToggleRequestedNative.AddLambda(
		[&ToggleCount, &LastToggledId](FGuid InstanceId)
		{
			++ToggleCount;
			LastToggledId = InstanceId;
		});

	TestFalse(TEXT("Toggle disabled by default"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("No request emitted while disabled"), ToggleCount, 0);

	Widget->SetRightClickToggleEnabled(true);
	TestTrue(TEXT("Toggle request accepted when enabled"), Widget->RequestBattleEnabledToggle());
	TestEqual(TEXT("One toggle request emitted"), ToggleCount, 1);
	TestEqual(TEXT("Toggle request carries instance id"), LastToggledId, Inst.InstanceId);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBattleDeckFullPreviewRejectsBackpackDropSpec,
	"Wacom.UI.Backpack.BattleDeckFullPreviewRejectsBackpackDrop",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBattleDeckFullPreviewRejectsBackpackDropSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.12: BattleDeck preview rejects Backpack-origin drops when capacity is full.
	TestFalse(
		TEXT("Full BattleDeck rejects Backpack-origin preview"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BattleDeck, EZoneKind::Backpack, 2, 2));

	TestTrue(
		TEXT("BattleDeck accepts Backpack-origin preview when there is capacity"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BattleDeck, EZoneKind::Backpack, 1, 2));

	TestTrue(
		TEXT("BattleDeck in-place preview is not rejected by capacity"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::BattleDeck, EZoneKind::BattleDeck, 2, 2));

	TestTrue(
		TEXT("Non-BattleDeck target preview is not rejected by BattleDeck capacity"),
		UWacomZoneDropTarget::ShouldPreviewDrop(EZoneKind::SpecialZone, EZoneKind::Backpack, 2, 2));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec,
	"Wacom.UI.Backpack.BurdenZoneTitleAndCardOrder",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackBurdenZoneTitleAndCardOrderSpec::RunTest(const FString& /*Parameters*/)
{
	// Feature: backpack-special-zone-stage-4-5, R6.8: BurdenZone title count and rendered card widgets preserve instance order.
	TestTrue(
		TEXT("BurdenZone title includes card count"),
		UWacomBackpackScreen::BuildBurdenZoneTitleText(3).ToString().Contains(TEXT("3")));

	TArray<FCardInstance> BurdenCards;
	TArray<TStrongObjectPtr<UCardDefinition>> CardDefinitions;
	for (int32 Index = 0; Index < 3; ++Index)
	{
		TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
		FCardInstance Inst;
		Inst.InstanceId = FGuid::NewGuid();
		Inst.Definition = Card.Get();

		CardDefinitions.Add(MoveTemp(Card));
		BurdenCards.Add(Inst);
	}

	for (int32 Index = 0; Index < BurdenCards.Num(); ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Widget(NewObject<UWacomDeckCardWidget>());
		Widget->SetCard(BurdenCards[Index], EZoneKind::BurdenZone, FGuid::NewGuid());

		UWacomCardDragOperation* DragOp = Cast<UWacomCardDragOperation>(Widget->BuildDragOperation());
		TestNotNull(TEXT("Burden card widget emits drag operation"), DragOp);
		if (!DragOp)
		{
			continue;
		}

		TestEqual(TEXT("Burden card order preserves instance id"), DragOp->InstanceId, BurdenCards[Index].InstanceId);
		TestEqual(TEXT("Burden card order preserves definition"), DragOp->Definition.Get(), BurdenCards[Index].Definition.Get());
		TestTrue(TEXT("Burden card source zone is BurdenZone"), DragOp->FromZone == EZoneKind::BurdenZone);
		TestFalse(TEXT("Burden card owner id normalized to invalid"), DragOp->FromZoneOwnerInstanceId.IsValid());
	}

	return true;
}
