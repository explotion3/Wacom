# Research: Floor 2 Production 47 DataAsset 播种与校验

## R1. Shared core versus copied Floor 2 builder

**Decision**: 抽取 `WacomEditor/Private` 共享 seed-only execution service；Floor 1/2 各提供 profile 与内容特有 invariant。

**Rationale**: Floor 1 现有实现约 2000 行，其中参数、报告、比较、保存和重载与主题无关。复制会让 no-overwrite 修复和 Floor 3 扩展发生漂移。

**Alternatives considered**: 复制文件改 namespace；初期更快但违反项目复用原则和 Spec 017 明确拒绝项。

## R2. Public and module boundary

**Decision**: 共享服务与 profile 保持 Private 非反射 C++；commandlet/Editor console 是制作入口，`WacomEditor` automation test view 是唯一测试桥。

**Rationale**: 该能力不属于 runtime、Blueprint 或跨模块 gameplay contract；现有 Build.cs 已包含全部依赖。

## R3. Existing asset ownership

**Decision**: inspect-first、seed-missing-only；正确 class 的已存在资产从不保存，structural drift 只报告。

**Rationale**: Production DataAsset 创建后转为人工权威，DisplayName、Description 和批准的平衡字段可继续调优。

**Alternatives considered**: force rebuild、restore defaults 或 whole-theme regeneration；都会覆盖人工内容。

## R4. Comparator policy

**Decision**: strict 模式对比完整 seed default，用于首次落地和第二次幂等验收；structural 模式归一化表现/平衡 tunables 后只守稳定身份、引用与有序规则拓扑。

**Rationale**: 与 Floor 1 已落地制作合同兼容，同时避免把初始平衡值永久锁死。

## R5. Grouping and failure recovery

**Decision**: Cards 12 → EnemyGraph 20 → NodeDefinitions 15 串行；每组先 transient/preflight，再逐包创建保存。中途失败停止、保留证据、不自动回滚已保存资产，后续用 seed-missing-only 续跑。

**Rationale**: UE Package 保存不是跨文件原子事务。禁止自动删除能避免误伤用户资产；依赖顺序确保引用可解析。

## R6. MCP execution without subagent

**Decision**: 主会话负责 lifecycle、AssertReady、writer lease、Git/LFS/hash；若当前会话没有 named Unreal endpoint，用户在已验证 Editor 控制台执行三条命名命令。

**Rationale**: 用户明确禁止继续用 subagent 调用 MCP；console command 与 commandlet 共用同一服务，不降低写入门禁。

## R7. Dirty source before Editor

**Decision**: 因用户要求全部验收后再提交，首次 Start 显式 `-AllowDirty`；启动前记录所有本轮文本/源码 dirty 路径与哈希。目标二进制初始不存在，不使用 `AllowExistingDirtyPackages`。

**Rationale**: 既满足 review-before-commit，也保持 writer 对新 `.uasset` 的精确 allowlist 审计。

## R8. Real-asset validation and PIE

**Decision**: 真实加载、Data Validation、AssetRegistry、failed-load、forbidden closure、hash、LFS 与 Battle/Run smoke 是本轮门禁；PIE 延后。

**Rationale**: 本轮没有 Floor 2 map/Host/Journey，PIE 无法覆盖这些 DataAsset；自动化能稳定验证规则结构。

## R9. Known external BugGirl pollution

**Decision**: 不修改 `DA_Character_BugGirl`，不削弱 validator；若 broad `Wacom.Data` 暴露既有 StarterDeck 失败，作为外部证据记录，不影响 Floor 2 精确闭包结论。

**Rationale**: 用户已明确接受该越界问题，且 Floor 2 manifest 不引用 Character。
