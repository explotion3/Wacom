# Research: Journey 成功结算与终局交接基线

## Decision 1: 终局由 Journey DataAsset 的 Floor-qualified handle 声明

- **Decision**: `SuccessTerminalNode` 使用 `FWacomMapNodeHandle`，并由 Editor validator 与 Runtime 初始化共同验证。
- **Rationale**: NodeId 是 Floor-scoped；显式 handle 能跨层稳定引用，避免 Actor label、EncounterId 或硬编码字符串成为规则来源。
- **Alternatives considered**: 自动选择最后一个 Boss（多 Boss/无出边时歧义）；GameMode 硬编码 Floor 3 NodeId（跨内容不可复用）；GameplayTag（本轮无新增 tag 且终局是结构身份）。

## Decision 2: Outcome 替代运行时活动布尔值

- **Decision**: `FRunState` 以 `ERunOutcome` 为权威，兼容查询从 Outcome 与既有失败门槛派生。
- **Rationale**: 旧 `bRunActive=false` 只能表达“不活动”，无法区分成功和失败；成功优先级也需要明确状态。
- **Alternatives considered**: 保留 bool 再加 `bSucceeded`（产生非法组合）；只在 Snapshot 加成功（规则状态不可持久）；复用 failure inactive（语义错误）。

## Decision 3: 成功在 Encounter settlement working state 内生成

- **Decision**: 只有 active terminal ticket 的非撤离 Victory，在奖励和节点完成后构建摘要、设置 Outcome，并追加末尾事件后一次提交。
- **Rationale**: 此处同时拥有 ticket、战果、奖励、压力、AP、节点生命周期和事务回滚能力。
- **Alternatives considered**: GameMode 看到 Victory 后写状态（跨层反向污染）；resolver 内直接判 Journey（Battle settlement helper 不应依赖 Journey topology）；提交后第二次 mutation（版本/广播不原子）。

## Decision 4: 终局 Victory 优先于同次战斗的失败门槛

- **Decision**: 一旦 terminal Victory 合法，raw Outcome 设置 Succeeded；最终压力照常计算并写摘要，兼容 `IsRunFailed()` 不再覆盖 Succeeded。
- **Rationale**: 用户已冻结 Guardian mutual-destruction/压力满仍成功的设计语义。
- **Alternatives considered**: 先判压力失败（与批准规则冲突）；钳制压力到 99（伪造最终状态）；增加特殊 UI-only override（规则不一致）。

## Decision 5: Save runtime summary 与磁盘 summary 分离

- **Decision**: v5 使用独立 Save USTRUCT 和显式 has-summary 标志，迁移/合法性检查在 apply 前完成。
- **Rationale**: 磁盘 schema 应稳定且不携带 transient/runtime helper；终态恢复必须能在任何资产解析或 RunState 写入前拒绝。
- **Alternatives considered**: 直接序列化 runtime summary（耦合未来运行时字段）；继续只存 bool（丢失成功）；自动把成功档重开成 inactive Run（制造不可操作状态）。

## Decision 6: 总结页是被动 CommonUI Screen，GameMode 拥有交接

- **Decision**: Screen 只消费 ViewData 并广播 continue intent；GameMode 监听 `JourneySucceeded`、等待 camera return 双 barrier、push Screen、teardown PrimaryLayout、下一帧 travel。
- **Rationale**: 保持 UI passive、复用既有战斗返回镜头流程，并将 push failure 纳入同一 idempotent fallback。
- **Alternatives considered**: Screen 直接 OpenLevel（UI 拥有流程）；立即 travel（看不到总结且破坏 staging）；重新恢复 exploration hand/toast（终态世界短暂可操作）。

## Decision 7: 不执行真实 Floor 3 PIE

- **Decision**: 本轮自动化覆盖规则、Save 与 App handoff；AssetRegistry 只读审计；Production 资产轮再跑 Golden Path PIE。
- **Rationale**: 当前无 Production Journey/Floor3 DataAsset 或终局场景，临时改 Debug 资产会扩大范围并产生二进制写入。
- **Alternatives considered**: builder 生成临时资产（明确禁止）；修改 Debug Journey（不应晋升）；把自动化称为 PIE（验证类型不实）。
