// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Blueprint/WidgetTree.h"
#include "Components/ScaleBox.h"
#include "UI/Card/WacomCardView.h"
#include "UI/WacomUITestAccess.h"
#include "UObject/StrongObjectPtr.h"

namespace WacomKnockdownChoiceDialogSpec
{
	FWacomKnockdownChoiceOptionViewData MakeOption(
		EKnockdownChoice Choice,
		bool bAvailable,
		const TCHAR* Label)
	{
		FWacomKnockdownChoiceOptionViewData Option;
		Option.Choice = Choice;
		Option.bAvailable = bAvailable;
		Option.ChoiceLabel = FText::FromString(Label);
		Option.BranchLabel = FText::FromString(Label);
		Option.RewardFallbackText = FText::FromString(TEXT("无卡牌奖励"));
		return Option;
	}

	FWacomKnockdownChoiceDialogViewData MakeView()
	{
		FWacomKnockdownChoiceDialogViewData View;
		View.TitleText = FText::FromString(TEXT("选择击倒结果"));
		View.PartNameText = FText::FromString(TEXT("已击倒：蛇首"));
		View.AidOption = MakeOption(EKnockdownChoice::Aid, true, TEXT("援助"));
		View.WithdrawOption = MakeOption(EKnockdownChoice::Withdraw, true, TEXT("撤离"));
		View.DestroyOption = MakeOption(EKnockdownChoice::Destroy, true, TEXT("破坏"));
		return View;
	}

	FWacomKnockdownChoiceSubmitDelegate MakeSubmitDelegate(
		TFunction<bool(EKnockdownChoice)> Handler)
	{
		FWacomKnockdownChoiceSubmitDelegate Delegate;
		Delegate.BindLambda(MoveTemp(Handler));
		return Delegate;
	}

	UScaleBox* FindRewardHost(UWacomKnockdownChoiceOptionWidget* Option)
	{
		return Option && Option->WidgetTree
			? Cast<UScaleBox>(Option->WidgetTree->FindWidget(TEXT("RewardCardHost")))
			: nullptr;
	}
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceDialogRefreshSpec,
	"Wacom.UI.Battle.KnockdownChoice.RefreshClearsAndReusesRewardCard",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceDialogRefreshSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownChoiceDialogSpec;
	TStrongObjectPtr<UWacomKnockdownChoiceDialogInputProbe> Dialog(
		NewObject<UWacomKnockdownChoiceDialogInputProbe>());
	Dialog->TakeWidget();

	FWacomKnockdownChoiceDialogViewData View = MakeView();
	View.AidOption.bHasRewardCard = true;
	View.AidOption.bHasRewardCardView = true;
	View.AidOption.RewardCardViewData.Name = FText::FromString(TEXT("援助卡"));
	Dialog->Configure(View, MakeSubmitDelegate([](EKnockdownChoice) { return false; }));

	TestEqual(TEXT("Part name binds"),
		Dialog->GetPartNameText(), FString(TEXT("已击倒：蛇首")));
	UScaleBox* RewardHost = FindRewardHost(Dialog->GetAidOption());
	if (!TestNotNull(TEXT("Aid reward host exists"), RewardHost))
	{
		return false;
	}
	TestEqual(TEXT("Full reward creates exactly one card"), RewardHost->GetChildrenCount(), 1);
	UWidget* FirstCard = RewardHost->GetChildAt(0);

	Dialog->Configure(View, MakeSubmitDelegate([](EKnockdownChoice) { return false; }));
	TestEqual(TEXT("Repeated refresh does not duplicate card"), RewardHost->GetChildrenCount(), 1);
	TestTrue(TEXT("Repeated refresh reuses card view"), RewardHost->GetChildAt(0) == FirstCard);

	View.AidOption.bHasRewardCard = false;
	View.AidOption.bHasRewardCardView = false;
	View.AidOption.RewardCardViewData = FWacomCardViewData();
	Dialog->Configure(View, MakeSubmitDelegate([](EKnockdownChoice) { return false; }));
	TestEqual(TEXT("No reward clears old card"), RewardHost->GetChildrenCount(), 0);
	TestTrue(TEXT("No reward does not disable available Aid"), Dialog->IsAidButtonEnabled());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceDialogSubmitGateSpec,
	"Wacom.UI.Battle.KnockdownChoice.SubmitGateAndFailureRecovery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceDialogSubmitGateSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownChoiceDialogSpec;
	TStrongObjectPtr<UWacomKnockdownChoiceDialogInputProbe> Dialog(
		NewObject<UWacomKnockdownChoiceDialogInputProbe>());
	Dialog->TakeWidget();

	FWacomKnockdownChoiceDialogViewData View = MakeView();
	View.DestroyOption.bAvailable = false;
	int32 SubmitCount = 0;
	Dialog->Configure(View, MakeSubmitDelegate([&SubmitCount](EKnockdownChoice)
	{
		++SubmitCount;
		return false;
	}));

	Dialog->SubmitChoice(EKnockdownChoice::Destroy);
	TestEqual(TEXT("Disabled option does not submit"), SubmitCount, 0);
	Dialog->SubmitChoice(EKnockdownChoice::Aid);
	TestEqual(TEXT("Available option submits"), SubmitCount, 1);
	TestFalse(TEXT("Rejected submit releases gate"), Dialog->HasSubmitPending());
	TestTrue(TEXT("Rejected submit restores Aid interaction"), Dialog->IsAidButtonEnabled());
	TestFalse(TEXT("Rejected submit preserves Destroy disabled state"), Dialog->IsDestroyButtonEnabled());

	Dialog->Configure(MakeView(), MakeSubmitDelegate([&SubmitCount](EKnockdownChoice)
	{
		++SubmitCount;
		return true;
	}));
	Dialog->SubmitChoice(EKnockdownChoice::Aid);
	Dialog->SubmitChoice(EKnockdownChoice::Aid);
	TestEqual(TEXT("Successful double intent commits once"), SubmitCount, 2);
	TestTrue(TEXT("Successful submit keeps gate locked until Modal deactivation"),
		Dialog->HasSubmitPending());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoiceDialogFocusAndBackSpec,
	"Wacom.UI.Battle.KnockdownChoice.FocusOrderAndBackConsumption",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoiceDialogFocusAndBackSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownChoiceDialogSpec;
	TStrongObjectPtr<UWacomKnockdownChoiceDialogInputProbe> Dialog(
		NewObject<UWacomKnockdownChoiceDialogInputProbe>());
	Dialog->TakeWidget();

	FWacomKnockdownChoiceDialogViewData View = MakeView();
	Dialog->Configure(View, MakeSubmitDelegate([](EKnockdownChoice) { return false; }));
	TestTrue(TEXT("Aid is default focus"),
		Dialog->GetDesiredFocusTarget() == Dialog->GetAidOption());

	View.AidOption.bAvailable = false;
	Dialog->Configure(View, MakeSubmitDelegate([](EKnockdownChoice) { return false; }));
	TestTrue(TEXT("Destroy precedes Withdraw when Aid unavailable"),
		Dialog->GetDesiredFocusTarget() == Dialog->GetDestroyOption());
	TestTrue(TEXT("Gamepad B is consumed"), Dialog->SendGamepadBackKeyDown().IsEventHandled());
	return true;
}
