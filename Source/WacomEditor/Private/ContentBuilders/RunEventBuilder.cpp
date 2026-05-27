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
		HandOverFang.Effects = { MakeRemoveCard(PoisonFang), MakeConsumeNode(1) };
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
}
