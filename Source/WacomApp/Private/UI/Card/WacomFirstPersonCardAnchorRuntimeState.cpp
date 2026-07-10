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

void FWacomFirstPersonCardAnchorRuntimeState::SetFeedbackHints(
	FName InSourceId,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& InHints)
{
	if (InSourceId.IsNone())
	{
		return;
	}

	FeedbackHintSourceId = InSourceId;
	FeedbackHints.Reset(InHints.Num());
	for (const FWacomFirstPersonCardLayerFeedbackHint& Hint : InHints)
	{
		if (Hint.CardInstanceId.IsValid()
			&& Hint.FeedbackKind != EWacomFirstPersonCardLayerFeedbackKind::None)
		{
			FeedbackHints.Add(Hint);
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

void FWacomFirstPersonCardAnchorRuntimeState::SetPresentationFrameFeedbackHints(
	FName InSourceId,
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& InHints)
{
	if (InSourceId.IsNone())
	{
		return;
	}

	PresentationFrameFeedbackHintSourceId = InSourceId;
	PresentationFrameFeedbackHints.Reset(InHints.Num());
	for (const FWacomFirstPersonCardLayerFeedbackHint& Hint : InHints)
	{
		if (Hint.CardInstanceId.IsValid()
			&& Hint.FeedbackKind != EWacomFirstPersonCardLayerFeedbackKind::None)
		{
			PresentationFrameFeedbackHints.Add(Hint);
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
	FeedbackHints.Reset();
	PresentationFrameHints.Reset();
	PresentationFrameFeedbackHints.Reset();
	TransitionHintSourceId = NAME_None;
	FeedbackHintSourceId = NAME_None;
	PresentationFrameHintSourceId = NAME_None;
	PresentationFrameFeedbackHintSourceId = NAME_None;
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

void FWacomFirstPersonCardAnchorRuntimeState::ClearFeedbackHints()
{
	FeedbackHints.Reset();
	FeedbackHintSourceId = NAME_None;
}

void FWacomFirstPersonCardAnchorRuntimeState::ClearPresentationFrameHints()
{
	PresentationFrameHints.Reset();
	PresentationFrameHintSourceId = NAME_None;
}

void FWacomFirstPersonCardAnchorRuntimeState::ClearPresentationFrameFeedbackHints()
{
	PresentationFrameFeedbackHints.Reset();
	PresentationFrameFeedbackHintSourceId = NAME_None;
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

bool FWacomFirstPersonCardAnchorRuntimeState::HasFeedbackHintsForCurrentSource() const
{
	return FeedbackHintSourceId == SourceId && FeedbackHints.Num() > 0;
}

bool FWacomFirstPersonCardAnchorRuntimeState::CanConsumeFeedbackHintsForCurrentSource() const
{
	return HasFeedbackHintsForCurrentSource()
		&& IsTransitionPresentationEnabled(SourceId);
}

TArray<FWacomFirstPersonCardLayerFeedbackHint>
FWacomFirstPersonCardAnchorRuntimeState::ConsumeFeedbackHintsForCurrentSource()
{
	TArray<FWacomFirstPersonCardLayerFeedbackHint> Result;
	if (CanConsumeFeedbackHintsForCurrentSource())
	{
		Result = MoveTemp(FeedbackHints);
		FeedbackHintSourceId = NAME_None;
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

bool FWacomFirstPersonCardAnchorRuntimeState::HasPresentationFrameFeedbackHintsForCurrentSource() const
{
	return !SourceId.IsNone() && PresentationFrameFeedbackHintSourceId == SourceId;
}

bool FWacomFirstPersonCardAnchorRuntimeState::HasPresentationFrameFeedbackHintsForSource(FName InSourceId) const
{
	return !InSourceId.IsNone()
		&& PresentationFrameFeedbackHintSourceId == InSourceId;
}

bool FWacomFirstPersonCardAnchorRuntimeState::CanConsumePresentationFrameFeedbackHintsForCurrentSource() const
{
	return HasPresentationFrameFeedbackHintsForCurrentSource()
		&& IsTransitionPresentationEnabled(SourceId);
}

TArray<FWacomFirstPersonCardLayerFeedbackHint>
FWacomFirstPersonCardAnchorRuntimeState::ConsumePresentationFrameFeedbackHintsForCurrentSource()
{
	TArray<FWacomFirstPersonCardLayerFeedbackHint> Result;
	if (CanConsumePresentationFrameFeedbackHintsForCurrentSource())
	{
		Result = MoveTemp(PresentationFrameFeedbackHints);
		PresentationFrameFeedbackHintSourceId = NAME_None;
	}
	return Result;
}
