# Contract: Floor 1 Knockdown Reward Production Readiness

## Closed by this documentation slice

- Eight branch reward cards have exact IDs, packages, names, descriptions, costs, rarities, keywords, targets and effects.
- Eleven PartDefinitions have exact Aid/Destroy reference mappings and null legacy requirements.
- Reward frequency and Floor 1 route yields are explicit: `14–17` critical path and `20` full exploration.
- Spec 011's Production count is fully specified as `38 core + 8 branch reward = 46`.
- The eight-card content design and package-leaf blocker is closed.

## Still blocked after this slice

1. None of the 46 DataAssets is created.
2. No SerpentWood builder or FormalProduction content test exists for the full package set.
3. No Data Validation, AssetRegistry, reference closure, failed-load, hash or dirty-package proof exists for these future assets.
4. Card art, audio, CardView presentation and balance-in-play remain unverified.
5. Reward volume may create backpack/collection pressure; current overflow behavior remains an open product question.
6. Production Journey/Floor DataAssets, world asset authority, Hosts, map and Golden Path PIE remain separate gates.
7. Non-card Aid/Destroy/Withdraw consequences and possible final-part exceptions remain open questions.

## Future controlled write set

- Core set: the exact 38 package paths from Spec 011.
- Increment: the exact 8 package paths from [card-manifest.md](./card-manifest.md).
- Total: exactly 46 new Production DataAssets.
- Existing Starter cards and `PoisonFang` remain read-only dependencies.
- Debug/Authoring/Test/BadgeDisplayTests/TrainingWarrior/legacy Snake assets are not valid Production dependencies.

## Next implementation exit criteria

- implement a dedicated or strictly scoped SerpentWood content builder/fixture whose write set is exactly 46 packages;
- create all cards before assigning Part references, then create Parts/Behaviors/Enemies/Encounters and node Definitions in dependency order;
- run default Unity `WacomEditor` compile for C++ builder/validator/test changes;
- run affected Data, Battle knockdown, Run reward, Pickup/Shop/RunEvent and generated-content tests;
- run builder twice and prove stable IDs, fields, references, counts, no duplicates and no unexpected dirty packages;
- execute FormalProduction validation for all eleven Parts and semantic reload validation for all 46 packages;
- audit AssetRegistry dependency closure, failed loads, protected dependency hashes and LFS state;
- perform card/readability/balance and backpack-volume PIE after formal content can be entered through a Production or controlled test path;
- keep map/Host creation in a separate authorized scene-authoring slice.

## Forbidden shortcuts

- Creating per-Part or per-Node reward card pairs beyond the eight Definitions.
- Filling only legacy `KnockdownRewardCard` on any FormalProduction Part.
- Using Debug/Test/TrainingWarrior content as a placeholder reference.
- Running a full content/card/material rebuild that can overwrite manual tuning.
- Creating empty assets only to satisfy count/path checks.
- Claiming runtime, AssetRegistry or PIE success from this text-only slice.

## Runtime regression statement

This slice changes no runtime code, Config, schema, tag, module, asset byte or map. Runtime regression surface is zero. The remaining risk is content balance and transcription during future asset generation; the manifest and static audits reduce, but do not eliminate, that future risk.
