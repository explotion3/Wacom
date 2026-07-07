// Copyright Wacom. All Rights Reserved.

#include "UI/Events/WacomRunEventPresentationState.h"

namespace
{
	const TArray<FRunEventChoiceSnapshot>& EmptyRunEventChoices()
	{
		static const TArray<FRunEventChoiceSnapshot> EmptyChoices;
		return EmptyChoices;
	}
}

TConstArrayView<FRunEventChoiceSnapshot> FWacomRunEventPresentationStateView::GetChoices() const
{
	if (Choices)
	{
		return TConstArrayView<FRunEventChoiceSnapshot>(*Choices);
	}
	return TConstArrayView<FRunEventChoiceSnapshot>(EmptyRunEventChoices());
}

int32 FWacomRunEventPresentationStateView::GetChoiceCount() const
{
	return Choices ? Choices->Num() : 0;
}

int32 FWacomRunEventPresentationStateView::GetPaymentZoneMappingCount() const
{
	return PaymentZoneToChoiceId ? PaymentZoneToChoiceId->Num() : 0;
}

bool FWacomRunEventPresentationStateView::FindChoice(
	FName ChoiceId,
	FRunEventChoiceSnapshot& OutChoice) const
{
	if (ChoiceId.IsNone() || !Choices)
	{
		return false;
	}

	const FRunEventChoiceSnapshot* FoundChoice = Choices->FindByPredicate(
		[ChoiceId](const FRunEventChoiceSnapshot& Choice)
		{
			return Choice.ChoiceId == ChoiceId;
		});
	if (!FoundChoice)
	{
		return false;
	}

	OutChoice = *FoundChoice;
	return true;
}

bool FWacomRunEventPresentationStateView::FindChoiceIdForPaymentZone(
	FName ZoneId,
	FName& OutChoiceId) const
{
	OutChoiceId = NAME_None;
	if (ZoneId.IsNone() || !PaymentZoneToChoiceId)
	{
		return false;
	}

	const FName* ChoiceId = PaymentZoneToChoiceId->Find(ZoneId);
	if (!ChoiceId || ChoiceId->IsNone())
	{
		return false;
	}

	OutChoiceId = *ChoiceId;
	return true;
}

bool FWacomRunEventPresentationStateView::FindPaymentChoiceForZone(
	FName ZoneId,
	FRunEventChoiceSnapshot& OutChoice) const
{
	FName ChoiceId;
	return FindChoiceIdForPaymentZone(ZoneId, ChoiceId)
		&& FindChoice(ChoiceId, OutChoice);
}

FString FWacomRunEventPresentationStateView::BuildPaymentZoneMappingDebugSummary() const
{
	if (!PaymentZoneToChoiceId)
	{
		return FString();
	}

	TArray<FString> Entries;
	Entries.Reserve(PaymentZoneToChoiceId->Num());
	for (const TPair<FName, FName>& Pair : *PaymentZoneToChoiceId)
	{
		Entries.Add(FString::Printf(TEXT("%s->%s"), *Pair.Key.ToString(), *Pair.Value.ToString()));
	}
	Entries.Sort();
	return FString::Join(Entries, TEXT(","));
}

bool FWacomRunEventPresentationStateEdit::IsValid() const
{
	return Choices && PaymentZoneToChoiceId;
}

FWacomRunEventPresentationStateView FWacomRunEventPresentationStateEdit::AsView() const
{
	return FWacomRunEventPresentationStateView{ Choices, PaymentZoneToChoiceId };
}

void FWacomRunEventPresentationStateEdit::ResetChoices() const
{
	if (Choices)
	{
		Choices->Reset();
	}
}

void FWacomRunEventPresentationStateEdit::SetChoices(
	TConstArrayView<FRunEventChoiceSnapshot> InChoices) const
{
	if (!Choices)
	{
		return;
	}

	Choices->Reset(InChoices.Num());
	for (const FRunEventChoiceSnapshot& Choice : InChoices)
	{
		Choices->Add(Choice);
	}
}

void FWacomRunEventPresentationStateEdit::ResetPaymentZoneMappings() const
{
	if (PaymentZoneToChoiceId)
	{
		PaymentZoneToChoiceId->Reset();
	}
}

void FWacomRunEventPresentationStateEdit::AddPaymentZoneMapping(FName ZoneId, FName ChoiceId) const
{
	if (!PaymentZoneToChoiceId || ZoneId.IsNone() || ChoiceId.IsNone())
	{
		return;
	}
	PaymentZoneToChoiceId->Add(ZoneId, ChoiceId);
}
