# Production Asset Requirements Quality Checklist

**Purpose**: Review whether the Spec 014 requirements are complete, precise, internally consistent, and ready to drive a controlled 46-package Unreal mutation. This checklist evaluates the written requirements, not the implementation result.

## Scope and authority

- [x] CHK001 The writable scope enumerates exactly 46 extensionless `/Game/...` packages and their required classes. [Completeness, Contract §asset-manifest]
- [x] CHK002 The four existing Shop dependencies are explicitly read-only and excluded from every writer lease. [Boundary, Contract §asset-manifest]
- [x] CHK003 Runtime schema, GameplayTags, SaveGame, Build.cs, modules, maps, Hosts, Blueprints, art, materials, UI, Snake, and TrainingWarrior are explicit non-goals. [Scope, Spec §Non-goals]
- [x] CHK004 The requirements distinguish first-seed authority from later manual authoring authority. [Clarity, Research Decision 1]
- [x] CHK005 The approved manually tunable fields and the stable fields requiring reviewed migration are each enumerated. [Completeness, Validation Contract]

## Seeder behavior

- [x] CHK006 Default execution is unambiguously inspect-only, with `SeedMissing` required for creation. [Safety, Command Contract]
- [x] CHK007 Existing correct-class assets are never modified, dirtied, or saved, including in strict mode. [Safety, Command Contract]
- [x] CHK008 Wrong-class, failed-load, collision, dependency, and partial-save outcomes have defined behavior without automatic cleanup. [Edge cases, Command Contract]
- [x] CHK009 The absence of Force/Replace/Regenerate/delete/repair modes is explicit. [Safety, Command Contract]
- [x] CHK010 Argument grammar, group default, report behavior, and exit codes 0/1/2/3 are defined. [Testability, Command Contract]
- [x] CHK011 Commandlet and Editor console entry are required to share one service and manifest. [Consistency, Plan §Structure]

## Content and dependency topology

- [x] CHK012 Group counts and dependency order are exact: Cards 12, EnemyGraph 19, NodeDefinitions 15. [Measurability, Asset Manifest]
- [x] CHK013 Stable identity/reference closure requirements cover Card, Behavior, Part, Enemy, Encounter, Event, Pickup, and Shop. [Coverage, Validation Contract]
- [x] CHK014 All 11 Parts' Aid/Destroy mapping and null legacy field are specified. [Completeness, Validation Contract]
- [x] CHK015 The 24 Intent, 13 Event Choice, six Encounter, four Pickup, and one Shop structure requirements are measurable. [Testability, Validation Contract]
- [x] CHK016 The historical Guardian Destroy ambiguity is explicitly corrected to AllEnemyParts for target mode and both effects. [Ambiguity resolution, Validation Contract]
- [x] CHK017 Forbidden Debug/Authoring/Test/BadgeDisplayTests/Snake/TrainingWarrior reference closure is defined. [Boundary, Validation Contract]

## Validation and Unreal ownership

- [x] CHK018 General validator, FormalProduction Part validation, Floor 1 structural validation, and optional strict seed comparison have distinct responsibilities. [Clarity, Validation Contract]
- [x] CHK019 Strict comparison is required for initial acceptance but is not misrepresented as the permanent authoring validator. [Lifecycle, Validation Contract]
- [x] CHK020 Automation is explicitly read-only; the only Production writes are the three named MCP writer windows. [Safety, Tasks]
- [x] CHK021 Exact role/endpoint/port/thread/project/branch identity and AssertReady/AssertClosedForBuild gates are specified. [Auditability, MCP Contract]
- [x] CHK022 Each writer lease requires the full group allowlist and fail-closes on paths outside it. [Safety, MCP Contract]
- [x] CHK023 Double-run requires 0 create/0 save and byte-identical hashes for all 46 packages. [Idempotence, MCP Contract]
- [x] CHK024 Binary path, LFS, audit JSON, report JSON, Editor session provenance, build, Automation, AssetRegistry, and failed-load evidence are all required in handoff. [Observability, MCP Contract]

## Delivery and residual risk

- [x] CHK025 Two commits separate source/tooling from binary content and prohibit binary creation before the first commit. [Integration safety, Plan §Commit Strategy]
- [x] CHK026 Blueprint compile and PIE skips are justified by the absence of BP/map/Host production assets, rather than reported as completed validation. [Honesty, Plan §Explicit skips]
- [x] CHK027 Card art/readability, balance, backpack growth, and Production Journey/Floor/map/Host integration remain explicit future risks. [Non-goal/risk, Quickstart]
- [x] CHK028 No requirement claims Floor 1 is playable merely because the 46 Definition assets exist. [Scope integrity, Spec §Success Criteria]

## Notes

- Checklist reviewed against `spec.md`, `plan.md`, `research.md`, `data-model.md`, all four contracts, and `tasks.md` on 2026-07-18.
- Checked means the requirement is adequately documented; it does not mean the corresponding asset or test already exists.
