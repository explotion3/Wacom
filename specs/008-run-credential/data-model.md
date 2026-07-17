# Data Model: Run 持久任务凭证

## 1. Stable identity

| Field | Type | Owner | Stability | Rule |
|---|---|---|---|---|
| `CredentialId` | `FName` | Content + WacomRun | Stable | 非空、区分大小写按 FName 语义、不可由卡牌或资产路径反推 |
| 蛇印 Credential | `FName` | Production content | Frozen | `Credential.Run.SerpentSigil` |
| 蛇印表现卡 | Existing CardId | WacomData content | Separate | `Card.Run.SerpentSigil`，不是授权真相 |

## 2. Static authoring data

### `UWacomRunPickupDefinition`

新增：

```text
GrantedCredentialIds: TArray<FName>
```

- 允许空数组：现有 Gold/Card Pickup 行为不变。
- 非空项必须唯一且不得为 `NAME_None`。
- 一个 Pickup 可授予多个 Credential。
- 主奖励仍由 `RewardType`、`GoldAmount`、`CardDefinition` 定义；Credential 是附加原子 grant，不是新的主奖励类型。

### `FWacomMapFloorEntrancePayload`

新增：

```text
RequiredCredentialIds: TArray<FName>
```

- 允许空数组：现有入口行为不变。
- 非空项必须唯一且不得为 `NAME_None`。
- 列表内全部满足；再与 `OwnedCardRequirements` 采用 AND。
- 条件非消耗。

## 3. Runtime state

### `FRunState::GrantedCredentialIds`

```text
TSet<FName> GrantedCredentialIds
```

Invariants:

1. 不包含 `NAME_None`。
2. 集合天然唯一。
3. 本轮只有 grant，没有 revoke/reset 之外的删除 API。
4. 不从 Backpack、BattleDeck、BurdenZone、SpecialZones、CollectedPickupIds 或 Actor 状态重算。
5. RunSession 对外只提供 `HasCredential(FName)` 查询；普通 UI 无写入口。

## 4. Atomic Pickup state transition

Input:

```text
PersistentId + UWacomRunPickupDefinition
```

Preconditions:

- `PersistentId != NAME_None`
- Definition 非空且静态配置有效
- `PersistentId` 尚未收集
- Treasure 节点活动/行动点等现有结算前置有效
- 所有 `GrantedCredentialIds` 非空唯一

Working-state transition:

```text
OldState
  -> apply Gold or Card main reward
  -> add each GrantedCredentialId idempotently
  -> add PersistentId to CollectedPickupIds
  -> settle Treasure node and AP through existing exploration contract
  -> commit one NewState + one notification
```

任一步失败：丢弃 working state；Gold/Card/Credential/Pickup/Node/AP/revision/通知均不改变。

重复 grant 不是失败；集合保持一份，主奖励与 Pickup 结算继续执行。

## 5. FloorEntrance evaluation

```text
bRequirementsMet =
  bAlreadyUnlocked
  OR (
    HasAll(RequiredCredentialIds)
    AND MatchesAll(OwnedCardRequirements)
  )
```

- RequiredCredentialIds 为空时 `HasAll == true`。
- 只有同名表现卡而没有 Credential 时不满足。
- Credential 不因通过入口被消费。
- Request 与 Confirm 之间再次根据当前 RunState 求值。

## 6. SaveGame v4

### Disk field

```text
TArray<FName> GrantedCredentialIds
```

Write:

- 从 RunState 集合复制。
- 按稳定词法顺序排序。
- 不写空值。

Read:

- 版本迁移成功后校验。
- 任一 `NAME_None` 或重复项导致 Apply 整体失败。
- 校验通过后写入临时 `FRunState`，最后一次替换权威状态。

Migration:

```text
v3 -> v4:
  GrantedCredentialIds = []
  SaveVersion = 4
```

禁止根据任何已保存卡牌反推。

## 7. Static validation relationships

对每个入口 `E` 和每个 `RequiredCredentialId C`：

1. 找到 Journey 当前 Floor 或允许的前置 Floor 中，固定 `UWacomRunPickupDefinition` 声明 `C` 的 Pickup node。
2. 当前 Floor 来源节点必须支配 `E`：移除来源节点后，Entry 不得仍可达 E。
3. 可绕过分支来源不构成保证来源。
4. 未声明 Credential 的 Debug Pickup 不参与保证来源。

## 8. Compatibility matrix

| Existing content/state | New field default | Result |
|---|---:|---|
| Debug Gold Pickup | `[]` | Gold 行为不变 |
| Debug Card Pickup | `[]` | Card 行为不变 |
| Existing FloorEntrance card conditions | `[]` | 继续只按 OwnedCardRequirements |
| New credential-only entrance | non-empty | 只按 Credential |
| Mixed entrance | non-empty + card requirements | 两组 AND |
| v3 SaveGame | field absent | 迁移为空集合 |

## 9. Explicit non-state

- Credential 不进入 Battle Snapshot/Command/Resolution。
- 不新增通用 UI Snapshot 或 Credential 列表。
- 不新增 GameplayTag。
- 不保存/推导 Actor、MapPosition、Transform 或资产路径。
- 本轮不补齐其它 RunState SaveGame 字段。
