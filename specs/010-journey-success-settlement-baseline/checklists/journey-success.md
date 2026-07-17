# Journey Success Requirements Checklist: Journey 成功结算与终局交接基线

**Purpose**: 供实现作者与 PR reviewer 审查终局、事务、Save 与 UI 交接要求是否完整、清晰且一致
**Created**: 2026-07-17
**Feature**: [spec.md](../spec.md)

## Requirement completeness

- [x] CHK001 是否定义了终局未配置与已配置非法两种不同处理语义？ [Completeness, Spec §FR-002–003]
- [x] CHK002 是否完整列出终局 Floor、NodeType、Boss、reachability、out-degree 与 FloorEntrance 约束？ [Completeness, Spec §FR-002]
- [x] CHK003 是否明确摘要全部固定字段及其事务取值时点？ [Completeness, Spec §FR-005, Data Model §3]
- [x] CHK004 是否覆盖终局胜利后的所有玩法写入类别与 Save 例外？ [Coverage, Spec §FR-010]
- [x] CHK005 是否定义正常 UI、Back 与 Screen push failure 的统一交接？ [Completeness, Spec §FR-017]

## Requirement clarity

- [x] CHK006 “同一终局 ticket”是否明确为 active ticket 且 Floor-qualified handle 精确匹配？ [Clarity, Spec §FR-006]
- [x] CHK007 “原子”是否明确包含 working-state、版本、广播、ticket 与事件顺序？ [Clarity, Spec §FR-007–008]
- [x] CHK008 Guardian 胜利优先是否明确覆盖压力线与 mutual destruction，且不钳制最终压力？ [Clarity, Spec §FR-009]
- [x] CHK009 Save v5 的 legacy 字段用途与 v5 权威字段是否无歧义？ [Clarity, Spec §FR-012–014]
- [x] CHK010 下一帧 travel、PrimaryLayout teardown 与幂等范围是否定义清楚？ [Clarity, Spec §FR-017]

## Requirement consistency

- [x] CHK011 Data terminal、Run event Node 与 summary terminal handle 是否使用同一身份？ [Consistency, Spec §FR-001/006/008]
- [x] CHK012 Succeeded 优先语义是否与兼容 `IsRunFailed()` 查询保持一致？ [Consistency, Data Model §2]
- [x] CHK013 v5 Succeeded 必带摘要是否与 runtime success 必生成摘要保持一致？ [Consistency, Spec §FR-005/013]
- [x] CHK014 Screen passive 要求是否与 GameMode 拥有 travel/focus/staging 一致？ [Consistency, Spec §FR-015–017]
- [x] CHK015 零 GameplayTag/Build.cs/asset 修改是否在 spec、plan、tasks 中一致？ [Consistency, Spec §Boundaries]

## Scenario and edge-case coverage

- [x] CHK016 是否覆盖撤离、普通 Boss、Defeat、Undetermined、invalid、stale 和 duplicate ticket？ [Coverage, Spec §US2]
- [x] CHK017 是否定义成功事务中摘要构建失败的 rollback 语义？ [Recovery, Contract §Atomic order]
- [x] CHK018 是否覆盖 legacy Journey 缺 terminal 仍运行但永不自动成功？ [Coverage, Spec §US1]
- [x] CHK019 是否覆盖 v4 inactive migration 与 v5 terminal restore rejection 的区别？ [Coverage, Spec §US3]
- [x] CHK020 是否覆盖 Screen 重复 intent、重复 event 与 stale callback 去重？ [Coverage, Contract app-summary-handoff]

## Acceptance criteria quality

- [x] CHK021 版本一次、广播一次、成功事件一个且最后的指标是否可客观测量？ [Measurability, Spec §SC-002]
- [x] CHK022 反例类别数量与期望零成功/零重复奖励是否可自动化？ [Measurability, Spec §SC-003]
- [x] CHK023 Save migration/roundtrip/invalid/rejection 是否都有可独立运行命名空间？ [Measurability, Spec §SC-005]
- [x] CHK024 UI 三条退出路径是否都有单次 travel 的统一可观察结果？ [Measurability, Spec §SC-006]

## Dependencies and exclusions

- [x] CHK025 是否明确 Production Floor 3 资产缺失只阻塞真实 PIE，不阻塞规则/App 自动化？ [Assumption, Spec §Assumptions]
- [x] CHK026 是否明确失败总结、History、Save 总开关、正式内容资产属于非目标？ [Scope, Spec §Boundaries]
- [x] CHK027 是否明确本轮不改变 AP、Camp、跨层门槛、Command/Resolution schema？ [Scope, Spec §Boundaries]
- [x] CHK028 是否说明 single-player ownership 并明确不新增 replication/RPC？ [Assumption, Plan §Ownership]

## Notes

- 深度：正式 implementation/review gate。
- 范围：静态终局、Run 原子事务、Save v5、CommonUI/App handoff；失败总结与 Production 资产明确排除。
