# 击倒事件 UI Stage 7

> 时间：2026-05 / 范围：GDD §6 击倒事件 + §10.5 节点重入 / 编译通过 / 自动化 103/103

## 一、做了什么

实现"敌方部位被击倒后，玩家可三选一（援助 / 破坏 / 撤离）"的完整闭环，覆盖战内 → UI → 战外的全链路。撤离作为"未完成战斗"的合法分支被钉死。

| 层 | 改动 |
|---|---|
| 数据契约 | `EBattlePhase::PendingKnockdownChoice` / `EKnockdownChoice` enum / `FKnockdownChoice` struct / `FBattleCommand::MakeKnockdownChoice` / `FBattleResultPacket` 新增 `bWithdrawn / KnockdownChoices / DestroyedPartIds` / `FBattleInitParams::PreDestroyedPartIds` / `EBattleEventType` 新增 `KnockdownChoiceRequested + KnockdownChoiceMade` |
| 战内核心 | `FBattleState::PendingKnockdownEvents/Choices/DestroyedPartIds` 三个新队列 / `FBattleState::RecordPartDestroyed` 统一部位破坏路径 / 新建 `FKnockdownChoiceResolver` 处理三选一 / `FBattleRules::CheckAndApplyBattleEnd` 改为"敌方全死且队列非空时不立即设 BattleEnd"|
| Session | `UBattleSession::Initialize` 应用 `PreDestroyedPartIds` / `SubmitCommand` 末尾切到 `PendingKnockdownChoice` 阶段 + 发 `KnockdownChoiceRequested` 事件 / `BuildResultPacket` 拷贝新字段，bWithdrawn 由 `KnockdownChoices` 含 `Withdraw` 推断 |
| Run 域 | `FRunState::BattleProgress: TMap<FName, FBattleProgressSnapshot>` 持久化撤离时的破坏部位 / `URunSession::BuildInitParamsForBattle(EnemyDef, TriggerPersistentId, OutParams)` 重载灌 `PreDestroyedPartIds` / `OnBattleFinishedFromTrigger` 区分撤离与真胜利 |
| GameMode | `EnterBattle` 传 TriggerPersistentId / `ExitBattle` 撤离时不销毁 Trigger、不进 DefeatedEnemies；战斗结束统一 `Run->ConsumeNode(1)` |
| UI | 新建 `UWacomKnockdownChoiceDialog`（Modal 层 + 三按钮 + ESC 不响应），BattleHUD 监听 `KnockdownChoiceRequested` 事件 push dialog |
| 测试 | `KnockdownChoiceSpec.cpp` 6 个 case 覆盖三种选项、可选性校验、队列重入；`PartDestroyedSpec` / `ExperienceSpec` 修复（部位破坏后插入 Aid 命令继续战斗）|
| 文档 | GDD §10 / §10.5 节点消耗时机改写 / WacomBattle.md §战内→战外回传表更新 / WacomRun.md `OnBattleFinishedFromTrigger` 段更新 |

## 二、关键决策（与主导钉死的）

### Q1 撤离 ≠ 完成战斗
- **撤离 Outcome=Victory，但敌人不进 DefeatedEnemies、Trigger 不销毁**——战斗节点不变"已完成"，玩家未来仍可触发同一场战斗
- 已破坏的部位按 `TriggerPersistentId` 索引存 `RunState.BattleProgress`，下次进入同一节点时维持破坏态
- `bWithdrawn` 不直接存为 BattleState 字段，而由 `KnockdownChoices` 含 `Withdraw` 在 `BuildResultPacket` 时推断——避免双信源不同步

### Q2 援助 / 破坏不消耗任何战内资源
- 不消耗手牌、不改战内状态、仅记账。具体效果留给 Stage 9 节点事件接入时按 `Choice` 分支触发
- 当前阶段 Run 层第一阶段仅记日志

### Q3 节点消耗时机
- GDD §10.5 改为"战斗结束时统一消耗 1 节点"，不论胜利 / 失败 / 撤离
- 旧版"触发即消耗 + 失败再退还"被废弃——失败本就把 Run 终止了，退还无意义
- 实现位置：`AWacomGameMode::ExitBattle` 末尾 `Run->ConsumeNode(1)`，仅在 `Outcome != Undetermined` 时调用

### Q4 经验只在 false→true 边沿结算
- 部位破坏只在 `bDestroyed` 从 false 变 true 的瞬间记一条 `FKnockdownExpGain`
- 撤离时持久化破坏态后，下次进入同一战斗、同一部位以已破坏态载入 → 不会再发 `EnemyPartHpEmptied` 事件 → 不会再记账
- 避免"反复撤离刷经验"的漏洞

### Q5 战斗结束判定的时序
- 关键修法：`FBattleRules::CheckAndApplyBattleEnd` 检测"敌方全死 + 玩家未死 + `PendingKnockdownEvents` 非空"时**不立即**设 BattleEnd，让玩家先把队列里的击倒选项处理完
- `FKnockdownChoiceResolver` 每处理一条选项后，若队列已空再调一次 `CheckAndApplyBattleEnd` 触发真正的 Victory
- 撤离分支直接绕过队列，立即设 BattleEnd

## 三、踩过的坑

### A. 撤离 BattleEnded 事件的 Count 字段
`KnockdownChoiceResolver` 中撤离分支发的 `BattleEnded` 事件 `Count = 1` 是按现有事件惯例填的"胜利标记"，**packet 区分撤离 / 真胜利**靠 `bWithdrawn` 字段，而不是这个事件 Count。BattleHUD 上层拿到 packet 才能区分两种 Victory。

### B. 部位破坏路径的统一
原来 EffectHandlers 和 PoisonResolver 各自有"部位 HP 归零 → set bDestroyed → push KnockdownExpGain"的内联代码。Stage 7 加了 `PendingKnockdownEvents` / `DestroyedPartIds` 两个队列后，三处同步维护风险高。统一抽到 `FBattleState::RecordPartDestroyed`，所有写部位破坏的路径都走它。一次到位避免后续遗漏。

### C. 测试 Spec 的"部位破坏后战斗卡住"
`PartDestroyedSpec` / `ExperienceSpec` 部分 case 在敌方全死后等不到 BattleEnd——根因就是 Q5 的时序变化：以前 CheckAndApplyBattleEnd 立即设 BattleEnd，现在要等队列处理完。修法是 case 末尾插入 `Aid` 命令清队列，让 `CheckAndApplyBattleEnd` 二次触发。

### D. `PreDestroyedPartIds` 的应用时机
必须在 BattleSession::Initialize 阶段、敌人 Parts 初始化完之后立即应用——直接对 `RuntimeEnemyPart::bDestroyed` 置位 + 把 HP 设为 0。不要走 `RecordPartDestroyed`（会记 ExpGain，违反 Q4）。

## 四、未做 / 留给后续

- **Stage 9 节点事件**：援助 / 破坏分支的实际效果落地（左手卡获得 buff / 永久强化某部位 / 触发特殊节点）
- **dialog 美术资源**：当前用 C++ 硬编码 CanvasPanel + Border + Button 布局，等美术给 WBP 后切（命名见 `WacomKnockdownChoiceDialog.h::PartNameText/AidButton/WithdrawButton/DestroyButton`，配 BindWidget 即可）
- **PartNameText 显示部位中文名**：当前从 `UEnemyPartDefinition::PartId` 取 FName。等 Definition 加 `DisplayName: FText` 字段后切到本地化文本
- **回顾**：Q4 的"边沿结算"模式可以推广到其他"反复进入同一节点可能刷资源"的场景（如商店购买 / 露营回 HP 等），未来 Stage 9 落地时套用同样的 `BattleProgress` 索引模式

## 五、验证

- **编译**：`Build.bat WacomEditor Win64 Development` → Result: Succeeded
- **自动化**：`Automation RunTests Wacom` → 105 OK / 0 FAIL（含新增 `KnockdownChoiceSpec` 7 case）
- **PIE 验证（手动）**：进战斗 → 击倒部位 → 看到 dialog → 测三种分支
  - Aid / Destroy：dialog 关闭，战斗继续
  - Withdraw：战斗立即结束，回到探索；返回原节点 Trigger 仍在；再次触发战斗时已破坏部位维持破坏态
  - 真胜利：Trigger 销毁，敌人进 DefeatedEnemies

## 七、Stage 7 收尾期间发现并修复的回归

落地后 PIE 验证又发现四个隐藏 bug，每个都按"找根因 + 选规范修法"路线走。

### 7.1 多部位同时破坏只弹一次 dialog（事件路径不闭合）
**根因**：一张牌一次破坏 N 个部位时 `PendingKnockdownEvents` 入队 N 条，但 `KnockdownChoiceRequested` 事件只在 `BattleSession::SubmitCommand` 末尾"首次入队"时发一次（判定是 `Phase != PendingKnockdownChoice`）；`KnockdownChoiceResolver` 处理完后不发新事件。后续 dialog 不出，phase 锁死。
**规范修法**：让 `KnockdownChoiceResolver::Resolve` 在 Aid/Destroy 处理完且队列仍非空时主动 `Emit(KnockdownChoiceRequested)`，与首次入队的发射点对称。
**回归**：`MultiPartDestroyedTriggersSequentialRequests` case（AoE 一次打死 3 部位）。

### 7.2 dialog 选完战斗不结束（命令收尾路径绕过 BattleHUD）
**根因**：`WacomKnockdownChoiceDialog::HandleAidClicked/...` 直接调 `Session.SubmitCommand` 然后 `DeactivateWidget`，**绕过了 `BattleHUD::AfterCommand`**。后果是 `ConsumeAndLogEvents`（push 下一个 dialog）和 `RefreshFromSnapshot`（广播 `OnBattleEndedNative`）都不触发，GameMode 永远收不到结束通知。
**规范修法**：恢复"BattleHUD 是命令提交的唯一入口"约定，新加 `UBattleHUD::OnKnockdownChoiceSelected(EKnockdownChoice)`，与 `OnWaitRequested / OnEndTurnRequested` 对称。Dialog 调它，HUD 内部统一走 `SubmitCommand → AfterCommand`。
**为什么不补 `ConsumeAndLogEvents` 调用兜底**：那是判空式修法，与 BattleHUD 命令入口架构方向相反；回路缺失只会越来越多，必须在 dialog 处闭环。

### 7.3 撤离后无法重入战斗（overlap 自动触发模型死循环）
**根因**：旧 `BattleTriggerActor` 用 BeginOverlap 自动触发，撤离回探索时玩家仍在 Sphere 内，永远不会有 EndOverlap → BeginOverlap 的循环，无法重入战斗。
**规范修法**：从 overlap 自动触发改为 use-key 交互模型（`ue5-world-interaction` skill 推荐路径）：
- Sphere Begin/EndOverlap 改成"加/减 PlayerController.CandidateTriggers"
- IA_Interact（E 键）→ `OnInteractPressed` → `PickClosestCandidate` → `Trigger.TryActivate(PC)` → `RequestEnterBattle`
- ExplorationHUD 加"按 E 战斗" Toast，由 PlayerController 在候选列表 / GameFlowState 变化时刷新
- GameMode `EnterBattle` / `ExitBattle` 末尾调 `WPC->RefreshInteractToast()`
**为什么不在撤离时把 Trigger Disable + 走出再 Enable**：那是状态修补；use-key 模型用"在范围内"作为前置 + 按键作为触发点，重入天然支持，是 RPG 节点的标准做法。

### 7.4 用 anchor 卡打死部位时对应分支误可选（流程时序错位）
**根因**：`WacomBattle.md §3` 第 6 步执行卡牌效果即破坏部位，第 9 步才"卡牌离开手牌"。`RecordPartDestroyed` 在第 6 步入队 `PendingKnockdownEvents`，此时正在打出的右手 anchor 仍在 Hand 数组里 → `Cards.Hand.Contains(RightHandInstanceId)` 返回 true → Destroy 误判可用。
**规范修法**：给 `RecordPartDestroyed` 加可选参数 `InflictedByCardId`，可用性判定时显式排除。`EffectHandlers` 在 `Ctx.SourceKind == Card` 时传 `Ctx.SourceInstanceId`，`PoisonResolver` 不传（中毒结算无"导致破坏的卡"概念）。
**为什么不在第 9 步之后才入队**：会破坏"所有破坏路径共用 `RecordPartDestroyed`"的统一口径（中毒/敌方行动等路径没有"卡牌离开手牌"步骤），后续维护更脆弱。
**回归**：`AnchorAsKillerExcludedFromChoice` case。

## 八、文件清单（含 7.x 修复）

**新建**
- `Source/WacomBattle/Private/Commands/KnockdownChoiceResolver.h/.cpp`
- `Source/WacomBattle/Private/Core/BattleState.cpp`（RecordPartDestroyed 实现）
- `Source/WacomApp/Public/UI/Battle/WacomKnockdownChoiceDialog.h`
- `Source/WacomApp/Private/UI/Battle/WacomKnockdownChoiceDialog.cpp`
- `Source/WacomTests/Private/Battle/KnockdownChoiceSpec.cpp`

**改**
- `Source/WacomCore/Public/Types/WacomEnums.h`（PendingKnockdownChoice + EKnockdownChoice）
- `Source/WacomBattle/Public/Session/BattleResultPacket.h / BattleSession.h / Commands/BattleCommand.h / Events/BattleEvent.h`
- `Source/WacomBattle/Private/Core/BattleState.h / BattleResolver.cpp / BattleRules.cpp / Session/BattleSession.cpp / Effects/EffectHandlers.cpp / Status/PoisonResolver.cpp`
- `Source/WacomRun/Public/RunState.h / RunSession.h`、`Private/RunSession.cpp`
- `Source/WacomApp/Public/UI/Battle/BattleHUD.h`（OnKnockdownChoiceSelected）
- `Source/WacomApp/Public/UI/Foundation/WacomExplorationHUD.h`（SetInteractToastVisible）
- `Source/WacomApp/Public/Actors/BattleTriggerActor.h`（use-key 模型）
- `Source/WacomApp/Public/GameFramework/WacomPlayerController.h`（CandidateTriggers + IA_Interact + RefreshInteractToast）
- `Source/WacomApp/Private/GameFramework/WacomGameMode.cpp / WacomPlayerController.cpp / Actors/BattleTriggerActor.cpp / UI/Battle/BattleHUD.cpp / UI/Battle/WacomKnockdownChoiceDialog.cpp / UI/Battle/DebugBattleHUD.cpp / UI/Foundation/WacomExplorationHUD.cpp`
- `Source/WacomTests/Private/Battle/PartDestroyedSpec.cpp / Run/ExperienceSpec.cpp`
- `Docs/Game_Design.md`（§10 / §10.5）/ `Docs/WacomBattle.md` / `Docs/WacomRun.md` / `Docs/WacomApp.md`（§5 BattleTriggerActor use-key 模型）
