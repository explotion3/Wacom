# Stage 0.3 - 卡牌系统扩展计划文档收尾

## 目标

`Docs/卡牌系统扩展计划.md` 第一批 + 第二批 + 第四批已落地，第三批（按需做的项）和"代码就位待接入调用点"项不能丢失但也不该和 `TODO.md` 重复维护。

按"分流而不是删除"思路，把剩余项搬到 TODO.md 后再删除本文档。

## 分流结果

### 已完成（第一批 / 第二批 / 第四批）

无需迁移：内容已体现在 `WacomBattle.md` / `WacomData.md` 和代码注册表中。

- Effect.Draw（含 MetaTag 源区域）/ Discard / Heal / ExhaustSelf
- Effect.GainKeyword / RemoveStatus / ModifyInitiative
- Magnitude.Source.HandCount / TargetStatusStacks
- FMagnitudeModifier 机制
- Passive.Trigger.OnTurnStart / OnTurnEnd / OnDraw / OnDiscard 注册（调用点未接入，已迁移）

### 移除（已被现有功能吸收）

- `Effect.RecoverFromDiscard`：已被 `Effect.Draw` 的 MetaTag 源区域参数吸收（MetaTag = `CardLocation.Discard`）

### 迁移到 TODO.md §1 / 卡牌扩展（按需做）

- `Effect.CopyCard`（复制手牌临时副本）
- `Magnitude.Source.DiscardCount`
- `Magnitude.Source.DestroyedPartCount`
- `Target.AllHandCards`
- `Target.Adjacent.Left`
- `Target.Adjacent.Right`
- `Target.RandomEnemyPart`

### 迁移到 TODO.md §1 / 卡牌扩展（已注册 Handler 但调用点未接入）

- `Passive.Trigger.OnTurnStart`
- `Passive.Trigger.OnTurnEnd`
- `Passive.Trigger.OnDraw`
- `Passive.Trigger.OnDiscard`

### 迁移到 TODO.md §1 / 卡牌扩展（新 GDD 触发的依赖项）

- `Passive.Trigger.OnEnemyPartDestroyed`：依赖 GDD §6 部位击倒事件 + §3.3 击倒经验值。需要在 `PartDestroyed` 路径触发，让玩家三选一（援助 / 破坏 / 撤离）有挂载点
- `Passive.Trigger.OnPlayerDamaged`：与战内伤口阈值（GDD §3.2 / §9.2）相关，可由 BattleSession flag 维护承接，不一定要走 Passive trigger。先观察

### 迁移到 TODO.md §2 / 临时写法

- `Magnitude.Source.TargetStatusStacks` 借用 `FCardEffect::TargetZone` 传 Status Tag → 给 `FCardEffect` / `FEffectContext` 加专用 `FilterTag` 字段
- `Effect.GainKeyword` / `Effect.RemoveStatus` 借用 `FEffectContext::MetaTag` 传 Keyword/Status Tag → 同上 `FilterTag` 字段

## 文件改动

- `Docs/TODO.md`：§1 加 3 个新表（卡牌扩展按需做 / 调用点未接入 / 新 GDD 依赖项）；§2 临时写法加 2 行字段复用
- `Docs/卡牌系统扩展计划.md`：删除

## 验证

- 不涉及代码 / 资产，无需编译或测试
- TODO.md 仍然 markdown 格式正确（手动检查）
- 没有其他文档引用已删除文档（grep 检查，命中仅为工具缓存）
