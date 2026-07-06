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
	EWacomFirstPersonCardLayerFrameCommitMode LastWrittenCommitMode =
		EWacomFirstPersonCardLayerFrameCommitMode::StateRefresh;
	int32 WriteCount = 0;
	int32 PresentationFrameWriteCount = 0;
	int32 ClearCount = 0;

protected:
	virtual UWacomFirstPersonCardAnchorComponent* ResolveFirstPersonCardAnchor() const override
	{
		return AnchorForTest;
	}

	virtual void WriteRuntimeCardLayerFrame(
		UWacomFirstPersonCardAnchorComponent& Anchor,
		const FWacomFirstPersonCardLayerPresentationFrame& Frame) override
	{
		Super::WriteRuntimeCardLayerFrame(
			Anchor,
			Frame);
		LastWrittenSourceId = Frame.SourceId;
		LastWrittenEntries = Frame.Entries;
		LastWrittenTransitionHints = Frame.TransitionHints;
		LastWrittenFeedbackHints = Frame.FeedbackHints;
		LastWrittenCommitMode = Frame.CommitMode;
		++WriteCount;
		if (Frame.CommitMode == EWacomFirstPersonCardLayerFrameCommitMode::PresentationFrame
			|| Frame.CommitMode == EWacomFirstPersonCardLayerFrameCommitMode::Suppressed)
		{
			++PresentationFrameWriteCount;
		}
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
