// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"
#include "RunStateTypes.h"

class UCardDefinition;
class UWacomRunEventDefinition;
struct FWacomRunEventChoiceDefinition;
struct FWacomRunEventNodeDefinition;

/**
 * RunEvent 的私有执行器。
 *
 * 只解释 UWacomRunEventDefinition 并修改 FRunState；不广播、不访问 UI、不暴露 Blueprint。
 * URunSession 仍负责 public API、生命周期入口和 NotifyRunStateChanged。
 */
struct FRunEventExecutor
{
	static const FWacomRunEventNodeDefinition* FindNode(const UWacomRunEventDefinition* EventDefinition, FName NodeId);
	static const FWacomRunEventChoiceDefinition* FindChoice(const FWacomRunEventNodeDefinition* Node, FName ChoiceId);

	static bool BeginEvent(FRunState& State, FName PersistentId, UWacomRunEventDefinition* EventDefinition);
	static FRunEventSnapshot BuildSnapshot(const FRunState& State);
	static FRunEventChoiceResult ChooseOption(FRunState& State, FName ChoiceId);
	static FRunDeckOperationValidation ValidateChoiceCardPayment(const FRunState& State, FName ChoiceId, FGuid PaidCardInstanceId);
	static FRunEventChoiceResult ChooseOptionWithPaidCard(FRunState& State, FName ChoiceId, FGuid PaidCardInstanceId);

	static bool IsEventCompleted(const FRunState& State, FName PersistentId);
	static bool IsChoiceAvailable(const FRunState& State, const FWacomRunEventChoiceDefinition& Choice, FName& OutDisabledReason);
	static FName ResolvePaymentZoneId(const FWacomRunEventChoiceDefinition& Choice);
	static void CollectCardPaymentCandidateInstanceIds(const FRunState& State, const FWacomRunEventChoiceDefinition& Choice, TArray<FGuid>& OutInstanceIds, FName& OutDisabledReason);

private:
	static bool TryResolvePressureType(FName PressureTypeId, EWacomPressureType& OutType);
	static bool ApplyChoiceEffects(FRunState& State, const FWacomRunEventChoiceDefinition& Choice, TArray<FRunEventChoiceEffectResult>* OutEffectResults, FName* OutDisabledReason);
	static bool DoesCardMatchPaymentFilter(const FCardInstance& Instance, const FWacomRunEventChoiceDefinition& Choice, FName& OutDisabledReason);
	static bool HasValidPaymentFilter(const FWacomRunEventChoiceDefinition& Choice);
	static FRunEventChoiceResult ChooseOptionInternal(FRunState& State, FName ChoiceId, TOptional<FGuid> PaidCardInstanceId);

	static bool AcquireCard(FRunState& State, UCardDefinition* Card);
};
