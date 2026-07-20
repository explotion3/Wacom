# Research: 商店卡牌强化规则与制作合同基线

## Decision 1：独立 Definition 链，而非运行时数值 delta

**Decision**: 每层强化是独立 `UCardDefinition`；实例强化只替换当前 Definition。

**Rationale**: Battle 已从实例 Definition 构造运行时卡；Save v5 已保存 DefinitionAssetPath。该方案同时复用 Data validation、卡面表现和现有持久语义。

**Alternatives considered**: 运行时修改 DataAsset 会污染全局且不可保存；通用 delta blob 难以验证并会复制 Battle 规则。

## Decision 2：CardId 精确身份 + UpgradeFamilyId 稳定族身份

**Decision**: 每层 CardId 唯一；显式链共享 UpgradeFamilyId。无链旧卡以 CardId 作为兼容族身份。

**Rationale**: 精确 Definition/版本选择与任务/支付的跨版本身份需要同时存在。

**Alternatives considered**: 所有层共用 CardId 会破坏唯一身份；仅追踪根 Definition 会让内容条件依赖资产指针。

## Decision 3：结构不变、数值可调的升级 comparator

**Decision**: 允许 BaseCost、主/PerfectRelease Effect 的 Magnitude/Duration 和表现变化；其它规则结构逐字段一致。每步至少有一个规则数值变化。

**Rationale**: v1 强化必须可解释且不能悄悄变成另一张机制卡。

**Alternatives considered**: 强制所有数值单调“变强”无法对 Cost、负值效果和条件化数值做通用证明；交给后续内容 Spec 冻结平衡。

## Decision 4：Shop 按来源稀有度静态定价

**Decision**: 每个 ShopDefinition opt-in，并为 White/Blue/Yellow 来源层配置非负价格；Purple/Intrinsic 无入口。

**Rationale**: 简单、可验证、允许不同商店定价，不把内容数值硬编码进 Run。

**Alternatives considered**: 全局价格公式和随机折扣扩大到经济系统；本轮拒绝。

## Decision 5：买卡与强化共用首次成功交易 AP

**Decision**: 保留历史 purchase 字段名，但其规则语义扩展为任何成功 shop commerce。Purchase/Upgrade 共用一个 private AP settlement。

**Rationale**: 玩家不应因 UI tab 不同而重复支付节点成本；抽取共同 settlement 避免两套阶段关闭逻辑漂移。

## Decision 6：Quote 是被动只读投影，Command 带陈旧保护

**Decision**: Snapshot 构建每个精确实例的 Quote；提交包含 InstanceId 与预期前后 Definition，价格由 Run 重算。

**Rationale**: 允许后续 UI 重建/刷新，并安全拒绝卡牌已移动、删除或被连续强化后的旧选择。

## Decision 7：不升级 SaveGame schema

**Decision**: 继续使用 v5 的 InstanceId + DefinitionAssetPath；ShopStates/upgrade service 仍不入档。

**Rationale**: 强化后的当前 Definition 已完整表达层级；新增冗余 tier 字段会产生双真相。

## Decision 8：Run 统一回收经济值

**Decision**: Run 规则返回 White/Blue/Yellow/Purple/Intrinsic=`1/2/3/4/0`；App 卡面调用该入口。

**Rationale**: 强化引入高稀有度后，现有两套 White/Blue mapping 会立即漂移。

## Decision 9：本轮不造半成品 UI

**Decision**: 现有 Shop Screen 保持 purchase-only；只冻结 Quote/Result 和访问配置传递。

**Rationale**: 没有 Production 强化资产时无法做真实交互验收。Spec 020 将 UI、Toast、正式卡和价格作为一个可 PIE 的垂直切片。
