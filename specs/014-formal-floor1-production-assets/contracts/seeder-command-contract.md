# Seeder Command Contract

## Shared service

`WacomEditor` owns one manifest-driven service. The commandlet, Editor console bridge, and tests are adapters around that service; they must not duplicate asset configuration.

## Commandlet

```text
-run=WacomBuildFormalFloor1Content
  [-SeedMissing]
  [-Group=Cards|EnemyGraph|NodeDefinitions|All]
  [-CompareSeedDefaults]
  [-Report=<absolute-or-project-relative-json-path>]
```

- Default mode is inspect-only.
- Default `Group` is `All`.
- `SeedMissing` creates only absent packages. It never saves an existing package.
- `CompareSeedDefaults` adds exact comparison against the initial seed object after structural validation.
- An omitted `Report` writes a deterministic JSON report under `Saved/FormalFloor1Content/`.
- Unknown switches, duplicate incompatible values, or invalid groups are argument errors.

Exit codes:

| Code | Meaning |
|---:|---|
| 0 | Requested inspection or seed completed and every selected entry validates. |
| 1 | Manifest, class, identity, dependency, structural, or strict comparison failure. |
| 2 | Invalid arguments. |
| 3 | Create, save, post-save load, or report-write failure. |

## Editor console bridge

The in-process Editor command uses the same option grammar without the commandlet prefix. Adapter parsing accepts either bare tokens or a single leading `-`, so MCP/UI console invocation and commandlet invocation resolve to identical options:

```text
Wacom.BuildFormalFloor1Content SeedMissing Group=Cards CompareSeedDefaults Report=<path>
```

The equivalent `Wacom.BuildFormalFloor1Content -SeedMissing -Group=Cards -CompareSeedDefaults -Report=<path>` form is also valid. The normalized report records only canonical option names.

The bridge is the formal mutation entry because it runs inside the exact Editor identity protected by the run/8140 MCP writer lease. It logs the final exit code and report path. A nonzero result is a failed group and must not be hidden by the bridge.

## Existing-package behavior

For every selected entry:

1. If the package exists, load it read-only.
2. If its class is wrong or it cannot load, fail before any selected missing package is created.
3. If its class is correct, validate it and record `Existing`; never call `Modify`, copy seed properties, mark dirty, or save it.
4. If it is missing and `SeedMissing` is absent, record `Missing` and return validation failure.
5. If it is missing and `SeedMissing` is present, create it once, configure it from the manifest, save only that package, reload/validate, and record `Created`.

There is no `Force`, `Replace`, `Regenerate`, delete, rename, or repair mode. Partial group failure leaves already saved new packages visible for audit; the service never auto-cleans or overwrites them on retry.

## JSON report

The report includes:

- schema version, timestamp, project path, branch/HEAD supplied by the environment when available;
- mode flags and requested group;
- manifest counts and selected count;
- per-entry package, class, stable identity, state (`Missing`, `Existing`, `Created`, `Failed`), validation tier, saved flag, object path, and diagnostics;
- aggregate created/existing/missing/failed/saved counts;
- final exit code and failure category.

Report creation itself must not dirty a content package. A report-write failure returns exit code 3 even if content saves succeeded, preserving an auditable failure rather than claiming success.
