# Stage 1.1 - 战外状态容器

## 目标

把 `FRunState` 从"R5/S1 战斗联通骨架"扩展为 GDD §3 / §8 / §11 描述的全部战外字段容器。
为 Stage 1.2（Battle→Run 回传契约）和后续业务（压力 / 经验 / 时段 / 背包 UI / 地图）准备数据基础。

只做**数据形状 + 访问方法**，不做行为逻辑（除少数有副作用的，如 RemoveFinger 同步加残疾）。

## 新增类型

### `RunStateTypes.h / .cpp`（新文件）

- `EWacomPressureType` 枚举：8 种压力（Hunger / Wound / Fatigue / Burden / Decay / Misdeed / Bloodlust / Disability）+ Count
- `ETimePhase` 枚举：5 时段（Morning / Day / Dusk / Night / Sunrise）+ Count
- `FPressureValues` USTRUCT：8 个独立 int32 字段 + Get / Set / Add / GetTotal 接口

为什么 8 个 int32 而不是 array / map：每条压力名字稳定（GDD 已定数）、debug 友好、字段拆开比 array 索引更难错。

### `SkillSlot.Placeholder` GameplayTag

新增到 `WacomCore/Public/Tags/WacomGameplayTags.h`：
- `SkillSlot_Placeholder` = `"SkillSlot.Placeholder"`

第一阶段技能内容未定（GDD §3.3 / §14.26-27），用占位 tag 累计已获得技能数。

## FRunState 新增字段

按 GDD 章节分组：

### §3.1 / §3.4：本体 HP

- `FingerCount = 10`（Initialize 时从 Character 读取）
- `HpPerFinger = 2`（每指 HP）

### §3.2：压力

- `Pressure: FPressureValues`（8 条）
- `HighHpThreshold = 0.5f`（战内伤口阈值 1）
- `LowHpThreshold = 0.8f`（战内伤口阈值 2）

### §3.3：经验值与技能

- `ExperienceCurrent = 0`
- `ExperienceCapacity = 10`
- `AcquiredSkills: TArray<FGameplayTag>`（用 array 而非 Container，重复入账才能涨数）

### §8：时间与昼夜

- `CurrentDayNumber = 1`
- `CurrentTimePhase = Morning`
- `RemainingNodeCount = 2`（清晨默认）
- `InitialNodeCount_Morning/Day/Dusk/Night/Sunrise`（数值常量化）

### §11：背包与备战卡组

- `Backpack: TArray<TObjectPtr<UCardDefinition>>`
- `BackpackCapacity = 12`
- `BattleDeck: TArray<TObjectPtr<UCardDefinition>>`

第一阶段：Initialize 时把 `Character->StarterDeck` 全量复制到 Backpack 和 BattleDeck，两者等值。
等"备战卡组与背包分离"业务做时，BattleDeck 由玩家在背包界面挑选。

## URunSession 新增 API

| 方法 | 职责 |
|---|---|
| `GetFingerCount` / `IsFingerDepleted` / `RemoveFinger(Count)` | 手指。RemoveFinger 同步加残疾（每指 +5%）|
| `GetPressureValue` / `GetTotalPressure` / `AddPressure(Type, Delta)` / `IsPressureCapReached` | 压力（clamp [0, 100]）|
| `IsRunFailed()` | 综合判定：bRunActive=false OR 压力满 OR 手指=0 |
| `GetExperienceCurrent / Capacity` / `GetAcquiredSkillCount` / `AddExperience(Amount)` | 经验。满 Capacity 自动入账（可多次溢出）|
| `GetCurrentTimePhase / RemainingNodeCount / CurrentDayNumber` / `ConsumeNode(Count)` / `AdvanceToNextPhase()` | 时段 / 节点 |
| `GetBackpack / BattleDeck / BackpackCapacity` | 卡组只读 |

## 行为约定

- `AddPressure` 不主动改 `bRunActive`；调用 `IsRunFailed()` 综合判定
- `RemoveFinger` 不主动改 `bRunActive`；同上
- `ConsumeNode` 在剩余 = 0 时自动调 `AdvanceToNextPhase`
- `AdvanceToNextPhase` 五时段循环 Morning → Day → Dusk → Night → Sunrise → Morning（次日 +1）
- 露营特殊推进（Night 直接跳次日 Morning）留到 Stage 8 节点事件接入时再加
- `BuildInitParamsForBattle` 当前继续读 `Character->StarterDeck`（间接通过 RunState.Character），等"备战卡组分离"业务做时改读 `RunState.BattleDeck`

## 自动化测试（新加）

`Wacom.Run.State.*` 共 8 个：

- `InitializePopulatesFromCharacter`：Initialize 后字段从 Character 读取 + 卡组复制 + 时段重置
- `RemoveFingerAddsDisability`：失去 1/2 指 → 残疾 +5/+10/+15
- `FingerZeroFailsRun`：手指掉光 → IsRunFailed = true
- `PressureCapFailsRun`：8 条加和 = 100 → IsRunFailed = true
- `PressureClampedToZeroAndHundred`：AddPressure 上下限 clamp
- `ExperienceFullGrantsSkill`：满 Capacity 入账，溢出多次入账，余数保留
- `ConsumeNodeAdvancesPhase`：节点用完自动推进
- `PhaseCycleAdvancesDay`：5 时段循环 + 跨日 +1

## 不做什么

- `MapNodeStates`（GDD §10 地图运行时状态）：留到 Stage 8 节点事件 + UMapDefinition 一起做
- 战内 → 战外回传（Stage 1.2）
- 备战卡组与背包真分离（需要 UI）
- 露营特殊推进
- 技能内容（GDD §3.3 / §14.26-27 待确认）
- `UWacomSaveGame` 字段同步（存档 Stage 0.1 暂停）

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（38/38 成功，30 旧 + 8 新）

## 文件改动

新增：
- `Source/WacomRun/Public/RunStateTypes.h`
- `Source/WacomRun/Private/RunStateTypes.cpp`
- `Source/WacomTests/Private/Run/RunStateSpec.cpp`

修改：
- `Source/WacomRun/Public/RunState.h`：加六大块字段
- `Source/WacomRun/Public/RunSession.h`：加新 API 声明
- `Source/WacomRun/Private/RunSession.cpp`：Initialize 同步 Backpack/BattleDeck/时段；加新 API 实现
- `Source/WacomCore/Public/Tags/WacomGameplayTags.h`：加 `SkillSlot_Placeholder`
- `Source/WacomCore/Private/Tags/WacomGameplayTags.cpp`：定义 SkillSlot_Placeholder
- `Docs/WacomData.md`：UCharacterDefinition 字段表更新（已在 Stage 0.2 后第一次完整同步）；GameplayTag 表加 SkillSlot / CardLocation / Passive.Trigger 缺漏
- `Docs/WacomRun.md`：FRunState 字段清单重写；URunSession API 表分组重写
