// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Session/BattleSession.h"
#include "UI/WacomUITestAccess.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomKnockdownChoiceDialogSpec
{
	FKnockdownChoiceView MakeView(
		const TCHAR* PartName,
		const TCHAR* AidCardId,
		const TCHAR* AidCardName,
		const TCHAR* DestroyCardId,
		const TCHAR* DestroyCardName)
	{
		FKnockdownChoiceView View;
		View.bHasPendingChoice = true;
		View.PartName = FText::FromString(PartName);
		View.AidOption.Choice = EKnockdownChoice::Aid;
		View.AidOption.bAvailable = true;
		View.AidOption.bHasRewardCard = true;
		View.AidOption.RewardCardId = FName(AidCardId);
		View.AidOption.RewardCardName = FText::FromString(AidCardName);
		View.WithdrawOption.Choice = EKnockdownChoice::Withdraw;
		View.WithdrawOption.bAvailable = true;
		View.DestroyOption.Choice = EKnockdownChoice::Destroy;
		View.DestroyOption.bAvailable = true;
		View.DestroyOption.bHasRewardCard = true;
		View.DestroyOption.RewardCardId = FName(DestroyCardId);
		View.DestroyOption.RewardCardName = FText::FromString(DestroyCardName);
		return View;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceDialogRewardBindingSpec,
	"Wacom.UI.Battle.KnockdownChoice.RewardBindingAndRefresh",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceDialogRewardBindingSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownChoiceDialogSpec;
	TStrongObjectPtr<UWacomKnockdownChoiceDialogInputProbe> Dialog(
		NewObject<UWacomKnockdownChoiceDialogInputProbe>());
	Dialog->TakeWidget();

	FKnockdownChoiceView View = MakeView(
		TEXT("蛇首"),
		TEXT("Reward.Test.Aid"), TEXT("援助卡"),
		TEXT("Reward.Test.Destroy"), TEXT("破坏卡"));
	Dialog->SetContext(nullptr, View);
	TestEqual(TEXT("Part name binds on first context"),
		Dialog->GetPartNameText(), FString(TEXT("蛇首")));
	TestEqual(TEXT("Aid reward binds on first context"),
		Dialog->GetAidRewardText(), FString(TEXT("奖励：援助卡")));
	TestEqual(TEXT("Destroy reward binds on first context"),
		Dialog->GetDestroyRewardText(), FString(TEXT("奖励：破坏卡")));

	View = MakeView(
		TEXT("蛇尾"),
		TEXT("Reward.Test.SecondAid"), TEXT("第二援助卡"),
		TEXT("Reward.Test.SecondDestroy"), TEXT("第二破坏卡"));
	View.AidOption.bAvailable = false;
	View.WithdrawOption.bAvailable = false;
	Dialog->SetContext(nullptr, View);

	TestEqual(TEXT("Part name refreshes on repeated SetContext"),
		Dialog->GetPartNameText(), FString(TEXT("蛇尾")));
	TestEqual(TEXT("Aid reward refreshes without stale text"),
		Dialog->GetAidRewardText(), FString(TEXT("奖励：第二援助卡")));
	TestEqual(TEXT("Destroy reward refreshes without stale text"),
		Dialog->GetDestroyRewardText(), FString(TEXT("奖励：第二破坏卡")));
	TestFalse(TEXT("Aid availability refreshes independently"),
		Dialog->IsAidButtonEnabled());
	TestFalse(TEXT("Withdraw availability refreshes independently"),
		Dialog->IsWithdrawButtonEnabled());
	TestTrue(TEXT("Destroy availability remains enabled"),
		Dialog->IsDestroyButtonEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceDialogEmptyRewardSpec,
	"Wacom.UI.Battle.KnockdownChoice.EmptyRewardDoesNotDisable",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceDialogEmptyRewardSpec::RunTest(
	const FString& /*Parameters*/)
{
	TStrongObjectPtr<UWacomKnockdownChoiceDialogInputProbe> Dialog(
		NewObject<UWacomKnockdownChoiceDialogInputProbe>());
	Dialog->TakeWidget();

	FKnockdownChoiceView View;
	View.bHasPendingChoice = true;
	View.PartName = FText::FromString(TEXT("无奖励部位"));
	View.AidOption.Choice = EKnockdownChoice::Aid;
	View.AidOption.bAvailable = true;
	View.DestroyOption.Choice = EKnockdownChoice::Destroy;
	View.DestroyOption.bAvailable = false;
	View.DestroyOption.DisabledReason = TEXT("RightHandMissing");
	View.WithdrawOption.Choice = EKnockdownChoice::Withdraw;
	View.WithdrawOption.bAvailable = true;
	Dialog->SetContext(nullptr, View);

	TestEqual(TEXT("Empty Aid reward has explicit copy"),
		Dialog->GetAidRewardText(), FString(TEXT("无卡牌奖励")));
	TestEqual(TEXT("Empty Destroy reward has explicit copy"),
		Dialog->GetDestroyRewardText(), FString(TEXT("无卡牌奖励")));
	TestTrue(TEXT("Empty reward does not disable available Aid"),
		Dialog->IsAidButtonEnabled());
	TestFalse(TEXT("Destroy follows supplied disabled state, not reward"),
		Dialog->IsDestroyButtonEnabled());
	TestTrue(TEXT("Withdraw remains independent of reward preview"),
		Dialog->IsWithdrawButtonEnabled());
	return true;
}
