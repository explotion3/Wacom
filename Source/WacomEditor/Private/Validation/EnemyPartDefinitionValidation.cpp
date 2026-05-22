// Copyright Wacom. All Rights Reserved.

#include "Validation/EnemyPartDefinitionValidation.h"

#include "Enemies/EnemyPartDefinition.h"

#define LOCTEXT_NAMESPACE "WacomEnemyPartDefinitionValidation"

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
}

bool FWacomEnemyPartDefinitionValidation::Validate(
	const UEnemyPartDefinition* EnemyPartDefinition,
	TArray<FText>& OutErrors)
{
	OutErrors.Reset();

	if (!EnemyPartDefinition)
	{
		AddValidationError(OutErrors, LOCTEXT("MissingEnemyPartDefinition", "EnemyPartDefinition 为空。"));
		return false;
	}

	if (EnemyPartDefinition->PartId.IsNone())
	{
		AddValidationError(OutErrors, LOCTEXT("MissingPartId", "PartId 不能为空。"));
	}

	if (EnemyPartDefinition->MaxHp <= 0)
	{
		AddValidationError(OutErrors, LOCTEXT("InvalidMaxHp", "MaxHp 必须大于 0。"));
	}

	if (EnemyPartDefinition->InitialIntentIndex < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeInitialIntentIndex", "InitialIntentIndex 不能为负数。"));
	}

	if (!EnemyPartDefinition->IntentSequence.IsEmpty()
		&& EnemyPartDefinition->InitialIntentIndex >= EnemyPartDefinition->IntentSequence.Num())
	{
		AddValidationError(OutErrors,
			FormatValidationError(TEXT("InitialIntentIndex {0} 超出 IntentSequence 数量 {1}。"),
				FString::FromInt(EnemyPartDefinition->InitialIntentIndex),
				FString::FromInt(EnemyPartDefinition->IntentSequence.Num())));
	}

	if (EnemyPartDefinition->ExperienceReward < 0)
	{
		AddValidationError(OutErrors, LOCTEXT("NegativeExperienceReward", "ExperienceReward 不能为负数。"));
	}

	for (int32 IntentIndex = 0; IntentIndex < EnemyPartDefinition->IntentSequence.Num(); ++IntentIndex)
	{
		const FIntentDefinition& Intent = EnemyPartDefinition->IntentSequence[IntentIndex];
		const FString IntentLabel = FString::Printf(TEXT("IntentSequence[%d]"), IntentIndex);

		if (Intent.IntentId.IsNone())
		{
			AddValidationError(OutErrors,
				FormatValidationError(TEXT("{0} 的 IntentId 不能为空。"), IntentLabel));
		}

		for (int32 EffectIndex = 0; EffectIndex < Intent.Effects.Num(); ++EffectIndex)
		{
			const FIntentEffect& Effect = Intent.Effects[EffectIndex];
			const FString EffectLabel = FString::Printf(TEXT("%s.Effects[%d]"), *IntentLabel, EffectIndex);

			if (!Effect.EffectType.IsValid())
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 EffectType 无效。"), EffectLabel));
			}

			if (Effect.Magnitude < 0)
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 Magnitude 不能为负数。"), EffectLabel));
			}

			if (Effect.Duration < 0)
			{
				AddValidationError(OutErrors,
					FormatValidationError(TEXT("{0} 的 Duration 不能为负数。"), EffectLabel));
			}
		}
	}

	return OutErrors.IsEmpty();
}

#undef LOCTEXT_NAMESPACE
