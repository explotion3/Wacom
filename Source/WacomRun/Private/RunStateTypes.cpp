// Copyright Wacom. All Rights Reserved.

#include "RunStateTypes.h"

namespace
{
	int32 ClampPressure(int32 Value)
	{
		return FMath::Clamp(Value, 0, 100);
	}
}

int32 FPressureValues::Get(EWacomPressureType Type) const
{
	switch (Type)
	{
	case EWacomPressureType::Hunger:     return Hunger;
	case EWacomPressureType::Wound:      return Wound;
	case EWacomPressureType::Fatigue:    return Fatigue;
	case EWacomPressureType::Burden:     return Burden;
	case EWacomPressureType::Decay:      return Decay;
	case EWacomPressureType::Misdeed:    return Misdeed;
	case EWacomPressureType::Bloodlust:  return Bloodlust;
	case EWacomPressureType::Disability: return Disability;
	default:
		ensureMsgf(false, TEXT("FPressureValues::Get 收到未知 EWacomPressureType=%d"), (int32)Type);
		return 0;
	}
}

void FPressureValues::Set(EWacomPressureType Type, int32 Value)
{
	const int32 Clamped = ClampPressure(Value);
	switch (Type)
	{
	case EWacomPressureType::Hunger:     Hunger     = Clamped; break;
	case EWacomPressureType::Wound:      Wound      = Clamped; break;
	case EWacomPressureType::Fatigue:    Fatigue    = Clamped; break;
	case EWacomPressureType::Burden:     Burden     = Clamped; break;
	case EWacomPressureType::Decay:      Decay      = Clamped; break;
	case EWacomPressureType::Misdeed:    Misdeed    = Clamped; break;
	case EWacomPressureType::Bloodlust:  Bloodlust  = Clamped; break;
	case EWacomPressureType::Disability: Disability = Clamped; break;
	default:
		ensureMsgf(false, TEXT("FPressureValues::Set 收到未知 EWacomPressureType=%d"), (int32)Type);
		break;
	}
}

void FPressureValues::Add(EWacomPressureType Type, int32 Delta)
{
	Set(Type, Get(Type) + Delta);
}
