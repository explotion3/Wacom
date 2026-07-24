// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Snapshots/BattlePileInspectionSnapshot.h"
#include "UI/Battle/BattleCardPileEntryWidget.h"
#include "UI/Battle/WacomBattleCardPileDetailsScreen.h"
#include "UI/Battle/WacomBattleCardPileDetailsStyle.h"
#include "UI/BattleCardPileDetailsTestAccess.h"
#include "UObject/StrongObjectPtr.h"

#if WITH_AUTOMATION_TESTS

namespace WacomBattleCardPileDetailsResponsiveLayoutSpec
{
	FBattlePileCardSnapshot MakeCard(
		UObject& Outer,
		const TCHAR* ObjectName,
		int32 RuntimeCost,
		uint32 StableId)
	{
		UCardDefinition* Definition = NewObject<UCardDefinition>(&Outer, ObjectName);
		Definition->CardId = FName(ObjectName);
		Definition->DisplayName = FText::FromString(ObjectName);
		Definition->BaseCost = RuntimeCost;

		FBattlePileCardSnapshot Card;
		Card.InstanceId = FGuid(0, 0, 0, StableId);
		Card.Definition = Definition;
		Card.Location = ECardLocation::Draw;
		Card.RuntimeCost = RuntimeCost;
		return Card;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsResponsiveRuntimeLayoutSpec,
	"Wacom.UI.Battle.CardPileDetails.Responsive.RuntimeLayoutPreservesInteraction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsResponsiveRuntimeLayoutSpec::RunTest(
	const FString&)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();
	UWacomBattleCardPileDetailsStyle* Style =
		NewObject<UWacomBattleCardPileDetailsStyle>(Screen.Get());
	Screen->SetAuthoringDefaults(Style, UBattleCardPileEntryWidget::StaticClass());

	FBattlePileInspectionSectionSnapshot Draw;
	Draw.Location = ECardLocation::Draw;
	Draw.Cards = {
		WacomBattleCardPileDetailsResponsiveLayoutSpec::MakeCard(
			*Screen, TEXT("LaterCard"), 2, 2),
		WacomBattleCardPileDetailsResponsiveLayoutSpec::MakeCard(
			*Screen, TEXT("PinnedCard"), 1, 1)
	};
	Draw.Count = Draw.Cards.Num();
	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.Sections.Add(Draw);
	Screen->SetPileDetailsContext(Snapshot, EWacomBattlePileDetailsTab::Draw);

	UWacomBattleCardPileItemViewModel* First =
		FWacomBattleCardPileDetailsTestAccess::GetItem(*Screen, 0);
	if (!TestNotNull(TEXT("responsive pile item exists"), First))
	{
		return false;
	}
	const FGuid FirstId = First->View.InstanceId;
	FWacomBattleCardPileDetailsTestAccess::ClickItem(*Screen, *First);

	TStrongObjectPtr<UBattleCardPileEntryWidget> Entry(
		NewObject<UBattleCardPileEntryWidget>());
	TSharedRef<SWidget> EntrySlate = Entry->TakeWidget();
	FWacomBattleCardPileDetailsTestAccess::AttachEntry(*Screen, *Entry, *First);

	FWacomBattleCardPileDetailsTestAccess::ApplyResponsiveLayout(
		*Screen,
		FVector2D(1280.0f, 720.0f),
		2.0f / 3.0f);
	Entry->RefreshResolvedLayout();
	FWacomBattleCardPileDetailsAutomationView View = Screen->GetAutomationTestView();
	TestTrue(TEXT("720p physical card width is approximately 160px"),
		FMath::IsNearlyEqual(
			View.ResolvedCardSize.X * View.ResolvedGlobalUIScale,
			160.2f,
			0.01f));
	TestTrue(TEXT("720p physical card height is approximately 227px"),
		FMath::IsNearlyEqual(
			View.ResolvedCardSize.Y * View.ResolvedGlobalUIScale,
			226.8f,
			0.01f));
	TestTrue(TEXT("entry uses the same local scale as the card"),
		View.ResolvedEntrySize.Equals(FVector2D(267.3f, 369.9f), 0.01f));
	TestTrue(TEXT("visible entry host refreshes without recreating the card"),
		FWacomBattleCardPileDetailsTestAccess::GetCardSize(*Entry).Equals(
			View.ResolvedCardSize,
			0.01f));
	TestTrue(TEXT("entry hit geometry refreshes with the same local scale"),
		FWacomBattleCardPileDetailsTestAccess::GetEntrySize(*Entry).Equals(
			View.ResolvedEntrySize,
			0.01f));
	const FVector2D OutlineSize =
		FWacomBattleCardPileDetailsTestAccess::GetOutlineSize(*Entry);
	TestTrue(TEXT("selection outline extent is scaled exactly once with the card"),
		FMath::IsNearlyEqual(OutlineSize.X - View.ResolvedCardSize.X, 10.8f, 0.01f)
		&& FMath::IsNearlyEqual(OutlineSize.Y - View.ResolvedCardSize.Y, 10.8f, 0.01f));
	TestEqual(TEXT("responsive refresh preserves the pinned card"),
		View.PinnedInstanceId, FirstId);

	FWacomBattleCardPileDetailsTestAccess::ApplyResponsiveLayout(
		*Screen,
		FVector2D(2560.0f, 1080.0f),
		1.0f);
	View = Screen->GetAutomationTestView();
	TestTrue(TEXT("1080p ultrawide uses the short edge and reference card size"),
		View.ResolvedCardSize.Equals(FVector2D(178.0f, 252.0f), 0.01f));
	TestTrue(TEXT("1080p reference tile size is 198 by 274"),
		View.ResolvedEntrySize.Equals(FVector2D(198.0f, 274.0f), 0.01f));

	FWacomBattleCardPileDetailsTestAccess::ApplyResponsiveLayout(
		*Screen,
		FVector2D(3840.0f, 2160.0f),
		1.0f);
	View = Screen->GetAutomationTestView();
	TestTrue(TEXT("4K card width caps at approximately 205px"),
		FMath::IsNearlyEqual(View.ResolvedCardSize.X, 204.7f, 0.01f));
	TestTrue(TEXT("4K card height caps at approximately 290px"),
		FMath::IsNearlyEqual(View.ResolvedCardSize.Y, 289.8f, 0.01f));
	TestEqual(TEXT("responsive changes keep stable sort order"),
		View.VisibleInstanceIds[0], FirstId);
	TestEqual(TEXT("responsive changes keep the pinned detail source"),
		View.PinnedInstanceId, FirstId);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCardPileDetailsRestingHandParitySpec,
	"Wacom.UI.Battle.CardPileDetails.Responsive.MatchesRestingHandCardBody",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCardPileDetailsRestingHandParitySpec::RunTest(
	const FString&)
{
	TStrongObjectPtr<UWacomBattleCardPileDetailsScreen> Screen(
		NewObject<UWacomBattleCardPileDetailsScreen>());
	TSharedRef<SWidget> ScreenSlate = Screen->TakeWidget();
	UWacomBattleCardPileDetailsStyle* Style =
		NewObject<UWacomBattleCardPileDetailsStyle>(Screen.Get());
	Screen->SetAuthoringDefaults(Style, UBattleCardPileEntryWidget::StaticClass());

	FWacomFirstPersonCardRestingPresentationProfile HandProfile;
	HandProfile.AuthoredRenderScale = 0.92f;
	Screen->SetRestingHandCardPresentationProfile(HandProfile);

	FBattlePileInspectionSectionSnapshot Draw;
	Draw.Location = ECardLocation::Draw;
	Draw.Cards.Add(WacomBattleCardPileDetailsResponsiveLayoutSpec::MakeCard(
		*Screen, TEXT("HandParityCard"), 1, 1));
	Draw.Count = Draw.Cards.Num();
	FBattlePileInspectionSnapshot Snapshot;
	Snapshot.Sections.Add(Draw);
	Screen->SetPileDetailsContext(Snapshot, EWacomBattlePileDetailsTab::Draw);

	struct FCase
	{
		const TCHAR* Label;
		FVector2D Viewport;
		float GlobalUIScale;
		FVector2D ExpectedPhysicalSize;
	};
	for (const FCase& TestCase : {
		FCase{
			TEXT("720p"),
			FVector2D(1280.0f, 720.0f),
			2.0f / 3.0f,
			FVector2D(136.16f, 193.20f) },
		FCase{
			TEXT("1080p"),
			FVector2D(1920.0f, 1080.0f),
			1.0f,
			FVector2D(204.24f, 289.80f) },
		FCase{
			TEXT("1440p"),
			FVector2D(2560.0f, 1440.0f),
			1.0f,
			FVector2D(272.32f, 386.40f) },
		FCase{
			TEXT("1080p ultrawide"),
			FVector2D(2560.0f, 1080.0f),
			1.0f,
			FVector2D(204.24f, 289.80f) } })
	{
		FWacomBattleCardPileDetailsTestAccess::ApplyResponsiveLayout(
			*Screen,
			TestCase.Viewport,
			TestCase.GlobalUIScale);
		const FWacomBattleCardPileDetailsAutomationView View =
			Screen->GetAutomationTestView();
		const FVector2D PhysicalCardBody =
			View.ResolvedCardSize * View.ResolvedGlobalUIScale;
		TestTrue(
			FString::Printf(TEXT("%s pile card matches resting hand physical body"), TestCase.Label),
			PhysicalCardBody.Equals(TestCase.ExpectedPhysicalSize, 0.01f));
	}
	return true;
}

#endif
