---
type: interaction-contract
scope: wacom-world-interaction
status: active
updated: 2026-06-05
tags:
  - wacom/app
  - wacom/world-interaction
  - wacom/targeting
  - wacom/authoring
---

# Wacom World Interaction 文档

> [!info] 本文职责
> 本文记录 WacomApp 世界交互和 target system 的当前事实：Run world interactable Actor、Battle scene target、Run menu drop target、target handle、debug / authoring / validation 约定。

> [!warning] 边界
> App 只负责命中、路由、提示和表现反馈。Battle / Run 规则真相仍在 `UBattleSession` 与 `URunSession`；静态 Definition 字段见 [WacomData.md](./WacomData.md)，内容校验见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)；UI 总入口见 [WacomUI.md](./WacomUI.md)，Battle UI 和 first-person hand 细节见 [WacomBattleUI.md](./WacomBattleUI.md) 与 [First_Person_Card_Layer_Design.md](./First_Person_Card_Layer_Design.md)。

## §1 总原则

世界交互入口分为 E 键、鼠标 hover、鼠标点击、first-person card drop 和 Run menu zone drop。所有入口最终都回到明确的领域出口：

- 普通探索交互：`IWacomWorldInteractable::TryInteract()`。
- Battle target click / drag：`UBattleHUD` 将玩家意图转为 Battle command 或 target selection。
- Run world card drop：`UWacomRunWorldCardDropReceiverComponent -> URunSession::Validate/SubmitRunWorldCardInteraction()`。
- Run menu zone drop：owning menu 解析和提交；RunEvent 支付进入 `URunSession::ChooseRunEventOptionWithPaidCardResult()`。

Actor 和 Widget 可以提供 target handle、debug view、hover prompt 和 preview feedback，但不能直接修改 Battle / Run 状态真相。

## §2 Run World Interactable Actor

探索期 `E` 交互使用 `IWacomWorldInteractable`。进入范围时注册到 PlayerController，离开范围或销毁时反注册；多个候选重叠时按 `GetInteractLocation()` 选最近且 `CanInteract=true` 的对象。

正式可放置 / 兼容入口：

| Actor | 用途 | 规则出口 |
|---|---|---|
| `ABattleTriggerActor` | 场景敌人战斗入口 | `AWacomGameMode::EnterBattle()` |
| `AWacomShopTriggerActor` | 场景商店入口 | `URunSession::BeginShopVisit()` + Shop Screen flow |
| `AWacomRunEventTriggerActor` | 场景探索事件入口 | `URunSession::BeginRunEvent()` + RunEvent Screen flow |
| `AWacomRunPickupActor` | 金币 Pickup fallback / 快速入口 | `URunSession::CollectGoldPickup()` |
| `AWacomRunCardPickupActor` | 固定单卡 Pickup fallback / 快速入口 | `URunSession::CollectCardPickup()` |
| `AWacomRunRewardPickupActor` | 数据驱动 Pickup 推荐入口 | Gold / Card pickup 规则入口 |
| `AWacomRunKeyChestActor` | Run world card interaction 验证入口 | `URunSession::SubmitRunWorldCardInteraction()` |

关卡实例必须使用场景级唯一 `PersistentId` 作为运行时状态 key。静态 Definition 的 `ShopId / EventId / PickupId / InteractionId` 是内容 ID、debug ID 或资产语义，不替代场景 `PersistentId`。

### Authoring 口径

- 正式关卡推荐使用 Definition 驱动的入口：`AWacomShopTriggerActor.ShopDefinition`、`AWacomRunEventTriggerActor.EventDefinition`、`AWacomRunRewardPickupActor.PickupDefinition`、`AWacomRunKeyChestActor.CardInteractionDefinition`。
- Definition 字段语义见 [WacomData.md](./WacomData.md)；生成样例和资产 validator 口径见 [WacomDataAuthoring.md](./WacomDataAuthoring.md)。
- 旧手工字段和专用 Pickup Actor 作为 fallback / 快速验证入口保留；不要把它们扩展成新的长期制作主线。
- Blueprint 只放默认外观和可见 primitive，不写 EventGraph 规则逻辑、不直接调用 RunSession。
- Prototype sample button 只用于 PIE / 开发验证；它们只改当前 Actor 配置和 facade 同步，不修改 RunState、不生成资产。

### Data Validation 口径

摆放实例应通过 Validate Map/Level 校验：

- 缺 `PersistentId` 通常是 error。
- 同 World 内重复 `PersistentId` 是 warning，因为它会共享运行时状态。
- 缺必要 Definition、无可用 reward / offer / event、无正向卡牌筛选、无效金币数或缺卡定义等配置错误是 error。
- BP 默认资产 / CDO 允许保持空配置，避免推荐摆放 BP 被误判为 invalid。

## §3 Click / Hover Target Contract

Run world click / hover 使用显式 opt-in：

- Actor 必须同时实现 `IWacomWorldInteractable` 和 `UWacomRunWorldClickableInteractable`。
- Actor 必须通过 `UWacomInteractionTargetComponent + UWacomRunWorldInteractionTargetBridgeComponent` 暴露 `Interaction.Target.Run.Object`。
- `ClickBounds` 只阻挡 `Visibility` trace，不产生 overlap；`TriggerSphere` 仍只服务 E 键近距离候选注册。
- PlayerController resolver 会校验 handle、source actor、clickable contract、world interactable contract 和 bridge，再把左键命中转回现有 `TryInteract()`。

打开 Backpack / Pause / Shop / RunEvent 等 GameMenu 时，Run world hover/click 不穿透场景。菜单 first-person card drag/drop 正在处理时，也会清理普通 Run world hover prompt 和 probe preview。

`UWacomRunWorldClickableInteractable` 提供 hover prompt 和通用 debug view。常见 reject reason 包括缺少 world interactable contract、缺少 clickable contract、缺少 bridge、目标 tag 不匹配或当前 flow 不允许交互。

## §4 Run World Card Drop

没有 active GameMenu / menu lease 时，探索期 first-person 卡牌可以拖到 Run world target。

提交要求：

- release 命中 `Interaction.Target.Run.Object`。
- 目标 Actor 有 `UWacomRunWorldCardDropReceiverComponent`。
- receiver 提供正向筛选：`AllowedCardDefinitions / AllowedCardIds / RequiredKeywords` 至少一个非空。
- `URunSession::ValidateRunWorldCardInteraction()` 接受精确 `SourceCardInstanceId`。

成功提交由 `SubmitRunWorldCardInteraction()` 在 RunSession 事务内完成：可选消耗源卡，按 reward payload 发放 Gold / Card，写入 `CompletedRunWorldInteractionIds`，最后广播一次 Run state changed。

失败反馈属于 App 表现层：preview 阶段只更新轻量有效 / 无效反馈；release 命中过目标且提交失败时才发 AppToast。文案优先来自 receiver failure contract，没有 receiver 可询问时由 PlayerController 提供通用配置异常 fallback。

KeyChest 是当前第一条 Run world card interaction 验证入口。普通 E 键或左键点击只显示“需要钥匙 / 宝箱已打开”提示，不直接结算奖励；拖卡成功后才提交 RunSession 事务。

## §5 Battle Scene Target

Battle scene enemy 推荐入口是 `ABattleTriggerActor.SceneEnemyHost + AWacomBattleEnemyActor + AWacomBattleEnemyPartActor`：

- Trigger 进入战斗时把显式 `SceneEnemyHost` 传给 `UBattleHUD`。
- HUD 只同步当前 Host 下 attached PartActor 的 bridge；同关卡其他 Host 的部位不参与当前战斗。
- 每个 PartActor 用稳定 `PartId` 作为 authoring id，运行时由 HUD 解析并写回当前 `PartInstanceId`。
- 有 Host 时默认隐藏旧 `EnemyInfoBar`；缺 Host 时仍保留 2D enemy fallback/debug。

`UWacomBattleEnemyPartWorldTargetBridgeComponent` 接收 hover probe、TargetSelect pending card、first-person drag preview、TargetConfirmed、DamageDealt、EnemyPartHpEmptied 等表现 cue。它只更新表现 / debug，不修改 BattleSession。

`AWacomPlayerController::TryRouteBattleSceneTargetClick()` 通过 cursor trace 命中 Component，扫描 `IWacomInteractionTargetProvider` 构建 handle。只有 `TargetKind=World`、`TargetTag=Interaction.Target.Battle.EnemyPart` 且 handle 属于当前 BattleHUD Host registry 时，才转发为 Battle enemy part 点击。

## §6 Run Menu Zone Target

`UWacomRunMenuDropTargetWidget` 是 Run GameMenu 专用 UMG Zone target bridge：

- 配置 `ZoneId / StableTargetId`。
- 构建 `FWacomInteractionTargetHandle(TargetKind=Zone)`。
- 提供 probe / invalid / released-probe / submit-ready / submitted 轻量 preview。
- 不直接调用 `URunSession`，也不参与背包旧 UMG DragDrop 规则提交。

PlayerController 只在 Exploration + active GameMenu + active menu lease 的 first-person card drag 中扫描注册过的 drop target widget，按后注册优先作为最上层命中。

`ResolveRunMenuCardDropIntent()` 统一解析 preview 和 release：

- 默认是 probe-only。
- owning menu 可以声明 `SubmitZoneTarget + ControllerDestroyOwnedCard`，用于 prototype 验证。
- `MenuHandled` 由菜单提交并回填结果；RunEventScreen 使用该模式提交卡牌支付事务。

## §7 Target Handle

`FWacomInteractionTargetHandle` 位于 `WacomCore/Public/Types/WacomInteractionTargetTypes.h`，是纯数据描述层：

| 字段 | 用途 |
|---|---|
| `TargetKind` | None / World / Card / Zone |
| `WorldTargetId` | World 目标运行时 FGuid |
| `CardInstanceId` | Card 目标的精确实例 id |
| `ZoneId` | UMG Zone 目标 FName |
| `TargetTag` | 目标语义 tag，例如 `Interaction.Target.Battle.EnemyPart` |
| `StableTargetId` | 稳定 authored/data id，例如 enemy part `PartId` |
| `SourceObject` | 命中来源 Component 弱引用 |
| `WorldLocation / ScreenPosition` | 命中位置 |

Battle 已接入 `UBattleSession::CanTargetWithCard()` 和 `ValidateTargetWithCard()`，用于 TargetSelect、first-person drag/drop world target 和 hand-card target validation。

通用 Run target resolver 尚未独立抽象；当前 Run world / Run menu drop 由 App resolver 识别 target，再进入明确的 RunSession 事务或 owning menu submit policy。

## §8 Debug / Compatibility

- Debug summary / debug view / log 入口只用于 PIE 排查，不作为正式规则入口。
- Actor `ConfigureDebug...Sample` 按 `Wacom|...|Prototype` 分类保留，只用于开发验证样例配置。
- `EnemyInfoBar / EnemyPartWidget`、legacy event log 等 compatibility / fallback UI 不作为新的 world interaction 制作主线；旧 2D hand 已删除，正式手牌走 first-person card layer。
- `TryRouteBattleSceneTargetClick` 等 protected test seam 保留给自动化测试；测试侧应通过 `WacomTests/Private` access wrapper 使用，不扩散生产 public API。
