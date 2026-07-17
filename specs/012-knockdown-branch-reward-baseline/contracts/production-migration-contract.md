# Production Content and Migration Contract

## Floor 1 reward identities

每个 SerpentWood Archetype 预留一对卡：

```text
Reward.SerpentWood.BrushSnake.Aid
Reward.SerpentWood.BrushSnake.Destroy
Reward.SerpentWood.MoltGuard.Aid
Reward.SerpentWood.MoltGuard.Destroy
Reward.SerpentWood.RootStalker.Aid
Reward.SerpentWood.RootStalker.Destroy
Reward.SerpentWood.ShallowGuardian.Aid
Reward.SerpentWood.ShallowGuardian.Destroy
```

路径根：

```text
/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/
/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/
/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/
/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/
```

本轮不冻结数值、类型、稀有度、关键词、效果或 package leaf name。

## Count contract

- Spec 011 的 38 个 DataAsset 保持“Floor 1 核心内容资产”集合。
- 分支奖励额外增加 8 张 CardDefinition。
- 未来 Floor 1 内容制作总量表达为 `38 core + 8 knockdown branch reward cards = 46 assets`。
- 本轮不修改 Spec 011 已交付 manifest 的历史行；长期 Docs 和未来实施任务说明扩展集合。

## Legacy removal gate

只有同时满足以下条件，才允许另案删除 `KnockdownRewardCard`：

1. TrainingWarrior Part 已授权迁移并通过 General/Formal 预期校验。
2. Snake Head/Body/Tail 已授权迁移。
3. 全部 Production Part 使用两个显式字段。
4. AssetRegistry/引用审计证明零资产依赖 legacy 字段。
5. builder 双跑和二进制迁移由资产所有者批准。
6. 删除轮重新编译、加载并运行 Battle/Run/UI 回归。

在 gate 完成前，legacy fallback 是正式兼容行为；混填始终非法。
