// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

class UWacomFloorMapDefinition;
class UWacomJourneyDefinition;

/** Journey/Floor authoring validation 的共享结果合同。 */
struct WACOMEDITOR_API FWacomMapDefinitionValidationReport
{
	TArray<FText> Errors;
	TArray<FText> Warnings;

	bool HasErrors() const { return !Errors.IsEmpty(); }
	bool HasWarnings() const { return !Warnings.IsEmpty(); }
	bool IsValid() const { return Errors.IsEmpty(); }

	void Append(const FWacomMapDefinitionValidationReport& Other)
	{
		Errors.Append(Other.Errors);
		Warnings.Append(Other.Warnings);
	}
};

/** Journey/Floor graph、typed content、reachability 与入口条件的共享制作期校验。 */
struct WACOMEDITOR_API FWacomMapDefinitionValidation
{
	static FWacomMapDefinitionValidationReport ValidateFloor(
		const UWacomFloorMapDefinition* FloorDefinition);
	static FWacomMapDefinitionValidationReport ValidateJourney(
		const UWacomJourneyDefinition* JourneyDefinition);

	/** 对一组 Journey 执行 JourneyId 项目级唯一性校验；不会重复执行单资产字段校验。 */
	static FWacomMapDefinitionValidationReport ValidateJourneyIds(
		TConstArrayView<const UWacomJourneyDefinition*> JourneyDefinitions);
};
