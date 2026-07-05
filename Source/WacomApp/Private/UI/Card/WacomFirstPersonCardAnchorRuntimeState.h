// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomInteractionTargetTypes.h"
#include "UI/Card/WacomCardPresentationTypes.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

class FWacomFirstPersonCardAnchorRuntimeState
{
public:
	bool SetEntries(FName SourceId, const TArray<FWacomFirstPersonCardLayerEntry>& Entries);
	void SetTransitionHints(FName SourceId, const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints);
	void SetFeedbackHints(FName SourceId, const TArray<FWacomFirstPersonCardLayerFeedbackHint>& Hints);
	void SetPresentationFrameHints(FName SourceId, const TArray<FWacomFirstPersonCardLayerTransitionHint>& Hints);
	void SetPresentationFrameFeedbackHints(FName SourceId, const TArray<FWacomFirstPersonCardLayerFeedbackHint>& Hints);
	void SetTransitionPresentationEnabled(FName SourceId, bool bEnabled);
	bool Clear(FName SourceId);
	void ClearTransientInteraction();
	void ClearTransitionHints();
	void ClearFeedbackHints();
	void ClearPresentationFrameHints();
	void ClearPresentationFrameFeedbackHints();

	bool HasRuntimeData() const { return bHasRuntimeData; }
	FName GetSourceId() const { return SourceId; }
	int32 GetCardCount() const { return Entries.Num(); }
	const TArray<FWacomCardViewData>& GetCardData() const { return CardData; }
	const TArray<FWacomFirstPersonCardLayerEntry>& GetEntries() const { return Entries; }
	FName GetTransitionHintSourceId() const { return TransitionHintSourceId; }
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& GetTransitionHints() const { return TransitionHints; }
	FName GetFeedbackHintSourceId() const { return FeedbackHintSourceId; }
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& GetFeedbackHints() const { return FeedbackHints; }
	FName GetPresentationFrameHintSourceId() const { return PresentationFrameHintSourceId; }
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& GetPresentationFrameHints() const { return PresentationFrameHints; }
	FName GetPresentationFrameFeedbackHintSourceId() const { return PresentationFrameFeedbackHintSourceId; }
	const TArray<FWacomFirstPersonCardLayerFeedbackHint>& GetPresentationFrameFeedbackHints() const { return PresentationFrameFeedbackHints; }
	bool IsTransitionPresentationEnabled(FName SourceId) const;
	bool HasTransitionHintsForCurrentSource() const;
	bool CanConsumeTransitionHintsForCurrentSource() const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> ConsumeTransitionHintsForCurrentSource();
	bool HasFeedbackHintsForCurrentSource() const;
	bool CanConsumeFeedbackHintsForCurrentSource() const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> ConsumeFeedbackHintsForCurrentSource();
	bool HasPresentationFrameHintsForCurrentSource() const;
	bool HasPresentationFrameHintsForSource(FName SourceId) const;
	bool CanConsumePresentationFrameHintsForCurrentSource() const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> ConsumePresentationFrameHintsForCurrentSource();
	bool HasPresentationFrameFeedbackHintsForCurrentSource() const;
	bool HasPresentationFrameFeedbackHintsForSource(FName SourceId) const;
	bool CanConsumePresentationFrameFeedbackHintsForCurrentSource() const;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> ConsumePresentationFrameFeedbackHintsForCurrentSource();

	FGuid GetHoveredCardInstanceId() const { return HoveredCardInstanceId; }
	void SetHoveredCardInstanceId(const FGuid& CardInstanceId) { HoveredCardInstanceId = CardInstanceId; }
	void ClearHoveredCardInstanceId() { HoveredCardInstanceId.Invalidate(); }

	FWacomInteractionTargetHandle GetHoveredCardTargetHandle() const { return HoveredCardTargetHandle; }
	void SetHoveredCardTargetHandle(const FWacomInteractionTargetHandle& CardTargetHandle)
	{
		HoveredCardTargetHandle = CardTargetHandle;
	}
	void ClearHoveredCardTargetHandle() { HoveredCardTargetHandle = FWacomInteractionTargetHandle(); }

private:
	TArray<FWacomCardViewData> CardData;
	TArray<FWacomFirstPersonCardLayerEntry> Entries;
	TArray<FWacomFirstPersonCardLayerTransitionHint> TransitionHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> FeedbackHints;
	TArray<FWacomFirstPersonCardLayerTransitionHint> PresentationFrameHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> PresentationFrameFeedbackHints;
	bool bHasRuntimeData = false;
	FName SourceId = NAME_None;
	FName TransitionHintSourceId = NAME_None;
	FName FeedbackHintSourceId = NAME_None;
	FName PresentationFrameHintSourceId = NAME_None;
	FName PresentationFrameFeedbackHintSourceId = NAME_None;
	TSet<FName> TransitionPresentationSuppressedSources;
	FGuid HoveredCardInstanceId;
	FWacomInteractionTargetHandle HoveredCardTargetHandle;
};
