# Stage 3 - 经验值机制：部位破坏 → 战外发放

## 目标

打通 GDD §3.3 经验值链路：战内击倒部位时记账 → 战斗结束打包 → Run 层结算到 `ExperienceCurrent`。
满 Capacity 时入账技能（接入到 Stage 1.1 已建好的 `TryConsumeExperienceForSkills`）。

## 数据契约扩展

### `UEnemyPartDefinition` 加字段

```cpp
UPROPERTY(EditDefaultsOnly) int32 ExperienceReward = 0;
```

蛇默认配置（GDD §3.3）：
- Head = 3 经验
- Body = 2 经验
- Tail = 2 经验

### `FKnockdownExpGain`（新 USTRUCT，加入 BattleResultPacket.h）

```cpp
USTRUCT(BlueprintType)
struct FKnockdownExpGain
{
    FName PartId;
    int32 ExpAmount = 0;
};
```

### `FBattleResultPacket` 加字段

```cpp
TArray<FKnockdownExpGain> KnockdownExpGains;
```

### `BattleState` 加字段

```cpp
TArray<FKnockdownExpGain> PendingKnockdownExpGains;
```

## 行为变更

### 战内：记账（先记不入账）

`bDestroyed false → true` 边沿处都加一条记账：

- `EffectHandlers.cpp` 伤害命中路径（玩家打死部位）
- `PoisonResolver.cpp` 中毒结算路径

每个部位只记一次（边沿条件保证）。
未填 `ExperienceReward` 的部位仍记一条 ExpAmount=0，让 Run 层有完整破坏列表。

### 战内：打包

`UBattleSession::BuildResultPacket()` 把 `PendingKnockdownExpGains` 直接拷给 packet。

### 战外：结算

`URunSession::OnBattleFinished` 末尾：

```cpp
if (Packet.Outcome == EBattleOutcome::Victory)  // 含同归于尽
{
    int32 TotalExp = 0;
    for (const FKnockdownExpGain& Gain : Packet.KnockdownExpGains)
    {
        TotalExp += Gain.ExpAmount;
    }
    if (TotalExp > 0) AddExperience(TotalExp);
}
```

- Victory（含同归于尽）正常结算
- Defeat 不结算（Run 已终止，发了无意义）
- Undetermined 在 OnBattleFinished 早期 return，不到这里

满 Capacity 入账由 Stage 1.1 已实现的 `AddExperience → TryConsumeExperienceForSkills` 链处理。

## 测试

新加 `Wacom.Run.Experience.*`（6 个）：

数据契约层：
- `VictoryGrantsExp`：累加多条 KnockdownExpGains
- `DefeatDoesNotGrant`：Defeat 不结算
- `MutualDestructionGrantsExp`：同归于尽正常结算 + bRunActive 仍 true
- `FullGrantsSkillFromBattle`：2.3 倍 Cap 的经验 → 2 技能 + 余 3
- `ZeroRewardRecordsButGrantsZero`：ExpAmount=0 记账但不增加经验

战内集成：
- `PartDestroyedRecordedInPacket`：构造 1 HP 部位 + 高伤害卡 → 一击破坏 → BuildResultPacket 包含 1 条 KnockdownExpGains，ExpAmount = 配置值

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（62/62 成功，56 旧 + 6 新）
- 资产重新生成：`-run=WacomRegenerateContent` PASS

## 文件改动

新增：
- `Source/WacomTests/Private/Run/ExperienceSpec.cpp`

修改：
- `Source/WacomData/Public/Enemies/EnemyPartDefinition.h`：加 ExperienceReward
- `Source/WacomBattle/Public/Session/BattleResultPacket.h`：加 FKnockdownExpGain + 包字段 KnockdownExpGains
- `Source/WacomBattle/Private/Core/BattleState.h`：加 PendingKnockdownExpGains
- `Source/WacomBattle/Private/Effects/EffectHandlers.cpp`：部位破坏路径加记账
- `Source/WacomBattle/Private/Status/PoisonResolver.cpp`：中毒破坏路径加记账
- `Source/WacomBattle/Private/Session/BattleSession.cpp`：BuildResultPacket 拷贝
- `Source/WacomRun/Private/RunSession.cpp`：OnBattleFinished 加经验结算
- `Source/WacomEditor/Private/ContentBuilders/SnakeBuilder.cpp`：BuildPart 加 ExperienceReward 参数 + 蛇配置
- `Content/Wacom/Enemies/Snake/DA_Part_Snake_*.uasset`：commandlet 重新生成
- `Docs/WacomData.md`：UEnemyPartDefinition 字段表加 ExperienceReward
- `Docs/WacomBattle.md`：BattleState 图加 PendingKnockdownExpGains；战内→战外回传表加 KnockdownExpGains 行；新增"部位破坏经验记账"段
- `Docs/WacomRun.md`：OnBattleFinished 行为说明加经验

## 不做什么

- 击倒事件 UI 三选一（Stage 7）
- 玩家三选一对其他压力的副作用（Stage 7）
- 真技能效果（GDD §14.26-27 待确认）
- 卡牌/事件给经验（GDD §3.3 提到，等 Stage 9 节点事件 / 卡牌效果时再设计）
