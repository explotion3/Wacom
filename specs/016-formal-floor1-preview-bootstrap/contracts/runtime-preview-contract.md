# Runtime Preview Contract

## Public seam

```cpp
virtual UWacomJourneyDefinition* ResolveJourneyDefinitionForNewRun();
```

- Declared on `AWacomGameMode` without `UFUNCTION`, Blueprint exposure, or writable Preview state.
- Base implementation returns `DefaultJourneyDefinition` unchanged.
- `AWacomPlayerController` calls it once for each attempted new `URunSession` initialization and uses the returned pointer in `FRunInitializationParams::Journey`.
- Initialization diagnostics name the actual resolved Journey pointer, not the backing property.

## Preview subclass

`AWacomRunFloorPreviewGameMode` is Blueprintable for normal GameMode class configuration. Its journey resolver has these gates in order:

1. Editor build is present.
2. `GetWorld()` is valid and `WorldType == EWorldType::PIE`.
3. `DefaultCharacter` is valid.
4. The existing App-private Descriptor resolver returns exactly one Descriptor with one valid, non-empty Floor.
5. If a Preview Journey is cached, Descriptor pointer, Floor pointer, FloorId, and Journey Floor pointer all still match.

Failure returns null and creates/replaces no Journey.

First success creates:

```text
Outer        = Preview GameMode
Flags        = RF_Transient
JourneyId    = Journey.Preview.<FloorId>
DisplayName  = [Preview] <Floor DisplayName or FloorId>
Floors       = [exact Descriptor Floor]
Characters   = [DefaultCharacter]
Terminal     = empty
AP/Decay     = UWacomJourneyDefinition defaults
```

Repeated success returns the same object address.

## Invariants

- Preview never mutates Descriptor, Floor, nodes, edges, character, or `DefaultJourneyDefinition`.
- Preview never creates or saves a DataAsset package.
- Preview never attempts Floor transition or Journey success synthesis.
- Non-PIE worlds cannot initialize through this subclass, even if the Blueprint is selected accidentally.
- Existing Authoring/Debug/Production base GameMode behavior remains byte-for-byte equivalent at the seam: `DefaultJourneyDefinition` is still the returned input.

## Stable diagnostics

Logs must identify the rejecting category and useful object/identity facts. Categories include:

```text
PreviewWorldNotPIE
PreviewCharacterMissing
DescriptorWorldInvalid
DescriptorMissing
DescriptorDuplicate
DescriptorFloorMissing
DescriptorFloorIdMissing
PreviewDescriptorDrift
PreviewFloorDrift
```

The existing Descriptor resolver detail names remain authoritative where applicable.
