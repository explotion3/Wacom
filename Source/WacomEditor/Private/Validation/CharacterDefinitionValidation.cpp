// Copyright Wacom. All Rights Reserved.

#include "Validation/CharacterDefinitionValidation.h"

#include "Characters/CharacterDefinition.h"

#define LOCTEXT_NAMESPACE "WacomCharacterDefinitionValidation"

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

bool FWacomCharacterDefinitionValidation::Validate(
	const UCharacterDefinition* CharacterDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!CharacterDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingCharacterDefinition", "CharacterDefinition 为空。"));
		return false;
	}

	if (CharacterDefinition->CharacterId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingCharacterId", "CharacterId 不能为空。"));
	}

	if (CharacterDefinition->FingerCount <= 0)
	{
		AddValidationError(OutErrors, LOCTEXT("InvalidFingerCount", "FingerCount 必须大于 0。"));
	}

	if (CharacterDefinition->HpPerFinger <= 0)
	{
		AddValidationError(OutErrors, LOCTEXT("InvalidHpPerFinger", "HpPerFinger 必须大于 0。"));
	}

	if (!CharacterDefinition->LeftHandCard)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingLeftHandCard", "LeftHandCard 不能为空。"));
	}

	if (!CharacterDefinition->RightHandCard)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingRightHandCard", "RightHandCard 不能为空。"));
	}

	if (CharacterDefinition->StarterDeck.IsEmpty())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingStarterDeck", "StarterDeck 不能为空。"));
	}

	for (int32 Index = 0; Index < CharacterDefinition->StarterDeck.Num(); ++Index)
	{
		const UCardDefinition* StarterCard = CharacterDefinition->StarterDeck[Index];

		if (!StarterCard)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("StarterDeck[{0}] 不能为空。"), FString::FromInt(Index)));
			continue;
		}

		if (StarterCard == CharacterDefinition->LeftHandCard)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("StarterDeck[{0}] 不能包含 LeftHandCard。"), FString::FromInt(Index)));
		}

		if (StarterCard == CharacterDefinition->RightHandCard)
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("StarterDeck[{0}] 不能包含 RightHandCard。"), FString::FromInt(Index)));
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
