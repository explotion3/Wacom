// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Fixtures/BattleTestFixtures.h"
#include "RunSession.h"
#include "RunState.h"
#include "RunStateTypes.h"
#include "Tags/WacomGameplayTags.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UCardDefinition* MakeValidationCard(
		FWacomBattleFixture& Fx,
		int32 Capacity = 0,
		bool bTypeBContainer = false)
	{
		UCardDefinition* Card = Fx.MakeNoopCard(0);
		Card->Rarity = WacomTags::Card_Rarity_White;
		Card->Physique.Capacity = Capacity;
		if (bTypeBContainer)
		{
			Card->Physique.CapacityEffect = WacomTags::Card_CapacityEffect_Placeholder;
		}
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunDeckSpecialZoneBattleEnabledValidationSpec,
	"Wacom.Run.Deck.SpecialZoneBattleEnabledValidation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunDeckSpecialZoneBattleEnabledValidationSpec::RunTest(const FString& /*Parameters*/)
{
	FWacomBattleFixture Fx;

	UCardDefinition* TypeA = MakeValidationCard(Fx, 6);
	UCardDefinition* TypeB = MakeValidationCard(Fx, 3, /*bTypeBContainer=*/true);
	UCardDefinition* Stored = MakeValidationCard(Fx);
	UCharacterDefinition* Character = Fx.MakeCharacter(
		Fx.MakeNoopCard(1),
		Fx.MakeNoopCard(1),
		{ TypeA, TypeB });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TestTrue(TEXT("Run initializes"), Run->Initialize(Character));
	Run->AcquireCardToRun(Stored);

	const FGuid OwnerId = Run->GetRunState().SpecialZones[0].OwnerInstanceId;
	const FGuid StoredId = Run->GetBackpack().Last().InstanceId;
	const FGuid NonSpecialId = Run->GetBackpack()[0].InstanceId;

	const FRunDeckOperationValidation MissingValidation =
		Run->ValidateSetSpecialZoneCardBattleEnabled(FGuid::NewGuid(), true);
	TestFalse(TEXT("Missing instance cannot toggle"), MissingValidation.bCanExecute);
	TestEqual(TEXT("Missing instance reason"), MissingValidation.DisabledReason, WacomRunDeckOperationReasons::CardNotFound());

	const FRunDeckOperationValidation NonSpecialValidation =
		Run->ValidateSetSpecialZoneCardBattleEnabled(NonSpecialId, true);
	TestFalse(TEXT("Non-SpecialZone instance cannot toggle"), NonSpecialValidation.bCanExecute);
	TestEqual(TEXT("Non-SpecialZone reason"), NonSpecialValidation.DisabledReason, WacomRunDeckOperationReasons::NotInSpecialZone());
	const FRunDeckOperationValidation NonSpecialToggleValidation =
		Run->ValidateToggleSpecialZoneCardBattleEnabled(NonSpecialId);
	TestFalse(TEXT("Non-SpecialZone toggle validation rejects"), NonSpecialToggleValidation.bCanExecute);
	TestEqual(TEXT("Non-SpecialZone toggle reason"), NonSpecialToggleValidation.DisabledReason, WacomRunDeckOperationReasons::NotInSpecialZone());

	TestTrue(TEXT("Move stored card into SpecialZone"), Run->MoveInstance(StoredId, EZoneKind::SpecialZone, OwnerId));

	const FRunDeckOperationValidation SpecialValidation =
		Run->ValidateSetSpecialZoneCardBattleEnabled(StoredId, true);
	TestTrue(TEXT("SpecialZone instance can toggle"), SpecialValidation.bCanExecute);
	TestTrue(TEXT("SpecialZone reason is none"), SpecialValidation.DisabledReason.IsNone());
	TestTrue(TEXT("SpecialZone toggle validation can execute"),
		Run->ValidateToggleSpecialZoneCardBattleEnabled(StoredId).bCanExecute);

	int32 BroadcastCount = 0;
	Run->OnRunStateChangedNative.AddLambda([&BroadcastCount]()
	{
		++BroadcastCount;
	});

	TestTrue(TEXT("Toggle through Run succeeds"), Run->ToggleSpecialZoneCardBattleEnabled(StoredId));
	TestEqual(TEXT("Toggle broadcasts once"), BroadcastCount, 1);
	TestTrue(TEXT("Toggle enables flag"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	TestFalse(TEXT("Rejected toggle fails"), Run->ToggleSpecialZoneCardBattleEnabled(NonSpecialId));
	TestEqual(TEXT("Rejected toggle does not broadcast"), BroadcastCount, 1);
	TestTrue(TEXT("Rejected toggle leaves flag enabled"), Run->GetRunState().SpecialZones[0].Cards[0].bBattleEnabledInSpecialZone);

	return true;
}
