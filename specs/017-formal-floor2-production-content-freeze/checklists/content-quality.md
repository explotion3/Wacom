# Content Quality Checklist: 正式 Floor 2 Production 内容合同冻结

**Purpose**: 核对 MoltCavern 设计值、引用、路线经济与 Production 交接质量。
**Created**: 2026-07-19
**Feature**: [spec.md](../spec.md)

## Enemy and Encounter

- [x] CHK001 4 个 Enemy、4 个 Behavior、12 个 Part、26 个 Intent 的总数与每 Archetype 分布一致。
- [x] CHK002 每个 Part 都有唯一 PartId、Sequence IntentSet 和有序 Intent；所有 Behavior 使用 Default phase、零 cooldown、空 selector/fallback。
- [x] CHK003 Damage/Poison/Slow 指向 Player，Shield 指向行动 Part 自身；Slow 使用 Default/1-card 投递。
- [x] CHK004 Enemy 总 HP/EXP 为 `21/2`、`36/4`、`34/5`、`70/10`。
- [x] CHK005 七 Encounter authored order 与 HP `21/36/42/36/34/57/70` 一致，最多两个敌人，bBoss 只留在 Floor payload。

## Cards, rewards and pickups

- [x] CHK006 12 张 Card 恰好为 4 fixed + 4 Aid + 4 Destroy，CardId/package 唯一。
- [x] CHK007 所有 Card 的 cost/rarity/keyword/TargetMode/effect order 与批准表一致，Physique/advanced fields 为空。
- [x] CHK008 12 个 Part 全部显式引用所属 Archetype 的 Aid/Destroy 卡对，legacy 引用为 0。
- [x] CHK009 四 Pickup 映射正确，MoltSeal 同时声明 Card 和 Credential。
- [x] CHK010 描述模板的 `{Effect.N}` 与冻结 Effects 顺序一致，不让自然语言成为规则来源。

## Event, Shop and route economy

- [x] CHK011 三 Event 恰好 10 个 Choice，全部 terminal/Automatic/complete/close，无 CardPayment。
- [x] CHK012 两个 RunFlag、Gold 与 Misdeed/Fatigue/Wound 条件/效果顺序精确且属于现有 schema。
- [x] CHK013 DeepWayfarer 五 Offer 的 identity/order/price 为批准的 `3/3/4/4/5`，外部三卡只读。
- [x] CHK014 Route A/B 从 0 Gold 均存在购买路径，信息路线分别服务 MoltingRite。
- [x] CHK015 路线奖励为 `17/18/17/18`，完整探索 24；AP 仍为 `8–9 / 14–15`。

## Manifest and readiness

- [x] CHK016 manifest 恰好 47 条，类型分布为 `4 Enemy / 4 Behavior / 12 Part / 7 Encounter / 3 Event / 4 Pickup / 1 Shop / 12 Card`。
- [x] CHK017 package、object leaf、stable ID 与节点 Definition 映射一致且无重复。
- [x] CHK018 manifest/Production 引用表无 Debug、Authoring、Test、BadgeDisplayTests、TrainingWarrior 或 Character 依赖。
- [x] CHK019 stable 与 tunable 字段边界明确，未来 seed-only 工具不得覆盖已有人工调参。
- [x] CHK020 文档冻结、资产制作、场景制作、跨层 handoff 和 PIE 的完成状态没有混写。
- [x] CHK021 BugGirl 已知外部污染保持可见，未修改资产或削弱 validator。

## Delivery

- [x] CHK022 本轮允许文件范围仅为 Markdown、`.specify/feature.json` 和 `AGENTS.md` 托管指针。
- [x] CHK023 Unreal 编译/测试/资产/PIE 跳过原因与后续验证门禁明确。
- [x] CHK024 用户审阅前不 stage、不 commit；确认后仍不 merge main、不 push。

## Notes

- 勾选表示内容合同自身通过文档核对，不代表 47 个 DataAsset 或 Floor 2 场景已经存在。
