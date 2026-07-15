# Contract: Run Exploration Core

## Initialization

```cpp
struct FRunInitializationParams
{
    UCharacterDefinition* Character;
    UWacomJourneyDefinition* Journey;
};

struct FRunInitializationResult
{
    FWacomStatus Status;
    TArray<FRunExplorationEvent> Events;
    FRunExplorationSnapshot PostSnapshot;
    bool IsOk() const;
};

FRunInitializationResult URunSession::Initialize(
    const FRunInitializationParams& Params);
```

- 新 API 是普通 C++ 唯一正式合同，不提供新的 bool/Blueprint compatibility wrapper。
- 完整 working state 成功后一次提交；失败保留旧 Session 全部状态。
- Debug/test callers 通过统一 fixture 获得最小合法 Journey。
- 为保证旧 Blueprint 资产可以在新 Debug Journey 构建并重存前继续加载，现有 `Initialize(UCharacterDefinition*)` 只在迁移阶段保留原实现；禁止新增调用，也禁止把它改造成调用新 API 的 wrapper。Debug Journey 迁移完成后，同一切片迁移全部生产/测试调用并删除该 UFUNCTION。

## Exploration command entry

```cpp
enum class ERunExplorationCommandType : uint8
{
    BeginTraversal,
    CompleteTraversal,
    CancelTraversal,
    MapTravel,
    ChooseNightExploration,
    BeginCamp,
    CancelCamp,
    RequestFloorTransition,
    ConfirmFloorTransition,
    CancelFloorTransition,
};

FRunExplorationResolution URunSession::ResolveExplorationCommand(
    const FRunExplorationCommand& Command);
```

Command 只携带意图需要的稳定身份、expected version 和 opaque token；App 不得提供 SourceNode、TargetNode、成本或生命周期目标值。

## Result invariants

成功结果：

- `Status.IsOk()`。
- `VersionBefore` 必须等于提交前 Session exploration version。
- `VersionAfter = VersionBefore + 1`。
- `PostSnapshot.StateVersion = VersionAfter`。
- Session 当前 Snapshot 与 PostSnapshot 一致。
- Events 已按规则提交顺序排列且只属于本次结果。

失败结果：

- Events 为空。
- `VersionAfter = VersionBefore`。
- Session、Action Point、压力、随机状态、tokens、资产引用均不改变。
- 错误 token、错误 Session、重复、过期和跳版本命令使用明确 Status code。

## Traversal lifecycle

```text
Idle at source
  -> BeginTraversal(EdgeId, ExpectedVersion)
  -> Pending ticket / CurrentNode remains source
  -> CompleteTraversal(Ticket) -> CurrentNode becomes target
  -> CancelTraversal(Ticket)   -> CurrentNode remains source
```

- Begin 仅接受当前节点的出边，且 target 已 Revealed。
- Pending 期间拒绝其它 traversal、MapTravel、node activity、Camp 和 Floor transition。
- Complete 负责 Visited/Resolved 自动规则、邻接 reveal 和 NodeContentRequested event。
- App 启动 Spline 失败必须提交 Cancel，而不是本地遗忘 ticket。

## Map Travel

- 仅当前 Floor、Resolved 目标、空闲状态。
- 免费且不改变压力、时间、lifecycle。
- App 必须在提交前验证 NodeAnchor；成功后使用 PostSnapshot/SceneRelocation event 定位。

## Floor transition

- Request 只创建带版本 confirmation token，不修改 Floor/卡牌/AP。
- Confirm 重新验证 requirements 和 token。
- Cancel 只释放 confirmation token；关闭未来确认 Modal 不会锁死探索。
- 成功不消费卡牌/AP，不清压力，不返回旧 Floor。
- 同一入口首次成功后永久记录 unlocked；重复/过期确认无副作用。

## Notification boundary

- `OnRunStateChangedNative` 保留给现有 ViewModel/provider，每个成功组合事务最多广播一次。
- App 的场景/动画应用以返回 Resolution 为准，不从 Session 拉事件，不建立 presentation queue。
