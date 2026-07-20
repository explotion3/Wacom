# Production Asset Requirements Checklist: Floor 2 Production 47 DataAsset 播种与校验

**Purpose**: Review completeness, clarity and consistency of the asset-authoring requirements
**Created**: 2026-07-19
**Feature**: [spec.md](../spec.md)

## Exact Scope and Identity

- [x] CHK001 Are the exact 47 writable packages, classes and stable IDs defined without wildcard expansion? [Completeness, Spec FR-007]
- [x] CHK002 Are group counts and dependency order quantified as 12/20/15? [Clarity, Spec FR-008]
- [x] CHK003 Are all read-only packages distinguished from writable manifest entries? [Consistency, Spec FR-012]
- [x] CHK004 Are forbidden runtime, scene, presentation and other-agent scopes explicit? [Coverage, Spec FR-016/017]

## Seeder and Ownership Semantics

- [x] CHK005 Is inspect-only versus seed-missing-only behavior unambiguous? [Clarity, Spec FR-005]
- [x] CHK006 Are existing correct class, wrong class, structural drift and missing dependency cases separately specified? [Coverage, Edge Cases]
- [x] CHK007 Are dangerous modes and exact exit categories defined? [Completeness, Spec FR-006]
- [x] CHK008 Is partial-save recovery specified without destructive cleanup? [Recovery, Edge Cases]
- [x] CHK009 Are strict and structural comparator responsibilities consistent with manual tuning ownership? [Consistency, Spec Assumptions]

## Content Contract

- [x] CHK010 Are the 47 class totals and all major nested counts objectively measurable? [Measurability, Spec SC-002/004]
- [x] CHK011 Are explicit branch rewards, Guardian Destroy and MoltSeal Credential requirements defined? [Completeness, Spec FR-010/011]
- [x] CHK012 Are ordered Encounter, Event, Pickup and Shop reference requirements defined? [Completeness, Spec FR-009/012]

## Mutation and Validation Evidence

- [x] CHK013 Is exact MCP identity and per-group allowlist ownership defined? [Traceability, Spec FR-013]
- [x] CHK014 Are dirty-source ownership and prohibited existing-dirty-binary behavior specified? [Safety, Spec FR-013]
- [x] CHK015 Are first-seed, strict inspect and second-run evidence requirements all quantified? [Measurability, Spec FR-014]
- [x] CHK016 Are compile, Automation, AssetRegistry, dependency closure, hash and LFS requirements complete? [Coverage, Spec FR-015]
- [x] CHK017 Are Blueprint/PIE skips and the remaining scene blocker explicitly bounded? [Clarity, Spec FR-018]
- [x] CHK018 Is user review required before any stage/commit and are Git restrictions explicit? [Governance, Spec FR-019/020]
