# Specification Quality Checklist: Floor 1 Production 46 DataAsset 播种与校验

**Purpose**: 在进入实现计划前核对规格的完整性、可测试性与边界
**Created**: 2026-07-18
**Feature**: [spec.md](../spec.md)

## Content Quality

- [x] CHK001 没有把实现细节当作用户价值；场景以审计、播种、调优边界、安全写入和真实资产验证组织
- [x] CHK002 明确 46 个资产、三组数量、四个只读依赖与零地图/Host 范围
- [x] CHK003 区分首次 seed 默认值、后续稳定结构和允许人工调优的字段
- [x] CHK004 明确默认 inspect、`SeedMissing`、strict compare 和错误退出语义
- [x] CHK005 明确 Unreal MCP run/8140、writer lease、allowlist、audit 与关闭后编译门禁

## Requirement Completeness

- [x] CHK006 每条 Functional Requirement 可由自动化、报告、Git/LFS 或 MCP audit 验证
- [x] CHK007 所有 46 资产类型与数量有精确验收标准
- [x] CHK008 已覆盖 missing、wrong class、existing asset、dependency failure、partial save 和 repeat run 边界
- [x] CHK009 已覆盖禁止 overwrite、replace、delete、全量 builder 和非法 Production 引用
- [x] CHK010 已明确退出码、JSON report、group 参数与机器可读结果
- [x] CHK011 已明确 C++ checkpoint、Automation、AssetRegistry、failed-load、hash、LFS 与跳过 PIE 原因
- [x] CHK012 已明确两笔提交与标准交接要求

## Rule and Architecture Alignment

- [x] CHK013 新逻辑归属 WacomEditor，runtime schema/规则模块零变更
- [x] CHK014 不新增 GameplayTag、SaveGame、Build.cs 或模块依赖
- [x] CHK015 默认结构 comparator 与可调数值边界不冲突
- [x] CHK016 FormalProduction Part、八卡映射、24 Intent、13 Choice 与四张 Shop 依赖均有合同入口
- [x] CHK017 Spec 013 Guardian Destroy 表格列错位已显式澄清为 `AllEnemyParts`
- [x] CHK018 长期 Docs 同步点完整，Spec Kit 不被当作长期唯一真相

## Readiness

- [x] CHK019 无待澄清规则问题；用户已确认 seed-only 和“文案与数值可调”口径
- [x] CHK020 实现可按无二进制 C++ 切片与三组资产切片独立验证

## Notes

- 全部条目在规格生成时通过。若 live schema 或最新 main 与 Spec 011/013 不一致，先更新研究/计划记录，再修改实现。
