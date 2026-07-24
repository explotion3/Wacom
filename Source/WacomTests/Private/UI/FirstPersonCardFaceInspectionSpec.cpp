// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Fixtures/WacomRunExplorationFixture.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/WacomPlayerCharacter.h"
#include "GameFramework/WacomPlayerController.h"
#include "Input/Events.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Card/WacomCardView.h"
#include "UI/Card/WacomFirstPersonCardLayerSlotWidget.h"
#include "UI/Card/WacomFirstPersonCardLayerWidget.h"
#include "UI/FirstPersonCardLayerTestAccess.h"
#include "UI/Foundation/WacomGameViewportClient.h"
#include "UI/GameViewportClientTestAccess.h"
#include "UI/RunFirstPersonCardLayerSpecReceiver.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomFirstPersonCardFaceInspectionSpec
{
	UWorld* FindAutomationWorld()
	{
		if (GEngine)
		{
			for (const FWorldContext& Context : GEngine->GetWorldContexts())
			{
				if (UWorld* World = Context.World())
				{
					return World;
				}
			}
		}
		return GWorld;
	}

	void EnableTestRunFace(UCardDefinition& Card)
	{
		Card.RunFace.bEnabled = true;
		Card.RunFace.Description = FText::FromString(TEXT("探索测试说明"));
		Card.RunFace.TargetMode = EWacomRunCardTargetMode::WorldTarget;
		Card.RunFace.PrimaryAction.ActionTag = WacomTags::Run_Card_Action_Unlock;
		Card.RunFace.PrimaryAction.Magnitude = 1;
	}

	FWacomFirstPersonCardLayerSlotView MakeDualFaceSlot(
		const FGuid& CardInstanceId,
		const FVector2D& Position,
		const bool bAllowLockedInspection = true)
	{
		FWacomFirstPersonCardLayerSlotView Slot;
		Slot.Index = 0;
		Slot.Entry.CardInstanceId = CardInstanceId;
		Slot.Entry.CardViewData.Name = FText::FromString(TEXT("战斗面"));
		Slot.Entry.CardViewData.Cost = 3;
		Slot.Entry.CardViewData.bShowCost = true;
		Slot.Entry.DefaultFaceContext = EWacomCardFaceContext::Battle;
		Slot.Entry.AlternateFaceCardViewData.Name = FText::FromString(TEXT("探索面"));
		Slot.Entry.AlternateFaceCardViewData.TypeText = FText::FromString(TEXT("探索"));
		Slot.Entry.AlternateFaceCardViewData.bShowCost = false;
		Slot.Entry.bHasAlternateFace = true;
		Slot.Entry.bAllowLockedFaceInspection = bAllowLockedInspection;
		Slot.Entry.bIsPlayable = true;
		Slot.Entry.InteractionIntent =
			EWacomFirstPersonCardInteractionIntent::CommitNoTarget;
		Slot.ScreenPosition = Position;
		Slot.WidgetPosition = Position;
		Slot.SnappedWidgetPosition = Position;
		Slot.InputHitCenter = Position;
		Slot.InputHitScale = 1.0f;
		Slot.InputHitOrder = 0;
		Slot.RenderScale = 1.0f;
		Slot.RenderOpacity = 1.0f;
		Slot.bProjected = true;
		return Slot;
	}

	FWacomFirstPersonCardDragConfig MakeInspectConfig()
	{
		FWacomFirstPersonCardDragConfig Config;
		Config.CardInspectHoldDelaySeconds = 0.05f;
		Config.CardDragStartThresholdPixels = 10.0f;
		Config.CardInspectScrubHandPaddingPixels = FVector2D(40.0f, 60.0f);
		return Config;
	}

	FPointerEvent MakeLeftMouseDownEvent(const FVector2D& ScreenPosition)
	{
		return FPointerEvent(
			0,
			ScreenPosition,
			ScreenPosition,
			TSet<FKey>{ EKeys::LeftMouseButton },
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());
	}

	FPointerEvent MakeLeftMouseUpEvent(const FVector2D& ScreenPosition)
	{
		return FPointerEvent(
			0,
			ScreenPosition,
			ScreenPosition,
			TSet<FKey>(),
			EKeys::LeftMouseButton,
			0.0f,
			FModifierKeysState());
	}

	class FFaceReceiver
	{
	public:
		int32 LockedCount = 0;
		int32 ChangedCount = 0;
		int32 ClosedCount = 0;
		EWacomCardFaceContext LastFace = EWacomCardFaceContext::Battle;

		void HandleLocked(
			const FGuid&,
			const EWacomCardFaceContext Face,
			const FWacomFirstPersonCardLayerSlotView&)
		{
			++LockedCount;
			LastFace = Face;
		}

		void HandleChanged(
			const FGuid&,
			const EWacomCardFaceContext Face,
			const FWacomFirstPersonCardLayerSlotView&)
		{
			++ChangedCount;
			LastFace = Face;
		}

		void HandleClosed(
			const FGuid&,
			const EWacomCardFaceContext Face,
			const FWacomFirstPersonCardLayerSlotView&)
		{
			++ClosedCount;
			LastFace = Face;
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunFirstPersonCardFaceDefaultsSpec,
	"Wacom.UI.RunFirstPersonCardLayer.FaceContext.RunFaceDefaultsAndBattleFallback",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunFirstPersonCardFaceDefaultsSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardFaceInspectionSpec;

	FWacomBattleFixture Fixture;
	UCardDefinition* DualFace = Fixture.MakeNoopCard(4);
	DualFace->CardId = TEXT("Test.DualFace");
	DualFace->DisplayName = FText::FromString(TEXT("共享名称"));
	EnableTestRunFace(*DualFace);
	UCardDefinition* BattleOnly = Fixture.MakeNoopCard(2);
	BattleOnly->CardId = TEXT("Test.BattleOnly");
	BattleOnly->DisplayName = FText::FromString(TEXT("旧卡"));
	UCardDefinition* Pack = Fixture.MakeNoopCard(0);
	Pack->CardId = TEXT("Test.Pack");
	Pack->Physique.Capacity = 3;
	UCharacterDefinition* Character =
		Fixture.MakeCharacter(nullptr, nullptr, { DualFace, BattleOnly, Pack });

	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	if (!TestTrue(
			TEXT("Run initializes"),
			InitializeRunSessionForTest(*Run, Character).IsOk()))
	{
		return false;
	}

	TStrongObjectPtr<UWacomRunFirstPersonCardSourceSpecProbeComponent> Source(
		NewObject<UWacomRunFirstPersonCardSourceSpecProbeComponent>());
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	TestTrue(
		TEXT("Run source builds entries"),
		Source->BuildRunFirstPersonCardEntries(*Run, Entries));
	if (!TestEqual(TEXT("Both physical cards are present"), Entries.Num(), 2))
	{
		return false;
	}

	const FWacomFirstPersonCardLayerEntry* DualEntry = Entries.FindByPredicate(
		[DualFace](const FWacomFirstPersonCardLayerEntry& Entry)
		{
			return Entry.CardViewData.Name.EqualTo(DualFace->DisplayName)
				&& Entry.DefaultFaceContext == EWacomCardFaceContext::Run;
		});
	const FWacomFirstPersonCardLayerEntry* BattleOnlyEntry = Entries.FindByPredicate(
		[](const FWacomFirstPersonCardLayerEntry& Entry)
		{
			return Entry.CardViewData.Name.ToString() == TEXT("旧卡");
		});
	if (!TestNotNull(TEXT("Dual-faced entry"), DualEntry)
		|| !TestNotNull(TEXT("Battle-only entry"), BattleOnlyEntry))
	{
		return false;
	}

	TestEqual(
		TEXT("Run face uses exploration type"),
		DualEntry->CardViewData.TypeText.ToString(),
		FString(TEXT("探索")));
	TestFalse(TEXT("Run face hides Battle cost"), DualEntry->CardViewData.bShowCost);
	TestTrue(TEXT("Run default hand exposes alternate face"), DualEntry->bHasAlternateFace);
	TestTrue(
		TEXT("Run default hand allows locked inspection"),
		DualEntry->bAllowLockedFaceInspection);
	TestTrue(
		TEXT("Alternate Battle face retains cost"),
		DualEntry->AlternateFaceCardViewData.bShowCost);
	TestEqual(
		TEXT("Alternate Battle face retains authored cost"),
		DualEntry->AlternateFaceCardViewData.Cost,
		4);
	TestEqual(
		TEXT("Entry preserves the physical instance identity"),
		DualEntry->CardInstanceId,
		Run->GetBattleDeck()[0].Definition == DualFace
			? Run->GetBattleDeck()[0].InstanceId
			: Run->GetBattleDeck()[1].InstanceId);

	TestEqual(
		TEXT("Legacy card defaults to Battle"),
		BattleOnlyEntry->DefaultFaceContext,
		EWacomCardFaceContext::Battle);
	TestFalse(
		TEXT("Legacy card has no alternate face"),
		BattleOnlyEntry->bHasAlternateFace);
	TestTrue(
		TEXT("Legacy Battle fallback retains cost"),
		BattleOnlyEntry->CardViewData.bShowCost);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLockedFaceInspectionSpec,
	"Wacom.UI.FirstPersonCardLayer.FaceInspection.LockToggleGateAndClose",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLockedFaceInspectionSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardFaceInspectionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC =
		World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer =
		NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FVector2D CardPosition(500.0f, 650.0f);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
		*Layer,
		MakeInspectConfig());
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(
		*Layer,
		FVector2D(1000.0f, 1000.0f));
	Layer->SetCardSlots({ MakeDualFaceSlot(CardId, CardPosition) });

	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot"), Slot))
	{
		PC->Destroy();
		return false;
	}

	FFaceReceiver Receiver;
	Layer->OnCardFaceInspectLockedNative.AddRaw(
		&Receiver,
		&FFaceReceiver::HandleLocked);
	Layer->OnCardFaceChangedNative.AddRaw(
		&Receiver,
		&FFaceReceiver::HandleChanged);
	Layer->OnCardFaceInspectClosedNative.AddRaw(
		&Receiver,
		&FFaceReceiver::HandleClosed);

	TestTrue(
		TEXT("Press begins gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
			*Layer,
			CardPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*Slot,
		0.06f,
		CardPosition);
	TestEqual(
		TEXT("Hold enters inspecting"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Inspecting);
	TestEqual(
		TEXT("Release inside hand releases capture"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(
			*Layer,
			CardPosition),
		EWacomFirstPersonCardPointerRouteAction::ReleaseMouseCapture);
	TestEqual(
		TEXT("Dual face release locks inspection"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::InspectLocked);
	TestTrue(TEXT("Layer exposes locked state"), Layer->IsLockedCardInspectionActive());
	TestFalse(TEXT("Locked inspection is not a gameplay drag"), Layer->IsCardDragGestureActive());
	TestEqual(TEXT("Locked event fires once"), Receiver.LockedCount, 1);
	TestEqual(
		TEXT("Default face remains visible on lock"),
		Slot->GetInnerCardView()->GetCardViewData().Name.ToString(),
		FString(TEXT("战斗面")));

	TestTrue(TEXT("First toggle starts"), Layer->TryToggleLockedCardFace());
	TestFalse(TEXT("Repeated toggle is gated"), Layer->TryToggleLockedCardFace());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.11f);
	TestEqual(
		TEXT("Data swaps at midpoint"),
		Slot->GetInnerCardView()->GetCardViewData().Name.ToString(),
		FString(TEXT("探索面")));
	TestEqual(TEXT("Face changed event fires at midpoint"), Receiver.ChangedCount, 1);
	TestEqual(TEXT("Changed face is Run"), Receiver.LastFace, EWacomCardFaceContext::Run);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.11f);
	TestTrue(TEXT("Toggle becomes available after playback"), Layer->TryToggleLockedCardFace());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.22f);
	TestEqual(
		TEXT("Second toggle restores Battle face"),
		Slot->GetInnerCardView()->GetCardViewData().Name.ToString(),
		FString(TEXT("战斗面")));

	FWacomFirstPersonCardInteractionFeedbackConfig ReducedMotionConfig =
		FWacomFirstPersonCardLayerTestAccess::View(*Slot).SlotRuntimeConfig.Interaction;
	ReducedMotionConfig.bReduceInteractionMotion = true;
	FWacomFirstPersonCardLayerTestAccess::SetInteractionFeedbackConfig(
		*Slot,
		ReducedMotionConfig);
	TestTrue(TEXT("Reduced-motion toggle starts"), Layer->TryToggleLockedCardFace());
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.05f);
	TestTrue(
		TEXT("Reduced motion does not horizontally fold"),
		Slot->GetCardView()->GetRenderTransform().Scale.Equals(FVector2D(1.0f, 1.0f)));
	TestEqual(
		TEXT("Reduced motion still swaps at midpoint"),
		Slot->GetInnerCardView()->GetCardViewData().Name.ToString(),
		FString(TEXT("探索面")));
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.05f);

	TestTrue(
		TEXT("Card-body click starts a locked face toggle"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
			*Layer,
			CardPosition));
	TestEqual(
		TEXT("Card-body release is consumed without releasing locked inspection"),
		FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(
			*Layer,
			CardPosition),
		EWacomFirstPersonCardPointerRouteAction::Handled);
	TestEqual(
		TEXT("Card-body release keeps inspection locked"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::InspectLocked);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 0.10f);
	TestEqual(
		TEXT("Card-body click completes the face toggle"),
		Slot->GetInnerCardView()->GetCardViewData().Name.ToString(),
		FString(TEXT("战斗面")));

	TestTrue(
		TEXT("Blank press closes locked inspection"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
			*Layer,
			FVector2D(40.0f, 40.0f)));
	TestEqual(
		TEXT("Close returns slot to idle"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestEqual(TEXT("Close event fires once"), Receiver.ClosedCount, 1);
	TestTrue(
		TEXT("Outside close consumes the matching release"),
		Layer->ConsumePendingLockedInspectionPointerRelease());
	TestFalse(
		TEXT("Release consumption is one-shot"),
		Layer->ConsumePendingLockedInspectionPointerRelease());
	TestEqual(
		TEXT("Close restores default face"),
		Slot->GetInnerCardView()->GetCardViewData().Name.ToString(),
		FString(TEXT("战斗面")));
	TestTrue(
		TEXT("Close restores local face transform"),
		Slot->GetCardView()->GetRenderTransform().Scale.Equals(FVector2D(1.0f, 1.0f)));
	TestEqual(
		TEXT("Close restores local opacity"),
		Slot->GetCardView()->GetRenderOpacity(),
		1.0f);

	Layer->OnCardFaceInspectLockedNative.RemoveAll(&Receiver);
	Layer->OnCardFaceChangedNative.RemoveAll(&Receiver);
	Layer->OnCardFaceInspectClosedNative.RemoveAll(&Receiver);
	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLockedInspectionPresentationRefreshSpec,
	"Wacom.UI.FirstPersonCardLayer.FaceInspection.LockedInspectionSurvivesPresentationRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLockedInspectionPresentationRefreshSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardFaceInspectionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}

	APlayerController* PC =
		World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer =
		NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("PlayerController"), PC)
		|| !TestNotNull(TEXT("Layer"), Layer))
	{
		return false;
	}

	const FVector2D CardPosition(500.0f, 650.0f);
	const FGuid CardId = FGuid::NewGuid();
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
		*Layer,
		MakeInspectConfig());
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(
		*Layer,
		FVector2D(1000.0f, 1000.0f));
	FWacomFirstPersonCardLayerSlotView InitialSlot =
		MakeDualFaceSlot(CardId, CardPosition);
	InitialSlot.AnchorWidgetPosition = FVector2D(500.0f, 600.0f);
	Layer->SetCardSlots({ InitialSlot });

	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot"), Slot))
	{
		PC->Destroy();
		return false;
	}

	TestTrue(
		TEXT("Press begins gesture"),
		FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
			*Layer,
			CardPosition));
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*Slot,
		0.06f,
		CardPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(
		*Layer,
		CardPosition);
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 1.0f);
	TestEqual(
		TEXT("Dual face is locked before refresh"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::InspectLocked);

	TestTrue(
		TEXT("Locked pointer move remains routed to the active slot"),
		FWacomFirstPersonCardLayerTestAccess::HandleSlotPointerMovedAtWidgetPosition(
			*Layer,
			*Slot,
			FVector2D(520.0f, 470.0f)));
	FWacomFirstPersonCardLayerSlotView RefreshedSlot =
		MakeDualFaceSlot(CardId, CardPosition);
	RefreshedSlot.AnchorWidgetPosition = InitialSlot.AnchorWidgetPosition;
	RefreshedSlot.bIsHovered = true;
	Layer->SetCardSlots({ RefreshedSlot });
	FWacomFirstPersonCardLayerTestAccess::TickSlotMotion(*Slot, 1.0f);

	TestEqual(
		TEXT("Same-card presentation refresh preserves locked inspection"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::InspectLocked);
	TestTrue(TEXT("Layer still exposes locked state"), Layer->IsLockedCardInspectionActive());
	TestTrue(
		TEXT("Locked card remains away from its hand position"),
		Slot->GetVisualSlotView().ScreenPosition.Y < CardPosition.Y - 100.0f);

	RefreshedSlot.Entry.bAllowLockedFaceInspection = false;
	Layer->SetCardSlots({ RefreshedSlot });
	TestEqual(
		TEXT("Semantic source takeover still closes locked inspection"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);

	PC->Destroy();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardLockedInspectionViewportBlankClickSpec,
	"Wacom.UI.FirstPersonCardLayer.FaceInspection.ViewportBlankClickClosesLockedInspection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardLockedInspectionViewportBlankClickSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardFaceInspectionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World)
		|| !TestTrue(TEXT("Slate application is initialized"), FSlateApplication::IsInitialized()))
	{
		return false;
	}

	AWacomPlayerController* PC = World->SpawnActor<AWacomPlayerController>(
		AWacomPlayerController::StaticClass(),
		FTransform::Identity);
	AWacomPlayerCharacter* Character = World->SpawnActor<AWacomPlayerCharacter>(
		AWacomPlayerCharacter::StaticClass(),
		FTransform::Identity);
	UWacomGameViewportClient* ViewportClient =
		NewObject<UWacomGameViewportClient>(GEngine);
	if (!TestNotNull(TEXT("Player controller"), PC)
		|| !TestNotNull(TEXT("Player character"), Character)
		|| !TestNotNull(TEXT("Viewport client"), ViewportClient))
	{
		return false;
	}

	auto Cleanup = [PC, Character, ViewportClient]()
	{
		if (ViewportClient)
		{
			FWacomGameViewportClientTestAccess::SetRouteOverrides(
				*ViewportClient,
				TOptional<bool>(),
				nullptr);
			FWacomGameViewportClientTestAccess::UnregisterInputPreProcessor(
				*ViewportClient);
		}
		if (PC)
		{
			PC->UnPossess();
			PC->Destroy();
		}
		if (Character)
		{
			Character->Destroy();
		}
	};

	PC->Possess(Character);
	UWacomFirstPersonCardAnchorComponent* Anchor =
		Character->GetFirstPersonCardAnchorComponent();
	UWacomFirstPersonCardLayerWidget* Layer =
		NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	if (!TestNotNull(TEXT("First-person card anchor"), Anchor)
		|| !TestNotNull(TEXT("First-person card layer"), Layer))
	{
		Cleanup();
		return false;
	}

	const FVector2D CardPosition(500.0f, 650.0f);
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
		*Layer,
		MakeInspectConfig());
	Layer->SetCardSlots({ MakeDualFaceSlot(FGuid::NewGuid(), CardPosition) });
	FWacomFirstPersonCardLayerTestAccess::SetCardLayer(*Anchor, Layer);
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Card slot"), Slot))
	{
		Cleanup();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
		*Layer,
		CardPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*Slot,
		0.06f,
		CardPosition);
	FWacomFirstPersonCardLayerTestAccess::RequestReleaseRouteActionAtWidgetPosition(
		*Layer,
		CardPosition);
	TestTrue(
		TEXT("Card is locked before viewport blank click"),
		Anchor->IsFirstPersonCardLockedInspectionActive());

	FWacomGameViewportClientTestAccess::SetRouteOverrides(
		*ViewportClient,
		true,
		PC);
	FWacomGameViewportClientTestAccess::RegisterInputPreProcessor(*ViewportClient);
	TestTrue(
		TEXT("Viewport preprocessor consumes blank press while inspection is locked"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonDown(
			*ViewportClient,
			MakeLeftMouseDownEvent(FVector2D(40.0f, 40.0f))));
	TestFalse(
		TEXT("Viewport blank press closes locked inspection"),
		Anchor->IsFirstPersonCardLockedInspectionActive());
	TestTrue(
		TEXT("Viewport preprocessor consumes the matching release"),
		FWacomGameViewportClientTestAccess::DispatchMouseButtonUp(
			*ViewportClient,
			MakeLeftMouseUpEvent(FVector2D(40.0f, 40.0f))));
	TestFalse(
		TEXT("Release reservation is cleared after viewport consumption"),
		Anchor->ConsumePendingFirstPersonCardLockedInspectionPointerRelease());

	Cleanup();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomFirstPersonCardSingleFaceReleaseSpec,
	"Wacom.UI.FirstPersonCardLayer.FaceInspection.SingleFaceKeepsLegacyRelease",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomFirstPersonCardSingleFaceReleaseSpec::RunTest(const FString&)
{
	using namespace WacomFirstPersonCardFaceInspectionSpec;

	UWorld* World = FindAutomationWorld();
	if (!TestNotNull(TEXT("Automation world"), World))
	{
		return false;
	}
	APlayerController* PC =
		World->SpawnActor<APlayerController>(APlayerController::StaticClass(), FTransform::Identity);
	UWacomFirstPersonCardLayerWidget* Layer =
		NewObject<UWacomFirstPersonCardLayerWidget>(PC);
	Layer->SetCardLayerInteractionEnabled(true);
	FWacomFirstPersonCardLayerTestAccess::SetCardDragConfig(
		*Layer,
		MakeInspectConfig());
	FWacomFirstPersonCardLayerTestAccess::SetViewportSizeOverride(
		*Layer,
		FVector2D(1000.0f, 1000.0f));

	const FVector2D Position(500.0f, 650.0f);
	FWacomFirstPersonCardLayerSlotView SlotView =
		MakeDualFaceSlot(FGuid::NewGuid(), Position);
	SlotView.Entry.bHasAlternateFace = false;
	SlotView.Entry.bAllowLockedFaceInspection = false;
	Layer->SetCardSlots({ SlotView });
	UWacomFirstPersonCardLayerSlotWidget* Slot = Layer->GetSlotWidgetAt(0);
	if (!TestNotNull(TEXT("Slot"), Slot))
	{
		PC->Destroy();
		return false;
	}

	FWacomFirstPersonCardLayerTestAccess::RequestPressAtWidgetPosition(
		*Layer,
		Position);
	FWacomFirstPersonCardLayerTestAccess::RequestGestureMove(
		*Slot,
		0.06f,
		Position);
	FWacomFirstPersonCardLayerTestAccess::RequestReleaseAtWidgetPosition(
		*Layer,
		Position);
	TestEqual(
		TEXT("Single-faced card returns to idle"),
		Slot->GetGestureStateForFirstPersonLayer(),
		EWacomFirstPersonCardGestureState::Idle);
	TestFalse(
		TEXT("Single-faced card never locks"),
		Layer->IsLockedCardInspectionActive());
	PC->Destroy();
	return true;
}
