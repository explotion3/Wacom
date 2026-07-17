# Research: 击倒分支奖励合同基线

## Decision 1 — 唯一奖励查询属于 `UEnemyPartDefinition`

**Decision**: 新增非反射 C++ `ResolveKnockdownRewardCard(EKnockdownChoice)`。Aid/Destroy 优先各自显式字段，空时回退 legacy；Withdraw/None 返回空。

**Rationale**: 奖励来源是静态部位内容。把兼容优先级放在 Data 定义旁，可让 Battle resolver、ViewData 和测试共用一份规则，且 `EKnockdownChoice` 已由底层 `WacomCore` 提供。

**Alternatives considered**:
- 在 Resolver 与 Availability 各写一份字段选择：会产生结算/预览漂移。
- 让 App 读取 `UEnemyPartDefinition`：违反 passive UI 与模块边界。
- 新建通用奖励表资产：超出当前简单分支卡范围，也会增加资产和 Save/Run 语义。

## Decision 2 — legacy 保留但禁止混填

**Decision**: `KnockdownRewardCard` 保留反射序列化并标记 deprecated；旧字段与任一新字段同时非空是制作错误。

**Rationale**: TrainingWarrior、Snake 二进制资产当前仍保存旧字段，本轮禁止重存。混填会让“新字段优先”的静默规则掩盖作者错误，必须由 validator 阻止。

**Alternatives considered**:
- 直接删除旧字段：现有资产会丢奖励并需要未授权迁移。
- 允许混填：会隐藏过渡状态，难以证明何时可以删除旧字段。

## Decision 3 — General 与 FormalProduction 两档校验

**Decision**: General 允许无奖励、纯 legacy 或纯新字段；FormalProduction 要求 Aid/Destroy 均显式配置且 legacy 为空。两档都拒绝混填。

**Rationale**: Debug/测试内容需要合法的零奖励状态，现有资产需要 legacy 兼容，正式内容则不能把缺奖励或 fallback 带入生产。

**Alternatives considered**:
- 路径字符串自动决定严格度：新路径/迁移路径容易误判，且 validator 规则不应依赖包名推断资产阶段。
- General 也强制双奖励：会使当前所有旧资产立即失效。

## Decision 4 — builder 只迁移未来写入源码

**Decision**: TrainingWarrior/Snake builder 以后同时写 Aid/Destroy 并清空 legacy；本轮不执行任何 builder。

**Rationale**: 这样下一次明确授权重建时会产生纯新字段资产，同时保证本轮二进制哈希不变。

**Alternatives considered**:
- 现在运行 builder：违反资产范围，且可能覆盖人工/其它 Agent 内容。
- 保持 builder 写旧字段：继续制造迁移债和 FormalProduction 不合格资产。

## Decision 5 — ViewData 只投影标量/文本

**Decision**: `FKnockdownChoiceOptionView` 增加 `bHasRewardCard`、`RewardCardId`、`RewardCardName`，不暴露 `UCardDefinition*`。

**Rationale**: App 只需要文本预览。CardId 提供稳定诊断，CardName 服务展示；不把规则对象或加载责任交给 UI。

**Alternatives considered**:
- 暴露 CardDefinition：会扩大 App 对 Data 对象的解释面，并诱导 Widget 构造完整卡面。
- 只暴露显示名：缺少稳定身份，难以测试/诊断。

## Decision 6 — 显示名 fallback

**Decision**: 有奖励时优先 `DisplayName`，为空则使用 `CardId`；卡指针为空时 `bHasRewardCard=false`，UI 显示“无卡牌奖励”。

**Rationale**: 既保持玩家可读，又能对不完整测试 fixture 提供确定结果，不从 UObject 名称推断稳定身份。

## Decision 7 — Run 合同不变

**Decision**: 继续使用现有 `FBattleGainedCard.SourceChoice` 与 `FBattleResultPacket.GainedCards`。Victory 和 Withdraw 持久化既得卡，Defeat 不持久化。

**Rationale**: 现有 packet 已能表达来源；分支差异只改变“选哪张定义”，不需要新的 Run state、SaveGame 或事件。

## Decision 8 — Floor 1 每敌人一对奖励卡

**Decision**: 四个 Archetype 各预留 `.Aid`/`.Destroy`，共 8 张，放入 `/Game/Wacom/Data/Cards/Rewards/SerpentWood/<Archetype>/`。

**Rationale**: 粒度与用户确认一致，避免 11 个部位产生 22 张未设计卡，也避免四个敌人共用一对导致内容辨识度不足。

**Alternatives considered**:
- 每部位一对：资产量和设计成本过大。
- 全 Floor 共用一对：无法表达敌人主题差异。

## Live-source audit

- 当前 Resolver 对 Aid/Destroy 都读取同一 `KnockdownRewardCard`，然后调用现有 grant service 并写 `SourceChoice`。
- Availability 已是 Resolver 与 public ViewData 共用 helper，适合在同一位置填奖励摘要。
- Run settlement 已按 `Outcome=Victory`（含 Withdraw）处理 GainedCards，Defeat/Undetermined 不处理。
- Dialog 的 Modal/Back/command flow 已稳定，只需扩展文本刷新。
- TrainingWarrior/Snake binary assets均由 legacy 字段驱动；本轮需要哈希证明未重存。
- Spec 011 的 38-package manifest不包含击倒分支奖励卡；本轮将它保持为“核心资产”并另列 8 张奖励卡，不回写或伪造已有 manifest 行。
