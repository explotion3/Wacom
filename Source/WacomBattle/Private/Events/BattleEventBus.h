// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

/**
 * 战斗事件总线。
 *
 * 所有 Resolver 通过本总线 Emit 事件。事件由 UBattleSession::ConsumeEvents
 * 整批取走后清空，保证上层每批 Event 不会重复读。
 *
 * Sequence 在总线内部自增，跨整场战斗唯一。
 * 总线不是反射结构，仅 WacomBattle/Private 内部使用。
 */
struct FBattleEventBus
{
	/** 发射一个事件。函数负责填入 Sequence。 */
	void Emit(FBattleEvent Event);

	/** 取走当前所有事件并清空队列。返回值按发射顺序排列。 */
	TArray<FBattleEvent> Consume();

	/** 只读查询。测试友好。 */
	int32 Num() const { return Pending.Num(); }
	int32 GetNextSequence() const { return NextSequence; }

	/** 创建只继承全局 Sequence、但没有 pending 事件的命令事务总线。 */
	FBattleEventBus BeginTransaction() const;

	/** 成功 commit 后接纳事务的下一个 Sequence，不复制事务事件。 */
	void CommitTransactionSequence(const FBattleEventBus& Transaction);

	/** 仅供旧 SubmitCommand/ConsumeEvents 兼容层保存已经分配好 Sequence 的事件。 */
	void AppendResolved(TConstArrayView<FBattleEvent> ResolvedEvents);

	/** 清空并重置序号。仅 Initialize 时使用。 */
	void Reset();

private:
	TArray<FBattleEvent> Pending;
	int32 NextSequence = 0;
};
