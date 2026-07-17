# Data Model: 击倒分支奖励合同基线

## 1. `UEnemyPartDefinition` reward fields

| Field | Type | Authoring role | Runtime resolution |
|---|---|---|---|
| `AidRewardCard` | `UCardDefinition*` | Aid 显式奖励 | Aid 首选 |
| `DestroyRewardCard` | `UCardDefinition*` | Destroy 显式奖励 | Destroy 首选 |
| `KnockdownRewardCard` | `UCardDefinition*` | deprecated legacy 兼容 | 仅在对应新字段为空时回退 |

统一查询：

```text
Aid      -> AidRewardCard ?? KnockdownRewardCard
Destroy  -> DestroyRewardCard ?? KnockdownRewardCard
Withdraw -> null
None     -> null
```

查询是非反射、只读、无加载/写入副作用。

## 2. Validation profile

| Profile | None configured | Legacy only | One/both new only | Legacy + any new | Both new explicit, legacy empty |
|---|---:|---:|---:|---:|---:|
| General | valid | valid | valid | error | valid |
| FormalProduction | error | error | error unless both | error | valid |

基础 `PartId/MaxHp/ExperienceReward` 校验在两档保持一致。FormalProduction 不要求 Aid/Destroy 指向不同卡，只要求作者显式填写两个分支。

## 3. `FKnockdownChoiceOptionView`

| Field | Meaning |
|---|---|
| `Choice` | 现有分支身份 |
| `bAvailable` | 现有规则可用性；不受奖励是否存在影响 |
| `DisabledReason` | 现有禁用原因 |
| `bHasRewardCard` | 统一查询是否返回卡 |
| `RewardCardId` | 奖励卡稳定 `CardId`；无奖励为 `None` |
| `RewardCardName` | `DisplayName`，为空时回退 CardId；无奖励为空文本 |

Aid/Destroy 从当前 pending 部位 Definition 构造。Withdraw 永远保持 `bHasRewardCard=false`。

## 4. Battle state transition

```text
PendingKnockdownChoice
  -> validate existing availability
  -> dequeue and record FKnockdownChoice
  -> emit KnockdownChoiceMade
  -> Withdraw: BattleEnd, no reward query/grant
  -> Aid/Destroy:
       ResolveKnockdownRewardCard(choice)
       if null: continue with zero reward
       if card:
         GrantCardToHand
         emit CardGained / checkpoint
         append FBattleGainedCard(SourceChoice=choice)
  -> next pending choice or PlayerAction/BattleEnd
```

整个流程仍位于一条 `ResolveCommand` working-state 事务；失败时状态、事件、journal 和版本零变化。

## 5. Run persistence (unchanged)

| Battle outcome | GainedCards persistence |
|---|---|
| Victory, not withdrawn | enters Run ownership |
| Victory, withdrawn | enters Run ownership |
| Defeat | not persisted |
| Undetermined | settlement rejected/no persistence |

`FBattleGainedCard.SourceChoice` 已区分 Aid/Destroy，不新增 packet 或 Run 字段。

## 6. UI state

Dialog 只缓存当前 `FKnockdownChoiceView`：

- `NativeConstruct` 应用缓存 ViewData。
- 每次 `SetContext` 替换整个 ViewData，并同步 PartName、三个按钮可用性与两段奖励文本。
- 无事件订阅、Tick、异步加载或规则对象缓存。
- 空奖励文本固定为“无卡牌奖励”；有奖励为“奖励：<RewardCardName>”。

## 7. Floor 1 future identities

| Archetype | Aid CardId | Destroy CardId | Package root |
|---|---|---|---|
| BrushSnake | `Reward.SerpentWood.BrushSnake.Aid` | `Reward.SerpentWood.BrushSnake.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/` |
| MoltGuard | `Reward.SerpentWood.MoltGuard.Aid` | `Reward.SerpentWood.MoltGuard.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/` |
| RootStalker | `Reward.SerpentWood.RootStalker.Aid` | `Reward.SerpentWood.RootStalker.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/` |
| ShallowGuardian | `Reward.SerpentWood.ShallowGuardian.Aid` | `Reward.SerpentWood.ShallowGuardian.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/` |

本轮只冻结这 8 个 CardId 和根目录，不冻结 package leaf name、数值、稀有度、关键词、效果、插画或描述。

## 8. Migration states

```text
LegacyOnly (existing binaries)
  -> ExplicitNewFields (authorized builder/asset migration)
  -> AssetRegistry confirms zero legacy dependencies
  -> RemoveLegacyField (future schema cleanup)
```

禁止 `Mixed` 状态进入提交后的制作内容。TrainingWarrior/Snake builder 源码在下一次授权运行时直接写 `ExplicitNewFields`。
