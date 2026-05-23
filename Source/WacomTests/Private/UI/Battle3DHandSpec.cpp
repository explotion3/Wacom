// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Battle3DHandSpecReceiver.h"

#include "Actors/WacomBattle3DHandPresenter.h"
#include "Actors/WacomBattleCardVisualActor.h"
#include "Cards/CardDefinition.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "InputCoreTypes.h"
#include "Misc/ScopeExit.h"
#include "Snapshots/BattleSnapshot.h"
#include "UI/Battle/CardWidget.h"

#include "UObject/StrongObjectPtr.h"

namespace WacomBattle3DHandSpec
{
	FHandCardSnapshot MakeHandCard(const int32 Index)
	{
		FHandCardSnapshot Snapshot;
		Snapshot.InstanceId = FGuid::NewGuid();
		Snapshot.RuntimeCost = Index;
		Snapshot.Zone = EHandZone::Both;
		Snapshot.bIsPlayable = true;
		return Snapshot;
	}

	FBattleSnapshot MakeBattleSnapshotWithCards(const int32 CardCount)
	{
		FBattleSnapshot Snapshot;
		for (int32 Index = 0; Index < CardCount; ++Index)
		{
			Snapshot.Hand.Cards.Add(MakeHandCard(Index));
		}
		Snapshot.Hand.NormalCardCount = CardCount;
		return Snapshot;
	}

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

	AWacomBattleCardVisualActor* SpawnCardActor(UWorld& World)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		return World.SpawnActor<AWacomBattleCardVisualActor>(
			AWacomBattleCardVisualActor::StaticClass(),
			FTransform::Identity,
			SpawnParameters);
	}

	AWacomBattle3DHandPresenter* SpawnPresenter(UWorld& World)
	{
		FActorSpawnParameters SpawnParameters;
		SpawnParameters.ObjectFlags |= RF_Transient;
		return World.SpawnActor<AWacomBattle3DHandPresenter>(
			AWacomBattle3DHandPresenter::StaticClass(),
			FTransform::Identity,
			SpawnParameters);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattle3DLayoutCenteredAndOrderedSpec,
	"Wacom.UI.Battle3D.Layout.CenteredAndOrdered",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattle3DLayoutCenteredAndOrderedSpec::RunTest(const FString& /*Parameters*/)
{
	const FTransform AnchorTransform(FRotator(0.0f, 90.0f, 0.0f), FVector::ZeroVector, FVector::OneVector);
	FWacomBattle3DHandLayoutParams Params;
	Params.Distance = 300.0f;
	Params.VerticalOffset = -40.0f;
	Params.CardSpacing = 50.0f;
	Params.FanYawDegrees = 6.0f;

	const FTransform EmptyTransform = AWacomBattle3DHandPresenter::ComputeCardTransform(
		0,
		0,
		AnchorTransform,
		Params);
	TestEqual(TEXT("Zero-card layout returns anchor transform"), EmptyTransform.GetLocation(), AnchorTransform.GetLocation());

	const FTransform Single = AWacomBattle3DHandPresenter::ComputeCardTransform(
		1,
		0,
		AnchorTransform,
		Params);
	TestEqual(TEXT("Single card is centered on lateral X"), Single.GetLocation().X, 0.0);
	TestEqual(TEXT("Single card faces back toward anchor yaw"), Single.Rotator().Yaw, -90.0);

	const FTransform Left = AWacomBattle3DHandPresenter::ComputeCardTransform(
		3,
		0,
		AnchorTransform,
		Params);
	const FTransform Center = AWacomBattle3DHandPresenter::ComputeCardTransform(
		3,
		1,
		AnchorTransform,
		Params);
	const FTransform Right = AWacomBattle3DHandPresenter::ComputeCardTransform(
		3,
		2,
		AnchorTransform,
		Params);

	TestEqual(TEXT("Three-card center is laterally centered"), Center.GetLocation().X, 0.0);
	TestEqual(TEXT("Left and right X are symmetric around center"),
		Left.GetLocation().X + Right.GetLocation().X,
		Center.GetLocation().X * 2.0);
	TestEqual(TEXT("Left and right yaw are symmetric around center"),
		Left.Rotator().Yaw + Right.Rotator().Yaw,
		Center.Rotator().Yaw * 2.0);
	TestEqual(TEXT("Center card faces back toward anchor yaw"), Center.Rotator().Yaw, -90.0);
	TestTrue(TEXT("Visual order increases from left to right along X"), Left.GetLocation().X > Center.GetLocation().X);
	TestTrue(TEXT("Visual order places last card on opposite side"), Center.GetLocation().X > Right.GetLocation().X);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattle3DCardVisualApplySnapshotSpec,
	"Wacom.UI.Battle3D.CardVisual.ApplySnapshot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattle3DCardVisualApplySnapshotSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UCardDefinition> Card(NewObject<UCardDefinition>());
	Card->CardId = TEXT("Battle3DApplySnapshot");
	Card->DisplayName = FText::FromString(TEXT("3D 手牌快照卡"));

	FHandCardSnapshot Snapshot = WacomBattle3DHandSpec::MakeHandCard(1);
	Snapshot.Definition = Card.Get();

	UWorld* World = WacomBattle3DHandSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Transient editor world"), World))
	{
		return false;
	}

	AWacomBattleCardVisualActor* Actor = WacomBattle3DHandSpec::SpawnCardActor(*World);
	if (!TestNotNull(TEXT("Card visual actor spawned"), Actor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	};

	Actor->ApplyCardSnapshot(Snapshot);

	TestEqual(TEXT("Actor keeps card instance id"), Actor->GetCardInstanceId(), Snapshot.InstanceId);
	TestEqual(TEXT("Actor keeps hand snapshot id"), Actor->GetCardSnapshot().InstanceId, Snapshot.InstanceId);

	UWidgetComponent* WidgetComponent = Actor->GetCardFaceWidget();
	if (!TestNotNull(TEXT("Card actor owns WidgetComponent"), WidgetComponent))
	{
		return false;
	}

	const UBoxComponent* InteractionBounds = Actor->GetInteractionBounds();
	if (!TestNotNull(TEXT("Card actor owns interaction bounds"), InteractionBounds))
	{
		return false;
	}

	const FVector BoxExtent = InteractionBounds->GetUnscaledBoxExtent();
	TestTrue(TEXT("Interaction bounds are thin on WidgetComponent normal axis"), BoxExtent.X < BoxExtent.Y);
	TestTrue(TEXT("Interaction bounds cover card width on WidgetComponent local Y"), BoxExtent.Y > 20.0);
	TestTrue(TEXT("Interaction bounds cover card height on WidgetComponent local Z"), BoxExtent.Z > 30.0);

	UCardWidget* CardWidget = Cast<UCardWidget>(WidgetComponent->GetUserWidgetObject());
	if (!TestNotNull(TEXT("WidgetComponent creates UCardWidget"), CardWidget))
	{
		return false;
	}

	TestEqual(TEXT("CardWidget receives instance id from actor snapshot"),
		CardWidget->GetCardInstanceId(),
		Snapshot.InstanceId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattle3DCardVisualClickAndHoverDelegatesSpec,
	"Wacom.UI.Battle3D.CardVisual.ClickAndHoverDelegates",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattle3DCardVisualClickAndHoverDelegatesSpec::RunTest(const FString& /*Parameters*/)
{
	FHandCardSnapshot Snapshot = WacomBattle3DHandSpec::MakeHandCard(2);
	TStrongObjectPtr<UWacomBattle3DHandSpecReceiver> Receiver(NewObject<UWacomBattle3DHandSpecReceiver>());

	UWorld* World = WacomBattle3DHandSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Transient editor world"), World))
	{
		return false;
	}

	AWacomBattleCardVisualActor* Actor = WacomBattle3DHandSpec::SpawnCardActor(*World);
	if (!TestNotNull(TEXT("Card visual actor spawned"), Actor))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Actor))
		{
			Actor->Destroy();
		}
	};

	const FTransform BaseTransform(FRotator(0.0f, 15.0f, 0.0f), FVector(10.0f, 20.0f, 30.0f), FVector(1.0f));
	const FVector HoverOffset(0.0f, 0.0f, 25.0f);

	Actor->ApplyCardSnapshot(Snapshot);
	Actor->SetBaseWorldTransform(BaseTransform);
	Actor->SetHoverOffset(HoverOffset);
	Actor->OnCardClickedNative.AddUObject(Receiver.Get(), &UWacomBattle3DHandSpecReceiver::HandleActorClicked);
	Actor->OnCardHoveredNative.AddUObject(Receiver.Get(), &UWacomBattle3DHandSpecReceiver::HandleActorHovered);
	Actor->OnCardUnhoveredNative.AddUObject(Receiver.Get(), &UWacomBattle3DHandSpecReceiver::HandleActorUnhovered);

	Actor->NotifyActorOnClicked(EKeys::LeftMouseButton);
	TestEqual(TEXT("Actor click broadcasts once"), Receiver->ActorClickCount, 1);
	TestEqual(TEXT("Actor click broadcasts instance id"), Receiver->LastActorClickedId, Snapshot.InstanceId);
	TestTrue(TEXT("Actor click broadcasts source actor"), Receiver->LastActorSource == Actor);

	Actor->NotifyActorBeginCursorOver();
	TestEqual(TEXT("Actor hover broadcasts once"), Receiver->ActorHoverCount, 1);
	TestEqual(TEXT("Actor hover broadcasts instance id"), Receiver->LastActorHoveredId, Snapshot.InstanceId);
	TestTrue(TEXT("Actor enters hovered state"), Actor->IsHovered());
	TestEqual(TEXT("Hover translates actor by configured offset"),
		Actor->GetActorLocation(),
		BaseTransform.GetLocation() + BaseTransform.TransformVectorNoScale(HoverOffset));

	Actor->NotifyActorEndCursorOver();
	TestEqual(TEXT("Actor unhover broadcasts once"), Receiver->ActorUnhoverCount, 1);
	TestEqual(TEXT("Actor unhover broadcasts instance id"), Receiver->LastActorUnhoveredId, Snapshot.InstanceId);
	TestFalse(TEXT("Actor exits hovered state"), Actor->IsHovered());
	TestEqual(TEXT("Unhover restores base location"), Actor->GetActorLocation(), BaseTransform.GetLocation());

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattle3DPresenterRefreshReusesActorsSpec,
	"Wacom.UI.Battle3D.Presenter.RefreshReusesActors",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattle3DPresenterRefreshReusesActorsSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleSnapshot First = WacomBattle3DHandSpec::MakeBattleSnapshotWithCards(3);

	UWorld* World = WacomBattle3DHandSpec::FindAutomationWorld();
	if (!TestNotNull(TEXT("Transient editor world"), World))
	{
		return false;
	}

	AWacomBattle3DHandPresenter* Presenter = WacomBattle3DHandSpec::SpawnPresenter(*World);
	if (!TestNotNull(TEXT("3D hand presenter spawned"), Presenter))
	{
		return false;
	}
	ON_SCOPE_EXIT
	{
		if (IsValid(Presenter))
		{
			Presenter->Destroy();
		}
	};

	Presenter->bFollowLocalPlayerCamera = false;
	Presenter->RefreshFromSnapshot(First);

	TestEqual(TEXT("First refresh spawns one actor per card"), Presenter->GetSpawnedCardActorCount(), 3);
	TestEqual(TEXT("First refresh preserves ordered ids"), Presenter->GetOrderedCardIds().Num(), 3);

	AWacomBattleCardVisualActor* FirstActor = Presenter->GetCardActor(First.Hand.Cards[0].InstanceId);
	AWacomBattleCardVisualActor* MiddleActor = Presenter->GetCardActor(First.Hand.Cards[1].InstanceId);
	AWacomBattleCardVisualActor* LastActor = Presenter->GetCardActor(First.Hand.Cards[2].InstanceId);
	TestNotNull(TEXT("First card actor exists"), FirstActor);
	TestNotNull(TEXT("Middle card actor exists"), MiddleActor);
	TestNotNull(TEXT("Last card actor exists"), LastActor);

	Presenter->RefreshFromSnapshot(First);
	TestEqual(TEXT("Second refresh with same ids keeps count"), Presenter->GetSpawnedCardActorCount(), 3);
	TestTrue(TEXT("Second refresh reuses first actor"), Presenter->GetCardActor(First.Hand.Cards[0].InstanceId) == FirstActor);
	TestTrue(TEXT("Second refresh reuses middle actor"), Presenter->GetCardActor(First.Hand.Cards[1].InstanceId) == MiddleActor);
	TestTrue(TEXT("Second refresh reuses last actor"), Presenter->GetCardActor(First.Hand.Cards[2].InstanceId) == LastActor);

	FBattleSnapshot RemovedMiddle = First;
	RemovedMiddle.Hand.Cards.RemoveAt(1);
	RemovedMiddle.Hand.NormalCardCount = RemovedMiddle.Hand.Cards.Num();

	Presenter->RefreshFromSnapshot(RemovedMiddle);
	TestEqual(TEXT("Refresh removes actor for missing snapshot card"), Presenter->GetSpawnedCardActorCount(), 2);
	TestTrue(TEXT("First actor remains after removal"), Presenter->GetCardActor(First.Hand.Cards[0].InstanceId) == FirstActor);
	TestTrue(TEXT("Last actor remains after removal"), Presenter->GetCardActor(First.Hand.Cards[2].InstanceId) == LastActor);
	TestNull(TEXT("Removed card actor is no longer addressable"), Presenter->GetCardActor(First.Hand.Cards[1].InstanceId));
	TestEqual(TEXT("Ordered ids shrink with snapshot"), Presenter->GetOrderedCardIds().Num(), 2);
	TestEqual(TEXT("Order keeps first id"), Presenter->GetOrderedCardIds()[0], First.Hand.Cards[0].InstanceId);
	TestEqual(TEXT("Order keeps last id after removal"), Presenter->GetOrderedCardIds()[1], First.Hand.Cards[2].InstanceId);

	return true;
}
