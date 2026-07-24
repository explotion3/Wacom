---
type: tag-reference
scope: wacom-gameplay-tags
status: active
updated: 2026-07-24
tags:
  - wacom/data
  - wacom/gameplay-tags
---

# Wacom Gameplay Tags 文档

> [!info] 本文职责
> 本文记录 Wacom 当前 Gameplay tag 命名空间和 tag catalog。所有 tag 必须在 `WacomCore/Public/Tags/WacomGameplayTags.h` 声明；业务代码严禁字符串拼 tag。

> [!warning] 可制作范围
> Tag 已声明不代表已经接入规则主流程。DataAsset 能否使用某个 Effect / Target / Condition / Passive trigger，以 [WacomDataAuthoring.md](./WacomDataAuthoring.md#battle-rule-content-authoring-matrix)、validator 和运行时 resolver 为准。

## §1 命名空间

| 命名空间 | 用途 |
|---|---|
| `Card.Keyword.*` | 卡牌静态关键词和少量临时关键词 |
| `Card.Rarity.*` | 卡牌稀有度 |
| `Card.CapacityEffect.*` | 容器卡容量效果 |
| `Run.Card.Action.*` | RunFace 唯一主动作的静态语义 |
| `HandZone.*` | 战斗手牌区域 |
| `Interaction.Target.*` | App target handle 的 world target 语义 |
| `Effect.*` | 卡牌 / 敌人意图效果类型 |
| `Magnitude.Source.*` | 卡牌效果数值来源 |
| `Condition.*` | 效果 / 被动条件 |
| `Status.*` | 战斗状态和护盾数值入口 |
| `Target.*` | Battle effect target 语义 |
| `ZoneHook.Trigger.*` | 卡牌区域 hook 触发点 |
| `Passive.Trigger.*` | 卡牌被动触发点 |
| `CardLocation.*` | 抽牌类效果的源区域 |
| `SkillSlot.*` | Run 层角色技能池占位 |

## §2 Card.Keyword

| Tag | 代码名 | 说明 |
|---|---|---|
| `Card.Keyword.Swift` | `Card_Keyword_Swift` | 迅捷 |
| `Card.Keyword.Retain` | `Card_Keyword_Retain` | 保留 |
| `Card.Keyword.Combo` | `Card_Keyword_Combo` | 连击 |
| `Card.Keyword.Companion` | `Card_Keyword_Companion` | 伙伴 |
| `Card.Keyword.Weapon` | `Card_Keyword_Weapon` | 武器 |
| `Card.Keyword.Tool` | `Card_Keyword_Tool` | 工具 |
| `Card.Keyword.Hand` | `Card_Keyword_Hand` | 手，左右手固有卡使用 |
| `Card.Keyword.Exhaust` | `Card_Keyword_Exhaust` | 临时关键词：本卡打出后进入消耗牌堆 |
| `Card.Keyword.BagProvider` | `Card_Keyword_BagProvider` | 历史 / 兼容内容标记；当前容量真相以 `Physique.Capacity > 0` 的容器卡为准 |
| `Card.Keyword.DeleteProvider` | `Card_Keyword_DeleteProvider` | 删牌能力提供者；RunSession 可读取玩家持有区是否存在该 tag |

## §3 Card.Rarity

| Tag | 代码名 | 说明 |
|---|---|---|
| `Card.Rarity.White` | `Card_Rarity_White` | 白色 |
| `Card.Rarity.Blue` | `Card_Rarity_Blue` | 蓝色 |
| `Card.Rarity.Yellow` | `Card_Rarity_Yellow` | 黄色 |
| `Card.Rarity.Purple` | `Card_Rarity_Purple` | 紫色 |
| `Card.Rarity.Intrinsic` | `Card_Rarity_Intrinsic` | 固有 |

### Run.Card.Action

`Run.Card.Action` 是 Validator 使用的根标签，不是可直接制作的动作；RunFace 必须选择下列具体子标签。动作的目标合法性、成本和结算由后续 Room / 目标事务定义，本轮只建立静态分类。

| Tag | 代码名 | 说明 |
|---|---|---|
| `Run.Card.Action.Reveal` | `Run_Card_Action_Reveal` | 揭示路线、房间或隐藏信息 |
| `Run.Card.Action.Unlock` | `Run_Card_Action_Unlock` | 解锁门、容器或机关 |
| `Run.Card.Action.Break` | `Run_Card_Action_Break` | 破坏障碍或脆弱目标 |
| `Run.Card.Action.Fix` | `Run_Card_Action_Fix` | 修复设施、路径或物件 |
| `Run.Card.Action.Ignite` | `Run_Card_Action_Ignite` | 点燃可交互目标 |
| `Run.Card.Action.Feed` | `Run_Card_Action_Feed` | 喂食生物或其它接收者 |

## §4 HandZone

| Tag | 代码名 | 说明 |
|---|---|---|
| `HandZone.Left` | `HandZone_Left` | 左手区 |
| `HandZone.Both` | `HandZone_Both` | 双手区 |
| `HandZone.Right` | `HandZone_Right` | 右手区 |

## §5 Interaction.Target

| Tag | 代码名 | 说明 |
|---|---|---|
| `Interaction.Target.Battle.EnemyPart` | `Interaction_Target_Battle_EnemyPart` | Battle scene world target，表示当前战斗敌方部位 |
| `Interaction.Target.Run.Object` | `Interaction_Target_Run_Object` | Run / 探索 world target，可被 App resolver 用于 click、hover 和 card drop |

## §6 Effect

| Tag | 代码名 | 说明 |
|---|---|---|
| `Effect.Damage` | `Effect_Damage` | 伤害 |
| `Effect.Heal` | `Effect_Heal` | 治疗玩家 HP，并移除治疗量 10% 的中毒层数，向下取整 |
| `Effect.ApplyStatus.Poison` | `Effect_ApplyStatus_Poison` | 施加中毒 |
| `Effect.ApplyStatus.Slow` | `Effect_ApplyStatus_Slow` | 敌方即时延迟当前意图；玩家投递下回合卡牌减速 |
| `Effect.ApplyStatus.Freeze` | `Effect_ApplyStatus_Freeze` | 敌方拦截后续卡牌推进；玩家投递下回合冻结卡 |
| `Effect.ApplyStatus.Twilight` | `Effect_ApplyStatus_Twilight` | 敌方延迟下一意图；玩家投递下回合整手牌暮气 |
| `Effect.Shuffle.Random` | `Effect_Shuffle_Random` | 随机腾挪 |
| `Effect.Shuffle.FromBothToOther` | `Effect_Shuffle_FromBothToOther` | 从双手区腾挪到其他区域 |
| `Effect.Shuffle.ToRandomZone` | `Effect_Shuffle_ToRandomZone` | 把本卡腾挪到随机区域 |
| `Effect.Card.AddCost` | `Effect_Card_AddCost` | 对目标卡 RuntimeCostModifier 增加 |
| `Effect.Card.ReduceCost` | `Effect_Card_ReduceCost` | 对目标卡 RuntimeCostModifier 减少 |
| `Effect.Card.DiscardSelected` | `Effect_Card_DiscardSelected` | 将 `Target.SelectedHandCard` 指定的普通手牌移入弃牌堆 |
| `Effect.Card.ExhaustSelected` | `Effect_Card_ExhaustSelected` | 将 `Target.SelectedHandCard` 指定的普通手牌移入消耗牌堆 |
| `Effect.Draw` | `Effect_Draw` | 从指定卡牌区域移动卡牌到手牌 |
| `Effect.Discard` | `Effect_Discard` | 随机弃掉手牌中的普通卡 |
| `Effect.ExhaustSelf` | `Effect_ExhaustSelf` | 标记本卡打出后进入消耗牌堆 |
| `Effect.GainKeyword` | `Effect_GainKeyword` | 给目标手牌临时添加关键词 |
| `Effect.RemoveStatus` | `Effect_RemoveStatus` | 移除目标持久 Combatant 状态层数；不移除即时敌方 Slow |
| `Effect.ModifyInitiative` | `Effect_ModifyInitiative` | 修改目标敌方部位当前先机 |

当前没有 `Effect.ApplyStatus.Burn` 或 `Status.Burn`。UI 的 Burn 徽章只是美术预留位，不能作为正式 DataAsset 配置依据。

## §7 Magnitude.Source

| Tag | 代码名 | 说明 |
|---|---|---|
| `Magnitude.Source.Literal` | `Magnitude_Source_Literal` | FinalMagnitude = `Magnitude` 字段 |
| `Magnitude.Source.RuntimeCost` | `Magnitude_Source_RuntimeCost` | FinalMagnitude = 本卡当前 RuntimeCost |
| `Magnitude.Source.HandCount` | `Magnitude_Source_HandCount` | Reserved；resolver 有入口，但当前制作矩阵不允许正式资产使用 |
| `Magnitude.Source.TargetStatusStacks` | `Magnitude_Source_TargetStatusStacks` | FinalMagnitude = 目标部位上 `TargetZone` 指定 Status tag 的层数 |

## §8 Condition

| Tag | 代码名 | 说明 |
|---|---|---|
| `Condition.Self.InZone` | `Condition_Self_InZone` | 本卡当前在指定区域 |
| `Condition.Target.HasStatus` | `Condition_Target_HasStatus` | 目标部位含指定状态 |

## §9 Status

| Tag | 代码名 | 说明 |
|---|---|---|
| `Status.Poison` | `Status_Poison` | 中毒 |
| `Status.Slow` | `Status_Slow` | 卡牌回合级费用增加；敌方侧只作为即时 Initiative cause |
| `Status.Freeze` | `Status_Freeze` | 敌方推进拦截层数或卡牌回合级出牌限制 |
| `Status.Twilight` | `Status_Twilight` | 敌方下一意图延迟层数或随卡持久费用状态 |
| `Status.Stunned` | `Status_Stunned` | 晕厥 |
| `Status.Shield` | `Status_Shield` | 护盾数值入口；不进入 `StatusStacks` |

## §10 Target

| Tag | 代码名 | 说明 |
|---|---|---|
| `Target.Self` | `Target_Self` | 自身；具体解析为 Player 或本卡取决于 EffectType |
| `Target.Player` | `Target_Player` | 玩家 |
| `Target.SingleEnemyPart` | `Target_SingleEnemyPart` | 单个敌方部位 |
| `Target.AllEnemyParts` | `Target_AllEnemyParts` | 所有敌方部位 |
| `Target.RandomHandCard` | `Target_RandomHandCard` | 手牌中随机一张 |
| `Target.ZoneHandCard` | `Target_ZoneHandCard` | 指定区域的手牌 |
| `Target.Adjacent.Right` | `Target_Adjacent_Right` | 相邻右方；tag 已声明，解析未实现 |
| `Target.LastShuffledCard` | `Target_LastShuffledCard` | 最近一次 Shuffle 的被移动卡 |
| `Target.SelectedHandCard` | `Target_SelectedHandCard` | `TargetMode=HandCard` 卡牌打出时，玩家选择的目标手牌 |

## §11 ZoneHook.Trigger

| Tag | 代码名 | 说明 |
|---|---|---|
| `ZoneHook.Trigger.OnPlay` | `ZoneHook_Trigger_OnPlay` | 本卡打出时 |
| `ZoneHook.Trigger.OnPerfectReleaseHit` | `ZoneHook_Trigger_OnPerfectReleaseHit` | 完美释放命中时 |

## §12 Passive.Trigger

| Tag | 代码名 | 说明 |
|---|---|---|
| `Passive.Trigger.AfterPlayed` | `Passive_Trigger_AfterPlayed` | 本卡打出完成后 |
| `Passive.Trigger.OnCompanionCount` | `Passive_Trigger_OnCompanionCount` | 全局 Companion 计数达阈值 |
| `Passive.Trigger.OnTwilightTriggered` | `Passive_Trigger_OnTwilightTriggered` | 暮气施加成功时 |
| `Passive.Trigger.OnTurnStart` | `Passive_Trigger_OnTurnStart` | 玩家回合开始时；dispatcher 方法存在，主流程未接入 |
| `Passive.Trigger.OnTurnEnd` | `Passive_Trigger_OnTurnEnd` | 玩家回合结束时；dispatcher 方法存在，主流程未接入 |
| `Passive.Trigger.OnDraw` | `Passive_Trigger_OnDraw` | 本卡被抽到手牌时；dispatcher 方法存在，主流程未接入 |
| `Passive.Trigger.OnDiscard` | `Passive_Trigger_OnDiscard` | 本卡被真正弃掉时；打出后自然进弃牌堆不触发 |

## UI Widget Registry

UI Widget tag 只作为 CommonUI 顶层软类注册身份，不进入规则状态或 DataAsset 效果制作。

| Tag | 代码名 | 说明 |
|---|---|---|
| `UI.Widget.RunMapScreen` | `UI_Widget_RunMapScreen` | 当前 Floor 地图 Screen；注册类必须继承 `UWacomRunMapScreen`，缺失时回退 C++ Screen |

## §13 CardLocation

`Effect.Draw` 使用 `FCardEffect::TargetZone` 指定源区域。未设置时默认抽牌堆。当前可制作源区域只有 Draw / Discard / Exhaust；`CardLocation.Hand` 是保留 tag，不是当前 Draw authoring contract 的合法源。

| Tag | 代码名 | 说明 |
|---|---|---|
| `CardLocation.Draw` | `CardLocation_Draw` | 抽牌堆 |
| `CardLocation.Discard` | `CardLocation_Discard` | 弃牌堆 |
| `CardLocation.Exhaust` | `CardLocation_Exhaust` | 消耗牌堆 |
| `CardLocation.Hand` | `CardLocation_Hand` | 手牌 |

## §14 SkillSlot

Run 层角色技能池的占位 tag。等技能列表正式定义后，按角色添加具体 `SkillSlot.*`。

| Tag | 代码名 | 说明 |
|---|---|---|
| `SkillSlot.Placeholder` | `SkillSlot_Placeholder` | 占位；满 10 经验入账一个，不挂效果 |

<a id="cardcapacityeffect"></a>
## §15 Card.CapacityEffect

`FCardPhysique::CapacityEffect` 使用此命名空间。空 tag 表示 A 类容器；有效 tag 表示 B 类容器，并为该主卡展开 SpecialZone。

| Tag | 代码名 | 说明 |
|---|---|---|
| `Card.CapacityEffect.Placeholder` | `Card_CapacityEffect_Placeholder` | 占位 tag，早期 B 类骨架使用；当前不再分配给具体卡 |
| `Card.CapacityEffect.WeaponDamagePlus3` | `Card_CapacityEffect_WeaponDamagePlus3` | 蛛茧绒囊容量效果；SpecialZone 内已选择入战且带武器关键词的卡，其 `Effect.Damage` 最终结算 +3 |

## §16 新增 tag 检查点

新增 Gameplay tag 时必须同步：

1. 在 `WacomCore/Public/Tags/WacomGameplayTags.h` 声明和注册。
2. 更新本文 catalog 和命名空间说明。
3. 若 tag 会进入 DataAsset，更新 [WacomData.md](./WacomData.md) 中相关字段口径。
4. 若 tag 是 Effect / Target / Condition / Passive / MagnitudeSource，更新 [WacomDataAuthoring.md](./WacomDataAuthoring.md) 的制作矩阵、validator 和 runtime fixture。
5. 若 tag 只服务 UI 美术预留，不要让正式 DataAsset 使用它；在 UI 文档中说明其占位身份。
