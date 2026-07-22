// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Events/BattleEvent.h"
#include "Tags/WacomGameplayTags.h"
#include "UI/Battle/WacomBattleEventPresentationBuilder.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomUIBattleResistancePresentationSpec,
	"Wacom.UI.Battle.Resistance.EventPresentation",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomUIBattleResistancePresentationSpec::RunTest(const FString& /*Parameters*/)
{
	FBattleEvent Event;
	Event.Type = EBattleEventType::ResistanceResolved;
	Event.Amount = 7;
	Event.Count = 5;
	Event.bSuccess = true;
	Event.Tag = WacomTags::Status_Stunned;
	TestEqual(
		TEXT("Success includes persistent stun and both peak segments"),
		UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
		FString(TEXT("抵抗成功：眩晕 +1（卡牌单段 7 / 敌方单段 5）")));

	Event.Amount = 5;
	Event.Count = 5;
	Event.bSuccess = false;
	Event.Tag = FGameplayTag();
	TestEqual(
		TEXT("Failure includes both peak segments"),
		UWacomBattleEventPresentationBuilder::FormatEventForPlayer(Event),
		FString(TEXT("抵抗失败（卡牌单段 5 / 敌方单段 5）")));
	return true;
}
