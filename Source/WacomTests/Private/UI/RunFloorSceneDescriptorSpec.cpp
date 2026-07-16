// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Engine/World.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Testing/WacomRunFloorSceneBindingAutomationTestView.h"

namespace WacomRunFloorSceneDescriptorSpec
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

		AWacomRunFloorSceneDescriptorActor* SpawnDescriptor(
			UWacomFloorMapDefinition* FloorDefinition = nullptr) const
		{
			AWacomRunFloorSceneDescriptorActor* Descriptor =
				World ? World->SpawnActor<AWacomRunFloorSceneDescriptorActor>() : nullptr;
			if (Descriptor)
			{
				Descriptor->FloorDefinition = FloorDefinition;
			}
			return Descriptor;
		}
	};

	UWacomFloorMapDefinition* MakeFloor(const FName FloorId)
	{
		UWacomFloorMapDefinition* Floor = NewObject<UWacomFloorMapDefinition>();
		Floor->FloorId = FloorId;
		return Floor;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorSceneDescriptorResolveSpec,
	"Wacom.UI.RunSceneBinding.Descriptor.ResolveFailureMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorSceneDescriptorResolveSpec::RunTest(const FString& Parameters)
{
	using namespace WacomRunFloorSceneDescriptorSpec;

	TestEqual(
		TEXT("Null World has a stable rejection"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			nullptr, TEXT("Floor.Expected")),
		FName(TEXT("DescriptorWorldInvalid")));

	FTransientWorldFixture Fixture;
	if (!TestNotNull(TEXT("Transient World created"), Fixture.World))
	{
		return false;
	}

	TestEqual(
		TEXT("Missing Descriptor is rejected"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			Fixture.World, TEXT("Floor.Expected")),
		FName(TEXT("DescriptorMissing")));

	AWacomRunFloorSceneDescriptorActor* First = Fixture.SpawnDescriptor();
	AWacomRunFloorSceneDescriptorActor* Second = Fixture.SpawnDescriptor();
	TestEqual(
		TEXT("Duplicate Descriptors are rejected before their payloads are read"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			Fixture.World, TEXT("Floor.Expected")),
		FName(TEXT("DescriptorDuplicate")));
	Second->Destroy();

	TestEqual(
		TEXT("Null Floor definition is rejected"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			Fixture.World, TEXT("Floor.Expected")),
		FName(TEXT("DescriptorFloorMissing")));

	UWacomFloorMapDefinition* EmptyIdentityFloor = MakeFloor(NAME_None);
	First->FloorDefinition = EmptyIdentityFloor;
	TestEqual(
		TEXT("Empty FloorId is rejected"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			Fixture.World, TEXT("Floor.Expected")),
		FName(TEXT("DescriptorFloorIdMissing")));

	UWacomFloorMapDefinition* ValidFloor = MakeFloor(TEXT("Floor.Actual"));
	First->FloorDefinition = ValidFloor;
	TestEqual(
		TEXT("Expected FloorId mismatch is rejected"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			Fixture.World, TEXT("Floor.Expected")),
		FName(TEXT("DescriptorFloorMismatch")));

	TestEqual(
		TEXT("A unique matching Descriptor resolves"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorStatus(
			Fixture.World, TEXT("Floor.Actual")),
		FName(TEXT("DescriptorResolved")));
	TestTrue(
		TEXT("Resolved Floor is the exact Descriptor payload"),
		FWacomRunFloorSceneBindingAutomationTestView::ResolveDescriptorFloor(
			Fixture.World, TEXT("Floor.Actual")) == ValidFloor);
	return true;
}

#endif
