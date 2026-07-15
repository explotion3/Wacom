// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Exploration/RunExplorationResolution.h"
#include "UObject/StrongObjectPtr.h"

class UCharacterDefinition;
class URunSession;
class UWacomFloorMapDefinition;
class UWacomJourneyDefinition;

/** 测试显式持有 Session 与一次性初始化结果。 */
struct WACOMTESTS_API FWacomInitializedRunExplorationSession
{
	URunSession* Session = nullptr;
	FRunInitializationResult Initialization;
};

/**
 * 为仍由各自测试构造 Character 的旧夹具创建最小正式 Journey，并返回完整初始化结果。
 * 仅用于测试迁移；调用方仍须显式检查 IsOk()，不得退化为只返回 bool 的兼容入口。
 */
WACOMTESTS_API FRunInitializationResult InitializeRunSessionForTest(
	URunSession& Session,
	UCharacterDefinition* Character,
	EWacomMapNodeType EntryNodeType = EWacomMapNodeType::Navigation);

/** 不依赖 Content 资产的 transient Journey/Floor/Session fixture。 */
class WACOMTESTS_API FWacomRunExplorationFixture
{
public:
	UCharacterDefinition* MakeCharacter(FName CharacterId = TEXT("Test.Character"));

	UWacomFloorMapDefinition* MakeLinearFloor(
		FName FloorId = TEXT("Test.Floor.1"),
		int32 NodeCount = 3);

	UWacomJourneyDefinition* MakeJourney(
		const TArray<UWacomFloorMapDefinition*>& Floors,
		FName JourneyId = TEXT("Test.Journey"));

	FWacomInitializedRunExplorationSession CreateInitializedSession(
		UCharacterDefinition* Character = nullptr,
		UWacomJourneyDefinition* Journey = nullptr);

private:
	template <typename TObjectType>
	TObjectType* Hold(TObjectType* Object)
	{
		Roots.Add(TStrongObjectPtr<UObject>(Object));
		return Object;
	}

	TArray<TStrongObjectPtr<UObject>> Roots;
};
