# Contract: Floor 1 Content Production Readiness

## Closed by this documentation slice

- Exact Floor 1 node content definitions are designed: 6 Encounter, 4 RunEvent, 4 Pickup, 1 Shop.
- Four enemy archetypes, eleven parts, four behaviors and all 24 intents are frozen.
- Four new cards, pickup mapping, fixed shop inventory and four events/13 choices are frozen.
- Route economy preserves a purchase path from both first branches.
- Floor 1 AP remains `8–9 / 14–15`.
- A 38-package Production manifest and strict future builder write set exist.

This closes the **Floor 1 content design** blocker only.

## Still blocked after this slice

1. **Binary implementation**: none of the 38 DataAssets exists yet.
2. **Data Validation and runtime smoke**: future assets have not been loaded, validated or executed.
3. **Knockdown branch P0**: new PartDefinitions intentionally have null reward; Aid/Destroy/Withdraw effects remain unresolved.
4. **World asset authority**: Production map/package source must be selected after AssetRegistry, reference and hash audit.
5. **Scene content**: Host Blueprints, multi-enemy staging, art, animation and formal world placement are not delivered.
6. **Production Golden Path**: no real Floor 1 DataAsset/map/PIE path exists yet.

## Next implementation exit criteria

Before Production Journey/Floor/map creation may proceed:

- create exactly the 38 manifest assets without modifying unrelated packages;
- compile `WacomEditor` if builder/validator/test C++ changes;
- run all affected `Wacom.Data.*`, `Wacom.Battle.*`, RunEvent/Pickup/Shop tests and generated-content validation;
- run the dedicated content builder twice, auditing semantic stability, duplicates, dirty packages and protected dependency hashes;
- run AssetRegistry/reference/failed-load audit proving every manifest asset exists and no forbidden dependency enters the closure;
- decide or explicitly keep blocked the knockdown three-branch production rule;
- separately approve Production map authority before any `uasset/umap` migration or formal scene authoring.

## Forbidden shortcuts

- Reusing a debug event/shop/pickup/card as a Production node payload.
- Referencing a test card or badge display fixture in Wayfarer.
- Referencing TrainingWarrior or the legacy Snake enemy as a SerpentWood archetype.
- Treating `KnockdownRewardCard=null` as an approved final Aid/Destroy rule.
- Creating empty DataAssets only to satisfy path/count checks.
- Running a full content/material/card rebuild that can overwrite manual tuning.
- Editing `L_Exploration` or any formal/Debug map before the world asset-authority audit.
- Claiming compile, Automation, AssetRegistry, Blueprint or PIE coverage from this documentation-only slice.

## Runtime regression statement

This slice has zero runtime regression surface because it changes no source, Config, module, tag, asset byte or map. Its risk is design transcription drift; static cross-artifact audits mitigate that risk. Runtime/content correctness remains unproven until the 38 assets are implemented and validated.
