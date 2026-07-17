# Contract: SaveGame v4 Credential schema

## Version

- `UWacomSaveGame::CurrentSaveVersion == 4`。
- 未来版本继续使用逐版本迁移链；本轮新增明确 `case 3`。

## Disk representation

```cpp
UPROPERTY()
TArray<FName> GrantedCredentialIds;
```

- 写入前从运行时集合复制并稳定排序。
- 数组不得包含 `NAME_None` 或重复项。
- 数组顺序不是业务语义，只服务确定性磁盘输出。

## Migration

```text
v3 -> v4: GrantedCredentialIds = []
```

不得从保存卡牌、InstanceId、DestroyedTriggerIds、Actor 或场景状态推断 Credential。

## Apply atomicity

- 先迁移，再校验，再构建临时 RunState。
- 空或重复 Credential 使整个 Apply 失败。
- 失败时当前 RunState、revision 和通知次数不变。
- 成功后运行时恢复为 `TSet<FName>`。

## Scope boundary

SaveGame 总开关继续关闭。本轮只扩展 schema，不宣称金币、时间、探索图、Pickup 完成等其它 RunState 已完整持久化。
