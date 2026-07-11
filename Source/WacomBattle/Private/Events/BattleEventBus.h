// Copyright Wacom. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Events/BattleEvent.h"

/**
 * 战斗事件总线。
 *
 * 所有 Resolver 通过本总线 Emit 事件。每次初始化或命令事务通过原子结果
 * 整批取走事件，Session 本身不保存等待上层消费的输出队列。
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

private:
	TArray<FBattleEvent> Pending;
	int32 NextSequence = 0;
};
