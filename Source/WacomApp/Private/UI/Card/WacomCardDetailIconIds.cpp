// Copyright Wacom. All Rights Reserved.

#include "WacomCardDetailIconIds.h"

namespace WacomCardDetailIconIds
{
	FString ToString(EWacomCardDetailIcon Icon)
	{
		const UEnum* IconEnum = StaticEnum<EWacomCardDetailIcon>();
		if (!IconEnum)
		{
			return TEXT("None");
		}

		const FString Name = IconEnum->GetNameStringByValue(static_cast<int64>(Icon));
		return Name.IsEmpty() ? TEXT("None") : Name;
	}

	EWacomCardDetailIcon FromString(const FString& Id)
	{
		if (Id.IsEmpty())
		{
			return EWacomCardDetailIcon::None;
		}

		const UEnum* IconEnum = StaticEnum<EWacomCardDetailIcon>();
		if (!IconEnum)
		{
			return EWacomCardDetailIcon::None;
		}

		for (int32 Index = 0; Index < IconEnum->NumEnums(); ++Index)
		{
			if (IconEnum->GetNameStringByIndex(Index) == Id)
			{
				return static_cast<EWacomCardDetailIcon>(IconEnum->GetValueByIndex(Index));
			}
		}

		return EWacomCardDetailIcon::None;
	}
}
