// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Types/WacomEnums.h"

struct FBattleState;
struct FRuntimeCardInstance;

/** 一次真实卡牌定位变化的稳定事实。 */
struct FCardZoneTransitionFact
{
	FGuid CardInstanceId;
	ECardLocation From = ECardLocation::Unknown;
	ECardLocation To = ECardLocation::Unknown;
	int32 FromIndex = INDEX_NONE;
	int32 ToIndex = INDEX_NONE;

	bool IsCrossZoneMove() const { return From != To; }
};

/**
 * 战斗内卡牌实例和定位容器的唯一写入 Module。
 *
 * Interface 只接受注册、跨区移动和同区重排意图。Implementation 原子维护
 * AllCards/CardIndexById、六个定位容器和 RuntimeCard.Location。规则层的事件、
 * 被动、抽牌数量、HandZone 与目标选择不下沉到本 Module。
 */
class FCardZoneAggregate final
{
public:
	/** 注册一个新运行时实例，并按 InitialLocation 放入唯一容器。Unknown 不入容器。 */
	static bool RegisterCard(
		FBattleState& State,
		FRuntimeCardInstance Card,
		ECardLocation InitialLocation,
		FCardZoneTransitionFact* OutFact = nullptr);

	/** 从实例当前权威位置移动到目标容器；TargetIndex=INDEX_NONE 表示追加。 */
	static bool MoveCard(
		FBattleState& State,
		const FGuid& CardInstanceId,
		ECardLocation TargetLocation,
		int32 TargetIndex = INDEX_NONE,
		FCardZoneTransitionFact* OutFact = nullptr);

	/** 只有当前位置与 ExpectedSource 一致时才移动。 */
	static bool MoveCardFrom(
		FBattleState& State,
		const FGuid& CardInstanceId,
		ECardLocation ExpectedSource,
		ECardLocation TargetLocation,
		int32 TargetIndex = INDEX_NONE,
		FCardZoneTransitionFact* OutFact = nullptr);

	/** 原子预检后，把 SourceLocation 中的全部卡按稳定顺序移动到目标容器。 */
	static bool MoveAllCards(
		FBattleState& State,
		ECardLocation SourceLocation,
		ECardLocation TargetLocation,
		TArray<FCardZoneTransitionFact>* OutFacts = nullptr);

	/** 仅重排同一容器；OrderedCardIds 必须与当前 membership 完全相同且无重复。 */
	static bool SetZoneOrder(
		FBattleState& State,
		ECardLocation Location,
		TConstArrayView<FGuid> OrderedCardIds);

	/** Fisher-Yates 重排指定容器，不改变 Location 或 membership。 */
	static bool ShuffleZone(
		FBattleState& State,
		ECardLocation Location,
		FRandomStream& Rng);

	/**
	 * 验证索引、重复 membership、容器与 Runtime Location 一致性。
	 * bAllowUnknown 仅供 Initialize 尚未把左右手锚点放入 Hand 的瞬时阶段。
	 */
	static bool ValidateInvariants(
		const FBattleState& State,
		FString* OutError = nullptr,
		bool bAllowUnknown = false);

private:
	static TArray<FGuid>* FindMutableZone(FBattleState& State, ECardLocation Location);
	static const TArray<FGuid>* FindZone(const FBattleState& State, ECardLocation Location);
};
