// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/WacomRunFirstPersonCardSourceComponent.h"
#include "RunFirstPersonCardLayerSpecReceiver.generated.h"

class UWacomFirstPersonCardAnchorComponent;

UCLASS()
class UWacomRunFirstPersonCardSourceSpecProbeComponent
	: public UWacomRunFirstPersonCardSourceComponent
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TObjectPtr<UWacomFirstPersonCardAnchorComponent> AnchorForTest = nullptr;

	TArray<FWacomFirstPersonCardLayerEntry> LastWrittenEntries;
	TArray<FWacomFirstPersonCardLayerTransitionHint> LastWrittenTransitionHints;
	TArray<FWacomFirstPersonCardLayerFeedbackHint> LastWrittenFeedbackHints;
	FName LastWrittenSourceId = NAME_None;
	FName LastClearedSourceId = NAME_None;
	int32 WriteCount = 0;
	int32 PresentationFrameWriteCount = 0;
	int32 ClearCount = 0;

protected:
	virtual UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const override
	{
		return AnchorForTest;
	}

	virtual void WriteRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId,
		const TArray<FWacomFirstPersonCardLayerEntry>& Entries) override
	{
		Super::WriteRuntimeCardLayerEntries(Anchor, SourceId, Entries);
		LastWrittenSourceId = SourceId;
		LastWrittenEntries = Entries;
		++WriteCount;
	}

	virtual void WriteRuntimeCardLayerPresentationFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardLayerPresentationFrame& Frame) override
	{
		Super::WriteRuntimeCardLayerPresentationFrame(
			Anchor,
			Frame);
		LastWrittenSourceId = Frame.SourceId;
		LastWrittenEntries = Frame.Entries;
		LastWrittenTransitionHints = Frame.TransitionHints;
		LastWrittenFeedbackHints = Frame.FeedbackHints;
		++WriteCount;
		++PresentationFrameWriteCount;
	}

	virtual void ClearRuntimeCardLayerEntries(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		FName SourceId) override
	{
		Super::ClearRuntimeCardLayerEntries(Anchor, SourceId);
		LastClearedSourceId = SourceId;
		LastWrittenEntries.Reset();
		LastWrittenTransitionHints.Reset();
		LastWrittenFeedbackHints.Reset();
		++ClearCount;
	}
};
