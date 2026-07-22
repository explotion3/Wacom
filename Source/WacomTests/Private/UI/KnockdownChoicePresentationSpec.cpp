// Copyright Wacom. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Cards/CardDefinition.h"
#include "Enemies/EnemyPartDefinition.h"
#include "Session/BattleSession.h"
#include "Snapshots/BattleSnapshot.h"
#include "UObject/StrongObjectPtr.h"

#include "../../../WacomApp/Private/UI/Battle/WacomKnockdownChoiceDialogPresentationBuilder.h"

namespace WacomKnockdownChoicePresentationSpec
{
	struct FFixture
	{
		TStrongObjectPtr<UCardDefinition> AidCard{
			NewObject<UCardDefinition>()};
		TStrongObjectPtr<UCardDefinition> DestroyCard{
			NewObject<UCardDefinition>()};
		TStrongObjectPtr<UEnemyPartDefinition> Part{
			NewObject<UEnemyPartDefinition>()};
		FGuid PartInstanceId = FGuid::NewGuid();
		FKnockdownChoiceView ChoiceView;
		FBattleSnapshot Snapshot;

		FFixture()
		{
			AidCard->CardId = TEXT("Reward.Test.Aid");
			AidCard->DisplayName = FText::FromString(TEXT("援助卡"));
			DestroyCard->CardId = TEXT("Reward.Test.Destroy");
			DestroyCard->DisplayName = FText::FromString(TEXT("破坏卡"));
			Part->PartId = TEXT("Part.Test");
			Part->AidRewardCard = AidCard.Get();
			Part->DestroyRewardCard = DestroyCard.Get();

			ChoiceView.bHasPendingChoice = true;
			ChoiceView.PartInstanceId = PartInstanceId;
			ChoiceView.PartId = Part->PartId;
			ChoiceView.PartName = FText::FromString(TEXT("测试部位"));
			ChoiceView.AidOption.Choice = EKnockdownChoice::Aid;
			ChoiceView.AidOption.bAvailable = true;
			ChoiceView.AidOption.bHasRewardCard = true;
			ChoiceView.AidOption.RewardCardId = AidCard->CardId;
			ChoiceView.AidOption.RewardCardName = AidCard->DisplayName;
			ChoiceView.WithdrawOption.Choice = EKnockdownChoice::Withdraw;
			ChoiceView.WithdrawOption.bAvailable = true;
			ChoiceView.DestroyOption.Choice = EKnockdownChoice::Destroy;
			ChoiceView.DestroyOption.bAvailable = true;
			ChoiceView.DestroyOption.bHasRewardCard = true;
			ChoiceView.DestroyOption.RewardCardId = DestroyCard->CardId;
			ChoiceView.DestroyOption.RewardCardName = DestroyCard->DisplayName;

			FEnemySnapshot& Enemy = Snapshot.Enemies.AddDefaulted_GetRef();
			FEnemyPartSnapshot& PartSnapshot = Enemy.Parts.AddDefaulted_GetRef();
			PartSnapshot.InstanceId = PartInstanceId;
			PartSnapshot.Definition = Part.Get();
		}
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoicePresentationCardSpec,
	"Wacom.UI.Battle.KnockdownChoice.Presentation.FullCardProjection",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoicePresentationCardSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownChoicePresentationSpec;
	FFixture Fixture;
	const FWacomKnockdownChoiceDialogViewData View =
		FWacomKnockdownChoiceDialogPresentationBuilder::Build(
			Fixture.ChoiceView,
			Fixture.Snapshot);

	TestTrue(TEXT("Aid builds full card"), View.AidOption.bHasRewardCardView);
	TestEqual(TEXT("Aid card name projects"),
		View.AidOption.RewardCardViewData.Name.ToString(), FString(TEXT("援助卡")));
	TestTrue(TEXT("Destroy builds full card"), View.DestroyOption.bHasRewardCardView);
	TestEqual(TEXT("Destroy card name projects"),
		View.DestroyOption.RewardCardViewData.Name.ToString(), FString(TEXT("破坏卡")));
	TestFalse(TEXT("Withdraw has no card face"), View.WithdrawOption.bHasRewardCardView);
	TestEqual(TEXT("Withdraw uses explicit no-reward copy"),
		View.WithdrawOption.RewardFallbackText.ToString(), FString(TEXT("无卡牌奖励")));
	TestTrue(TEXT("Aid copy states no left-card consumption"),
		View.AidOption.DescriptionText.ToString().Contains(TEXT("不消耗左手牌")));
	TestTrue(TEXT("Destroy copy states no right-card consumption"),
		View.DestroyOption.DescriptionText.ToString().Contains(TEXT("不消耗右手牌")));
	return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
	FWacomKnockdownChoicePresentationFallbackSpec,
	"Wacom.UI.Battle.KnockdownChoice.Presentation.SafeFallbacks",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWacomKnockdownChoicePresentationFallbackSpec::RunTest(
	const FString& /*Parameters*/)
{
	using namespace WacomKnockdownChoicePresentationSpec;
	FFixture Fixture;

	Fixture.ChoiceView.AidOption.RewardCardId = TEXT("Reward.Mismatch");
	FWacomKnockdownChoiceDialogViewData View =
		FWacomKnockdownChoiceDialogPresentationBuilder::Build(
			Fixture.ChoiceView,
			Fixture.Snapshot);
	TestFalse(TEXT("CardId mismatch falls back"), View.AidOption.bHasRewardCardView);
	TestTrue(TEXT("CardId mismatch preserves legal choice"), View.AidOption.bAvailable);
	TestEqual(TEXT("CardId mismatch preserves reward label"),
		View.AidOption.RewardFallbackText.ToString(), FString(TEXT("奖励：援助卡")));

	Fixture.ChoiceView.AidOption.bHasRewardCard = false;
	Fixture.ChoiceView.AidOption.RewardCardId = NAME_None;
	Fixture.ChoiceView.AidOption.RewardCardName = FText::GetEmpty();
	View = FWacomKnockdownChoiceDialogPresentationBuilder::Build(
		Fixture.ChoiceView,
		Fixture.Snapshot);
	TestFalse(TEXT("No reward has no full card"), View.AidOption.bHasRewardCardView);
	TestEqual(TEXT("No reward uses explicit copy"),
		View.AidOption.RewardFallbackText.ToString(), FString(TEXT("无卡牌奖励")));
	TestTrue(TEXT("No reward preserves legal choice"), View.AidOption.bAvailable);

	Fixture.ChoiceView.AidOption.bHasRewardCard = true;
	Fixture.ChoiceView.AidOption.RewardCardId = Fixture.AidCard->CardId;
	Fixture.ChoiceView.AidOption.RewardCardName = Fixture.AidCard->DisplayName;
	Fixture.Snapshot.Enemies.Reset();
	View = FWacomKnockdownChoiceDialogPresentationBuilder::Build(
		Fixture.ChoiceView,
		Fixture.Snapshot);
	TestFalse(TEXT("Missing definition falls back"), View.AidOption.bHasRewardCardView);
	TestTrue(TEXT("Missing definition preserves legal choice"), View.AidOption.bAvailable);

	Fixture.ChoiceView.WithdrawOption.bAvailable = false;
	Fixture.ChoiceView.WithdrawOption.DisabledReason = TEXT("NoLivingEnemyPart");
	View = FWacomKnockdownChoiceDialogPresentationBuilder::Build(
		Fixture.ChoiceView,
		Fixture.Snapshot);
	TestEqual(TEXT("Final-part Withdraw reason is localized in builder"),
		View.WithdrawOption.DisabledReasonText.ToString(),
		FString(TEXT("敌人已无存活部位，无法撤离")));
	return true;
}
