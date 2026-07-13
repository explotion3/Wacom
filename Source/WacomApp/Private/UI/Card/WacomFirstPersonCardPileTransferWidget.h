// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/Widget.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"
#include "UI/Card/WacomFirstPersonCardPileTransferPlayback.h"
#include "WacomFirstPersonCardPileTransferWidget.generated.h"

class SFirstPersonCardPileTransfer;

DECLARE_MULTICAST_DELEGATE_OneParam(
	FWacomFirstPersonCardPileTransferProgressNative,
	const FWacomFirstPersonCardPileTransferProgressView&);

/** App-private UMG host for the batched Slate pile-transfer renderer. */
UCLASS()
class UWacomFirstPersonCardPileTransferWidget : public UWidget
{
	GENERATED_BODY()

public:
	UWacomFirstPersonCardPileTransferWidget(const FObjectInitializer& ObjectInitializer);
	virtual ~UWacomFirstPersonCardPileTransferWidget() override;

	void SetConfig(const FWacomFirstPersonCardPileTransferConfig& InConfig);
	bool Play(
		const FWacomFirstPersonCardPileTransferHint& Hint,
		const TArray<FVector2D>& SourcePositions,
		const FVector2D& TargetPosition);
	void TickPlayback(float DeltaSeconds);
	void ForceComplete();
	void ResetPlayback();
	bool IsPlaybackActive() const;

	FWacomFirstPersonCardPileTransferProgressNative OnProgressNative;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

private:
	TUniquePtr<FWacomFirstPersonCardPileTransferPlayback> Playback;
	TSharedPtr<SFirstPersonCardPileTransfer> SlateWidget;
	FWacomFirstPersonCardPileTransferConfig Config;
	int32 LastBroadcastLaunchedCount = INDEX_NONE;
	int32 LastBroadcastArrivedCount = INDEX_NONE;
	bool bCompletionBroadcast = false;
	struct FQueuedTransfer
	{
		FWacomFirstPersonCardPileTransferHint Hint;
		TArray<FVector2D> SourcePositions;
		FVector2D TargetPosition = FVector2D::ZeroVector;
	};
	TArray<FQueuedTransfer> PendingTransfers;

	void PlayRequestedSounds();
	bool StartTransfer(const FQueuedTransfer& Transfer);
};
