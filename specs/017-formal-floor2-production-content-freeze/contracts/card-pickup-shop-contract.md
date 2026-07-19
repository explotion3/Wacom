# Contract: MoltCavern Card, Pickup and Shop

## 1. Card set

- 恰好 12 张新 CardDefinition：4 张固定 Pickup/Run 卡 + 8 张 Aid/Destroy 卡。
- 全部 Physique 为零，PerfectRelease/ZoneHook/Passive 为空，不使用 Swift/Exhaust。
- Aid 只有 Tool，Destroy 只有 Weapon；MoltSeal 无关键词。
- CardId、TargetMode、有序 Effect type/target 与 package 是稳定结构；DisplayName/Description 是可调表现文本。
- 具体费用、稀有度、数值和描述模板见 [data-model.md](../data-model.md) §5。

## 2. Part reward assignment

| Archetype | Parts | Aid | Destroy | Legacy |
|---|---|---|---|---|
| ScaleCrawler | Head, Body | `Reward.MoltCavern.ScaleCrawler.Aid` | `Reward.MoltCavern.ScaleCrawler.Destroy` | null |
| StoneScaleGuard | Head, Carapace, Tail | `Reward.MoltCavern.StoneScaleGuard.Aid` | `Reward.MoltCavern.StoneScaleGuard.Destroy` | null |
| VenomHunter | Head, Coil, VenomSac | `Reward.MoltCavern.VenomHunter.Aid` | `Reward.MoltCavern.VenomHunter.Destroy` | null |
| CavernGuardian | Head, Body, Tail, MoltCore | `Reward.MoltCavern.CavernGuardian.Aid` | `Reward.MoltCavern.CavernGuardian.Destroy` | null |

每个已处理 Part 只授予所选 Aid 或 Destroy 一张独立实例。相同 CardId 可重复；不增加 claimed set、去重、领取上限、替代货币或 AP。

## 3. Pickup mapping

| Pickup | Card | Credential |
|---|---|---|
| FungalCache | GlowcapPoultice | none |
| MineralCache | CrystalWard | none |
| VenomCrystalCache | VenomShard | none |
| MoltSeal | `Card.Run.MoltSeal` | `Credential.Run.MoltSeal` |

MoltSeal 的 Card 与 Credential 使用现有原子 Pickup 事务；Floor 3 入口只检查非消耗 Credential，不从 Card 存在性推断资格。

## 4. DeepWayfarer offer order

| # | Card | Gold |
|---:|---|---:|
| 1 | `Reward.SerpentWood.HerbalPoultice` | 3 |
| 2 | `Starter.ChitinWard` | 3 |
| 3 | `Starter.MoltCut` | 4 |
| 4 | `Reward.MoltCavern.GlowcapPoultice` | 4 |
| 5 | `Reward.MoltCavern.VenomShard` | 5 |

库存固定，不使用随机池、权重、动态价格或重复 Offer。三张 Floor 1/Starter 卡是只读依赖。浏览/离开 0 AP；首次成功购买 1 AP，且只有该行为产生 Floor 2 的 `8–9 / 14–15` 区间。

## 5. Reward-volume risk

关键路线产生 17 或 18 张击倒分支卡，完整探索 24 张。现有 Victory/Withdraw/Defeat、战内手牌上限与 Run 背包/负重兜底继续生效；是否需要奖励替换、容量门槛或同敌去重仍是后续设计问题。
