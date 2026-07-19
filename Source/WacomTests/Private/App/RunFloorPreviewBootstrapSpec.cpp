// Copyright Wacom. All Rights Reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "Actors/WacomRunFloorSceneDescriptorActor.h"
#include "Characters/CharacterDefinition.h"
#include "Engine/World.h"
#include "GameFramework/WacomGameMode.h"
#include "GameFramework/WacomRunFloorPreviewGameMode.h"
#include "Map/WacomFloorMapDefinition.h"
#include "Map/WacomJourneyDefinition.h"

namespace WacomRunFloorPreviewBootstrapSpec
{
	struct FTransientWorldFixture
	{
		explicit FTransientWorldFixture(const EWorldType::Type WorldType)
			: World(UWorld::CreateWorld(WorldType, false))
		{
		}

		~FTransientWorldFixture()
		{
			if (World)
			{
				World->DestroyWorld(false);
			}
		}

		template <typename TGameMode>
		TGameMode* SpawnGameMode() const
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			return World
				? World->SpawnActor<TGameMode>(
					TGameMode::StaticClass(), FTransform::Identity, Params)
				: nullptr;
		}

		AWacomRunFloorSceneDescriptorActor* SpawnDescriptor(
			UWacomFloorMapDefinition* Floor) const
		{
			FActorSpawnParameters Params;
			Params.ObjectFlags |= RF_Transient;
			AWacomRunFloorSceneDescriptorActor* Descriptor = World
				? World->SpawnActor<AWacomRunFloorSceneDescriptorActor>(
					AWacomRunFloorSceneDescriptorActor::StaticClass(),
					FTransform::Identity, Params)
				: nullptr;
			if (Descriptor)
			{
				Descriptor->FloorDefinition = Floor;
			}
			return Descriptor;
		}

		UWorld* World = nullptr;
	};

	UWacomFloorMapDefinition* MakeFloor(
		UObject* Outer,
		const FName FloorId = TEXT("Floor.Main.01"),
		const TCHAR* DisplayName = TEXT("蛇巢浅林"))
	{
		UWacomFloorMapDefinition* Floor =
			NewObject<UWacomFloorMapDefinition>(Outer, NAME_None, RF_Transient);
		Floor->FloorId = FloorId;
		Floor->DisplayName = FText::FromString(DisplayName);
		return Floor;
	}

	void ConfigureCharacter(AWacomGameMode& GameMode)
	{
		GameMode.DefaultCharacter =
			NewObject<UCharacterDefinition>(&GameMode, NAME_None, RF_Transient);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorPreviewBaseAndEnvironmentSpec,
	"Wacom.App.RunFloorPreviewBootstrap.BaseAndEnvironment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorPreviewBaseAndEnvironmentSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomRunFloorPreviewBootstrapSpec;

	FTransientWorldFixture GameFixture(EWorldType::Game);
	AWacomGameMode* Base = GameFixture.SpawnGameMode<AWacomGameMode>();
	if (!TestNotNull(TEXT("Base GameMode spawns"), Base))
	{
		return false;
	}
	UWacomJourneyDefinition* ConfiguredJourney =
		NewObject<UWacomJourneyDefinition>(Base, NAME_None, RF_Transient);
	Base->DefaultJourneyDefinition = ConfiguredJourney;
	TestTrue(TEXT("Base resolver preserves DefaultJourneyDefinition"),
		Base->ResolveJourneyDefinitionForNewRun() == ConfiguredJourney);

	AWacomRunFloorPreviewGameMode* Preview =
		GameFixture.SpawnGameMode<AWacomRunFloorPreviewGameMode>();
	ConfigureCharacter(*Preview);
	GameFixture.SpawnDescriptor(MakeFloor(Preview));
	AddExpectedError(TEXT("PreviewWorldNotPIE"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("Preview rejects a non-PIE world"),
		Preview->ResolveJourneyDefinitionForNewRun());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorPreviewFailureMatrixSpec,
	"Wacom.App.RunFloorPreviewBootstrap.FailureMatrix",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorPreviewFailureMatrixSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomRunFloorPreviewBootstrapSpec;

	{
		FTransientWorldFixture Fixture(EWorldType::PIE);
		AWacomRunFloorPreviewGameMode* Preview =
			Fixture.SpawnGameMode<AWacomRunFloorPreviewGameMode>();
		Fixture.SpawnDescriptor(MakeFloor(Preview));
		AddExpectedError(TEXT("PreviewCharacterMissing"),
			EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Missing character is rejected"),
			Preview->ResolveJourneyDefinitionForNewRun());
	}
	{
		FTransientWorldFixture Fixture(EWorldType::PIE);
		AWacomRunFloorPreviewGameMode* Preview =
			Fixture.SpawnGameMode<AWacomRunFloorPreviewGameMode>();
		ConfigureCharacter(*Preview);
		AddExpectedError(TEXT("DescriptorMissing"),
			EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Missing Descriptor is rejected"),
			Preview->ResolveJourneyDefinitionForNewRun());
	}
	{
		FTransientWorldFixture Fixture(EWorldType::PIE);
		AWacomRunFloorPreviewGameMode* Preview =
			Fixture.SpawnGameMode<AWacomRunFloorPreviewGameMode>();
		ConfigureCharacter(*Preview);
		UWacomFloorMapDefinition* Floor = MakeFloor(Preview);
		Fixture.SpawnDescriptor(Floor);
		Fixture.SpawnDescriptor(Floor);
		AddExpectedError(TEXT("DescriptorDuplicate"),
			EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Duplicate Descriptors are rejected"),
			Preview->ResolveJourneyDefinitionForNewRun());
	}
	{
		FTransientWorldFixture Fixture(EWorldType::PIE);
		AWacomRunFloorPreviewGameMode* Preview =
			Fixture.SpawnGameMode<AWacomRunFloorPreviewGameMode>();
		ConfigureCharacter(*Preview);
		Fixture.SpawnDescriptor(MakeFloor(Preview, NAME_None));
		AddExpectedError(TEXT("DescriptorFloorIdMissing"),
			EAutomationExpectedErrorFlags::Contains, 1);
		TestNull(TEXT("Empty Floor identity is rejected"),
			Preview->ResolveJourneyDefinitionForNewRun());
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFloorPreviewSuccessAndDriftSpec,
	"Wacom.App.RunFloorPreviewBootstrap.SuccessAndDrift",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFloorPreviewSuccessAndDriftSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomRunFloorPreviewBootstrapSpec;

	FTransientWorldFixture Fixture(EWorldType::PIE);
	AWacomRunFloorPreviewGameMode* Preview =
		Fixture.SpawnGameMode<AWacomRunFloorPreviewGameMode>();
	if (!TestNotNull(TEXT("Preview GameMode spawns"), Preview))
	{
		return false;
	}
	ConfigureCharacter(*Preview);
	UWacomFloorMapDefinition* Floor = MakeFloor(Preview);
	AWacomRunFloorSceneDescriptorActor* Descriptor =
		Fixture.SpawnDescriptor(Floor);
	if (!TestNotNull(TEXT("Descriptor spawns"), Descriptor))
	{
		return false;
	}

	UWacomJourneyDefinition* Journey =
		Preview->ResolveJourneyDefinitionForNewRun();
	if (!TestNotNull(TEXT("Valid PIE facts resolve a Journey"), Journey))
	{
		return false;
	}
	TestEqual(TEXT("Preview Journey identity"), Journey->JourneyId,
		FName(TEXT("Journey.Preview.Floor.Main.01")));
	TestEqual(TEXT("Preview title"), Journey->DisplayName.ToString(),
		FString(TEXT("[Preview] 蛇巢浅林")));
	TestEqual(TEXT("Exactly one Floor"), Journey->Floors.Num(), 1);
	TestTrue(TEXT("Journey uses the exact Descriptor Floor"),
		Journey->Floors[0] == Floor);
	TestEqual(TEXT("Exactly one supported character"),
		Journey->SupportedCharacters.Num(), 1);
	TestTrue(TEXT("Journey uses the configured character"),
		Journey->SupportedCharacters[0] == Preview->DefaultCharacter);
	TestFalse(TEXT("Preview has no success terminal"),
		Journey->SuccessTerminalNode.IsValid());
	TestTrue(TEXT("Journey is transient"), Journey->HasAnyFlags(RF_Transient));
	TestTrue(TEXT("GameMode owns the transient Journey"),
		Journey->GetOuter() == Preview);
	TestEqual(TEXT("Default Morning AP is preserved"),
		Journey->PhaseBudgets.Morning, 2);
	TestEqual(TEXT("Default Day AP is preserved"),
		Journey->PhaseBudgets.Day, 6);
	TestTrue(TEXT("Repeated resolution returns the same instance"),
		Preview->ResolveJourneyDefinitionForNewRun() == Journey);

	Descriptor->FloorDefinition = MakeFloor(Preview, TEXT("Floor.Main.02"),
		TEXT("蛇蜕洞窟"));
	AddExpectedError(TEXT("PreviewFloorDrift"),
		EAutomationExpectedErrorFlags::Contains, 1);
	TestNull(TEXT("Changing the Descriptor Floor fails closed"),
		Preview->ResolveJourneyDefinitionForNewRun());
	return true;
}

#endif
