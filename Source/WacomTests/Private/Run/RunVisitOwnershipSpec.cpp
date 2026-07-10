// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Events/RunEventDefinition.h"
#include "RunSession.h"

#include "UObject/StrongObjectPtr.h"

namespace
{
	UWacomRunEventDefinition* MakeVisitOwnershipEvent(UObject* Outer, FName EventId)
	{
		UWacomRunEventDefinition* Event = NewObject<UWacomRunEventDefinition>(Outer);
		Event->EventId = EventId;
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventNodeDefinition StartNode;
		StartNode.NodeId = Event->StartNodeId;
		Event->Nodes.Add(StartNode);
		return Event;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopVisitRejectsReentrantBeginSpec,
	"Wacom.Run.VisitOwnership.ShopRejectsReentrantBegin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopVisitRejectsReentrantBeginSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const TArray<FRunShopOfferInput> Offers;
	const FName FirstShopId(TEXT("Shop.VisitOwnership.First"));
	const FName SecondShopId(TEXT("Shop.VisitOwnership.Second"));

	TestTrue(TEXT("First shop visit begins"), Run->BeginShopVisit(FirstShopId, Offers));
	TestFalse(TEXT("A second Begin cannot replace an active shop transaction"),
		Run->BeginShopVisit(SecondShopId, Offers));

	const FRunShopSnapshot Snapshot = Run->BuildCurrentShopSnapshot();
	TestTrue(TEXT("The first shop visit remains active"), Snapshot.bIsActive);
	TestEqual(TEXT("The first shop retains transaction ownership"), Snapshot.ShopId, FirstShopId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventVisitRejectsReentrantBeginSpec,
	"Wacom.Run.VisitOwnership.EventRejectsReentrantBegin",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventVisitRejectsReentrantBeginSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> FirstEvent(
		MakeVisitOwnershipEvent(Run.Get(), TEXT("Event.VisitOwnership.First.Definition")));
	TStrongObjectPtr<UWacomRunEventDefinition> SecondEvent(
		MakeVisitOwnershipEvent(Run.Get(), TEXT("Event.VisitOwnership.Second.Definition")));
	const FName FirstEventId(TEXT("Event.VisitOwnership.First"));
	const FName SecondEventId(TEXT("Event.VisitOwnership.Second"));

	TestTrue(TEXT("First event visit begins"), Run->BeginRunEvent(FirstEventId, FirstEvent.Get()));
	TestFalse(TEXT("A second Begin cannot replace an active event transaction"),
		Run->BeginRunEvent(SecondEventId, SecondEvent.Get()));

	const FRunEventSnapshot Snapshot = Run->BuildCurrentRunEventSnapshot();
	TestTrue(TEXT("The first event visit remains active"), Snapshot.bIsActive);
	TestEqual(TEXT("The first event retains transaction ownership"), Snapshot.PersistentId, FirstEventId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunShopVisitRejectsStaleEndSpec,
	"Wacom.Run.VisitOwnership.ShopRejectsStaleEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunShopVisitRejectsStaleEndSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	const TArray<FRunShopOfferInput> Offers;

	TestTrue(TEXT("First shop visit begins"), Run->BeginShopVisit(TEXT("Shop.VisitOwnership.TokenA"), Offers));
	const FGuid FirstToken = Run->GetActiveShopVisitToken();
	TestTrue(TEXT("First shop visit has a valid token"), FirstToken.IsValid());
	Run->EndShopVisit();

	TestTrue(TEXT("Second shop visit begins"), Run->BeginShopVisit(TEXT("Shop.VisitOwnership.TokenB"), Offers));
	const FGuid SecondToken = Run->GetActiveShopVisitToken();
	TestTrue(TEXT("Second shop visit has a distinct token"), SecondToken.IsValid() && SecondToken != FirstToken);
	TestFalse(TEXT("A stale first token cannot end the second shop visit"), Run->EndShopVisitIfOwned(FirstToken));
	TestTrue(TEXT("The second shop visit remains active"), Run->IsShopVisitActive());
	TestEqual(TEXT("The second shop remains active"), Run->BuildCurrentShopSnapshot().ShopId, FName(TEXT("Shop.VisitOwnership.TokenB")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomRunEventVisitRejectsStaleEndSpec,
	"Wacom.Run.VisitOwnership.EventRejectsStaleEnd",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomRunEventVisitRejectsStaleEndSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<URunSession> Run(NewObject<URunSession>());
	TStrongObjectPtr<UWacomRunEventDefinition> EventA(
		MakeVisitOwnershipEvent(Run.Get(), TEXT("Event.VisitOwnership.TokenA.Definition")));
	TStrongObjectPtr<UWacomRunEventDefinition> EventB(
		MakeVisitOwnershipEvent(Run.Get(), TEXT("Event.VisitOwnership.TokenB.Definition")));

	TestTrue(TEXT("First event visit begins"), Run->BeginRunEvent(TEXT("Event.VisitOwnership.TokenA"), EventA.Get()));
	const FGuid FirstToken = Run->GetActiveRunEventVisitToken();
	TestTrue(TEXT("First event visit has a valid token"), FirstToken.IsValid());
	Run->EndRunEvent();

	TestTrue(TEXT("Second event visit begins"), Run->BeginRunEvent(TEXT("Event.VisitOwnership.TokenB"), EventB.Get()));
	const FGuid SecondToken = Run->GetActiveRunEventVisitToken();
	TestTrue(TEXT("Second event visit has a distinct token"), SecondToken.IsValid() && SecondToken != FirstToken);
	TestFalse(TEXT("A stale first token cannot end the second event visit"), Run->EndRunEventIfOwned(FirstToken));
	TestTrue(TEXT("The second event visit remains active"), Run->IsRunEventActive());
	TestEqual(TEXT("The second event remains active"), Run->BuildCurrentRunEventSnapshot().PersistentId, FName(TEXT("Event.VisitOwnership.TokenB")));
	return true;
}
