# Contract: Floor 2 MCP write boundary

## Session identity

```text
Role=run
Endpoint=ue_wacom_run
Port=8140
ThreadId=019f69c0-56e1-7280-b959-6b796da49af0
ProjectRoot=D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-scene-baseline\Wacom
ExpectedBranch=codex/formal-floor2-production-assets
```

Every MCP operation is preceded by `AssertReady`. Editor lifecycle forbids branch/HEAD changes and C++ compile.

## Dirty baseline

User approval requires no commit before final review. `Start -AllowDirty` is permitted only after recording the owner, path and SHA-256 for every current source/test/Spec/Docs change. All 47 target binary packages begin absent and clean; `AllowExistingDirtyPackages` is prohibited.

## Writer groups

Writers are serial and non-overlapping:

1. Cards exact 12 package rows.
2. EnemyGraph exact 20 package rows.
3. NodeDefinitions exact 15 package rows.

The complete lists are copied verbatim in [asset-manifest.md](asset-manifest.md). Directory scans, wildcard allowlists and all-Content leases are forbidden.

## Evidence and release

Before ReleaseWriter record command report, `git status --short`, actual changed paths, per-file SHA-256 and LFS status. Release must produce an audit JSON and reject any allowlist-external change. Failure never triggers automatic clean/reset/delete.

After all groups: run second group seeds, release all writers, close Editor normally, then pass `AssertClosedForBuild` before final compile.
