// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UI/Card/WacomFirstPersonCardLayerTypes.h"

struct FWacomFirstPersonCardPileTransferGlyphView
{
	FVector2D Position = FVector2D::ZeroVector;
	FVector2D Size = FVector2D::ZeroVector;
	float RotationRadians = 0.0f;
	float Opacity = 0.0f;
	float ArrivalPulse = 0.0f;
};

class WACOMAPP_API FWacomFirstPersonCardPileTransferPlayback
{
public:
	bool Start(
		const FWacomFirstPersonCardPileTransferHint& Hint,
		const FWacomFirstPersonCardPileTransferConfig& Config,
		const FVector2D& SourcePosition,
		const FVector2D& TargetPosition);

	FWacomFirstPersonCardPileTransferProgressView Tick(float DeltaSeconds);
	FWacomFirstPersonCardPileTransferProgressView ForceComplete();
	void Reset();

	bool IsActive() const { return bActive; }
	bool IsReducedMotion() const { return bReducedMotion; }
	const TArray<FWacomFirstPersonCardPileTransferGlyphView>& GetGlyphs() const { return Glyphs; }
	const FWacomFirstPersonCardPileTransferStyleData& GetStyle() const { return Style; }
	int32 GetTotalCount() const { return CardInstanceIds.Num(); }
	float GetCompletionPulse() const { return CompletionPulse; }
	bool ConsumeStartSoundRequest();
	bool ConsumeTravelSoundRequest();
	bool ConsumeCompleteSoundRequest();

private:
	TArray<FGuid> CardInstanceIds;
	TArray<FWacomFirstPersonCardPileTransferGlyphView> Glyphs;
	FWacomFirstPersonCardPileTransferStyleData Style;
	FVector2D Source = FVector2D::ZeroVector;
	FVector2D Target = FVector2D::ZeroVector;
	float ElapsedSeconds = 0.0f;
	float StaggerSeconds = 0.0f;
	float TotalSeconds = 0.0f;
	float CompletionPulse = 0.0f;
	uint32 BaseSeed = 0;
	int32 ArrivedCount = 0;
	int32 EventSequence = INDEX_NONE;
	bool bActive = false;
	bool bReducedMotion = false;
	bool bStartSoundRequested = false;
	bool bTravelSoundRequested = false;
	bool bCompleteSoundRequested = false;

	uint32 MakeGlyphSeed(int32 Index) const;
	void UpdateGlyphs();
};
