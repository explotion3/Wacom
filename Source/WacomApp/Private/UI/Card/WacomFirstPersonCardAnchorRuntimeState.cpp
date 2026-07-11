// Copyright Wacom. All Rights Reserved.

#include "UI/Card/WacomFirstPersonCardAnchorRuntimeState.h"

bool FWacomFirstPersonCardAnchorRuntimeState::SetEntries(
	FName InSourceId,
	const TArray<FWacomFirstPersonCardLayerEntry>& InEntries)
{
	if (InSourceId.IsNone())
	{
		return false;
	}

	const bool bSourceChanged = SourceId != InSourceId;
	SourceId = InSourceId;
	if (bSourceChanged && PresentationAnchorSourceId != InSourceId)
	{
		PresentationAnchorSourceId = NAME_None;
		PresentationAnchors = FWacomFirstPersonCardPresentationAnchorSet();
	}
	Entries.Reset(InEntries.Num());
	CardData.Reset(InEntries.Num());
	for (FWacomFirstPersonCardLayerEntry Entry : InEntries)
	{
		Entry.bIsPlayable = Entry.bIsPlayable && !Entry.CardViewData.bDisabled;
		Entry.CardViewData.bDisabled = !Entry.bIsPlayable;
		Entries.Add(Entry);
		CardData.Add(Entry.CardViewData);
	}
	bHasRuntimeData = true;
	return bSourceChanged;
}

void FWacomFirstPersonCardAnchorRuntimeState::SetTransitionHints(
	FName InSourceId,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& InHints)
{
	if (InSourceId.IsNone())
	{
		return;
	}

	TransitionHintSourceId = InSourceId;
	TransitionHints.Reset(InHints.Num());
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : InHints)
	{
		if (Hint.CardInstanceId.IsValid()
			&& (Hint.TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Default
				|| Hint.bPlayCommitFeedback))
		{
			TransitionHints.Add(Hint);
		}
	}
}

void FWacomFirstPersonCardAnchorRuntimeState::SetPresentationFrameHints(
	FName InSourceId,
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& InHints)
{
	if (InSourceId.IsNone())
	{
		return;
	}

	PresentationFrameHintSourceId = InSourceId;
	PresentationFrameHints.Reset(InHints.Num());
	for (const FWacomFirstPersonCardLayerTransitionHint& Hint : InHints)
	{
		if (Hint.CardInstanceId.IsValid()
			&& (Hint.TransitionKind != EWacomFirstPersonCardSlotTransitionKind::Default
				|| Hint.bPlayCommitFeedback))
		{
			PresentationFrameHints.Add(Hint);
		}
	}
}

void FWacomFirstPersonCardAnchorRuntimeState::SetTransitionPresentationEnabled(
	FName InSourceId,
	bool bEnabled)
{
	if (InSourceId.IsNone())
	{
		return;
	}

	if (bEnabled)
	{
		TransitionPresentationSuppressedSources.Remove(InSourceId);
	}
	else
	{
		TransitionPresentationSuppressedSources.Add(InSourceId);
	}
}

bool FWacomFirstPersonCardAnchorRuntimeState::Clear(FName InSourceId)
{
	if (InSourceId.IsNone() || SourceId != InSourceId)
	{
		return false;
	}

	SourceId = NAME_None;
	CardData.Reset();
	Entries.Reset();
	TransitionHints.Reset();
	PresentationFrameHints.Reset();
	TransitionHintSourceId = NAME_None;
	PresentationFrameHintSourceId = NAME_None;
	PresentationAnchorSourceId = NAME_None;
	PresentationAnchors = FWacomFirstPersonCardPresentationAnchorSet();
	TransitionPresentationSuppressedSources.Remove(InSourceId);
	ClearTransientInteraction();
	bHasRuntimeData = false;
	return true;
}

void FWacomFirstPersonCardAnchorRuntimeState::ClearTransientInteraction()
{
	HoveredCardInstanceId.Invalidate();
	HoveredCardTargetHandle = FWacomInteractionTargetHandle();
}

void FWacomFirstPersonCardAnchorRuntimeState::ClearTransitionHints()
{
	TransitionHints.Reset();
	TransitionHintSourceId = NAME_None;
}

void FWacomFirstPersonCardAnchorRuntimeState::SetPresentationAnchors(
	FName InSourceId,
	const FWacomFirstPersonCardPresentationAnchorSet& InAnchors)
{
	if (InSourceId.IsNone())
	{
		return;
	}
	PresentationAnchorSourceId = InSourceId;
	PresentationAnchors = InAnchors;
}

void FWacomFirstPersonCardAnchorRuntimeState::ClearPresentationAnchors(FName InSourceId)
{
	if (!InSourceId.IsNone() && PresentationAnchorSourceId == InSourceId)
	{
		PresentationAnchorSourceId = NAME_None;
		PresentationAnchors = FWacomFirstPersonCardPresentationAnchorSet();
	}
}

const FWacomFirstPersonCardPresentationAnchorSet&
FWacomFirstPersonCardAnchorRuntimeState::GetPresentationAnchorsForCurrentSource() const
{
	static const FWacomFirstPersonCardPresentationAnchorSet EmptyAnchors;
	return !SourceId.IsNone() && PresentationAnchorSourceId == SourceId
		? PresentationAnchors
		: EmptyAnchors;
}

void FWacomFirstPersonCardAnchorRuntimeState::ClearPresentationFrameHints()
{
	PresentationFrameHints.Reset();
	PresentationFrameHintSourceId = NAME_None;
}

bool FWacomFirstPersonCardAnchorRuntimeState::IsTransitionPresentationEnabled(FName InSourceId) const
{
	return InSourceId.IsNone()
		|| !TransitionPresentationSuppressedSources.Contains(InSourceId);
}

bool FWacomFirstPersonCardAnchorRuntimeState::HasTransitionHintsForCurrentSource() const
{
	return TransitionHintSourceId == SourceId && TransitionHints.Num() > 0;
}

bool FWacomFirstPersonCardAnchorRuntimeState::CanConsumeTransitionHintsForCurrentSource() const
{
	return HasTransitionHintsForCurrentSource()
		&& IsTransitionPresentationEnabled(SourceId);
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomFirstPersonCardAnchorRuntimeState::ConsumeTransitionHintsForCurrentSource()
{
	TArray<FWacomFirstPersonCardLayerTransitionHint> Result;
	if (CanConsumeTransitionHintsForCurrentSource())
	{
		Result = MoveTemp(TransitionHints);
		TransitionHintSourceId = NAME_None;
	}
	return Result;
}

bool FWacomFirstPersonCardAnchorRuntimeState::HasPresentationFrameHintsForCurrentSource() const
{
	return !SourceId.IsNone() && PresentationFrameHintSourceId == SourceId;
}

bool FWacomFirstPersonCardAnchorRuntimeState::HasPresentationFrameHintsForSource(FName InSourceId) const
{
	return !InSourceId.IsNone()
		&& PresentationFrameHintSourceId == InSourceId;
}

bool FWacomFirstPersonCardAnchorRuntimeState::CanConsumePresentationFrameHintsForCurrentSource() const
{
	return HasPresentationFrameHintsForCurrentSource()
		&& IsTransitionPresentationEnabled(SourceId);
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomFirstPersonCardAnchorRuntimeState::ConsumePresentationFrameHintsForCurrentSource()
{
	TArray<FWacomFirstPersonCardLayerTransitionHint> Result;
	if (CanConsumePresentationFrameHintsForCurrentSource())
	{
		Result = MoveTemp(PresentationFrameHints);
		PresentationFrameHintSourceId = NAME_None;
	}
	return Result;
}
