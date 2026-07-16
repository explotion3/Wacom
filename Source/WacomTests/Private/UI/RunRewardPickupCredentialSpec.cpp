// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "GameFramework/WacomPlayerController.h"
#include "Pickups/RunPickupDefinition.h"
#include "RunSession.h"
#include "UI/WacomShopRunEventTestProbes.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UnrealType.h"

namespace
{
	void InjectCredentialRunSession(AWacomPlayerController* PlayerController, URunSession* Run)
	{
		if (FObjectProperty* Property = FindFProperty<FObjectProperty>(
			PlayerController->GetClass(), TEXT("RunSession")))
		{
			Property->SetObjectPropertyValue_InContainer(PlayerController, Run);
		}
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIRunRewardPickupCredentialForwardingSpec,
	"Wacom.UI.WorldInteraction.RunRewardPickupCredential.ActorForwardsFullDefinitionToAtomicRunSettlement",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIRunRewardPickupCredentialForwardingSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<AWacomPlayerControllerProbe> PlayerController(
		NewObject<AWacomPlayerControllerProbe>());
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>(PlayerController.Get()));
	TStrongObjectPtr<AWacomRunRewardPickupClickProbe> Pickup(
		NewObject<AWacomRunRewardPickupClickProbe>());
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>(Pickup.Get()));
	TStrongObjectPtr<UWacomRunPickupDefinition> Definition(
		NewObject<UWacomRunPickupDefinition>(Pickup.Get()));
	InjectCredentialRunSession(PlayerController.Get(), Run.Get());

	Card->CardId = TEXT("Card.Run.SerpentSigil");
	Definition->PickupId = TEXT("Pickup.Run.SerpentSigil");
	Definition->RewardType = EWacomRunPickupRewardType::Card;
	Definition->CardDefinition = Card.Get();
	Definition->GrantedCredentialIds = { TEXT("Credential.Run.SerpentSigil") };
	Pickup->PersistentId = TEXT("Floor.Main.01.Node.Key.01");
	Pickup->PickupDefinition = Definition.Get();
	Pickup->bDestroyWhenCollected = false;

	TestTrue(TEXT("Reward pickup interaction succeeds"),
		Pickup->TryInteract_Implementation(PlayerController.Get()));
	TestTrue(TEXT("Actor path grants credential from full definition"),
		Run->HasCredential(TEXT("Credential.Run.SerpentSigil")));
	TestTrue(TEXT("Actor path marks persistent pickup id"),
		Run->IsPickupCollected(Pickup->PersistentId));
	TestTrue(TEXT("Actor path still grants presentation card"),
		Run->GetRunState().Backpack.ContainsByPredicate([Card](const FCardInstance& Instance)
		{
			return Instance.Definition == Card.Get();
		})
		|| Run->GetRunState().BurdenZone.ContainsByPredicate([Card](const FCardInstance& Instance)
		{
			return Instance.Definition == Card.Get();
		}));
	return true;
}
