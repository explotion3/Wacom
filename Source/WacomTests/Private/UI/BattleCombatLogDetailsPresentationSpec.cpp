// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Cards/CardDefinition.h"
#include "Characters/CharacterDefinition.h"
#include "Components/Image.h"
#include "Enemies/EnemyDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "Fixtures/BattleTestFixtures.h"
#include "Session/BattleSession.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleCombatLogBuilder.h"
#include "UI/Battle/WacomBattleCombatLogDetailsEntryWidget.h"
#include "UI/Battle/WacomBattleStatusTooltipWidget.h"
#include "UObject/StrongObjectPtr.h"
#include "Widgets/SWidget.h"

namespace WacomBattleCombatLogDetailsPresentationSpec
{
	constexpr TCHAR DetailsEntryClassPath[] =
		TEXT("/Game/Wacom/UI/Battle/CombatLog/WBP_BattleCombatLogDetailsEntry.WBP_BattleCombatLogDetailsEntry_C");

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

	FWacomBattleCombatLogCommandContext MakePlayCardContext(
		const TCHAR* CardName)
	{
		FWacomBattleCombatLogCommandContext Context;
		Context.CommandKind = EWacomBattleCombatLogCommandKind::PlayCard;
		Context.TurnNumber = 1;
		Context.CardName = FText::FromString(CardName);
		return Context;
	}

	FBattleEvent MakeEvent(
		const EBattleEventType Type,
		const int32 Sequence,
		const FBattleEnemyPartKey& PartKey = FBattleEnemyPartKey())
	{
		FBattleEvent Event;
		Event.Type = Type;
		Event.Sequence = Sequence;
		Event.ActorEnemyPartKey = PartKey;
		return Event;
	}

	FWacomInitializedBattleSession CreateSession(
		FWacomBattleFixture& Fixture,
		UEnemyDefinition* Enemy)
	{
		UCharacterDefinition* Character = Fixture.MakeCharacter(
			Fixture.MakeNoopCard(0),
			Fixture.MakeNoopCard(0),
			{
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0),
				Fixture.MakeNoopCard(0)
			});
		return Fixture.CreateInitializedSession(Character, Enemy, 17);
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsHierarchyPresentationSpec,
	"Wacom.UI.Battle.CombatLogDetails.Presentation.TargetsAndThreeLevelHierarchy",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsHierarchyPresentationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatLogDetailsPresentationSpec;

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(
		100, 100, 100, 5, 5, 5);
	Enemy->DisplayName = FText::FromString(TEXT("蛇"));
	Enemy->Parts[0].PartDef->DisplayName = FText::FromString(TEXT("头部"));
	Enemy->Parts[1].PartDef->DisplayName = FText::FromString(TEXT("身体"));
	Enemy->Parts[2].PartDef->DisplayName = FText::FromString(TEXT("尾部"));
	const FWacomInitializedBattleSession Initialized =
		CreateSession(Fixture, Enemy);
	const FBattleSnapshot Snapshot = Initialized.Session->BuildSnapshot();
	const FBattleEnemyPartKey HeadKey =
		FWacomBattleFixture::FindPartKey(Snapshot, 0);
	const FBattleEnemyPartKey TailKey =
		FWacomBattleFixture::FindPartKey(Snapshot, 2);

	TArray<FBattleEvent> Events;
	Events.Add(MakeEvent(EBattleEventType::CardPlayed, 1, TailKey));
	FBattleEvent Resistance =
		MakeEvent(EBattleEventType::ResistanceResolved, 2, TailKey);
	Resistance.Amount = 7;
	Resistance.Count = 3;
	Resistance.bSuccess = true;
	Events.Add(Resistance);
	FBattleEvent Stunned =
		MakeEvent(EBattleEventType::StatusApplied, 3, TailKey);
	Stunned.Tag = WacomTags::Status_Stunned;
	Stunned.Amount = 1;
	Events.Add(Stunned);
	FBattleEvent Damage =
		MakeEvent(EBattleEventType::DamageDealt, 4, HeadKey);
	Damage.Amount = 30;
	Events.Add(Damage);

	const FWacomBattleCombatLogDetailsBatchView Batch =
		UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
			MakePlayCardContext(TEXT("刀光掠影")),
			Events,
			Snapshot,
			Snapshot);
	if (!TestEqual(TEXT("One player root group is projected"), Batch.Groups.Num(), 1))
	{
		return false;
	}
	const FWacomBattleCombatLogDetailsGroupView& Group = Batch.Groups[0];
	TestEqual(TEXT("Root action stays at depth zero"), Group.RootAction.Depth, 0);
	TestEqual(
		TEXT("Root action keeps the card name"),
		Group.RootAction.MessageText.ToString(),
		FString(TEXT("刀光掠影")));
	if (!TestEqual(
		TEXT("Resistance fact, status and damage remain ordered"),
		Group.Entries.Num(),
		4))
	{
		return false;
	}

	const FWacomBattleCombatLogDetailsEntryView& ResistanceEntry =
		Group.Entries[0];
	const FWacomBattleCombatLogDetailsEntryView& ResistanceFact =
		Group.Entries[1];
	TestEqual(
		TEXT("Multi-part target includes enemy and part"),
		ResistanceEntry.TargetLabel.ToString(),
		FString(TEXT("[蛇·尾部]")));
	TestEqual(TEXT("Resistance result uses depth one"), ResistanceEntry.Depth, 1);
	TestEqual(
		TEXT("Resistance result is concise"),
		ResistanceEntry.MessageText.ToString(),
		FString(TEXT("抵抗成功")));
	TestEqual(TEXT("Resistance fact uses depth two"), ResistanceFact.Depth, 2);
	TestTrue(
		TEXT("Resistance fact proves the numeric comparison"),
		ResistanceFact.MessageText.ToString().Contains(
			TEXT("卡牌单段 7 > 敌方单段 3")));
	TestEqual(
		TEXT("Status remains attached to the same stable target"),
		Group.Entries[2].TargetLabel.ToString(),
		FString(TEXT("[蛇·尾部]")));
	TestEqual(
		TEXT("Damage uses its own target instead of the previous event"),
		Group.Entries[3].TargetLabel.ToString(),
		FString(TEXT("[蛇·头部]")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsCardGainedPresentationSpec,
	"Wacom.UI.Battle.CombatLogDetails.Presentation.CardGainedShowsSourceAndCardName",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsCardGainedPresentationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatLogDetailsPresentationSpec;

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = Fixture.MakeThreePartEnemy(
		100, 100, 100, 5, 5, 5);
	Enemy->DisplayName = FText::FromString(TEXT("蛇"));
	Enemy->Parts[1].PartDef->DisplayName = FText::FromString(TEXT("身体"));
	const FWacomInitializedBattleSession Initialized =
		CreateSession(Fixture, Enemy);
	const FBattleSnapshot Snapshot = Initialized.Session->BuildSnapshot();
	const FBattleEnemyPartKey BodyKey =
		FWacomBattleFixture::FindPartKey(Snapshot, 1);

	UCardDefinition* RewardCard = Fixture.MakeNoopCard(0);
	RewardCard->DisplayName = FText::FromString(TEXT("毒牙"));
	FBattleEvent CardGained =
		MakeEvent(EBattleEventType::CardGained, 1, BodyKey);
	CardGained.CardInstanceId = FGuid::NewGuid();
	CardGained.CardDefinition = RewardCard;

	FWacomBattleCombatLogCommandContext Context;
	Context.CommandKind =
		EWacomBattleCombatLogCommandKind::KnockdownChoice;
	Context.KnockdownChoice = EKnockdownChoice::Aid;
	Context.TurnNumber = 1;
	const FWacomBattleCombatLogDetailsBatchView Batch =
		UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
			Context,
			{ CardGained },
			Snapshot,
			Snapshot);
	if (!TestTrue(
		TEXT("Aid details include one card-gained result"),
		Batch.Groups.Num() == 1
			&& Batch.Groups[0].Entries.Num() == 1))
	{
		return false;
	}
	const FWacomBattleCombatLogDetailsEntryView& Entry =
		Batch.Groups[0].Entries[0];
	TestEqual(
		TEXT("Card gain preserves its source enemy part"),
		Entry.TargetLabel.ToString(),
		FString(TEXT("[蛇·身体]")));
	TestEqual(
		TEXT("Card gain describes the acquired object"),
		Entry.MessageText.ToString(),
		FString(TEXT("获得卡牌")));
	TestEqual(
		TEXT("Card gain exposes the acquired card name"),
		Entry.ValueText.ToString(),
		FString(TEXT("「毒牙」")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsTargetFallbackPresentationSpec,
	"Wacom.UI.Battle.CombatLogDetails.Presentation.SinglePartPlayerAndCardFallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsTargetFallbackPresentationSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatLogDetailsPresentationSpec;

	FWacomBattleFixture Fixture;
	UEnemyDefinition* Enemy = Fixture.MakeSinglePartEnemy(100, 5);
	Enemy->DisplayName = FText::FromString(TEXT("木偶"));
	Enemy->Parts[0].PartDef->DisplayName = FText::FromString(TEXT("核心"));
	const FWacomInitializedBattleSession Initialized =
		CreateSession(Fixture, Enemy);
	const FBattleSnapshot Snapshot = Initialized.Session->BuildSnapshot();
	const FBattleEnemyPartKey PartKey =
		FWacomBattleFixture::FindPartKey(Snapshot, 0);

	TArray<FBattleEvent> Events;
	Events.Add(MakeEvent(EBattleEventType::CardPlayed, 1, PartKey));
	FBattleEvent EnemyDamage =
		MakeEvent(EBattleEventType::DamageDealt, 2, PartKey);
	EnemyDamage.Amount = 5;
	Events.Add(EnemyDamage);
	FBattleEvent PlayerDamage =
		MakeEvent(EBattleEventType::DamageDealt, 3);
	PlayerDamage.Amount = 3;
	Events.Add(PlayerDamage);
	FBattleEvent CardCost =
		MakeEvent(EBattleEventType::CardRuntimeCostChanged, 4);
	CardCost.CardInstanceId = FGuid(
		0x12345678,
		0x9abcdef0,
		0x12345678,
		0x9abcdef0);
	CardCost.Amount = -1;
	Events.Add(CardCost);

	const FWacomBattleCombatLogDetailsBatchView Batch =
		UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
			MakePlayCardContext(TEXT("测试卡")),
			Events,
			Snapshot,
			Snapshot);
	if (!TestTrue(TEXT("Fallback projection has one group"),
		Batch.Groups.Num() == 1 && Batch.Groups[0].Entries.Num() == 3))
	{
		return false;
	}
	const TArray<FWacomBattleCombatLogDetailsEntryView>& Entries =
		Batch.Groups[0].Entries;
	TestEqual(
		TEXT("Single-part enemy omits the redundant part name"),
		Entries[0].TargetLabel.ToString(),
		FString(TEXT("[木偶]")));
	TestEqual(
		TEXT("Player target uses an explicit bracketed label"),
		Entries[1].TargetLabel.ToString(),
		FString(TEXT("[玩家]")));
	TestTrue(
		TEXT("Unresolved card target remains visible through its stable id"),
		Entries[2].TargetLabel.ToString().Contains(
			CardCost.CardInstanceId.ToString(
				EGuidFormats::DigitsWithHyphensLower)));

	FBattleEvent EqualResistance =
		MakeEvent(EBattleEventType::ResistanceResolved, 5, PartKey);
	EqualResistance.Amount = 8;
	EqualResistance.Count = 8;
	EqualResistance.bSuccess = false;
	const FWacomBattleCombatLogDetailsBatchView EqualBatch =
		UWacomBattleCombatLogBuilder::BuildCombatLogDetailsBatch(
			MakePlayCardContext(TEXT("测试卡")),
			{ MakeEvent(EBattleEventType::CardPlayed, 1, PartKey),
				EqualResistance },
			Snapshot,
			Snapshot);
	TestTrue(
		TEXT("Equal resistance uses the visible less-or-equal operator"),
		EqualBatch.Groups[0].Entries[1].MessageText.ToString().Contains(
			TEXT("8 ≤ 敌方单段 8")));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleCombatLogDetailsHistoricalStatusTooltipSpec,
	"Wacom.UI.Battle.CombatLogDetails.Presentation.HistoricalStatusTooltip",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleCombatLogDetailsHistoricalStatusTooltipSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomBattleCombatLogDetailsPresentationSpec;

	TStrongObjectPtr<UWacomBattleCombatLogDetailsEntryWidget> EntryWidget(
		NewObject<UWacomBattleCombatLogDetailsEntryWidget>());
	const TSharedRef<SWidget> EntrySlate = EntryWidget->TakeWidget();

	FWacomBattleCombatLogDetailsEntryView Entry;
	Entry.EntryKind = EWacomBattleCombatLogDetailsEntryKind::Result;
	Entry.Depth = 1;
	Entry.SourceEventType = EBattleEventType::StatusApplied;
	Entry.IconTag = WacomTags::Status_Poison;
	Entry.StatusInspectionHost =
		EWacomBattleStatusInspectionHost::EnemyPart;
	Entry.StatusDelta = -2;
	Entry.bShowStatusTooltip = true;
	Entry.MessageText = FText::FromString(TEXT("中毒"));
	EntryWidget->SetDetailsEntryData(Entry, FSlateBrush());

	TestEqual(TEXT("Status result uses result indentation"),
		EntryWidget->GetAppliedIndentWidth(),
		28.0f);
	TestTrue(TEXT("Status result enables historical tooltip semantics"),
		EntryWidget->HasHistoricalStatusTooltip());

	UImage* Icon = Cast<UImage>(
		EntryWidget->GetWidgetFromName(TEXT("EntryIcon")));
	if (!TestNotNull(TEXT("Details entry owns the status icon"), Icon))
	{
		return false;
	}
	const TSharedPtr<SWidget> IconSlateWidget = Icon->GetCachedWidget();
	TestTrue(
		TEXT("Historical status icon exposes its tooltip through the real Slate hover path"),
		IconSlateWidget.IsValid()
			&& IconSlateWidget->GetToolTip().IsValid());
	UWacomBattleStatusTooltipWidget* Tooltip =
		Cast<UWacomBattleStatusTooltipWidget>(
			Icon->ToolTipWidgetDelegate.Execute());
	if (!TestNotNull(TEXT("Historical status tooltip is created lazily"), Tooltip))
	{
		return false;
	}
	TestTrue(TEXT("Tooltip stays in historical event mode"),
		Tooltip->IsShowingHistoricalStatusEvent());
	TestEqual(TEXT("Tooltip preserves this event's signed delta"),
		Tooltip->GetHistoricalStatusDelta(),
		-2);
	TestEqual(TEXT("Tooltip keeps the enemy host rules"),
		Tooltip->GetStatusView().InspectionHost,
		EWacomBattleStatusInspectionHost::EnemyPart);
	TestFalse(TEXT("Catalog-backed status rules remain available"),
		Tooltip->GetStatusView().CoreEffectText.IsEmpty());

	Entry.IconTag = WacomTags::Card_Keyword_Weapon;
	Entry.StatusInspectionHost =
		EWacomBattleStatusInspectionHost::Unknown;
	Entry.StatusDelta = 1;
	EntryWidget->SetDetailsEntryData(Entry, FSlateBrush());
	UWacomBattleStatusTooltipWidget* UnknownTooltip =
		Cast<UWacomBattleStatusTooltipWidget>(
			Icon->ToolTipWidgetDelegate.Execute());
	if (!TestNotNull(TEXT("Unknown status keeps a safe tooltip"), UnknownTooltip))
	{
		return false;
	}
	TestEqual(
		TEXT("Unknown status falls back to its complete tag"),
		UnknownTooltip->GetStatusView().DisplayName.ToString(),
		WacomTags::Card_Keyword_Weapon.GetTag().GetTagName().ToString());
	TestFalse(
		TEXT("Unknown status fallback rules remain readable"),
		UnknownTooltip->GetStatusView().CoreEffectText.IsEmpty());

	EntryWidget->ClearDetailsEntry();
	TestFalse(TEXT("Clearing the row removes tooltip eligibility"),
		EntryWidget->HasHistoricalStatusTooltip());
	TestFalse(TEXT("Cleared icon unbinds the tooltip delegate"),
		Icon->ToolTipWidgetDelegate.IsBound());
	TestFalse(TEXT("Clearing the row removes the Slate tooltip"),
		IconSlateWidget.IsValid()
			&& IconSlateWidget->GetToolTip().IsValid());

	UWorld* World = FindAutomationWorld();
	UClass* FormalEntryClass =
		LoadClass<UWacomBattleCombatLogDetailsEntryWidget>(
			nullptr,
			DetailsEntryClassPath);
	if (!TestNotNull(TEXT("Automation world"), World)
		|| !TestNotNull(TEXT("Formal details Entry class"), FormalEntryClass))
	{
		return false;
	}
	TStrongObjectPtr<UWacomBattleCombatLogDetailsEntryWidget> FormalEntry(
		CreateWidget<UWacomBattleCombatLogDetailsEntryWidget>(
			World,
			FormalEntryClass));
	if (!TestNotNull(TEXT("Formal details Entry instance"), FormalEntry.Get()))
	{
		return false;
	}
	const TSharedRef<SWidget> FormalEntrySlate = FormalEntry->TakeWidget();
	Entry.IconTag = WacomTags::Status_Stunned;
	Entry.StatusInspectionHost =
		EWacomBattleStatusInspectionHost::EnemyPart;
	Entry.StatusDelta = 1;
	FormalEntry->SetDetailsEntryData(Entry, FSlateBrush());
	UImage* FormalIcon = Cast<UImage>(
		FormalEntry->GetWidgetFromName(TEXT("EntryIcon")));
	if (!TestNotNull(TEXT("Formal details Entry owns the status icon"), FormalIcon))
	{
		return false;
	}
	const TSharedPtr<SWidget> FormalIconSlate = FormalIcon->GetCachedWidget();
	TestTrue(
		TEXT("Formal details status icon exposes its tooltip through Slate hover"),
		FormalIcon->GetVisibility() == ESlateVisibility::Visible
			&& FormalIconSlate.IsValid()
			&& FormalIconSlate->GetToolTip().IsValid());
	return true;
}
