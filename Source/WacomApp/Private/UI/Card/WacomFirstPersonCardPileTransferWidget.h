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
		const FVector2D& SourcePosition,
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
	int32 LastBroadcastArrivedCount = INDEX_NONE;
	bool bCompletionBroadcast = false;

	void PlayRequestedSounds();
};
