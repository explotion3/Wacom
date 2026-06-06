// Copyright Wacom. All Rights Reserved.

#include "Validation/EncounterDefinitionValidation.h"

#include "Encounters/EncounterDefinition.h"

#define LOCTEXT_NAMESPACE "WacomEncounterDefinitionValidation"

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
}

bool FWacomEncounterDefinitionValidation::Validate(
	const UEncounterDefinition* EncounterDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!EncounterDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEncounterDefinition", "EncounterDefinition 为空。"));
		return false;
	}

	if (EncounterDefinition->EncounterDefinitionId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEncounterDefinitionId", "EncounterDefinitionId 不能为空。"));
	}

	if (EncounterDefinition->EnemySlots.IsEmpty())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEnemySlots", "EnemySlots 不能为空。"));
	}

	TSet<FName> SeenEnemySlotIds;
	for (int32 Index = 0; Index < EncounterDefinition->EnemySlots.Num(); ++Index)
	{
		const FEncounterEnemySlot& Slot = EncounterDefinition->EnemySlots[Index];
		const FString IndexText = FString::FromInt(Index);

		if (Slot.EnemySlotId.IsNone())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("EnemySlots[{0}] 的 EnemySlotId 不能为空。"), IndexText));
		}
		else if (SeenEnemySlotIds.Contains(Slot.EnemySlotId))
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("EnemySlots[{0}] 的 EnemySlotId 重复。"), IndexText));
		}
		else
		{
			SeenEnemySlotIds.Add(Slot.EnemySlotId);
		}

		if (!Slot.EnemyDefinition)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("EnemySlots[{0}] 缺少 EnemyDefinition。"), IndexText));
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
