# Quickstart: 正式 Floor 1 设计冻结验证

## 1. Workspace preflight

```powershell
$Project = 'D:\UE_Project\5.7\WacomWorktrees\run-level-authoring-baseline\Wacom'
Set-Location $Project
git branch --show-current
git status --short --branch
git lfs status
```

Expected:

- Branch is `codex/run-level-authoring-baseline`.
- Before edits the worktree and LFS status are clean.
- This feature changes only Markdown plus `.specify/feature.json` and the managed AGENTS Spec Kit pointer.

An unrelated `UnrealEditor-Cmd` from another worktree does not block this documentation slice. Do not terminate it and do not run a builder.

## 2. Canonical artifacts

- Requirements: [spec.md](./spec.md)
- Design decisions: [research.md](./research.md)
- Canonical Node/Edge tables: [data-model.md](./data-model.md)
- Stable identity contract: [contracts/stable-identity-contract.md](./contracts/stable-identity-contract.md)
- Graph/content contract: [contracts/floor1-graph-content-contract.md](./contracts/floor1-graph-content-contract.md)
- Production blockers: [contracts/production-readiness-gate.md](./contracts/production-readiness-gate.md)

## 3. Static acceptance matrix

| Check | Expected |
|---|---|
| Journey identities | 1 Journey + 3 ordered Floors |
| Floor 1 nodes | 20 unique NodeIds |
| Floor 1 edges | 21 unique EdgeIds |
| Node type counts | 4 Navigation / 6 Encounter / 4 RunEvent / 4 Treasure / 1 Shop / 1 FloorEntrance |
| Reachability | All 20 reachable from `Node.Entry` |
| First merge | Route A/B end at `Node.Junction.02` |
| Second merge | Route C/D end at `Node.Key.01` |
| Key dominance | Guardian and Exit unreachable when Key is removed |
| Camp | All 4 Navigation nodes allow Camp |
| Default layout | Finite, inside 1920×1080, no overlaps |
| Critical path | 8 AP without Shop purchase, 9 with purchase |
| Full exploration | 14 AP without Shop purchase, 15 with purchase |
| Debug references | 0 references in Production content tables |

## 4. Identity and boundary audit

```powershell
rg -n "Journey\.Main\.01|Floor\.Main\.0[123]|Card\.Run\.SerpentSigil" `
  specs/007-formal-floor1-content-freeze Docs/WacomMap.md Docs/WacomDataAuthoring.md

rg -n "Journey\.Authoring|Floor\.Authoring\.01|DA_.*Debug|/Debug/" `
  specs/007-formal-floor1-content-freeze/contracts `
  specs/007-formal-floor1-content-freeze/data-model.md
```

The second command may find explicit forbidden-reference explanations in `production-readiness-gate.md`; it must not find any Production content table mapping to those assets.

Confirm no implementation files changed:

```powershell
git diff --name-only | rg "^(Source|Config|Content)/"
```

Expected: no output.

## 5. AP audit

Shortest approved route uses one first branch and Route C:

```text
Main Encounter 1
+ first branch 3
+ Route C Event 1
+ optional first Shop purchase 0 or 1
+ Key 1
+ Elite 1
+ Guardian 1
= 8 or 9 AP
```

Full exploration:

```text
Main Encounter 1
+ Route A 3
+ Route B 3
+ Route C 1 or 2
+ Route D 3
+ Key/Elite/Guardian 3
= 14 or 15 AP
```

## 6. Production blocker audit

Before any future `uasset/umap` work, all answers must be Yes:

- [ ] Can `Card.Run.SerpentSigil` no longer be permanently lost, or can it be deterministically recovered?
- [ ] Do `Floor.Main.02` and `Floor.Main.03` have approved valid graphs?
- [ ] Do all 15 content slot IDs resolve to non-Debug Production definitions?
- [ ] Has the production map authority been selected after AssetRegistry/reference/hash audit?

For this slice every box intentionally remains unchecked. The design freeze can complete; Production asset implementation cannot start.

## 7. Final documentation validation

```powershell
rg -n "\[NEEDS CLARIFICATION|\[FEATURE|\[DATE|\[###|\$ARGUMENTS" `
  specs/007-formal-floor1-content-freeze/spec.md `
  specs/007-formal-floor1-content-freeze/plan.md `
  specs/007-formal-floor1-content-freeze/tasks.md `
  specs/007-formal-floor1-content-freeze/research.md `
  specs/007-formal-floor1-content-freeze/data-model.md `
  specs/007-formal-floor1-content-freeze/contracts

git diff --check
git status --short
git lfs status
```

Expected:

- No unresolved template markers.
- `git diff --check` succeeds.
- No `uasset/umap` or LFS changes.

## 8. Skipped Unreal validation

| Validation | Status | Reason | Remaining risk |
|---|---|---|---|
| WacomEditor compile | Skipped | No source, Build.cs, Config or reflection change | None for this documentation slice |
| Automation | Skipped | No runtime or validator behavior changed | Static design has not yet been encoded as assets |
| AssetRegistry / Blueprint compile | Skipped | No asset created or modified | Future Production asset slice must run both |
| PIE | Skipped | No formal Floor 1 world exists | Future scene slice must validate traversal/content return flows |
| Binary hashes | Not applicable | No binary migration/write | Git diff must remain free of Content changes |

## 9. Validation record — 2026-07-17

- Branch preflight: PASS, `codex/run-level-authoring-baseline` at `a9c693828e6388638ddd1c3b49ec5c43b9981bd3`.
- Initial worktree/LFS: PASS, clean.
- Related editor process: one `UnrealEditor-Cmd` belongs to `C:\Users\ahhh\.codex\worktrees\1171\Wacom` DreamShader compile; not touched.
- Spec quality checklist: PASS, no clarification markers.
- Node/Edge/type/reachability/dominance/layout/AP audits: PASS — 20 Nodes、21 Edges、20 reachable；类型分布为 4/6/4/4/1/1；四个 Navigation 均允许 Camp；Key 支配 Guardian/Exit；坐标均在 1920×1080 内且不重合；AP 为 8–9 / 14–15。
- Identity/content boundary audit: PASS — Journey/Floor/Node/Edge/Card ID 非空且唯一；15 个 Production 内容槽不映射 Authoring/Debug 资产；Host PersistentId 只按 `<FloorId>.<NodeId>` 派生。
- Cross-artifact analysis: PASS — 12 条 FR、7 条 SC、21 个任务，FR task coverage 为 100%；未发现 Critical/High/Medium 冲突。Spec Kit 前置脚本因当前合法分支名不匹配其数字 feature-branch 约定而拒绝运行，已在不改 branch 的前提下完成同等人工矩阵审阅，并修正 Floor 2/3 readiness 措辞。
- Long-term Docs sync: PASS — `Docs/WacomMap.md`、`Docs/WacomDataAuthoring.md`、`Docs/TODO.md` 与 `Docs/Questions.md` 已同步；`Docs/Architecture.md` 无架构事实变化，无需修改。
- Placeholder/non-target audit: PASS — 无模板残留，无 `Source/`、`Config/`、`Content/`、`.uasset/.umap` 或 LFS 变更。
- Unreal validation: intentionally skipped per §8.
