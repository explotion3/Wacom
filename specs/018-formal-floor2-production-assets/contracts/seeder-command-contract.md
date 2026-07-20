# Contract: Formal Floor 2 content seeder command

## Entrypoints

```text
Commandlet: WacomBuildFormalFloor2Content
Editor:     Wacom.BuildFormalFloor2Content
```

Both invoke the same Floor 2 profile and shared private execution service.

## Arguments

| Argument | Default | Meaning |
|---|---|---|
| `Group=Cards|EnemyGraph|NodeDefinitions|All` | All | selected exact manifest subset |
| `SeedMissing` | false | create only missing packages |
| `CompareSeedDefaults` | false | strict full default comparison |
| `Report=<path>` | Saved profile/group path | JSON evidence path |

Bare and single-leading-dash forms are accepted. Duplicate values, empty report, invalid group and unknown arguments fail parsing. `Force`、`Replace`、`Regenerate` are unknown and rejected.

## Behavior

- Inspect-only never creates or saves a Content package.
- Seed performs complete selected-group preflight before first save.
- Existing correct class is validated and never saved.
- Missing package is created with exact object leaf/class, configured, validated, saved and reloaded.
- Existing wrong class/load failure/structural drift fail closed.
- Mid-save failure stops remaining entries and preserves evidence; no automatic deletion.

## Exit codes

| Code | Category |
|---:|---|
| 0 | success |
| 1 | manifest/preflight/validation/missing inspect |
| 2 | arguments |
| 3 | create/configure/save/reload/report write |

JSON schema stays compatible with Floor 1 schemaVersion 1 and includes group/options/counts/exit/failure plus ordered entry diagnostics.
