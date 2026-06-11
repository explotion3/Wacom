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
	TransitionHintSourceId = NAME_None;
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

bool FWacomFirstPersonCardAnchorRuntimeState::HasTransitionHintsForCurrentSource() const
{
	return TransitionHintSourceId == SourceId && TransitionHints.Num() > 0;
}

TArray<FWacomFirstPersonCardLayerTransitionHint>
FWacomFirstPersonCardAnchorRuntimeState::ConsumeTransitionHintsForCurrentSource()
{
	TArray<FWacomFirstPersonCardLayerTransitionHint> Result;
	if (HasTransitionHintsForCurrentSource())
	{
		Result = MoveTemp(TransitionHints);
		TransitionHintSourceId = NAME_None;
	}
	return Result;
}
