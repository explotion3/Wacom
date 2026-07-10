// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedActionKeyMapping.h"
#include "InputAction.h"
#include "InputCoreTypes.h"
#include "InputMappingContext.h"
#include "Input/WacomInputContextCoordinatorSubsystem.h"
#include "UI/WacomUITestAccess.h"

namespace WacomInputContextSpec
{
	struct FCoordinatorHarness
	{
		TStrongObjectPtr<APlayerController> PC;
		TStrongObjectPtr<ULocalPlayer> LocalPlayer;
		TStrongObjectPtr<UWacomInputContextCoordinatorSubsystem> Coordinator;

		FCoordinatorHarness()
			: PC(NewObject<APlayerController>())
			, LocalPlayer(NewObject<ULocalPlayer>(GEngine))
			, Coordinator(NewObject<UWacomInputContextCoordinatorSubsystem>(LocalPlayer.Get()))
		{
			Coordinator->InitializeForPlayerController(PC.Get());
		}
	};

	bool HasMapping(const UInputMappingContext* MappingContext, const TCHAR* ActionName, const FKey& Key)
	{
		if (!MappingContext)
		{
			return false;
		}

		for (const FEnhancedActionKeyMapping& Mapping : MappingContext->GetMappings())
		{
			const UInputAction* Action = Mapping.Action.Get();
			if (Action && Action->GetName() == ActionName && Mapping.Key == Key)
			{
				return true;
			}
		}
		return false;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextProfilesTest,
	"Wacom.UI.InputContext.Profiles",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextProfilesTest::RunTest(const FString& Parameters)
{
	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	TestTrue(TEXT("Exploration shows cursor for Run Tunnel movement"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Battle);
	TestTrue(TEXT("Battle shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::MainMenu);
	TestTrue(TEXT("Main menu shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextBattleRestoresExplorationTest,
	"Wacom.UI.InputContext.BattleRestoresExploration",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextBattleRestoresExplorationTest::RunTest(const FString& Parameters)
{
	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Battle);
	TestTrue(TEXT("Battle shows cursor"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	TestTrue(TEXT("Exploration returns to visible cursor profile"), Harness.Coordinator->ShouldShowMouseCursorForCurrentContextForTest());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextReplacesActiveExplorationMappingTest,
	"Wacom.UI.InputContext.ReplacesActiveExplorationMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextReplacesActiveExplorationMappingTest::RunTest(const FString& Parameters)
{
	struct FMappingOperation
	{
		bool bAdded = false;
		const UInputMappingContext* MappingContext = nullptr;
		int32 Priority = INDEX_NONE;
	};

	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	TStrongObjectPtr<UInputMappingContext> OldExplorationContext(NewObject<UInputMappingContext>());
	TStrongObjectPtr<UInputMappingContext> NewExplorationContext(NewObject<UInputMappingContext>());
	TArray<FMappingOperation> Operations;
	FWacomUITestAccess::SetInputMappingOperationObserver(
		*Harness.Coordinator,
		[&Operations](const bool bAdded, const UInputMappingContext* MappingContext, const int32 Priority)
		{
			Operations.Add({bAdded, MappingContext, Priority});
		});

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Exploration);
	Harness.Coordinator->SetMappingContexts(OldExplorationContext.Get(), nullptr);
	if (!TestEqual(TEXT("Initial exploration context is added once"), Operations.Num(), 1))
	{
		FWacomUITestAccess::SetInputMappingOperationObserver(*Harness.Coordinator, {});
		return false;
	}
	TestTrue(TEXT("Initial operation adds the old exploration context"),
		Operations[0].bAdded && Operations[0].MappingContext == OldExplorationContext.Get());

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(OldExplorationContext.Get(), nullptr);
	TestEqual(TEXT("Reapplying the same active exploration context is a no-op"), Operations.Num(), 0);

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(NewExplorationContext.Get(), nullptr);

	if (TestEqual(TEXT("Replacing an active context removes old then adds new"), Operations.Num(), 2))
	{
		TestTrue(TEXT("Old exploration context is removed first"),
			!Operations[0].bAdded && Operations[0].MappingContext == OldExplorationContext.Get());
		TestTrue(TEXT("New exploration context is added second"),
			Operations[1].bAdded
			&& Operations[1].MappingContext == NewExplorationContext.Get()
			&& Operations[1].Priority == 0);
	}

	TestEqual(TEXT("Flow remains exploration after replacement"),
		Harness.Coordinator->GetFlowContext(), EWacomInputFlowContext::Exploration);
	TestTrue(TEXT("Exploration mapping remains active after replacement"),
		FWacomUITestAccess::IsExplorationMappingActive(*Harness.Coordinator));
	TestEqual(TEXT("Configured exploration context becomes the replacement"),
		FWacomUITestAccess::GetExplorationMappingContext(*Harness.Coordinator),
		static_cast<const UInputMappingContext*>(NewExplorationContext.Get()));

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(nullptr, nullptr);
	if (TestEqual(TEXT("Clearing an active exploration context removes it once"), Operations.Num(), 1))
	{
		TestTrue(TEXT("Clearing removes the replacement exploration context"),
			!Operations[0].bAdded && Operations[0].MappingContext == NewExplorationContext.Get());
	}
	TestFalse(TEXT("Exploration mapping becomes inactive after clearing"),
		FWacomUITestAccess::IsExplorationMappingActive(*Harness.Coordinator));
	TestNull(TEXT("Configured exploration context is cleared"),
		FWacomUITestAccess::GetExplorationMappingContext(*Harness.Coordinator));

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(nullptr, nullptr);
	TestEqual(TEXT("Reapplying null exploration context is a no-op"), Operations.Num(), 0);

	FWacomUITestAccess::SetInputMappingOperationObserver(*Harness.Coordinator, {});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextReplacesActiveBattleMappingTest,
	"Wacom.UI.InputContext.ReplacesActiveBattleMapping",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextReplacesActiveBattleMappingTest::RunTest(const FString& Parameters)
{
	struct FMappingOperation
	{
		bool bAdded = false;
		const UInputMappingContext* MappingContext = nullptr;
		int32 Priority = INDEX_NONE;
	};

	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	TStrongObjectPtr<UInputMappingContext> OldBattleContext(NewObject<UInputMappingContext>());
	TStrongObjectPtr<UInputMappingContext> NewBattleContext(NewObject<UInputMappingContext>());
	TArray<FMappingOperation> Operations;
	FWacomUITestAccess::SetInputMappingOperationObserver(
		*Harness.Coordinator,
		[&Operations](const bool bAdded, const UInputMappingContext* MappingContext, const int32 Priority)
		{
			Operations.Add({bAdded, MappingContext, Priority});
		});

	Harness.Coordinator->SetFlowContext(EWacomInputFlowContext::Battle);
	Harness.Coordinator->SetMappingContexts(nullptr, OldBattleContext.Get());
	if (!TestEqual(TEXT("Initial battle context is added once"), Operations.Num(), 1))
	{
		FWacomUITestAccess::SetInputMappingOperationObserver(*Harness.Coordinator, {});
		return false;
	}
	TestTrue(TEXT("Initial operation adds the old battle context at battle priority"),
		Operations[0].bAdded
		&& Operations[0].MappingContext == OldBattleContext.Get()
		&& Operations[0].Priority == 1);

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(nullptr, OldBattleContext.Get());
	TestEqual(TEXT("Reapplying the same active battle context is a no-op"), Operations.Num(), 0);

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(nullptr, NewBattleContext.Get());
	if (TestEqual(TEXT("Replacing an active battle context removes old then adds new"), Operations.Num(), 2))
	{
		TestTrue(TEXT("Old battle context is removed first"),
			!Operations[0].bAdded && Operations[0].MappingContext == OldBattleContext.Get());
		TestTrue(TEXT("New battle context is added second"),
			Operations[1].bAdded
			&& Operations[1].MappingContext == NewBattleContext.Get()
			&& Operations[1].Priority == 1);
	}

	TestEqual(TEXT("Flow remains battle after replacement"),
		Harness.Coordinator->GetFlowContext(), EWacomInputFlowContext::Battle);
	TestTrue(TEXT("Battle mapping remains active after replacement"),
		FWacomUITestAccess::IsBattleMappingActive(*Harness.Coordinator));
	TestEqual(TEXT("Configured battle context becomes the replacement"),
		FWacomUITestAccess::GetBattleMappingContext(*Harness.Coordinator),
		static_cast<const UInputMappingContext*>(NewBattleContext.Get()));

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(nullptr, nullptr);
	if (TestEqual(TEXT("Clearing an active battle context removes it once"), Operations.Num(), 1))
	{
		TestTrue(TEXT("Clearing removes the replacement battle context"),
			!Operations[0].bAdded && Operations[0].MappingContext == NewBattleContext.Get());
	}
	TestFalse(TEXT("Battle mapping becomes inactive after clearing"),
		FWacomUITestAccess::IsBattleMappingActive(*Harness.Coordinator));
	TestNull(TEXT("Configured battle context is cleared"),
		FWacomUITestAccess::GetBattleMappingContext(*Harness.Coordinator));

	Operations.Reset();
	Harness.Coordinator->SetMappingContexts(nullptr, nullptr);
	TestEqual(TEXT("Reapplying null battle context is a no-op"), Operations.Num(), 0);

	FWacomUITestAccess::SetInputMappingOperationObserver(*Harness.Coordinator, {});
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomInputContextInteractionEventLeaseTest,
	"Wacom.UI.InputContext.InteractionEventsAreOwnerRefCounted",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomInputContextInteractionEventLeaseTest::RunTest(const FString& Parameters)
{
	WacomInputContextSpec::FCoordinatorHarness Harness;
	if (!TestNotNull(TEXT("PlayerController"), Harness.PC.Get()))
	{
		return false;
	}

	if (!TestNotNull(TEXT("Input coordinator"), Harness.Coordinator.Get()))
	{
		return false;
	}

	Harness.Coordinator->ReleaseAllPlayerControllerInteractionEvents();

	const bool bOriginalClickEvents = Harness.PC->bEnableClickEvents;
	const bool bOriginalMouseOverEvents = Harness.PC->bEnableMouseOverEvents;
	Harness.PC->bEnableClickEvents = false;
	Harness.PC->bEnableMouseOverEvents = false;

	TStrongObjectPtr<APlayerController> FirstOwner(NewObject<APlayerController>());
	TStrongObjectPtr<APlayerController> SecondOwner(NewObject<APlayerController>());

	Harness.Coordinator->AcquirePlayerControllerInteractionEvents(FirstOwner.Get(), true, true);
	Harness.Coordinator->AcquirePlayerControllerInteractionEvents(SecondOwner.Get(), true, false);
	TestTrue(TEXT("Click events enabled by leases"), Harness.PC->bEnableClickEvents);
	TestTrue(TEXT("Mouse over events enabled by first lease"), Harness.PC->bEnableMouseOverEvents);

	Harness.Coordinator->ReleasePlayerControllerInteractionEvents(FirstOwner.Get(), true, true);
	TestTrue(TEXT("Second owner keeps click events enabled"), Harness.PC->bEnableClickEvents);
	TestFalse(TEXT("Mouse over restores when its only lease releases"), Harness.PC->bEnableMouseOverEvents);

	Harness.Coordinator->ReleasePlayerControllerInteractionEvents(SecondOwner.Get(), true, false);
	TestFalse(TEXT("All click leases released"), Harness.PC->bEnableClickEvents);
	TestFalse(TEXT("All mouse over leases released"), Harness.PC->bEnableMouseOverEvents);

	Harness.PC->bEnableClickEvents = bOriginalClickEvents;
	Harness.PC->bEnableMouseOverEvents = bOriginalMouseOverEvents;
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomExplorationInputMappingAssetContractTest,
	"Wacom.UI.InputContext.ExplorationMappingAssetContract",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomExplorationInputMappingAssetContractTest::RunTest(const FString& Parameters)
{
	const UInputMappingContext* ExplorationIMC = LoadObject<UInputMappingContext>(
		nullptr,
		TEXT("/Game/Wacom/Input/IMC_Exploration.IMC_Exploration"));
	if (!TestNotNull(TEXT("IMC_Exploration asset"), ExplorationIMC))
	{
		return false;
	}

	TestTrue(TEXT("E maps to IA_Interact"),
		WacomInputContextSpec::HasMapping(ExplorationIMC, TEXT("IA_Interact"), EKeys::E));
	TestTrue(TEXT("B maps to IA_OpenBackpack"),
		WacomInputContextSpec::HasMapping(ExplorationIMC, TEXT("IA_OpenBackpack"), EKeys::B));
	TestTrue(TEXT("Escape maps to IA_OpenMenu"),
		WacomInputContextSpec::HasMapping(ExplorationIMC, TEXT("IA_OpenMenu"), EKeys::Escape));

	return true;
}
