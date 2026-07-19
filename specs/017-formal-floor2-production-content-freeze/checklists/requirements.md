# Requirements Quality Checklist: 正式 Floor 2 Production 内容合同冻结

**Purpose**: 验证 Spec 017 的需求完整、明确、可静态验收，不把未来资产制作或运行时能力误写为已完成。
**Created**: 2026-07-19
**Feature**: [spec.md](../spec.md)

## Scope and ownership

- [x] CHK001 纯文档范围与禁止修改的 Source、Config、Content、GameplayTag、Build.cs、SaveGame 和二进制资产均已明确。
- [x] CHK002 `WacomData / WacomBattle / WacomRun / WacomEditor / WacomTests` 的未来职责边界已写清，且本轮不声称修改模块。
- [x] CHK003 Floor 3、Production Journey、跨层 world handoff、场景/Host、美术与背包规则均明确排除。
- [x] CHK004 用户审阅前不得提交的交付门禁已写成可验证要求。

## Content completeness

- [x] CHK005 4 Enemy、4 Behavior、12 Part、26 Intent 与 7 Encounter 的身份、顺序和数值均有唯一权威表。
- [x] CHK006 12 Card、4 Pickup、3 Event/10 Choice 与 1 Shop 的字段、顺序和引用均有唯一权威表。
- [x] CHK007 47-package manifest 的分类、路径规则、stable ID 和只读依赖边界均已定义。
- [x] CHK008 12 个 Part 的 Aid/Destroy 映射与 legacy-null 要求覆盖完整。

## Rule and economy clarity

- [x] CHK009 Intent 与 Card 的 Shield target 差异、Slow 手牌投递、Effect 顺序和现有 schema 边界没有歧义。
- [x] CHK010 A/B 获取购买力、情报服务 D 路、Shop 首购 AP 和跨 Floor 资源累计均已明确。
- [x] CHK011 路线奖励 `17/18/17/18/24` 与 AP `8–9 / 14–15` 均能从已列 Encounter/节点推导。
- [x] CHK012 重复卡、Withdraw、最后部位、背包溢出与其它击倒后果未被本轮静默重设计。

## Validation and risk

- [x] CHK013 纯文档验证集合覆盖跨工件一致性、计数、唯一性、schema、禁止引用、Git/LFS 与范围审计。
- [x] CHK014 编译、Automation、AssetRegistry、Builder、Blueprint 与 PIE 的跳过原因和后续补证据门禁已明确。
- [x] CHK015 `DA_Character_BugGirl` 已知污染被明确隔离，不修改资产、不削弱 validator、不误报 Production closure。
- [x] CHK016 所有成功标准均可用静态文档或 Git 状态验证，没有主观且不可测试的表述。

## Notes

- 本清单只评估需求质量，不代表 47 个未来 DataAsset 已创建或运行时已验证。
