// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RunState.h"

struct FWacomRunEventPresentationStateView
{
	const TArray<FRunEventChoiceSnapshot>* Choices = nullptr;
	const TMap<FName, FName>* PaymentZoneToChoiceId = nullptr;

	TConstArrayView<FRunEventChoiceSnapshot> GetChoices() const;
	int32 GetChoiceCount() const;
	int32 GetPaymentZoneMappingCount() const;
	bool FindChoice(FName ChoiceId, FRunEventChoiceSnapshot& OutChoice) const;
	bool FindChoiceIdForPaymentZone(FName ZoneId, FName& OutChoiceId) const;
	bool FindPaymentChoiceForZone(FName ZoneId, FRunEventChoiceSnapshot& OutChoice) const;
	FString BuildPaymentZoneMappingDebugSummary() const;
};

struct FWacomRunEventPresentationStateEdit
{
	TArray<FRunEventChoiceSnapshot>* Choices = nullptr;
	TMap<FName, FName>* PaymentZoneToChoiceId = nullptr;

	bool IsValid() const;
	FWacomRunEventPresentationStateView AsView() const;
	void ResetChoices() const;
	void SetChoices(TConstArrayView<FRunEventChoiceSnapshot> InChoices) const;
	void ResetPaymentZoneMappings() const;
	void AddPaymentZoneMapping(FName ZoneId, FName ChoiceId) const;
};
