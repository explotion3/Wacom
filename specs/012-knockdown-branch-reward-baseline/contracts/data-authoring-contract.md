# Data and Authoring Contract

## Runtime query

`UEnemyPartDefinition::ResolveKnockdownRewardCard(EKnockdownChoice)` 是分支奖励唯一查询：

- Aid：`AidRewardCard` 优先，空时回退 `KnockdownRewardCard`。
- Destroy：`DestroyRewardCard` 优先，空时回退 `KnockdownRewardCard`。
- Withdraw/None/未知值：返回空。

调用方不得直接复制优先级逻辑。

## Property authoring

- `AidRewardCard`、`DestroyRewardCard` 是正式制作字段。
- `KnockdownRewardCard` 保留序列化兼容，但 Details/编译提示必须明确 deprecated。
- 新正式资产不得填写 legacy 字段。
- legacy 与任一新字段混填属于错误，不允许用“新字段优先”静默通过。

## Validation profiles

`FWacomEnemyPartDefinitionValidation` 提供：

- `General`：现有 Editor validator 默认使用；允许无奖励、纯 legacy 或纯新字段。
- `FormalProduction`：未来 SerpentWood/Production builder smoke 显式使用；要求 Aid/Destroy 均非空且 legacy 为空。

两档均执行既有 ID、HP、EXP 校验并拒绝混填。

## Builder boundary

TrainingWarrior 与 Snake builder 下次授权运行时：

```text
AidRewardCard = existing reward
DestroyRewardCard = existing reward
KnockdownRewardCard = null
```

本功能只修改 builder 源码，不执行 builder、不保存 package、不把现有资产声称为已迁移。
