// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomWorldShopActor.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "../../../../WacomApp/Private/UI/Shop/WacomWorldShopEntryBoundsGuard.h"

namespace WacomWorldShopEntryBoundsGuardSpec
{
	struct FTransientWorldFixture
	{
		UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);

		~FTransientWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}

		AWacomWorldShopActor* SpawnShop() const
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			return World
				? World->SpawnActor<AWacomWorldShopActor>(
					AWacomWorldShopActor::StaticClass(),
					FTransform::Identity,
					Params)
				: nullptr;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomWorldShopEntryBoundsGuardSpec,
	"Wacom.UI.WorldShop.Input.FormalEntryBoundsYieldToWorldCards",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomWorldShopEntryBoundsGuardSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomWorldShopEntryBoundsGuardSpec;

	FTransientWorldFixture Fixture;
	AWacomWorldShopActor* Shop = Fixture.SpawnShop();
	if (!TestNotNull(TEXT("Formal shop spawns"), Shop))
	{
		return false;
	}
	UBoxComponent* ClickBounds = Shop->GetClickBounds();
	if (!TestNotNull(TEXT("Formal shop owns entry click bounds"), ClickBounds))
	{
		return false;
	}

	TestEqual(
		TEXT("Entry bounds initially support Run world click"),
		ClickBounds->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(
		TEXT("Entry bounds initially block the shared visibility trace"),
		ClickBounds->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);

	FWacomWorldShopEntryBoundsGuard Guard;
	TestTrue(
		TEXT("Formal host owner resolves its own entry bounds"),
		Guard.SuppressForHost(Shop));
	TestTrue(TEXT("Guard reports active suppression"), Guard.IsSuppressing());
	TestEqual(
		TEXT("Entry bounds stop occluding world cards while shop owns input"),
		ClickBounds->GetCollisionEnabled(),
		ECollisionEnabled::NoCollision);

	Guard.Restore();
	TestFalse(TEXT("Guard reports restored state"), Guard.IsSuppressing());
	TestEqual(
		TEXT("Entry bounds restore their exact pre-visit collision mode"),
		ClickBounds->GetCollisionEnabled(),
		ECollisionEnabled::QueryOnly);
	TestEqual(
		TEXT("Restored entry bounds can open the shop again"),
		ClickBounds->GetCollisionResponseToChannel(ECC_Visibility),
		ECR_Block);
	return true;
}

#endif
