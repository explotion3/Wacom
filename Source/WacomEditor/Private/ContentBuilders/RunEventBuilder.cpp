// Copyright Wacom. All Rights Reserved.

#include "ContentBuilders/RunEventBuilder.h"
#include "ContentBuilders/ContentBuilderHelpers.h"

#include "Cards/CardDefinition.h"
#include "Events/RunEventDefinition.h"

namespace
{
	using namespace Wacom::ContentBuilder;

	UCardDefinition* LoadGeneratedCard(const FString& ObjectPath)
	{
		UCardDefinition* Card = LoadObject<UCardDefinition>(nullptr, *ObjectPath);
		if (!Card)
		{
			UE_LOG(LogTemp, Error, TEXT("[RunEventBuilder] Failed to load card asset: %s"), *ObjectPath);
		}
		return Card;
	}

	FWacomRunEventEffectDefinition MakeGainCard(UCardDefinition* Card)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::GainCard;
		Effect.CardDefinition = Card;
		return Effect;
	}

	FWacomRunEventEffectDefinition MakeRemoveCard(UCardDefinition* Card)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::RemoveCard;
		Effect.CardDefinition = Card;
		return Effect;
	}

	FWacomRunEventEffectDefinition MakeAddGold(int32 Amount)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::AddGold;
		Effect.Value = Amount;
		return Effect;
	}

	FWacomRunEventEffectDefinition MakeAddPressure(FName PressureType, int32 Amount)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::AddPressure;
		Effect.PressureType = PressureType;
		Effect.Value = Amount;
		return Effect;
	}

	FWacomRunEventEffectDefinition MakeConsumeNode(int32 Count)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::ConsumeNode;
		Effect.Value = Count;
		return Effect;
	}

	FWacomRunEventEffectDefinition MakeSetRunFlag(FName FlagId)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::SetRunFlag;
		Effect.FlagId = FlagId;
		return Effect;
	}

	FWacomRunEventEffectDefinition MakeClearRunFlag(FName FlagId)
	{
		FWacomRunEventEffectDefinition Effect;
		Effect.Type = EWacomRunEventEffectType::ClearRunFlag;
		Effect.FlagId = FlagId;
		return Effect;
	}

	FWacomRunEventConditionDefinition MakeMinGold(int32 Amount)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::MinGold;
		Condition.Value = Amount;
		return Condition;
	}

	FWacomRunEventConditionDefinition MakeHasCard(UCardDefinition* Card)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::HasCard;
		Condition.CardDefinition = Card;
		return Condition;
	}

	FWacomRunEventConditionDefinition MakeRunFlagSet(FName FlagId)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::RunFlagSet;
		Condition.FlagId = FlagId;
		return Condition;
	}

	FWacomRunEventConditionDefinition MakeRunFlagNotSet(FName FlagId)
	{
		FWacomRunEventConditionDefinition Condition;
		Condition.Type = EWacomRunEventConditionType::RunFlagNotSet;
		Condition.FlagId = FlagId;
		return Condition;
	}

	UWacomRunEventDefinition* BuildDebugSnakeGiftEvent(UCardDefinition* PoisonFang)
	{
		const FString PackagePath = MakePackagePath(EventsRoot(), TEXT("DA_Event_DebugSnakeGift"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UWacomRunEventDefinition* Event = CreateOrReplaceAsset<UWacomRunEventDefinition>(Pkg, TEXT("DA_Event_DebugSnakeGift"));
		if (!Event) { return nullptr; }

		Event->EventId = TEXT("Event.DebugSnakeGift");
		Event->DisplayName = FText::FromString(TEXT("蛇巢遗物"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition TakeGift;
		TakeGift.ChoiceId = TEXT("TakeGift");
		TakeGift.LabelText = FText::FromString(TEXT("收下毒牙"));
		TakeGift.Effects = { MakeGainCard(PoisonFang), MakeConsumeNode(1) };
		TakeGift.NextNodeId = TEXT("End");

		FWacomRunEventChoiceDefinition PayRespect;
		PayRespect.ChoiceId = TEXT("PayRespect");
		PayRespect.LabelText = FText::FromString(TEXT("留下 1 金币"));
		PayRespect.Conditions = { MakeMinGold(1) };
		PayRespect.Effects = { MakeAddGold(-1), MakeAddPressure(TEXT("Misdeed"), -1), MakeConsumeNode(1) };
		PayRespect.NextNodeId = TEXT("End");

		FWacomRunEventChoiceDefinition HandOverFang;
		HandOverFang.ChoiceId = TEXT("HandOverFang");
		HandOverFang.LabelText = FText::FromString(TEXT("交出毒牙"));
		HandOverFang.Conditions = { MakeHasCard(PoisonFang) };
		HandOverFang.CardPayment.bRequiresOwnedCardPayment = true;
		HandOverFang.CardPayment.PaymentZoneId = TEXT("RunEvent.Pay.Fang");
		HandOverFang.CardPayment.AllowedCardDefinitions.Add(PoisonFang);
		HandOverFang.Effects = { MakeConsumeNode(1) };
		HandOverFang.NextNodeId = TEXT("End");

		FWacomRunEventChoiceDefinition Leave;
		Leave.ChoiceId = TEXT("Leave");
		Leave.LabelText = FText::FromString(TEXT("离开"));
		Leave.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition StartNode;
		StartNode.NodeId = TEXT("Start");
		StartNode.TitleText = FText::FromString(TEXT("蛇巢遗物"));
		StartNode.BodyText = FText::FromString(TEXT("一枚还带着温热毒意的獠牙躺在枯叶里。"));
		StartNode.Choices = { TakeGift, PayRespect, HandOverFang, Leave };

		FWacomRunEventChoiceDefinition Close;
		Close.ChoiceId = TEXT("Close");
		Close.LabelText = FText::FromString(TEXT("继续前进"));
		Close.bCloseEventAfterResolve = true;
		Close.bMarkEventCompleted = true;

		FWacomRunEventNodeDefinition EndNode;
		EndNode.NodeId = TEXT("End");
		EndNode.TitleText = FText::FromString(TEXT("蛇巢遗物"));
		EndNode.BodyText = FText::FromString(TEXT("枝叶重新合拢，像是什么都没有发生。"));
		EndNode.Choices = { Close };

		Event->Nodes = { StartNode, EndNode };

		SaveAssetPackage(Pkg, Event, PackagePath);
		return Event;
	}

	UWacomRunEventDefinition* BuildDebugFlagRewardEvent(UCardDefinition* PoisonFang)
	{
		static const FName InspectedFlag = TEXT("DebugFlagReward.Inspected");
		static const FName GoldGrantedFlag = TEXT("DebugFlagReward.GoldGranted");
		static const FName RewardClaimedFlag = TEXT("DebugFlagReward.RewardClaimed");

		const FString PackagePath = MakePackagePath(EventsRoot(), TEXT("DA_Event_DebugFlagReward"));
		UPackage* Pkg = FindOrCreatePackage(PackagePath);
		if (!Pkg) { return nullptr; }

		UWacomRunEventDefinition* Event = CreateOrReplaceAsset<UWacomRunEventDefinition>(Pkg, TEXT("DA_Event_DebugFlagReward"));
		if (!Event) { return nullptr; }

		Event->EventId = TEXT("Event.DebugFlagReward");
		Event->DisplayName = FText::FromString(TEXT("标记奖励样例"));
		Event->StartNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition InspectMark;
		InspectMark.ChoiceId = TEXT("InspectMark");
		InspectMark.LabelText = FText::FromString(TEXT("调查刻痕"));
		InspectMark.Conditions = { MakeRunFlagNotSet(InspectedFlag) };
		InspectMark.Effects = { MakeSetRunFlag(InspectedFlag) };

		FWacomRunEventChoiceDefinition DebugGrantGold;
		DebugGrantGold.ChoiceId = TEXT("DebugGrantGold");
		DebugGrantGold.LabelText = FText::FromString(TEXT("调试：获得 3 金币"));
		DebugGrantGold.Conditions = { MakeRunFlagNotSet(GoldGrantedFlag) };
		DebugGrantGold.Effects = { MakeAddGold(3), MakeSetRunFlag(GoldGrantedFlag) };

		FWacomRunEventChoiceDefinition ClaimGoldReward;
		ClaimGoldReward.ChoiceId = TEXT("ClaimGoldReward");
		ClaimGoldReward.LabelText = FText::FromString(TEXT("支付 3 金币领取毒牙"));
		ClaimGoldReward.Conditions =
		{
			MakeRunFlagSet(InspectedFlag),
			MakeRunFlagNotSet(RewardClaimedFlag),
			MakeMinGold(3),
		};
		ClaimGoldReward.Effects =
		{
			MakeAddGold(-3),
			MakeGainCard(PoisonFang),
			MakeSetRunFlag(RewardClaimedFlag),
		};
		ClaimGoldReward.NextNodeId = TEXT("Rewarded");

		FWacomRunEventChoiceDefinition Leave;
		Leave.ChoiceId = TEXT("Leave");
		Leave.LabelText = FText::FromString(TEXT("离开"));
		Leave.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition StartNode;
		StartNode.NodeId = TEXT("Start");
		StartNode.TitleText = FText::FromString(TEXT("标记奖励样例"));
		StartNode.BodyText = FText::FromString(TEXT("石壁上刻着三道浅痕，像是在等待某个条件被满足。"));
		StartNode.Choices = { InspectMark, DebugGrantGold, ClaimGoldReward, Leave };

		FWacomRunEventChoiceDefinition TryClaimAgain;
		TryClaimAgain.ChoiceId = TEXT("TryClaimAgain");
		TryClaimAgain.LabelText = FText::FromString(TEXT("再次领取奖励"));
		TryClaimAgain.Conditions =
		{
			MakeRunFlagNotSet(RewardClaimedFlag),
			MakeMinGold(3),
		};
		TryClaimAgain.Effects =
		{
			MakeAddGold(-3),
			MakeGainCard(PoisonFang),
			MakeSetRunFlag(RewardClaimedFlag),
		};

		FWacomRunEventChoiceDefinition ResetFlags;
		ResetFlags.ChoiceId = TEXT("ResetFlags");
		ResetFlags.LabelText = FText::FromString(TEXT("调试：重置标记"));
		ResetFlags.Conditions = { MakeRunFlagSet(RewardClaimedFlag) };
		ResetFlags.Effects =
		{
			MakeClearRunFlag(InspectedFlag),
			MakeClearRunFlag(GoldGrantedFlag),
			MakeClearRunFlag(RewardClaimedFlag),
		};
		ResetFlags.NextNodeId = TEXT("Start");

		FWacomRunEventChoiceDefinition Close;
		Close.ChoiceId = TEXT("Close");
		Close.LabelText = FText::FromString(TEXT("离开"));
		Close.bCloseEventAfterResolve = true;

		FWacomRunEventNodeDefinition RewardedNode;
		RewardedNode.NodeId = TEXT("Rewarded");
		RewardedNode.TitleText = FText::FromString(TEXT("奖励已领取"));
		RewardedNode.BodyText = FText::FromString(TEXT("刻痕沉入石面，毒牙落入你的行囊。"));
		RewardedNode.Choices = { TryClaimAgain, ResetFlags, Close };

		Event->Nodes = { StartNode, RewardedNode };

		SaveAssetPackage(Pkg, Event, PackagePath);
		return Event;
	}
}

namespace Wacom::ContentBuilder
{
	UWacomRunEventDefinition* BuildRunEventContent()
	{
		UCardDefinition* PoisonFang = LoadGeneratedCard(
			MakeObjectPath(MakePackagePath(RewardCardsRoot(), TEXT("DA_Card_PoisonFang"))));
		if (!PoisonFang)
		{
			return nullptr;
		}

		UWacomRunEventDefinition* SnakeGift = BuildDebugSnakeGiftEvent(PoisonFang);
		UWacomRunEventDefinition* FlagReward = BuildDebugFlagRewardEvent(PoisonFang);
		return (SnakeGift && FlagReward) ? SnakeGift : nullptr;
	}
}
