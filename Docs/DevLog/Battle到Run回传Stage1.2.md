# Stage 1.2 - Battle → Run 回传契约 + 同归于尽规则

## 目标

把 GDD §9.2 的"战内 → 战外回传表"落到代码：

- 定义 `FBattleResultPacket` 数据结构作为 Battle/Run 模块边界的契约
- `BattleSession::BuildResultPacket()` 在战斗结束时打包数据
- `URunSession::OnBattleFinished` 签名重写为接受 packet
- GameMode 串起 Session 释放前 build packet → 传给 Run

同时落实 GDD §9.2 的"同归于尽"规则：玩家与敌方同时倒下 = Victory + 战外 +10% 伤口 + 不触发战外失败。

## 新数据契约

### `FBattleResultPacket`（新文件 `WacomBattle/Public/Session/BattleResultPacket.h`）

```cpp
USTRUCT(BlueprintType)
struct WACOMBATTLE_API FBattleResultPacket
{
    EBattleOutcome Outcome = EBattleOutcome::Undetermined;
    bool bCrossedHighHpThreshold = false;
    bool bCrossedLowHpThreshold = false;
    bool bMutualDestruction = false;
};
```

不引入 `FKnockdownExpGain / FKnockdownChoice` —— 击倒事件相关字段 Stage 7 接入时再加，避免空占位 struct。

## 行为变更

### `FBattleRules::CheckAndApplyBattleEnd`

```
敌方全死 + 玩家 HP=0 → Victory + bMutualDestruction=true（同归于尽）
敌方全死 + 玩家 HP>0 → Victory
敌方未全死 + 玩家 HP=0 → Defeat
```

实现单一 if 分支变更，逻辑清晰。

### `UBattleSession::BuildResultPacket() const`

字段 1:1 拷贝 BattleState → Packet。仅在 Phase=BattleEnd 时调用语义有效。

### `URunSession::OnBattleFinished` 签名重写

```cpp
void OnBattleFinished(const FBattleResultPacket& Packet, UEnemyDefinition* EnemyDef);
```

处理：
- Outcome 主分支：Victory 加击败列表 / Defeat 终止 / Undetermined 跳过全部结算
- 疲劳 +1%（任何非 Undetermined）
- bCrossedHighHpThreshold → 伤口 +1%
- bCrossedLowHpThreshold → 伤口 +5%
- bMutualDestruction → 伤口 +10%（不改 bRunActive）

### `AWacomGameMode::ExitBattle`

调整字段清空顺序：
- 反订阅 HUD 委托 → BuildResultPacket → Pop HUD + 清 Session → 后续清理用 Packet.Outcome 判断

## BattleState 新字段

```cpp
bool bCrossedHighHpThreshold = false;  // Stage 6 接入维护
bool bCrossedLowHpThreshold = false;   // Stage 6 接入维护
bool bMutualDestruction = false;       // Stage 1.2 已接入
```

`bCrossedHighHpThreshold` / `bCrossedLowHpThreshold` 在 Stage 1.2 永远是 false。
flag 维护逻辑（玩家受伤路径检查阈值跨越）等 Stage 6 接入。
现在的状态：契约通了，等触发逻辑填进来。

## 测试

新加 `Wacom.Run.Result.*`（6 个）：

- `FatigueOnEveryBattle`：胜负都加疲劳 +1
- `HighHpThresholdAddsWound1`：flag → 伤口 +1
- `LowHpThresholdAddsWound5`：flag → 伤口 +5
- `MutualDestructionAddsWound10`：flag → 伤口 +10 + bRunActive 仍 true
- `AllFlagsAccumulate`：累加正确（伤口 16 = 1+5+10）
- `UndeterminedSkipsAccumulation`：Undetermined 不结算压力

战内同归于尽集成测试**未加**：
- `BattleRules.h` 在 `WacomBattle/Private/`，外部测试不可见
- 通过 fixture 构造低 HP 玩家 + 同归于尽场景需要新加 fixture API + 配套敌人意图，本轮 scope 之外
- `CheckAndApplyBattleEnd` 修改极小（一个 if 分支），人工 review 充分
- Stage 6 在玩家受伤路径接入 flag 维护时一并加战内集成测试

## 同归于尽规则文档

更新 `Docs/Game_Design.md` §6 部位击倒后追加"同归于尽"段落，明确：
- Outcome 仍判 Victory
- 战内 BattleState.bMutualDestruction = true
- 战外 +10% 伤口
- 不触发战外失败（玩家 Run 继续）

## 验证

- 编译：`Build.bat WacomEditor Win64 Development` PASS
- 自动化测试：`Automation RunTests Wacom` PASS（44/44 成功，38 旧 + 6 新）

## 文件改动

新增：
- `Source/WacomBattle/Public/Session/BattleResultPacket.h`
- `Source/WacomTests/Private/Run/BattleResultSpec.cpp`

修改：
- `Source/WacomBattle/Private/Core/BattleState.h`：加 3 个 flag 字段
- `Source/WacomBattle/Private/Core/BattleRules.cpp`：CheckAndApplyBattleEnd 同归于尽分支
- `Source/WacomBattle/Public/Session/BattleSession.h`：加 BuildResultPacket 声明 + include packet 头
- `Source/WacomBattle/Private/Session/BattleSession.cpp`：BuildResultPacket 实现
- `Source/WacomRun/Public/RunSession.h`：OnBattleFinished 签名重写 + include packet 头
- `Source/WacomRun/Private/RunSession.cpp`：OnBattleFinished 实现重写
- `Source/WacomApp/Private/GameFramework/WacomGameMode.cpp`：ExitBattle 串接 packet
- `Docs/Game_Design.md`：§6 部位击倒下追加"同归于尽"段
- `Docs/WacomBattle.md`：加 §12 战斗结束 / 同归于尽；BattleState 结构图加 3 flag
- `Docs/WacomRun.md`：OnBattleFinished 描述更新

## 已就位 / 等接入

- 数据通道（Battle → Run）✅
- 同归于尽 ✅
- 疲劳 +1% ✅
- 伤口阈值字段就位、Run 层结算就位 ✅；触发逻辑 ⏸ 等 Stage 6
- 击倒事件相关字段 ⏸ 等 Stage 7
