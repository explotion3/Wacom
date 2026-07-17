# Contract Quality Checklist: 击倒分支奖励合同基线

**Purpose**: 在实现前验证 Data、Battle、UI 与 Production migration 合同是否完整、可测试且边界一致。
**Created**: 2026-07-17
**Feature**: [spec.md](../spec.md)

## Data 与兼容性

- [x] CHK001 是否明确规定 Aid/Destroy 新字段优先、legacy fallback 及 Withdraw/None 永远为空？[Completeness, Spec FR-001–FR-003]
- [x] CHK002 是否区分 General 与 FormalProduction 校验，并覆盖无奖励、legacy-only、new-only 与混填？[Coverage, Spec US1]
- [x] CHK003 是否明确 legacy 字段的删除前置条件，而不是给出无期限兼容承诺？[Lifecycle, Spec FR-015]
- [x] CHK004 是否明确 builder 只改变未来写入源码，禁止本轮执行或重存现有资产？[Scope, Spec FR-005]

## Battle 原子结算

- [x] CHK005 是否规定 Resolver 与 ViewData 必须使用同一查询，避免奖励预览与实际结算漂移？[Consistency, Spec FR-006/FR-010]
- [x] CHK006 是否覆盖空奖励、手牌上限、连续击倒、Withdraw、Victory 与 Defeat 的现有语义？[Edge Cases, Spec US2/SC-006]
- [x] CHK007 是否明确只授予所选分支、`SourceChoice` 保留来源且不改变 ResultPacket/Run/Save schema？[Compatibility, Spec FR-006/FR-016]
- [x] CHK008 是否防止奖励缺失反向改变选择可用性？[Separation of Concerns, Spec FR-007]

## 被动 UI

- [x] CHK009 ViewData 是否只暴露存在性、稳定 ID 与显示名，不暴露可写 DataAsset 指针？[Boundary, Spec FR-009]
- [x] CHK010 是否定义显示名 fallback、空奖励文案和连续 `SetContext` 刷新行为？[Completeness, Spec US3/Edge Cases]
- [x] CHK011 是否明确 UI 不自行判断奖励合法性、不改变 Modal 焦点/Back/命令流程？[Separation of Concerns, Spec FR-012]
- [x] CHK012 是否要求新建小型 Dialog spec 而非继续扩张既有巨型测试文件？[Maintainability, Plan Checkpoint 3]

## Production 与交付

- [x] CHK013 是否枚举 4 个 Archetype × 2 个分支的 8 个稳定 ID 与四个主题目录？[Traceability, Production migration contract]
- [x] CHK014 是否把数量表述为“38 个核心资产 + 8 张分支奖励卡”，并明确本轮不设计数值、不创建资产？[Scope, Spec FR-013/FR-014]
- [x] CHK015 是否为每个 C++ checkpoint 定义编译与聚焦测试，并包含最终 AssetRegistry、Blueprint、哈希、Git/LFS 审计？[Verification, Spec FR-018/FR-019]
- [x] CHK016 是否明确禁止 GameplayTag、SaveGame、Build.cs、模块依赖和二进制资产变化？[Non-Goals, Spec FR-016/SC-009]

## Notes

- 用户给出的实现计划已消除本轮所有会改变规则或资产语义的开放决策。
- 未完成的 Aid/Destroy 其它效果与八张卡具体数值继续保留在长期 Docs，不阻塞本合同基线。
