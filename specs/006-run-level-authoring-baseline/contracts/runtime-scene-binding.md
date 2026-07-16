# Contract: Runtime Run Floor Scene Binding

## Public authoring surface

```cpp
UCLASS(Blueprintable)
class WACOMAPP_API AWacomRunFloorSceneDescriptorActor : public AActor
{
    GENERATED_BODY()

public:
    AWacomRunFloorSceneDescriptorActor();

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Wacom|Run|Floor",
        meta = (ToolTip = "当前关卡实现的唯一 Floor 数据。一个独立 Run Floor 关卡必须且只能放置一个声明 Actor；不得为空。"))
    TObjectPtr<UWacomFloorMapDefinition> FloorDefinition;

    const UWacomFloorMapDefinition* GetFloorDefinition() const;
};
```

The actor:

- has Tick disabled;
- has no collision or interaction contract;
- is hidden in game;
- exposes no Blueprint setter or Run command;
- stores no node graph or target world.

## App-private resolution

`FWacomRunFloorSceneDescriptorResolver` consumes a loaded `UWorld` and optional expected `FloorId` from the explicit Run snapshot. It returns exactly one of:

- resolved descriptor + Floor definition + matching FloorId;
- stable rejection: invalid world, missing, duplicate, null definition, empty identity, identity mismatch.

The resolver never mutates the World, Session, Descriptor or DataAsset.

## Atomic PlayerController refresh

`AWacomPlayerController::RefreshRunExplorationPresentationBinding()` follows this order:

1. Read one current `FRunExplorationSnapshot` and capture its version/FloorId.
2. Resolve and validate the World Descriptor against that snapshot.
3. Create a new working `FWacomRunSceneBindingRegistry` scoped to the resolved FloorId.
4. Enumerate scene Anchor/Path/Branch/activity hosts into the working registry.
5. Revalidate the Run snapshot version, active Floor and current Node before commit.
6. Prepare a working presentation coordinator against the complete working registry without touching the installed generation.
7. Tear down the previous generation, install the working registry/coordinator pair, and commit the prepared initialization once.

Any failure before step 7:

- returns `false` and a stable log detail;
- does not reset or partly populate the installed registry;
- does not submit a Run command or alter the Run snapshot;
- does not move the camera, change HUD mode or suppress/reveal cards as a recovery side effect.

World teardown may separately invalidate weak scene objects, but failure still cannot install a partial replacement.

## Scene actor identity

- Anchor remains bound by `NodeId`.
- Path remains bound by `EdgeId` and must resolve the declared source/target nodes.
- BranchTarget remains a choice presentation for an existing legal Edge; it does not determine graph legality.
- Activity host remains bound to a node identity and node type/payload contract.
- Descriptor selects the Floor; individual scene actors do not repeat or override FloorId.
