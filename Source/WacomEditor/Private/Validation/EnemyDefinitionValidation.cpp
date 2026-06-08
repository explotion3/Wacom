// Copyright Wacom. All Rights Reserved.

#include "Validation/EnemyDefinitionValidation.h"

#include "Enemies/EnemyBehaviorDefinition.h"
#include "Enemies/EnemyDefinition.h"
#include "Validation/EnemyBehaviorDefinitionValidation.h"

#define LOCTEXT_NAMESPACE "WacomEnemyDefinitionValidation"

namespace
{
	void AddValidationError(TArray<FText>& OutErrors, const FText& Message)
	{
		OutErrors.Add(Message);
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A)
	{
		return FText::FromString(FString::Format(Format, { A }));
	}

	FText FormatValidationError(const TCHAR* Format, const FString& A, const FString& B)
	{
		return FText::FromString(FString::Format(Format, { A, B }));
	}

	const FWacomEnemyPhaseDefinition* FindPhase(
		const UEnemyBehaviorDefinition* BehaviorDefinition,
		FName PhaseId)
	{
		if (!BehaviorDefinition || PhaseId.IsNone())
		{
			return nullptr;
		}

		for (const FWacomEnemyPhaseDefinition& Phase : BehaviorDefinition->Phases)
		{
			if (Phase.PhaseId == PhaseId)
			{
				return &Phase;
			}
		}
		return nullptr;
	}

	bool PhaseHasIntentSet(
		const FWacomEnemyPhaseDefinition* Phase,
		FName IntentSetId)
	{
		return Phase && !IntentSetId.IsNone() && Phase->IntentSets.ContainsByPredicate(
			[IntentSetId](const FWacomEnemyIntentSetDefinition& IntentSet)
			{
				return IntentSet.IntentSetId == IntentSetId;
			});
	}

	void AppendBehaviorErrors(
		TArray<FText>& OutErrors,
		const FString& Label,
		const TArray<FText>& BehaviorErrors)
	{
		for (const FText& Error : BehaviorErrors)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0}: {1}"), Label, Error.ToString()));
		}
	}
}

bool FWacomEnemyDefinitionValidation::Validate(
	const UEnemyDefinition* EnemyDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!EnemyDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEnemyDefinition", "EnemyDefinition 为空。"));
		return false;
	}

	if (EnemyDefinition->EnemyId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEnemyId", "EnemyId 不能为空。"));
	}

	if (EnemyDefinition->Parts.IsEmpty())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingParts", "Parts 不能为空。"));
	}

	if (EnemyDefinition->DefaultBehavior)
	{
		TArray<FText> BehaviorErrors;
		if (!FWacomEnemyBehaviorDefinitionValidation::Validate(
			EnemyDefinition->DefaultBehavior,
			BehaviorErrors,
			EnemyDefinition))
		{
			AppendBehaviorErrors(OutErrors, TEXT("DefaultBehavior"), BehaviorErrors);
		}

		if (!EnemyDefinition->DefaultPhaseId.IsNone()
			&& !FindPhase(EnemyDefinition->DefaultBehavior, EnemyDefinition->DefaultPhaseId))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("DefaultPhaseId 没有对应 DefaultBehavior Phase：{0}。"),
					EnemyDefinition->DefaultPhaseId.ToString()));
		}
	}
	else if (!EnemyDefinition->DefaultPhaseId.IsNone())
	{
		AddValidationError(OutErrors,
			LOCTEXT("DefaultPhaseWithoutBehavior", "DefaultPhaseId 需要配合 DefaultBehavior 使用。"));
	}

	TSet<FName> UsedPartSlotIds;
	for (int32 Index = 0; Index < EnemyDefinition->Parts.Num(); ++Index)
	{
		const FEnemyPartSlot& Slot = EnemyDefinition->Parts[Index];
		const FString SlotLabel = FString::Printf(TEXT("Parts[%d]"), Index);

		if (Slot.PartSlotId.IsNone())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 PartSlotId 不能为空。"), SlotLabel));
		}
		else if (UsedPartSlotIds.Contains(Slot.PartSlotId))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("重复 PartSlotId：{0}。"), Slot.PartSlotId.ToString()));
		}
		UsedPartSlotIds.Add(Slot.PartSlotId);

		if (!Slot.PartDef)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("Parts[{0}] 缺少 PartDef。"), FString::FromInt(Index)));
		}

		if (Slot.BehaviorOverride)
		{
			TArray<FText> BehaviorErrors;
			if (!FWacomEnemyBehaviorDefinitionValidation::Validate(
				Slot.BehaviorOverride,
				BehaviorErrors,
				EnemyDefinition))
			{
				AppendBehaviorErrors(OutErrors,
					FString::Printf(TEXT("%s.BehaviorOverride"), *SlotLabel),
					BehaviorErrors);
			}
		}

		if (!Slot.InitialIntentSetId.IsNone())
		{
			const UEnemyBehaviorDefinition* Behavior =
				Slot.BehaviorOverride ? Slot.BehaviorOverride.Get() : EnemyDefinition->DefaultBehavior.Get();
			if (!Behavior)
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 配置了 InitialIntentSetId 但没有 BehaviorDefinition。"),
						SlotLabel));
				continue;
			}

			const FName PhaseId = !EnemyDefinition->DefaultPhaseId.IsNone()
				? EnemyDefinition->DefaultPhaseId
				: Behavior->InitialPhaseId;
			const FWacomEnemyPhaseDefinition* Phase = FindPhase(Behavior, PhaseId);
			if (!PhaseHasIntentSet(Phase, Slot.InitialIntentSetId))
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0}.InitialIntentSetId 没有对应 intent set：{1}。"),
						SlotLabel,
						Slot.InitialIntentSetId.ToString()));
			}
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
