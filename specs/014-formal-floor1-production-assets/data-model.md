# Data Model: Floor 1 Production 46 DataAsset 播种与校验

## 1. Manifest entry

每条 `FFormalFloor1AssetManifestEntry`（实现保持 Editor-private 非反射类型）包含：

| Field | Meaning |
|---|---|
| `PackagePath` | 完整 `/Game/...` package，无扩展名 |
| `AssetName` | 与 package leaf 一致的 UObject name |
| `AssetClass` | 期望 DataAsset class |
| `StableId` | CardId/PartId/BehaviorId/EnemyId/EncounterDefinitionId/EventId/PickupId/ShopId |
| `Group` | Cards / EnemyGraph / NodeDefinitions |
| `Dependencies` | 必须在创建本条前有效的 manifest 或 read-only package |
| `ConfigureExpected` | 填写首次 seed 完整 editable 字段 |
| `ValidateStableStructure` | 比较长期稳定身份、引用和规则形状 |

Manifest 不使用 UObject path 推导 stable ID，不使用 DisplayName 选择类型，也不暴露给 runtime。

## 2. Build options

```text
Mode                  Inspect | SeedMissing
Group                 Cards | EnemyGraph | NodeDefinitions | All
bCompareSeedDefaults  false by default
ReportPath            optional; defaults under Saved/FormalFloor1Content
Invocation            Commandlet | EditorConsole | AutomationTest
```

未知 Group、重复冲突参数、Content 内 ReportPath 或不安全绝对路径均为 argument failure。

## 3. Package state

| State | Meaning | Saves? | Success? |
|---|---|---:|---:|
| `Missing` | inspect 发现不存在 | no | no |
| `Created` | SeedMissing 新建并保存、reload 成功 | yes | yes |
| `Present` | existing class 正确且结构通过 | no | yes |
| `WrongClass` | package/object 存在但 class 不匹配 | no | no |
| `MissingDependency` | 外部或前置引用缺失/非法 | no | no |
| `InvalidStructure` | shared/exact validation 失败 | no | no |
| `SeedDrift` | strict expected 不同 | no | no in strict mode |
| `SaveFailed` | create/save/reload 失败 | attempted | no |
| `NotProcessed` | 早先 fatal error 后停止 | no | no |

## 4. Build report

JSON schema v1：

```json
{
  "schemaVersion": 1,
  "mode": "SeedMissing",
  "group": "Cards",
  "compareSeedDefaults": true,
  "succeeded": true,
  "exitCode": 0,
  "counts": {
    "expected": 12,
    "present": 0,
    "created": 12,
    "saved": 12,
    "valid": 12
  },
  "savedPackages": ["/Game/..."],
  "packages": [
    {
      "packagePath": "/Game/...",
      "assetName": "DA_...",
      "class": "/Script/WacomData.CardDefinition",
      "stableId": "...",
      "group": "Cards",
      "state": "Created",
      "saved": true,
      "errors": [],
      "warnings": []
    }
  ],
  "errors": [],
  "warnings": []
}
```

Report 写入失败属于 create/save/report failure（exit 3），但不删除已保存资产。

## 5. Stable versus tunable matrix

| Asset | Stable structure | Tunable after seed |
|---|---|---|
| Card | package/class/CardId、keywords、TargetMode、Effect count/order/type/target/target-zone、required empty advanced fields | name/description、cost、rarity、magnitudes |
| Behavior | BehaviorId、InitialPhaseId、phase/set/intent IDs、slot binding、Sequence、effect count/order/type/target、empty rules/cooldowns/fallback | intent display text、initiative、resistance、effect magnitude、hand-affliction numeric count |
| Part | PartId、Aid/Destroy exact refs、legacy null | display text、HP、EXP |
| Enemy | EnemyId、DefaultBehavior/Phase、part slot order/ref/intent-set、null override | display text |
| Encounter | EncounterDefinitionId、enemy slot order/id/ref | display text |
| Event | EventId、start/node/choice IDs、condition/effect count/order/type、flag/pressure identity、policy、next/terminal/payment structure | display/node/choice text、condition/effect numeric values |
| Pickup | PickupId、RewardType、card ref、credential set/order | unused GoldAmount |
| Shop | ShopId、offer card order | display text、prices |

Rarity 被归入可调数值/平衡字段，但仍必须是当前 Card rarity schema 允许值；Keyword 是稳定功能分类。

## 6. Seed text defaults

这些文本用于首次播种，不属于稳定身份；后续允许人工改写。

### Core cards

| CardId | Seed description |
|---|---|
| `Reward.SerpentWood.HerbalPoultice` | `恢复 {Effect.0} 点生命。` |
| `Reward.SerpentWood.HunterSnare` | `使一个敌方部位的当前意图延后 {Effect.0} 点先机。` |
| `Reward.SerpentWood.MoltWard` | `获得 {Effect.0} 点护盾。` |
| `Card.Run.SerpentSigil` | `从抽牌堆抽取 {Effect.0} 张牌。` |

八张分支卡使用 Spec 013 `contracts/card-manifest.md` 的描述模板；Guardian Destroy 的 TargetMode 按 AllEnemyParts 纠正。

### Enemy/Part/Intent display defaults

- Enemy: 林地伏蛇、蛇蜕守卫、盘根伏蛇、浅巢守卫。
- Parts: 头部、躯体、甲壳、尾部、盘身、冠鳞，按各 archetype 组合使用。
- Intent: 使用冻结 suffix 的中文显示词；规则从 IntentId/Effects 读取，不解析文本。

### Event seed prose

| Event | Display / Node title | Body intent | Choice labels in frozen order |
|---|---|---|---|
| CastSkin | 蛇蜕事件 / 林间蛇蜕 | 林地里留着一张尚有余温的完整蛇蜕。 | 研究纹路；卖掉蛇蜕；原样留下 |
| HunterTrace | 猎人痕迹 / 泥地遗迹 | 毒雾边缘散落着猎人的行囊与断裂足迹。 | 辨认足迹；搜走行囊；掩埋遗骸 |
| MerchantRumor | 行商情报 / 林下行商 | 行商压低声音，等你拿情报、金币或风险交换路线。 | 交换蛇蜕线索；购买地图；偷听传闻；婉拒 |
| PoisonMarsh | 毒沼抉择 / 毒沼边缘 | 沼气遮住前路，标记、供品或硬闯都能带你穿过。 | 沿标记路线；焚烧供品；涉水硬闯 |

## 7. State transitions

```text
Inspect
  -> load manifest/dependencies
  -> Missing | Present | Invalid*
  -> validate/report only

SeedMissing
  -> preflight whole group dependencies/classes
  -> for each entry in dependency order
       Missing -> create expected -> save -> unload/reload -> validate -> Created
       Present -> validate only -> Present
       Invalid -> stop; preserve prior saved packages; report remainder NotProcessed
```

`CompareSeedDefaults` 在结构 validation 后执行，不改变状态数据。

## 8. Runtime/persistence impact

46 个 DataAsset 是静态内容。没有新增 runtime state、Snapshot、Command、Event、Result、SaveGame、GameplayTag 或 map binding。RunEvent flags 仍是当前 Run 内存态；SerpentSigil Credential grant 仍由 Pickup existing transaction解释。
