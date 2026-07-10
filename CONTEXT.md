# Wacom Domain Context

本文件固定 Wacom 规则实现中的核心领域词汇，帮助代码、文档和测试使用同一套名称。

## Language

**PlayCard Transaction（出牌事务）**:
消费 Prepared PlayCard 后，从 `CardPlayed` 到卡牌效果、被动、中毒、敌方行动和战斗结束判断的一次完整同步规则结算。
_Avoid_: 出牌预览流程、UI 出牌流程

**PlayCard Evaluation（出牌求值）**:
读取当前 BattleState，为候选出牌生成规范化执行绑定、运行时费用和当前可提交性的只读规则事实。
_Avoid_: 出牌预检、UI 目标校验

**Target Probe（目标探测）**:
针对一个显式目标对象、不校验阶段与费用的严格 PlayCard 目标兼容性求值。
_Avoid_: 预览焦点校验、完整出牌校验

**Preview Focus（预览焦点）**:
Target Preview 中用于单目标表现摘要、但不构成 PlayCard 执行绑定的当前指向对象。
_Avoid_: 执行目标、命令目标

**Prepared PlayCard（已准备出牌）**:
通过完整 PlayCard Evaluation、携带状态版本与规范化执行事实、可交给 PlayCard Transaction 的一次性只读准备结果。
_Avoid_: 已提交出牌、可缓存命令

**Action Preview（行动预览）**:
在复制的战斗状态上执行出牌事务的确定性部分后，向表现层返回的只读预测事实。
_Avoid_: 模拟结算、UI 伤害计算

**Unresolved Fact（未决事实）**:
行动预览因随机选择或未知效果而明确不展开的后续结果。
_Avoid_: 预览失败、预测误差

**RunEvent Choice Evaluation（RunEvent 选项求值）**:
读取当前 RunState，按定义顺序生成一个 RunEvent 选项的条件、卡牌支付需求事实和当前可提交性。
_Avoid_: UI 可用性判断、后果模拟

**Battle Card Zone Transition（战斗卡牌区域迁移）**:
一批真实卡牌区域移动及其 Location、事件和被动后果的一次同步有序规则操作；它不承诺嵌套效果失败时回滚。
_Avoid_: 先移动再通知、弃牌事件服务、卡牌移动 Helper

## Relationships

- **Target Probe** 和 Target Preview 使用同一份 **PlayCard Evaluation** 目标规则，但前者要求一个具体显式目标。
- 正式提交与 **Action Preview** 共享完整的 **PlayCard Evaluation**，并只把 **Prepared PlayCard** 交给 **PlayCard Transaction**。
- **Preview Focus** 只属于 Target Preview 表现事实，不会改变 **Prepared PlayCard** 的执行绑定。
- 一次 **Action Preview** 消费一份复制状态上的 **PlayCard Transaction**。
- 正式提交与 **Action Preview** 共享同一份 **PlayCard Transaction** 顺序。
- 一个 **Action Preview** 可以包含零个或多个 **Unresolved Fact**。
- RunEvent Snapshot 与正式选项提交分别在各自调用时执行一次新的 **RunEvent Choice Evaluation**。
- **RunEvent Choice Evaluation** 只读取当前事实；选项 Effects 在求值通过后才进入 working-state 事务。
- 一个 **PlayCard Transaction** 可以包含零个或多个 **Battle Card Zone Transition**。
- **Battle Card Zone Transition** 只为真实成功移动发布事实；Action Preview adapter 继续透传到弃牌后的嵌套被动。

## Example dialogue

> **开发者：** “这张牌的弃牌被动需要在 Action Preview 里再写一套吗？”
> **规则设计：** “不需要。让 Action Preview 在副本上执行同一份 PlayCard Transaction；随机后续记录为 Unresolved Fact。”

> **开发者：** “全体攻击卡 hover 到一个敌人部位时，这个部位会变成正式命令目标吗？”
> **规则设计：** “不会。它只是 Preview Focus；PlayCard Evaluation 会为 AllEnemyParts 生成不带单部位绑定的 Prepared PlayCard。”

> **开发者：** “RunEvent 按钮已经显示可用，正式提交能直接信这个 Snapshot 吗？”
> **规则设计：** “不能。正式提交要针对最新 RunState 重新执行 RunEvent Choice Evaluation。”

## Flagged ambiguities

- “Preview” 曾同时指轻量 Target Preview 和完整 Action Preview；本文中的 **Action Preview** 只指完整出牌后的确定性净结果预测。
- Target Preview 中的 **Preview Focus** 曾被当作正式执行目标；现在两者明确分离，`None / Self / AllEnemyParts` 的规范化命令不携带显式目标。
- RunEvent Snapshot 的 `bAvailable` 是生成时的 **RunEvent Choice Evaluation** 事实，不是可缓存的提交授权。
