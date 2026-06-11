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
	bool Clear(FName SourceId);
	void ClearTransientInteraction();
	void ClearTransitionHints();

	bool HasRuntimeData() const { return bHasRuntimeData; }
	FName GetSourceId() const { return SourceId; }
	int32 GetCardCount() const { return Entries.Num(); }
	const TArray<FWacomCardViewData>& GetCardData() const { return CardData; }
	const TArray<FWacomFirstPersonCardLayerEntry>& GetEntries() const { return Entries; }
	FName GetTransitionHintSourceId() const { return TransitionHintSourceId; }
	const TArray<FWacomFirstPersonCardLayerTransitionHint>& GetTransitionHints() const { return TransitionHints; }
	bool HasTransitionHintsForCurrentSource() const;
	TArray<FWacomFirstPersonCardLayerTransitionHint> ConsumeTransitionHintsForCurrentSource();

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
	bool bHasRuntimeData = false;
	FName SourceId = NAME_None;
	FName TransitionHintSourceId = NAME_None;
	FGuid HoveredCardInstanceId;
	FWacomInteractionTargetHandle HoveredCardTargetHandle;
};
