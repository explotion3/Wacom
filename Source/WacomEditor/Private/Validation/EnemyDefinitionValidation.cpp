// Copyright Wacom. All Rights Reserved.

#include "Validation/EnemyDefinitionValidation.h"

#include "Enemies/EnemyDefinition.h"

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

	for (int32 Index = 0; Index < EnemyDefinition->Parts.Num(); ++Index)
	{
		if (!EnemyDefinition->Parts[Index].PartDef)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("Parts[{0}] 缺少 PartDef。"), FString::FromInt(Index)));
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
