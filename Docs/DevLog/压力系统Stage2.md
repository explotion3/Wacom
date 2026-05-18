# Stage 2 - 压力系统 trigger 完整接入

## 目标

把 GDD §3.2 八种压力的触发条件全部接入。Stage 1.1 已经把字段、API、Clamp 做好；
本 Stage 把所有 trigger 串起来，让玩家在游戏世界里的每一个动作都按规则改变压力值。

## 8 种压力 trigger 接入清单

按"触发源"分组：

### A. 时段定时（4 种触发，接入到 `AdvanceToNextPhase`）

| 触发条件 | 压力 | 量 |
|---|---|---|
| 进入 Morning | 饥饿 | +5% |
| 进入 Morning（PrevPhase=Sunrise，即跨日完成一天）| 腐朽 | +5% |
| 进入 Dusk | 饥饿 | +5% |
| 进入 Sunrise | 疲劳 | +10% |

实现：`AdvanceToNextPhase` 末尾调用 `OnPhaseEntered(NewPhase, PrevPhase)`。

注意：Initialize 不触发副作用（建档不算"进入清晨"）。

### B. 战内 → 战外（已就位 / 待 Stage 6）

| 触发 | 状态 |
|---|---|
| 每场战斗后疲劳 +1% | ✅ Stage 1.2 |
| 同归于尽伤口 +10% | ✅ Stage 1.2 |
| 战内首次跨 HighHpThreshold → +1% 伤口 | flag 通道 ✅，触发逻辑 ⏸ Stage 6 |
| 战内首次跨 LowHpThreshold → +5% 伤口 | flag 通道 ✅，触发逻辑 ⏸ Stage 6 |

本 Stage 不动战内逻辑。

### C. 战外行为（4 种新公开 API，等其他 stage 调用）

| 触发条件 | 压力 | 量 | 调用方 |
|---|---|---|---|
| `OnRightHandDestructiveAction()` | 伤口 | +1% | Stage 9 节点事件 |
| `OnCompanionCardPermanentlyDestroyed()` | 嗜血 | +1% | Stage 4 背包 UI |
| `OnTheftCommitted()` | 劣迹 | 第 n 次 +(n*(n+1)/2 +1)% | Stage 9 节点事件 |
| `RemoveFinger()` 内置副作用 → 残疾 +5%/指 | 残疾 | +5% | ✅ Stage 1.1 |

劣迹用**b 增量语义**：
- n=1 → +2%（total 2）
- n=2 → +4%（total 6）
- n=3 → +7%（total 13）
- n=4 → +11%（total 24）

`OnTheftCommitted` 内部 `++TheftCount` 后用公式累加。`TheftCount` 加进 FRunState。

### D. 反向重算（1 种 API）

`RecomputeBurden()`：每次背包增减后调用。

公式：超出 BackpackCapacity 的卡数 n → Burden = n*(n+1)/2（覆盖语义，不是累加）。

由 Stage 4 背包业务接入调用。

## 减少 / 归零

新增 API：

```cpp
SetPressure(Type, Value)             // 覆盖
RemovePressure(Type, Amount > 0)     // AddPressure 的负向别名
ClearPressure(Type)                   // 单条归零
```

各压力的"具体减少条件"待定（GDD §14 待确认问题 9）。

## 测试

新加 `Wacom.Run.Pressure.*`（12 个）：

时段定时：
- `MorningAtInitDoesNotTrigger`
- `EnterDuskAddsHunger`
- `EnterSunriseAddsFatigue`
- `CompletingDayAddsDecayAndHunger`（完整循环到次日 Morning）

战外行为：
- `RightHandActionAddsWound`
- `CompanionDestroyedAddsBloodlust`
- `TheftAccumulatesByFormula`（n=1..4 验证 2/6/13/24）

负重：
- `BurdenZeroWhenWithinCapacity`
- `BurdenSetByOverCount`（覆盖语义验证）

减少：
- `RemovePressureClampsAtZero`
- `ClearPressureSetsZero`
- `SetPressureClamps`

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（56/56 成功，44 旧 + 12 新）

## 文件改动

新增：
- `Source/WacomTests/Private/Run/PressureSpec.cpp`

修改：
- `Source/WacomRun/Public/RunSession.h`：加 SetPressure/RemovePressure/ClearPressure/4 个战外触发器/RecomputeBurden + private OnPhaseEntered
- `Source/WacomRun/Private/RunSession.cpp`：实现上述方法 + AdvanceToNextPhase 末尾调 OnPhaseEntered
- `Source/WacomRun/Public/RunState.h`：加 TheftCount 字段
- `Docs/WacomRun.md`：FRunState 字段表加 TheftCount；URunSession API 表加压力相关方法

## 不做什么

- 战内伤口阈值 flag 维护（Stage 6）
- 节点事件 UI（Stage 9）
- 背包 UI（Stage 4）
- 露营特殊推进 / 跳过 Sunrise 时的腐朽处理（Stage 8）
- 各压力的具体减少条件（GDD §14 待确认）
- 状态效果显示层（卡面变化 / 咳嗽 / 移速）

## 下一步

Stage 4 备战卡组 + 背包 UI（Stage 6 战内伤口接入需要先有玩家受伤路径具体可视，Stage 3 经验也比较独立——按用户优先级排）。
