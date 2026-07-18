# Unreal MCP Write Contract

## Fixed identity

| Field | Required value |
|---|---|
| Role | `run` |
| Endpoint | `ue_wacom_run` |
| Port | `8140` |
| ThreadId | `019f69c0-56e1-7280-b959-6b796da49af0` |
| Branch | `codex/formal-floor1-production-assets` |
| ProjectRoot | `D:\UE_Project\5.7\WacomWorktrees\formal-floor1-production-assets\Wacom` |

`Scripts/Invoke-WacomUnrealMcp.ps1` and `Scripts/UnrealMcp/Endpoints.json` are authoritative. The retired port 8000 server and manual `StartServer` are forbidden.

## Lifecycle gates

1. Before first Editor start in the new worktree: `AssertClosedForBuild`, default Unity `WacomEditor` build, then script `Start` with exact Role, ProjectRoot, and ExpectedBranch.
2. Before every MCP call: `AssertReady`. A responsive port or `IsPIERunning` alone is not identity proof.
3. During an Editor lifecycle: do not switch branch, update HEAD, compile C++, or run another writer.
4. Before compilation: release any writer lease, close Editor normally, then pass `AssertClosedForBuild`.

## Serial writer groups

Each mutation gets a separate lease. The `Packages` allowlist is the complete extensionless package list from [asset-manifest.md](./asset-manifest.md) for that group:

1. Cards: exactly 12 packages.
2. EnemyGraph: exactly 19 packages.
3. NodeDefinitions: exactly 15 packages.

The four existing Shop card dependencies remain read-only and are excluded from all leases. No lease covers `/Game/Wacom/Maps`, Blueprints, materials, art, Debug/Authoring content, or any other Agent-owned asset.

For each group:

1. `AssertReady`.
2. `AcquireWriter` with fixed ThreadId and the full group package allowlist.
3. Invoke the in-process `Wacom.BuildFormalFloor1Content` command through the disposable asset agent/MCP for that exact Editor session.
4. Save only new missing packages in the allowlist.
5. Inspect report, Editor log, actual `git status`, LFS paths, and hashes before releasing the lease.
6. `ReleaseWriter` and retain its audit JSON.
7. Run read-only inspection/Automation before proceeding to the next group.

Whitelist drift or any changed package outside the active group is a fail-closed incident. Nothing is automatically cleaned; stop and report the exact paths.

## Double-run and handoff evidence

- Execute all three groups a second time in the same validated Editor lifecycle; each run must report 0 created and 0 saved.
- Compare first-pass and second-pass SHA-256 for all 46 assets.
- Compare before/after SHA-256 for the four read-only Shop cards.
- Handoff records ProjectRoot, branch/HEAD, Editor PID/start time/SessionId, endpoint/port, three writer audit JSON paths, three build report JSON paths, actual 46 `.uasset` paths, LFS status/fsck, and all validation results.
- No `.umap`, Blueprint, shared card, Enemy material, DreamShader, Backpack, or other Agent-owned binary may change.
