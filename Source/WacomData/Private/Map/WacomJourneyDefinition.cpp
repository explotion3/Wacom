// Copyright Wacom. All Rights Reserved.

#include "Map/WacomJourneyDefinition.h"

#include "Map/WacomFloorMapDefinition.h"

namespace
{
	int32 EvaluateRoundedNonNegative(const FRuntimeFloatCurve& Curve, const float X, const int32 Fallback)
	{
		const FRichCurve* RichCurve = Curve.GetRichCurveConst();
		if (!RichCurve || RichCurve->GetNumKeys() == 0)
		{
			return Fallback;
		}

		return FMath::Max(0, FMath::RoundToInt(RichCurve->Eval(X, static_cast<float>(Fallback))));
	}
}

UWacomJourneyDefinition::UWacomJourneyDefinition()
{
	BaseDecayCurve.GetRichCurve()->AddKey(1.0f, 5.0f);

	FRichCurve* OverstayCurve = OverstayDecayCurve.GetRichCurve();
	OverstayCurve->AddKey(1.0f, 0.0f);
	OverstayCurve->AddKey(3.0f, 0.0f);
	OverstayCurve->AddKey(4.0f, 2.0f);
	OverstayCurve->AddKey(5.0f, 5.0f);
	OverstayCurve->AddKey(6.0f, 9.0f);
	OverstayCurve->AddKey(7.0f, 12.0f);
}

const UWacomFloorMapDefinition* UWacomJourneyDefinition::FindFloor(const FName FloorId) const
{
	const TObjectPtr<UWacomFloorMapDefinition>* Found = Floors.FindByPredicate(
		[FloorId](const TObjectPtr<UWacomFloorMapDefinition>& Floor)
		{
			return Floor && Floor->FloorId == FloorId;
		});
	return Found ? Found->Get() : nullptr;
}

int32 UWacomJourneyDefinition::FindFloorIndex(const FName FloorId) const
{
	return Floors.IndexOfByPredicate([FloorId](const TObjectPtr<UWacomFloorMapDefinition>& Floor)
	{
		return Floor && Floor->FloorId == FloorId;
	});
}

int32 UWacomJourneyDefinition::EvaluateBaseDecay(const int32 JourneyDay) const
{
	return EvaluateRoundedNonNegative(BaseDecayCurve, static_cast<float>(FMath::Max(1, JourneyDay)), 5);
}

int32 UWacomJourneyDefinition::EvaluateOverstayDecay(const int32 FloorDay) const
{
	const int32 SafeFloorDay = FMath::Max(1, FloorDay);
	const int32 Fallback = SafeFloorDay <= 3 ? 0
		: SafeFloorDay == 4 ? 2
		: SafeFloorDay == 5 ? 5
		: SafeFloorDay == 6 ? 9
		: 12;
	return EvaluateRoundedNonNegative(OverstayDecayCurve, static_cast<float>(SafeFloorDay), Fallback);
}
