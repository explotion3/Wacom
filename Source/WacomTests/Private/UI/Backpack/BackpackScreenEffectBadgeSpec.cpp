// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"
#include "Fixtures/WacomRunExplorationFixture.h"

#include "../BackpackScreenTestAccess.h"
#include "BackpackScreenSpecFixture.h"

#include "Blueprint/WidgetTree.h"
#include "UI/Backpack/WacomBackpackScreen.h"
#include "UI/Backpack/WacomDeckCardWidget.h"
#include "UI/Card/WacomCardEffectBadgeWidget.h"
#include "UI/Card/WacomCardDetailPanel.h"
#include "UI/Card/WacomCardDetailPlainTextRenderer.h"
#include "UI/Card/WacomCardDetailSectionWidget.h"
#include "UI/Card/WacomCardPresentationBuilder.h"
#include "UI/Card/WacomCardView.h"
#include "UI/CardViewTestAccess.h"
#include "UI/CardViewSpecReceiver.h"

#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/Image.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"
#include "PaperSprite.h"
#include "RunSession.h"
#include "Tags/WacomGameplayTags.h"

#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackCardDetailController.h"
#include "../../../../WacomApp/Private/UI/Backpack/WacomBackpackWorkspaceSceneBuilder.h"

#include "UObject/StrongObjectPtr.h"

using namespace WacomBackpackScreenSpecFixture;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectStatsHostFiltersUnsupportedKindsSpec,
	"Wacom.UI.Backpack.CardViewEffectStatsHostFiltersUnsupportedKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectStatsHostFiltersUnsupportedKindsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.Name = FText::FromString(TEXT("徽章过滤测试"));

	FWacomCardViewEffectBadge DamageBadge;
	DamageBadge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	DamageBadge.Value = 7;
	Data.EffectBadges.Add(DamageBadge);

	FWacomCardViewEffectBadge FreezeBadge;
	FreezeBadge.Kind = EWacomCardViewEffectBadgeKind::Freeze;
	FreezeBadge.Value = 1;
	Data.EffectBadges.Add(FreezeBadge);


	FWacomCardViewEffectBadge ShieldBadge;
	ShieldBadge.Kind = EWacomCardViewEffectBadgeKind::Shield;
	ShieldBadge.Value = 5;
	Data.EffectBadges.Add(ShieldBadge);

	CardView->SetCardViewData(Data);

	UPanelWidget* EffectStatsHost = CardView->GetEffectStatsHostForTest();
	TestNotNull(TEXT("Fallback CardView creates EffectStatsHost"), EffectStatsHost);
	if (!EffectStatsHost)
	{
		return false;
	}

	TestEqual(TEXT("EffectStatsHost renders only art-backed badge kinds"), EffectStatsHost->GetChildrenCount(), 2);
	TestEqual(TEXT("EffectStatsHost remains visible when supported badges exist"),
		EffectStatsHost->GetVisibility(),
		ESlateVisibility::HitTestInvisible);

	Data.EffectBadges.Reset();
	Data.EffectBadges.Add(FreezeBadge);
	CardView->SetCardViewData(Data);

	TestEqual(TEXT("EffectStatsHost renders no unsupported-only badges"), EffectStatsHost->GetChildrenCount(), 0);
	TestEqual(TEXT("EffectStatsHost collapses when only unsupported badges exist"),
		EffectStatsHost->GetVisibility(),
		ESlateVisibility::Collapsed);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsFillSequentiallySpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsFillSequentially",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsFillSequentiallySpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Freeze, 1));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 5));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Poison, 2));
	CardView->SetCardViewData(Data);

	UPanelWidget* LegacyHost = CardView->GetEffectStatsHostForTest();
	TestNotNull(TEXT("Slot probe has legacy EffectStatsHost"), LegacyHost);
	if (LegacyHost)
	{
		TestEqual(TEXT("Slot mode clears legacy host"), LegacyHost->GetChildrenCount(), 0);
		TestEqual(TEXT("Slot mode collapses legacy host"), LegacyHost->GetVisibility(), ESlateVisibility::Collapsed);
	}

	const UWacomCardEffectBadgeWidget* Slot1Badge = GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(0));
	const UWacomCardEffectBadgeWidget* Slot2Badge = GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1));
	const UWacomCardEffectBadgeWidget* Slot3Badge = GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(2));
	TestNotNull(TEXT("Slot1 has badge"), Slot1Badge);
	TestNotNull(TEXT("Slot2 has badge"), Slot2Badge);
	TestNotNull(TEXT("Slot3 has badge"), Slot3Badge);
	if (Slot1Badge && Slot2Badge && Slot3Badge)
	{
		TestTrue(TEXT("Slot1 receives first supported badge"),
			Slot1Badge->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Damage);
		TestTrue(TEXT("Slot2 skips unsupported Freeze and receives Shield"),
			Slot2Badge->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Shield);
		TestTrue(TEXT("Slot3 receives next supported badge"),
			Slot3Badge->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Poison);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsHideEmptyAndOverflowSpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsHideEmptyAndOverflow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsHideEmptyAndOverflowSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	for (int32 Index = 0; Index < 5; ++Index)
	{
		Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, Index + 1));
	}
	CardView->SetCardViewData(Data);

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UPanelWidget* Slot = CardView->GetEffectBadgeSlotForTest(SlotIndex);
		TestNotNull(FString::Printf(TEXT("Slot %d exists"), SlotIndex + 1), Slot);
		if (Slot)
		{
			TestEqual(FString::Printf(TEXT("Slot %d renders one badge"), SlotIndex + 1), Slot->GetChildrenCount(), 1);
			TestEqual(FString::Printf(TEXT("Slot %d is visible"), SlotIndex + 1),
				Slot->GetVisibility(),
				ESlateVisibility::HitTestInvisible);
		}
	}

	Data.EffectBadges.SetNum(2);
	CardView->SetCardViewData(Data);

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UPanelWidget* Slot = CardView->GetEffectBadgeSlotForTest(SlotIndex);
		if (!Slot)
		{
			continue;
		}

		const bool bExpectedOccupied = SlotIndex < 2;
		TestTrue(FString::Printf(TEXT("Slot %d keeps at most one reusable badge child after short refresh"), SlotIndex + 1),
			Slot->GetChildrenCount() <= 1);
		TestEqual(FString::Printf(TEXT("Slot %d visibility after short refresh"), SlotIndex + 1),
			Slot->GetVisibility(),
			bExpectedOccupied ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsIgnoreUnsupportedKindsSpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsIgnoreUnsupportedKinds",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsIgnoreUnsupportedKindsSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Freeze, 1));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Slow, 2));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Draw, 3));
	CardView->SetCardViewData(Data);

	for (int32 SlotIndex = 0; SlotIndex < 4; ++SlotIndex)
	{
		UPanelWidget* Slot = CardView->GetEffectBadgeSlotForTest(SlotIndex);
		if (!Slot)
		{
			continue;
		}

		TestEqual(FString::Printf(TEXT("Unsupported-only Slot %d has no badge"), SlotIndex + 1),
			Slot->GetChildrenCount(),
			0);
		TestEqual(FString::Printf(TEXT("Unsupported-only Slot %d collapsed"), SlotIndex + 1),
			Slot->GetVisibility(),
			ESlateVisibility::Collapsed);
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectBadgeSlotsReuseSpec,
	"Wacom.UI.Backpack.CardViewEffectBadgeSlotsReuseWidgetsAcrossRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectBadgeSlotsReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewEffectBadgeSlotProbe> CardView(NewObject<UWacomCardViewEffectBadgeSlotProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 5));
	CardView->SetCardViewData(Data);

	const UWacomCardEffectBadgeWidget* FirstSlotInitial =
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(0));
	const UWacomCardEffectBadgeWidget* SecondSlotInitial =
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1));
	TestNotNull(TEXT("Initial first slot badge"), FirstSlotInitial);
	TestNotNull(TEXT("Initial second slot badge"), SecondSlotInitial);
	if (!FirstSlotInitial || !SecondSlotInitial)
	{
		return false;
	}

	Data.EffectBadges[0].Value = 8;
	Data.EffectBadges[1].Value = 6;
	CardView->SetCardViewData(Data);

	TestTrue(TEXT("First slot reuses badge widget"),
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(0)) == FirstSlotInitial);
	TestTrue(TEXT("Second slot reuses badge widget"),
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1)) == SecondSlotInitial);
	TestEqual(TEXT("Reused first slot badge gets updated value"),
		FirstSlotInitial->GetEffectBadgeData().Value,
		8);


	Data.EffectBadges.SetNum(1);
	CardView->SetCardViewData(Data);
	UPanelWidget* SecondSlot = CardView->GetEffectBadgeSlotForTest(1);
	TestNotNull(TEXT("Second slot still exists"), SecondSlot);
	if (SecondSlot)
	{
		TestEqual(TEXT("Second slot collapses when no badge is assigned"),
			SecondSlot->GetVisibility(),
			ESlateVisibility::Collapsed);
		TestTrue(TEXT("Second slot may retain one reusable hidden child"),
			SecondSlot->GetChildrenCount() <= 1);
	}

	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 9));
	CardView->SetCardViewData(Data);
	TestTrue(TEXT("Second slot reuses hidden badge when occupied again"),
		GetSingleSlotBadgeForTest(CardView->GetEffectBadgeSlotForTest(1)) == SecondSlotInitial);
	TestEqual(TEXT("Reused hidden badge receives new value"),
		SecondSlotInitial->GetEffectBadgeData().Value,
		9);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardViewEffectStatsHostReuseSpec,
	"Wacom.UI.Backpack.CardViewEffectStatsHostFallbackReusesWidgetsAcrossRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardViewEffectStatsHostReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardViewSpecProbe> CardView(NewObject<UWacomCardViewSpecProbe>());
	CardView->TakeWidget();

	FWacomCardViewData Data;
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Damage, 7));
	Data.EffectBadges.Add(MakeCardViewEffectBadgeForTest(EWacomCardViewEffectBadgeKind::Shield, 5));
	CardView->SetCardViewData(Data);

	UPanelWidget* Host = CardView->GetEffectStatsHostForTest();
	TestNotNull(TEXT("Fallback EffectStatsHost exists"), Host);
	if (!Host || Host->GetChildrenCount() < 2)
	{
		return false;
	}

	UWacomCardEffectBadgeWidget* FirstBadge = Cast<UWacomCardEffectBadgeWidget>(Host->GetChildAt(0));
	UWacomCardEffectBadgeWidget* SecondBadge = Cast<UWacomCardEffectBadgeWidget>(Host->GetChildAt(1));
	TestNotNull(TEXT("First fallback badge"), FirstBadge);
	TestNotNull(TEXT("Second fallback badge"), SecondBadge);
	if (!FirstBadge || !SecondBadge)
	{
		return false;
	}

	Data.EffectBadges[0].Value = 8;
	Data.EffectBadges[1].Value = 6;
	CardView->SetCardViewData(Data);
	TestTrue(TEXT("First fallback badge is reused"), Host->GetChildAt(0) == FirstBadge);
	TestTrue(TEXT("Second fallback badge is reused"), Host->GetChildAt(1) == SecondBadge);
	TestEqual(TEXT("Fallback reused badge gets updated data"), FirstBadge->GetEffectBadgeData().Value, 8);

	Data.EffectBadges.SetNum(1);
	CardView->SetCardViewData(Data);
	TestEqual(TEXT("Fallback host trims extra badge widgets"), Host->GetChildrenCount(), 1);
	TestTrue(TEXT("Fallback first badge remains reused after trim"), Host->GetChildAt(0) == FirstBadge);

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeWidgetDataSpec,
	"Wacom.UI.Backpack.CardEffectBadgeWidgetData",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeWidgetDataSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeWidget> BadgeWidget(NewObject<UWacomCardEffectBadgeWidget>());

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 7;
	Badge.DisplayText = FText::FromString(TEXT("伤7"));

	BadgeWidget->SetEffectBadgeData(Badge);
	BadgeWidget->TakeWidget();
	BadgeWidget->SetEffectBadgeData(Badge);

	TestTrue(TEXT("Badge kind preserved"),
		BadgeWidget->GetEffectBadgeData().Kind == EWacomCardViewEffectBadgeKind::Damage);
	TestEqual(TEXT("Badge value preserved"), BadgeWidget->GetEffectBadgeData().Value, 7);
	TestEqual(TEXT("Value text getter reports current value"), BadgeWidget->GetValueText().ToString(), TEXT("7"));

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeWidgetThreeDigitSpec,
	"Wacom.UI.Backpack.CardEffectBadgeWidgetUsesThreeDigitImages",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeWidgetThreeDigitSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeSpecProbe> BadgeWidget(NewObject<UWacomCardEffectBadgeSpecProbe>());
	UPaperSprite* ZeroSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* SevenSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	BadgeWidget->SetDigitSpriteForTest(0, ZeroSprite);
	BadgeWidget->SetDigitSpriteForTest(7, SevenSprite);
	BadgeWidget->SetMinimumDigitCountForTest(3);
	BadgeWidget->SetInteriorDigitPaddingForTest(FMargin(1.0f, 0.0f, 1.0f, 0.0f));
	BadgeWidget->TakeWidget();

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 7;
	BadgeWidget->SetEffectBadgeData(Badge);

	UPanelWidget* DigitHost = BadgeWidget->GetDigitHostForTest();
	TestNotNull(TEXT("Effect badge fallback creates DigitHost"), DigitHost);
	if (!DigitHost)
	{
		return false;
	}

	TestEqual(TEXT("Single digit value renders as three image digits"), DigitHost->GetChildrenCount(), 3);
	if (DigitHost->GetChildrenCount() >= 3)
	{
		const UImage* FirstDigit = Cast<UImage>(DigitHost->GetChildAt(0));
		const UImage* SecondDigit = Cast<UImage>(DigitHost->GetChildAt(1));
		const UImage* ThirdDigit = Cast<UImage>(DigitHost->GetChildAt(2));
		TestTrue(TEXT("First digit is zero sprite"),
			FirstDigit && FirstDigit->GetBrush().GetResourceObject() == ZeroSprite);
		TestTrue(TEXT("Second digit is zero sprite"),
			SecondDigit && SecondDigit->GetBrush().GetResourceObject() == ZeroSprite);
		TestTrue(TEXT("Third digit is seven sprite"),
			ThirdDigit && ThirdDigit->GetBrush().GetResourceObject() == SevenSprite);

		const UHorizontalBoxSlot* MiddleSlot = SecondDigit ? Cast<UHorizontalBoxSlot>(SecondDigit->Slot) : nullptr;
		TestNotNull(TEXT("Middle digit has horizontal slot"), MiddleSlot);
		if (MiddleSlot)
		{
			const FMargin Padding = MiddleSlot->GetPadding();
			TestEqual(TEXT("Middle digit left padding"), Padding.Left, 1.0f);
			TestEqual(TEXT("Middle digit right padding"), Padding.Right, 1.0f);
		}
	}

	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBackpackCardEffectBadgeDigitReuseSpec,
	"Wacom.UI.Backpack.CardEffectBadgeDigitImagesReuseAcrossValueRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBackpackCardEffectBadgeDigitReuseSpec::RunTest(const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomCardEffectBadgeSpecProbe> BadgeWidget(NewObject<UWacomCardEffectBadgeSpecProbe>());
	UPaperSprite* ZeroSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* OneSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* TwoSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	UPaperSprite* ThreeSprite = NewObject<UPaperSprite>(BadgeWidget.Get());
	BadgeWidget->SetDigitSpriteForTest(0, ZeroSprite);
	BadgeWidget->SetDigitSpriteForTest(1, OneSprite);
	BadgeWidget->SetDigitSpriteForTest(2, TwoSprite);
	BadgeWidget->SetDigitSpriteForTest(3, ThreeSprite);
	BadgeWidget->SetMinimumDigitCountForTest(3);
	BadgeWidget->TakeWidget();

	FWacomCardViewEffectBadge Badge;
	Badge.Kind = EWacomCardViewEffectBadgeKind::Damage;
	Badge.Value = 12;
	BadgeWidget->SetEffectBadgeData(Badge);

	UPanelWidget* DigitHost = BadgeWidget->GetDigitHostForTest();
	TestNotNull(TEXT("Effect badge DigitHost exists"), DigitHost);
	if (!DigitHost || DigitHost->GetChildrenCount() < 3)
	{
		return false;
	}

	UWidget* FirstDigit = DigitHost->GetChildAt(0);
	UWidget* SecondDigit = DigitHost->GetChildAt(1);
	UWidget* ThirdDigit = DigitHost->GetChildAt(2);
	const FWacomCardEffectBadgeAutomationTestView InitialView = FWacomCardViewTestAccess::View(*BadgeWidget);

	BadgeWidget->SetEffectBadgeData(Badge);
	TestEqual(TEXT("Equivalent badge data skips apply"),
		FWacomCardViewTestAccess::View(*BadgeWidget).ApplyCount,
		InitialView.ApplyCount);
	TestEqual(TEXT("Equivalent badge data skips digit update"),
		FWacomCardViewTestAccess::View(*BadgeWidget).DigitImageUpdateCount,
		InitialView.DigitImageUpdateCount);

	Badge.Value = 13;
	BadgeWidget->SetEffectBadgeData(Badge);
	TestEqual(TEXT("Effect badge keeps three digit images"), DigitHost->GetChildrenCount(), 3);
	TestTrue(TEXT("First effect digit image reused"), DigitHost->GetChildAt(0) == FirstDigit);
	TestTrue(TEXT("Second effect digit image reused"), DigitHost->GetChildAt(1) == SecondDigit);
	TestTrue(TEXT("Third effect digit image reused"), DigitHost->GetChildAt(2) == ThirdDigit);

	const UImage* ReusedThirdDigit = Cast<UImage>(DigitHost->GetChildAt(2));
	TestTrue(TEXT("Third reused digit gets updated sprite"),
		ReusedThirdDigit && ReusedThirdDigit->GetBrush().GetResourceObject() == ThreeSprite);

	return true;
}
