// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceInteractionModel.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceNavigationController.h"
#include "../BackpackScreenTestAccess.h"
#include "Cards/CardDefinition.h"
#include "Components/CanvasPanel.h"
#include "InputCoreTypes.h"
#include "UI/Backpack/WacomBackpackWorkspaceWidget.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UObject/StrongObjectPtr.h"

namespace
{
FWacomBackpackWorkspaceNavigationTarget MakeNavigationInputTarget(
	const EWacomBackpackWorkspaceNavigationTargetKind Kind,
	const FVector2D Center,
	const FWacomBackpackZoneKey& Zone = FWacomBackpackZoneKey())
{
	FWacomBackpackWorkspaceNavigationTarget Target;
	Target.Kind = Kind;
	Target.Zone = Zone;
	Target.Center = Center;
	return Target;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceNavigationInputMappingSpec,
	"Wacom.UI.Backpack.Workspace.NavigationInput.KeyboardGamepadMappings",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceNavigationInputMappingSpec::RunTest(
	const FString& Parameters)
{
	constexpr int32 CardCount = 3;
	TStrongObjectPtr<UWacomBackpackWorkspaceWidget> Workspace(
		NewObject<UWacomBackpackWorkspaceWidget>());
	const TSharedRef<SWidget> RetainedWorkspaceSlate =
		Workspace->TakeWidget();
	TSharedPtr<FWacomBackpackWorkspaceInteractionModel> Model =
		MakeShared<FWacomBackpackWorkspaceInteractionModel>();
	Workspace->SetInteractionModel(Model, nullptr);

	TStrongObjectPtr<UCardDefinition> Definition(
		NewObject<UCardDefinition>());
	Definition->CardId = TEXT("Backpack.Navigation.Input");
	TArray<TStrongObjectPtr<UWacomDeckCardWidget>> OwnedCards;
	TArray<TObjectPtr<UWacomDeckCardWidget>> Cards;
	for (int32 Index = 0; Index < CardCount; ++Index)
	{
		TStrongObjectPtr<UWacomDeckCardWidget> Card(
			NewObject<UWacomDeckCardWidget>());
		FCardInstance Instance;
		Instance.InstanceId = FGuid(Index + 1, 810, 811, 812);
		Instance.Definition = Definition.Get();
		FRunStorageCardView StorageCard;
		StorageCard.Instance = Instance;
		StorageCard.PhysicalZone = EZoneKind::SpecialZone;
		StorageCard.ZoneOwnerInstanceId = FGuid(99, 810, 811, 812);
		StorageCard.bCanToggleBattleEnabledInSpecialZone = true;
		Card->SetStorageCardView(StorageCard);
		Card->SetWorkspaceDisplayZone(
			EZoneKind::SpecialZone,
			StorageCard.ZoneOwnerInstanceId);
		Workspace->GetStaticCardLayer()->AddChildToCanvas(Card.Get());
		Workspace->PrimeCardBaseLayout(
			*Card,
			FVector2D(180.0f + Index * 260.0f, 240.0f),
			FVector2D(220.0f, 320.0f),
			0.0f,
			Index);
		Cards.Add(Card.Get());
		OwnedCards.Add(MoveTemp(Card));
	}
	Workspace->BindWorkspaceCards(Cards, 810);

	int32 HelpRequestCount = 0;
	Workspace->OnControlsHelpRequestedNative.AddLambda(
		[&HelpRequestCount]()
		{
			++HelpRequestCount;
		});
	TestTrue(TEXT("F1 is handled by the Navigation runtime"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::F1));
	TestEqual(TEXT("F1 broadcasts exactly one help intent"),
		HelpRequestCount,
		1);

	TestTrue(TEXT("Keyboard Space selects the stable focused card"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::SpaceBar));
	TestEqual(TEXT("Space selects one card"),
		Model->GetSelection().OrderedSelectedInstanceIds.Num(),
		1);
	TestTrue(TEXT("Gamepad X uses the same selection action"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::Gamepad_FaceButton_Left));
	TestTrue(TEXT("Gamepad X toggles the same card off"),
		Model->GetSelection().OrderedSelectedInstanceIds.IsEmpty());
	TestTrue(TEXT("Space restores the current source before select-all"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::SpaceBar));

	TestTrue(TEXT("Ctrl+A is handled"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::A,
			true));
	TestEqual(TEXT("Ctrl+A selects every movable entity"),
		Model->GetSelection().OrderedSelectedInstanceIds.Num(),
		CardCount);
	TestTrue(TEXT("Right navigation is consumed inside the Workspace"),
		FWacomBackpackScreenTestAccess::NavigateWorkspace(
			*Workspace,
			EUINavigation::Right));
	TestTrue(TEXT("Navigation restores semantic focus"),
		FWacomBackpackScreenTestAccess::
			IsWorkspaceSemanticNavigationActive(*Workspace));

	TestTrue(TEXT("Gamepad A starts the same multi-card carry"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::Gamepad_FaceButton_Bottom));
	TestTrue(TEXT("The selected entity set becomes the carry set"),
		Model->IsCarrying()
			&& Model->GetCarry().RemainingInstanceIds.Num()
				== CardCount);
	const int32 InitialCarryIndex = Model->GetCarry().CurrentIndex;
	TestTrue(TEXT("Keyboard Q cycles the carried current card"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::Q));
	TestEqual(TEXT("Q moves toward the previous visible carry card"),
		Model->GetCarry().CurrentIndex,
		InitialCarryIndex - 1);
	TestTrue(TEXT("Gamepad RB uses the same carry cycling action"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::Gamepad_RightShoulder));
	TestEqual(TEXT("RB restores the previous current carry index"),
		Model->GetCarry().CurrentIndex,
		InitialCarryIndex);
	TestTrue(TEXT("Gamepad B cancels transient interaction first"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::Gamepad_FaceButton_Right));
	TestFalse(TEXT("Gamepad B clears carry state"),
		Model->IsCarrying());
	TestFalse(TEXT("Gamepad B clears logical mouse capture"),
		Model->IsMouseCaptured());

	int32 ToggleRequestCount = 0;
	for (UWacomDeckCardWidget* Card : Cards)
	{
		Card->OnBattleEnabledToggleRequestedNative.AddLambda(
			[&ToggleRequestCount](const FGuid)
			{
				++ToggleRequestCount;
			});
	}
	TestTrue(TEXT("Keyboard T requests the idle focused-card action"),
		FWacomBackpackScreenTestAccess::SendWorkspaceKeyDown(
			*Workspace,
			EKeys::T));
	TestEqual(TEXT("T broadcasts one special-card toggle intent"),
		ToggleRequestCount,
		1);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackWorkspaceNavigationReleaseTargetSpec,
	"Wacom.UI.Backpack.Workspace.NavigationInput.ReleaseTargets",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackWorkspaceNavigationReleaseTargetSpec::RunTest(
	const FString& Parameters)
{
	const FWacomBackpackZoneKey Flux =
		FWacomBackpackZoneKey::Make(EZoneKind::Backpack);
	const FWacomBackpackZoneKey Pile =
		FWacomBackpackZoneKey::Make(
			EZoneKind::BattleDeck);
	FWacomBackpackWorkspaceNavigationController Navigation;
	Navigation.ReconcileTargets(
		TArray<FWacomBackpackWorkspaceNavigationTarget>{
			MakeNavigationInputTarget(
				EWacomBackpackWorkspaceNavigationTargetKind::Flux,
				FVector2D(0.0f, 0.0f),
				Flux),
			MakeNavigationInputTarget(
				EWacomBackpackWorkspaceNavigationTargetKind::Pile,
				FVector2D(100.0f, 0.0f),
				Pile),
			MakeNavigationInputTarget(
				EWacomBackpackWorkspaceNavigationTargetKind::Delete,
				FVector2D(200.0f, 0.0f)) });
	Navigation.ActivateSemanticFocus();

	EWacomBackpackWorkspaceReleaseTargetKind Kind =
		EWacomBackpackWorkspaceReleaseTargetKind::Pointer;
	FWacomBackpackZoneKey Zone;
	TestTrue(TEXT("Focused Flux resolves a semantic release target"),
		Navigation.GetFocusedReleaseTarget(true, Kind, Zone));
	TestEqual(TEXT("Flux preserves explicit target kind"),
		Kind,
		EWacomBackpackWorkspaceReleaseTargetKind::Flux);
	TestTrue(TEXT("Flux preserves its zone identity"),
		Zone == Flux);

	TestTrue(TEXT("Navigation reaches the pile target"),
		Navigation.Move(EUINavigation::Right));
	TestTrue(TEXT("Focused pile resolves a semantic release target"),
		Navigation.GetFocusedReleaseTarget(true, Kind, Zone));
	TestEqual(TEXT("Pile preserves explicit target kind"),
		Kind,
		EWacomBackpackWorkspaceReleaseTargetKind::Pile);
	TestTrue(TEXT("Pile preserves its zone identity"),
		Zone == Pile);

	TestTrue(TEXT("Navigation reaches the delete target"),
		Navigation.Move(EUINavigation::Right));
	TestTrue(TEXT("Focused delete resolves a semantic release target"),
		Navigation.GetFocusedReleaseTarget(true, Kind, Zone));
	TestEqual(TEXT("Delete preserves explicit target kind"),
		Kind,
		EWacomBackpackWorkspaceReleaseTargetKind::Delete);
	TestFalse(TEXT("Targets are not actionable outside carry"),
		Navigation.GetFocusedReleaseTarget(false, Kind, Zone));

	Navigation.NotifyPointerInput();
	TestFalse(TEXT("Pointer input relinquishes semantic release targeting"),
		Navigation.GetFocusedReleaseTarget(true, Kind, Zone));
	return true;
}

#endif
