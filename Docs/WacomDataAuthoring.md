---
type: data-authoring-reference
scope: wacom-data-authoring
status: active
updated: 2026-07-17
tags:
  - wacom/data
  - wacom/authoring
  - wacom/validation
---

# WacomData Authoring 文档

> [!info] 本文职责
> 本文记录 Wacom 静态内容制作、内容生成、资产校验和 Battle rule content authoring matrix。字段语义入口见 [WacomData.md](./WacomData.md)，Gameplay tag catalog 见 [WacomGameplayTags.md](./WacomGameplayTags.md)。

> [!warning] 模块边界
> 内容生成和资产校验位于 `WacomEditor`，自动化验证位于 `WacomTests`。`WacomData` 只保存静态类型，不依赖 Editor、Battle、Run 或 UI。

## §1 Authoring 边界

| 入口 | 职责 | 不是 |
|---|---|---|
| DataAsset 字段 | 保存静态配置和内容 ID | 运行时状态、交易记录、UI 状态 |
| `WacomRegenerateContent` | 重建当前示例 / 调试 DataAsset | 运行时加载入口或策划数据库 |
| `WacomBuildEnemyPack` | 晋升已授权敌人素材并幂等构建正式敌人包 | 通用文件复制器、运行时加载入口 |
| Editor Validator | 阻断结构错误和当前规则未接入配置 | 数值平衡、文案质量、剧情合法性校验 |
| Battle authoring matrix | 说明当前可写入 DataAsset 并能被 resolver 执行的规则组合 | 自动开放所有已声明 Gameplay tag |
| 自动化测试 | 防止生成资产、validator 和 runtime resolver 漂移 | 正式内容设计审核 |

静态 Definition 的 `EncounterDefinitionId / ShopId / EventId / PickupId / InteractionId` 是内容识别 ID。场景运行时状态使用 Actor `PersistentId`，不要用静态 ID 替代。

Logical Map 使用另一套稳定身份：`JourneyId`、`FloorId`、`NodeId` 和 `EdgeId`。其中 Node/Edge ID 只要求 Floor 内唯一；跨层记录、校验和运行时结果必须使用带 `FloorId` 的 handle。图连接只能在 Floor DataAsset 中制作，不能从场景 Actor 连线反向生成规则图。

## §2 内容生成 Commandlet

`UWacomRegenerateContentCommandlet` 位于 `Source/WacomEditor/Private/Commandlets/WacomRegenerateContentCommandlet.cpp`，用于重建当前示例 DataAsset。

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomRegenerateContent -NoSplash -Unattended
```

Builder 当前职责：

| Builder | 产物 |
|---|---|
| `BuildSnakeContent()` | 幂等生成 Snake 规则数据、`DA_Encounter_SnakeSingle` 与 `BP_EnemyHost_Snake`；只读取已提交的 `/Game/Wacom` Placeholder |
| `BuildEncounterContent()` | 其它旧 Encounter 生成入口；单蛇 Encounter 已由 `BuildSnakeContent()` 统一拥有 |
| `BuildTrainingWarriorContent()` | TrainingWarrior 规则数据、奖励卡、语义动画 Style、Host Blueprint 与单敌人 Encounter；只读取正式 `/Game/Wacom` 素材 |
| `BuildBugGirlContent()` | 虫妹角色、左右手、伙伴初始牌、容器 / 功能卡、starter pack、debug key、卡对卡测试卡、badge 测试卡 |
| `BuildShopContent()` | `DA_Shop_DebugSnake`，正式调试商品保留原价，测试 / 调试卡统一 0 金币 |
| `BuildRunEventContent()` | `DA_Event_DebugSnakeGift`、`DA_Event_DebugFlagReward` |
| `BuildRunPickupDefinitionContent()` | `DA_Pickup_DebugGold3`、`DA_Pickup_DebugPoisonFang` |
| `BuildRunWorldCardInteractionDefinitionContent()` | `DA_RunWorldCardInteraction_DebugKeyGold3` |

改 Builder 后应运行 commandlet 落盘资产，并跑对应 `Wacom.Data.*`、`Wacom.Battle.*` 或 Run/UI smoke。Commandlet 只辅助内容制作，不参与运行时规则。

TrainingWarrior 使用独立 enemy-pack 入口：

```powershell
# 首次从已授权本地 PaperAssets 晋升五组动画及其受控依赖闭包
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<ProjectRoot>\Wacom.uproject' `
  -run=WacomBuildEnemyPack -Pack=TrainingWarrior -PromoteArt `
  -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge

# 日常只用已经提交的正式素材重建内容，不要求本地 /Game/Art
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' '<ProjectRoot>\Wacom.uproject' `
  -run=WacomBuildEnemyPack -Pack=TrainingWarrior `
  -Unattended -NoPause -NoSplash -NullRHI -NoDreamShaderEditorBridge
```

`-ForceArtRefresh` 只允许与 `-PromoteArt` 同用。晋升服务先验证 Idle / Attack / Block / Cleave / Downed 的 Flipbook、Sprite、Texture 闭包完全位于 BattleWarrior 源目录，再用 `IAssetTools::AdvancedCopyPackages` 一次复制并重写引用；不会复制 Item，也不会删除目标目录中的未知资产。正式目标完整时跳过复制；目标不完整且未指定 `-PromoteArt` 时失败。`WacomRegenerateContent` 会同步调用 TrainingWarrior builder，但绝不读取或晋升 `/Game/Art`。

Snake 使用同一套 manifest-driven enemy-pack 服务，但素材语义是“受控占位”而不是正式美术：

```powershell
# 首次或明确重晋升 Slime Idle 的受控占位闭包
-run=WacomBuildEnemyPack -Pack=Snake -PromotePlaceholderArt

# 日常幂等重建 Snake 数据、Encounter 与三部位 Host
-run=WacomBuildEnemyPack -Pack=Snake
```

Snake 的 `-ForceArtRefresh` 只允许与 `-PromotePlaceholderArt` 同用；`-PromoteArt` 会明确失败。晋升闭包固定为一个 Slime Idle Flipbook、四个 Sprite 和一个 Texture，另从正式副本生成 Head / Body / Tail 三个单帧 Destroyed Flipbook。目标完整时不复制，本地 `/Game/Art` 可以缺席；目标不完整时必须显式晋升。`WacomRegenerateContent` 只消费已经提交的 `/Game/Wacom/Art/Placeholders/Enemies/Snake`，不会执行素材晋升。

单部位敌人紧凑 UI 使用独立、幂等的 Editor builder，不接入 `WacomRegenerateContent`，也不触碰旧 Enemy WBP：

```powershell
# 创建或更新受合同标记管理的新 WBP、Intent Style 与四张像素图标
-run=WacomBuildEnemyUI -BuildSinglePartCompact

# 只读检查父类、bindings、动画、Style 映射和图标资源
-run=WacomBuildEnemyUI -InspectSinglePartCompact
```

`DA_EnemyIntentPresentation_Default` 属于 UI-only 制作资产，不是战斗规则 schema。新增 Intent 图标时必须填写准确且唯一的稳定 `IntentId` 与有效 Brush；不允许用显示名、动画名或 effect 自动推断。空 ID、重复 ID 和无效 Brush 会被 Data Validation 拒绝；未配置 Intent 运行时使用 fallback 星形，不阻断战斗。

## §3 当前生成内容

核心角色与卡牌：

| 资产组 | 内容 |
|---|---|
| `DA_Character_BugGirl` | 虫妹角色、左右手卡和正式 StarterDeck；测试 / 调试卡不进入初始牌组 |
| 固有手牌 | `DA_Card_LeftHand`、`DA_Card_RightHand` |
| 伙伴初始牌 | 朝光暮蝶、拂晓飞蛾、赤腹工蚁、烁光蝶、暮蛉 |
| 容器 / 功能卡 | 虫妹的小布袋、蛛茧绒囊、暮色引虫灯 |
| Starter pack | 毒针、几丁护片、触须探路、蜕壳切、轻蜕壳、丝线佯攻 |
| 奖励卡 | `DA_Card_PoisonFang`；`DA_Card_BrokenCleave` 是 TrainingWarrior Body 的正式 Aid / Destroy 奖励 |
| Debug / 测试卡 | DebugKey、卡对卡加费 / 减费 / 弃置 / 消耗、关键词筛选目标卡、按当前费用抽牌测试卡 |
| Badge 测试卡 | Damage / Poison / Shield / Heal 的卡面徽章显示测试卡；Burn 只有 UI 预留，不生成正式测试卡 |

核心敌人、商店与 Run 内容：

| 资产组 | 内容 |
|---|---|
| `DA_Enemy_Snake` | 蛇敌人，包含 Head / Body / Tail 三个部位 |
| `DA_Behavior_Snake` | 蛇行为资产，Default phase 下为 Head / Body / Tail 提供三套 `Sequence` intent set |
| 蛇部位 | Head / Body / Tail 配置 HP、经验和毒牙奖励；部位资产不承载行为，行为统一写入 `DA_Behavior_Snake` |
| `DA_Encounter_SnakeSingle` | 正式单蛇 Encounter 样例，`EncounterDefinitionId=Encounter.Snake.Single`，`EnemySlots[0]=Enemy -> DA_Enemy_Snake` |
| Snake Host 内容包 | `/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake`；`MultiPartVisualLayers` 下按 Definition 顺序生成 Head / Body / Tail，每段使用错帧 Slime Idle 与独立单帧 Destroyed，占位资产禁止正式出包 |
| TrainingWarrior 内容包 | `DA_Enemy_TrainingWarrior`、单 Body Part、Attack → Guard → Cleave 行为、`DA_Encounter_TrainingWarriorSingle`、语义动画 Style 与 `BP_EnemyHost_TrainingWarrior` |
| `DA_Shop_DebugSnake` | 固定卖毒牙、部分正式卡、starter pack、debug key、卡对卡测试卡、按当前费用抽牌测试卡和 badge 测试卡 |
| `DA_Event_DebugSnakeGift` | 蛇巢遗物调试事件：获得毒牙、单卡支付交出毒牙、金币 / 压力与显式 Action Point policy |
| `DA_Event_DebugFlagReward` | RunFlag 与 `MinGold + AddGold(-N)` 组合样例 |
| `DA_Pickup_DebugGold3` | 数据驱动金币 PickupDefinition，固定获得 3 金币 |
| `DA_Pickup_DebugPoisonFang` | 数据驱动固定卡牌 PickupDefinition，固定获得毒牙 |
| `DA_RunWorldCardInteraction_DebugKeyGold3` | 通用 Run world card interaction 样例，接受 DebugKey，奖励 3 金币并消耗源卡 |

生成内容测试路径集中在 `FWacomGeneratedBattleContentAssets`。该 helper 只属于 `WacomTests`，不作为运行时加载或策划配置来源。

<a id="asset-validation"></a>
## §4 Asset Validation

Editor Validator 由 `WacomEditor` 注册到 `UEditorValidatorSubsystem`。共享校验函数供编辑器 Validate Assets 和自动化测试复用。

| Validator | 校验对象 | 共享校验函数 |
|---|---|---|
| `UWacomCardDefinitionValidator` | `UCardDefinition` | `FWacomCardDefinitionValidation::Validate()` |
| `UWacomEnemyPartDefinitionValidator` | `UEnemyPartDefinition` | `FWacomEnemyPartDefinitionValidation::Validate()` |
| `UWacomEnemyDefinitionValidator` | `UEnemyDefinition` | `FWacomEnemyDefinitionValidation::Validate()` |
| `UWacomEnemyBehaviorDefinitionValidator` | `UEnemyBehaviorDefinition` | `FWacomEnemyBehaviorDefinitionValidation::Validate()` |
| `UWacomCharacterDefinitionValidator` | `UCharacterDefinition` | `FWacomCharacterDefinitionValidation::Validate()` |
| `UWacomEncounterDefinitionValidator` | `UEncounterDefinition` | `FWacomEncounterDefinitionValidation::Validate()` |
| `UWacomShopDefinitionValidator` | `UShopDefinition` | `FWacomShopDefinitionValidation::Validate()` |
| `UWacomRunEventDefinitionValidator` | `UWacomRunEventDefinition` | `FWacomRunEventDefinitionValidation::Validate()` |
| `UWacomRunPickupDefinitionValidator` | `UWacomRunPickupDefinition` | `FWacomRunPickupDefinitionValidation::Validate()` |
| `UWacomRunWorldCardInteractionDefinitionValidator` | `UWacomRunWorldCardInteractionDefinition` | `FWacomRunWorldCardInteractionDefinitionValidation::Validate()` |
| `UWacomMapDefinitionValidator` | `UWacomJourneyDefinition`、`UWacomFloorMapDefinition` | `FWacomMapDefinitionValidationReport`；检查身份、端点、可达性、typed payload、入口条件、必填 DisplayName、MapPosition 制作边界，以及 Journey success terminal 的末层/Encounter Boss/可达/无出边合同 |

当前校验边界：

- Card / EnemyPart / Enemy / EnemyBehavior / Character 校验 ID、基础数值、必填引用、数组索引、Gameplay tag 命名空间和当前 battle rule content contract。它们不校验文案质量、数值平衡、流派构筑、固定卡组数量或生成资产路径。
- Enemy 校验 `PartSlotId` 必填且不重复，并在配置 `DefaultBehavior / BehaviorOverride / InitialIntentSetId` 时检查对应 phase 和 intent set 是否存在。
- Scene Enemy Host 的诊断与写入严格分层，不属于 `WacomData` schema 或自动 Validator mutation。`WacomApp` 的纯 Authoring Report 统一读取 Host、PartActor / ChildActor template 与 `EnemyDefinition`，给出 missing、unknown、duplicate、mismatch、surplus、无效定义槽位和按定义顺序排列的同步计划；Validator、debug view 与 Details 都消费这份实时报告，求值永远不刷新视觉、不派生身份、不创建组件且不 dirty package。内容人员在 Host Details 显式点击“从 EnemyDefinition 同步部位”，由 `WacomEditor` service 应用计划：唯一匹配槽位的 `PartId` 从 `PartDefinition.PartId` 派生，缺失槽位生成零相对变换的 PartActor ChildActorComponent；已有 transform、`HitBoundsExtent`、`ImpactAnchorRelativeLocation` 和 `VisualLayers` 保留。空、未知或重复槽位只标记 surplus，不自动删除；无效定义槽位跳过并报告。对 Host Blueprint 模板执行时，新增部位写入 Blueprint SCS 并标记结构变化；对关卡 Host 实例执行时，新增 runtime-safe `UWacomBattleEnemyPartChildActorComponent` 作为 transactional InstanceComponent，持久保存明确的派生身份并在 ChildActor 重建后恢复，不反向改写来源 Blueprint，也不依赖 UE 的实验性 per-instance ChildActor 属性开关。多选 Host 共用一次事务，无实际变化不创建事务或 dirty package；正式复用内容应优先在 Host Blueprint prefab 上同步，再 Compile / Save。
- Host validator 继续只读校验并给出修复方向：重复 `PartSlotId` 为错误；缺失槽位、PartId 与槽位定义不一致、surplus 部位以及制作模式缺少对应视觉资源为可定位 warning。Validator 不在扫描资产或 Validate Map 时生成、删除或改写 PartActor。
- PartActor VisualLayer 的破损资源是可选制作数据，不影响 Authoring Ready。Validator 会拒绝 `StaticSprite + DestroyedFlipbook`、`Flipbook + DestroyedSprite` 的模式错配，拒绝非有限 / 非正的 Destroyed Flipbook PlayRate，以及不在 `[0,1]` 内的 `DestroyedVisualSwapNormalizedTime`。同步部位服务继续原样保留 `VisualLayers`，不会生成、猜测或删除破损资源；首个正式 Multi-Part enemy pack 必须由内容人员明确提供逐层资源。
- PartActor 行动动画同样是可选制作数据，不影响 Authoring Ready。`UWacomBattleEnemyPartAnimationStyle` 必须填写唯一 `TargetVisualLayerId`；显式 Intent map 的 key、Flipbook 和有限正数 PlayRate 必须有效，目标层必须在 PartActor 上唯一存在且为有效 Flipbook Layer。Style 只适用于 `MultiPartVisualLayers` Host；同步部位服务不会生成 Action Clip、猜 Intent 映射或覆盖 Style。缺少正式行动素材时应保持 Style 为空，不得用 Idle 冒充攻击动画。
- 正式 Pixel Impact System 的 Graph 只由 `WacomBuildBattleEnemyPartImpactNiagara` 写入。当前生成合同包含六个 Emitter 和固定 `EffectKind=0/1/2/3` 映射；运行 `-InspectOnly` 只读验证版本、User Parameter、Renderer 和编译结果。`Scripts/SetupBattleEnemyPartImpactAssets.py` 幂等写入 Destroyed Style 数值但不覆盖人工声音引用，也不会在已有 DreamShader 材质 / MI 合同正确时重存它们。
- EnemyBehavior 校验 `BehaviorId`、phase、intent set、intent、selector rule、condition、cooldown authoring 和敌人意图 effect contract；可选传入 owning EnemyDefinition 时，会额外校验 `AppliesToPartSlotId / PartDestroyed` 等部位槽引用。
- Character 会校验 `StarterDeck` 不包含左右手卡。
- Shop 校验 `ShopId`、`Offers`、Offer 卡牌和非负价格；不校验重复商品、价格平衡或商品池规则。
- RunEvent 校验事件图结构、ID、引用、NextNode、卡牌条件 / 效果、卡牌支付筛选和 ZoneId、事件状态目标、RunFlag、压力 ID，以及 `Automatic / Free / Fixed` Action Point policy。正成本非 terminal choice 是错误；早期由 effect 单独扣减探索预算的做法已删除。金币门槛 / 扣费组合中的 authoring 风险可以给 warning。
- RunPickupDefinition 校验固定单一主奖励配置：`PickupId`、奖励类型、金币数量或卡牌引用；可选 `GrantedCredentialIds` 必须全部非 `None` 且 Definition 内唯一。
- RunWorldCardInteractionDefinition 校验 `InteractionId`、至少一个正向卡牌筛选和有效奖励项。
- Actor 摆放实例的 `PersistentId`、重复 ID、receiver 和 facade 配置属于 map / level validation，见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#2-run-world-interactable-actor)。
- Map Floor 与每个 Node 的 `DisplayName` 必填；`ShortDescription` 可空。`MapPosition` 合法闭区间是 `[0,1920] × [0,1080]`，越界或完全重合为 error，节点中心距离小于 `48 px` 为 warning；这些坐标不参与规则距离或合法性。

不要把 Validator 放进 `WacomData`，否则运行时模块会反向依赖编辑器能力。

Map validation 的 report 和执行器归 `WacomEditor`；transient graph fixtures 与自动化归 `WacomTests`。`WacomData` 只提供可反射的静态 authoring types，不依赖 `WacomRun`、关卡 Actor 或 Editor API。

当前 `L_Exploration` 使用 `/Game/Wacom/Data/Map/Authoring/DA_Floor_LevelAuthoring_01` 与 `DA_Journey_LevelAuthoring`。这是承接现有可玩图的过渡制作基线，不是正式 Floor 1；`GM_Wacom` 继续指向 Authoring Journey，Debug builder 禁止修改这三个正式/Authoring Package。正式设计已在 2026-07-17 独立冻结为 `Journey.Main.01` 与 `Floor.Main.01/02/03`，不会通过重命名或覆盖 Authoring/Debug 资产落地。

正式 Production Map 资产路径预留为：

```text
/Game/Wacom/Data/Map/Production/DA_Journey_Main_01
/Game/Wacom/Data/Map/Production/DA_Floor_Main_01
/Game/Wacom/Data/Map/Production/DA_Floor_Main_02
/Game/Wacom/Data/Map/Production/DA_Floor_Main_03
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
/Game/Wacom/Maps/Run/L_Run_Floor_Main_02
/Game/Wacom/Maps/Run/L_Run_Floor_Main_03
```

这些路径当前不存在，也不授权 builder 或迁移脚本创建。正式三层各 20 Node/21 Edge canonical graph、内容槽、跨层门槛与 Journey 节奏见 [WacomMap](./WacomMap.md#wacommap) §9；不得创建最小空壳图、伪 FloorEntrance 或 terminal Actor 特例绕过 Production readiness gate。

正式内容 Host 的 `PersistentId` 不另建人工注册表，统一按 `<FloorId>.<NodeId>` 派生。例如 `Node.Route.A.01` 在正式首层的 runtime key 为 `Floor.Main.01.Node.Route.A.01`；Host 的 `RunMapNodeBinding.NodeId/NodeType` 仍必须等于 Floor DataAsset 节点。Navigation 没有内容 Host PersistentId，Path/Branch 在单 Floor World 内继续只保存 EdgeId。

Floor 1 为 15 个内容节点预留以下 Production definition IDs：

```text
Encounter.SerpentWood.Scout
Encounter.SerpentWood.MoltGuard
Encounter.SerpentWood.Ambush
Encounter.SerpentWood.RootStalker
Encounter.SerpentWood.EliteSentinel
Encounter.SerpentWood.ShallowGuardian

Event.SerpentWood.CastSkin
Event.SerpentWood.HunterTrace
Event.SerpentWood.MerchantRumor
Event.SerpentWood.PoisonMarsh

Pickup.SerpentWood.HerbCache
Pickup.SerpentWood.HunterCache
Pickup.SerpentWood.MoltCache
Pickup.SerpentWood.SerpentSigil

Shop.SerpentWood.Wayfarer
Card.Run.SerpentSigil
Credential.Run.SerpentSigil
```

### Floor 1 SerpentWood Production 内容制作合同

Floor 1 的 15 个节点 Definition 已从“只预留职责”升级为完整内容合同。Spec 011 冻结 38 个核心 DataAsset；击倒分支合同在此基础上额外预留 8 张 CardDefinition，未来总量为 `38 core + 8 branch reward = 46`：

| Type | Count | Contract |
|---|---:|---|
| Encounter Definition | 6 | 上述 6 个 `Encounter.SerpentWood.*` |
| RunEvent Definition | 4 | 上述 4 个 `Event.SerpentWood.*` |
| Pickup Definition | 4 | 上述 4 个 `Pickup.SerpentWood.*` |
| Shop Definition | 1 | `Shop.SerpentWood.Wayfarer` |
| Card Definition | 4 | 3 个 `Reward.SerpentWood.*` + `Card.Run.SerpentSigil` |
| Enemy Definition | 4 | BrushSnake / MoltGuard / RootStalker / ShallowGuardian |
| Enemy Behavior Definition | 4 | 每敌人一份 `Default` + per-part `Sequence` 行为 |
| Enemy Part Definition | 11 | 2 + 3 + 2 + 4 个部位 |
| **Core subtotal** | **38** | Spec 011 manifest 集合 |
| Knockdown branch Card Definition | 8 | 四个敌人各一张 Aid 与一张 Destroy |
| **Production total** | **46** | 38 core + 8 branch reward |

主题路径固定为：

```text
/Game/Wacom/Data/Enemies/SerpentWood/<Archetype>/
/Game/Wacom/Data/Encounters/SerpentWood/
/Game/Wacom/Data/Events/SerpentWood/
/Game/Wacom/Data/Pickups/SerpentWood/
/Game/Wacom/Data/Shops/SerpentWood/
/Game/Wacom/Data/Cards/Rewards/SerpentWood/
/Game/Wacom/Data/Cards/Rewards/SerpentWood/<Archetype>/
/Game/Wacom/Data/Cards/Run/SerpentWood/
```

Enemy 资产使用 `DA_Enemy_<Archetype>`、`DA_Behavior_<Archetype>`、`DA_Part_<Archetype>_<Part>`；其它资产使用 `DA_Encounter_<Slot>`、`DA_Event_<Slot>`、`DA_Pickup_<Slot>`、`DA_Shop_Wayfarer`、`DA_Card_<CardName>`。内部 ID 才是规则身份，不能由 package path、DisplayName 或资产名反推。

四个 EnemyId 固定为：

```text
Enemy.SerpentWood.BrushSnake
Enemy.SerpentWood.MoltGuard
Enemy.SerpentWood.RootStalker
Enemy.SerpentWood.ShallowGuardian
```

BehaviorId 使用 `SerpentWood.<Archetype>.Behavior`，PartId 使用 `SerpentWood.<Archetype>.<Part>`。所有 11 个正式部位必须清空 deprecated `KnockdownRewardCard`，并按所属 Archetype 显式填写 `AidRewardCard` 与 `DestroyRewardCard`。四组身份为 `Reward.SerpentWood.<Archetype>.Aid/Destroy`，各放在 `/Game/Wacom/Data/Cards/Rewards/SerpentWood/<Archetype>/`；八张卡本轮只冻结 ID、目录与数量，具体数值另案。Encounter 敌人组合、24 条 Intent、4 张核心新卡、Pickup/Shop/RunEvent 精确字段见 [WacomData.md](./WacomData.md) §13；Spec 011 的 manifest 保留历史 38 core package 清单，未来实施清单必须显式追加 8 张分支奖励卡后才可宣称 Floor 1 Production 完整。

EnemyPart 奖励校验分两档：

| Profile | 允许 | 拒绝 |
|---|---|---|
| `General` | 无奖励、纯 legacy、只使用一个或两个显式新字段 | legacy 与任一新字段混填 |
| `FormalProduction` | Aid/Destroy 两个显式字段都存在且 legacy 为空 | 缺任一显式分支、残留 legacy、任何混填 |

TrainingWarrior 与 Snake 的现有二进制资产继续由 General + legacy fallback 读取。两个 builder 的未来写入源码已经改为同时填写 Aid/Destroy 并清空 legacy，但在资产所有者授权迁移前不得运行 builder 或重存资产。删除 legacy 字段必须等待 TrainingWarrior、Snake 和全部正式 Part 完成授权迁移，并由 AssetRegistry/引用审计证明零旧字段依赖。

Wayfarer 允许只读引用三张现有正式 Starter 卡和 `/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang`；该卡 live `CardId=PoisonFang`，不新建主题副本。Production Definition 不得引用 `Debug`、Authoring、`Test.*`、BadgeDisplayTests 或 TrainingWarrior 内容；TrainingWarrior 只作为当前 enemy-pack 制作范式参考。

后续 SerpentWood builder 的核心写集合以 Spec 011 的 38 个 package 为基础；在 8 张奖励卡 package leaf name 与数值合同另案冻结并扩展 manifest 前，不得创建不受清单约束的奖励资产。Starter 与 PoisonFang 是只读依赖。不得通过全量 `WacomRegenerateContent` 或 Debug builder 顺带写入地图、Player/Host Blueprint、UI、材质、卡牌表现、背包或其它 Agent 资产。实现轮必须双跑 builder，检查 ID/引用/计数/字段稳定、重复资产、dirty package 和只读依赖哈希；本轮未授权或运行该 builder。

Floor 2 为 15 个内容节点预留 `MoltCavern` namespace：

```text
Encounter.MoltCavern.ScaleScout
Encounter.MoltCavern.StoneScaleGuard
Encounter.MoltCavern.HatcheryAmbush
Encounter.MoltCavern.BridgeSentinel
Encounter.MoltCavern.VenomHunter
Encounter.MoltCavern.EliteMolter
Encounter.MoltCavern.CavernGuardian

Event.MoltCavern.CastoffEcho
Event.MoltCavern.LostDelver
Event.MoltCavern.MoltingRite

Pickup.MoltCavern.FungalCache
Pickup.MoltCavern.MineralCache
Pickup.MoltCavern.VenomCrystalCache
Pickup.MoltCavern.MoltSeal

Shop.MoltCavern.DeepWayfarer
Card.Run.MoltSeal
Credential.Run.MoltSeal
```

Floor 3 为 16 个内容节点预留 `VenomCore` namespace：

```text
Encounter.VenomCore.CoreVanguard
Encounter.VenomCore.VeinGuardian
Encounter.VenomCore.BroodPatrol
Encounter.VenomCore.InnerSentinel
Encounter.VenomCore.ToxinStalker
Encounter.VenomCore.EliteHarvester
Encounter.VenomCore.FinalVanguard
Encounter.VenomCore.CoreGuardian

Event.VenomCore.VeinResonance
Event.VenomCore.CoreWhisper
Event.VenomCore.SacrificeChoice
Event.VenomCore.HeartPulse

Pickup.VenomCore.AntidoteCache
Pickup.VenomCore.RitualCache
Pickup.VenomCore.VenomReservoir
Pickup.VenomCore.CoreBoon
```

三层共 46 个节点内容槽。Floor 1 的 15 个槽已经由 Spec 011 冻结敌人组合、事件选项、库存和奖励数值；Floor 2/3 的 31 个槽仍只冻结职责和命名入口。视觉资产、Host、世界 Transform 与三层正式场景继续另案。现有 `DA_Event_Debug*`、`DA_Shop_Debug*`、`DA_Pickup_Debug*`、`DA_RunWorldCardInteraction_Debug*` 与 `DA_Card_DebugKey` 只能服务 Debug/测试，不得作为 Production typed payload、蛇印/蜕印或终局占位。

`Pickup.SerpentWood.SerpentSigil` 的正式 Definition 必须以 Card 作为固定主奖励，并在 `GrantedCredentialIds` 中授予 `Credential.Run.SerpentSigil`；`Node.Exit.01` 的 FloorEntrance 只在 `RequiredCredentialIds` 中引用该 Credential，不把 `Card.Run.SerpentSigil` 写回 `OwnedCardRequirements`。两者由同一稳定 ID 对接，但表现卡和资格状态互不推断。

`Pickup.MoltCavern.MoltSeal` 使用相同通用合同：固定主奖励为 `Card.Run.MoltSeal`，`GrantedCredentialIds` 授予 `Credential.Run.MoltSeal`；Floor 2 `Node.Exit.01` 只要求 Credential 并指向 `Floor.Main.03`。不得从表现卡推断、撤销或补算资格。

Floor 3 `Node.Guardian.01` 是无出边的 success terminal，不配置 FloorEntrance payload。Production `DA_Journey_Main_01` 必须配置 `DisplayName` 和 `SuccessTerminalNode={Floor.Main.03, Node.Guardian.01}`。`/Game/Wacom/Data/Map/Production/` 下缺失 terminal 是 validation error；旧 Debug/Authoring Journey 可保持未配置并产生 warning，Runtime 仍允许启动但不会自动成功。已配置 terminal 在 Editor 与 Runtime 都必须位于最后一层、引用存在的 `Encounter + bBoss=true` 节点、从 Entry 可达、无出边，且最后一层不得包含 FloorEntrance。不得用 Actor label、Level Blueprint、EncounterId、legacy `bRunActive` 或伪 TargetFloorId 实现终局。

Map validator 会拒绝空/重复 Credential requirement，以及不存在于入口前置不可绕过固定 Pickup 中的 grant。现有 Debug Pickup 默认 grant 数组为空，不能被晋升或复制成 Production 蛇印入口占位。

蛇印任务凭证门禁、Floor 2/3 图冻结、通用 Journey success、Floor 1 核心内容设计和击倒分支奖励 schema 均已完成。正式资产制作仍被 46 个非 Debug 节点 Definition 的实际创建与 validation、Floor 1 的 `38 core + 8 branch reward cards`、八张卡具体效果、Floor 2/3 支持内容设计，以及 production map AssetRegistry/引用/哈希权威审计阻塞；关闭这些条件前不创建或绑定正式 Journey/Floor/map。Floor 1 原始图门禁见 `specs/007-formal-floor1-content-freeze/`，通用 Credential 合同见 `specs/008-run-credential/`，图与 pacing readiness 见 `specs/009-formal-floor23-journey-pacing-freeze/`，成功合同见 `specs/010-journey-success-settlement-baseline/`，Floor 1 核心内容冻结证据见 `specs/011-formal-floor1-production-content-freeze/`，击倒分支奖励合同见 `specs/012-knockdown-branch-reward-baseline/`。

每个可独立加载的 Run Floor map 必须放置且只放置一个 `AWacomRunFloorSceneDescriptorActor` 并引用对应 Floor。场景验证可从编辑器执行 `Tools -> Wacom -> Validate Current Run Floor`，或从命令行执行：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomValidateRunFloorScene -Map=/Game/Wacom/Maps/L_Exploration -NoSplash -Unattended -NoDreamShaderEditorBridge
```

validator 只读检查 Descriptor、Anchor/Path/Branch/content host 身份与 typed payload。Spline 少于 2 点、长度不超过 `10 cm`、非有限 Transform 或方向颠倒为 Error；端点距离 `<= 100 cm` 通过，`(100, 300] cm` 为 Warning，`> 300 cm` 为 Error。诊断按 Severity/Code/ObjectPath 排序；命令退出 `0/1/2` 分别表示通过或仅 Warning、合同 Error、参数/加载/Descriptor 解析失败。它不得调用 `Modify`、标脏、修复或保存。

Run 探索 Debug 内容由 `UWacomBuildRunExplorationDebugAssetsCommandlet` 可重复构建：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildRunExplorationDebugAssets -NoSplash -Unattended -NoDreamShaderEditorBridge
```

命令的唯一写集合是 `/Game/Wacom/Data/Map/DA_Journey_Debug`、`DA_Floor_Debug_01`、`/Game/Wacom/Debug/GameModes/GM_WacomRunDebug` 和 `/Game/Wacom/Maps/Debug/L_RunExploration_Debug`。Player BP 与 `/Game/Wacom/Run/Path/Blueprints` 下的 Anchor/Path/Branch Blueprint 是只读依赖：缺失或父类错误时立即失败，不创建替代资产，也不编译、标脏或保存它们。正式 `L_Exploration`、Authoring Journey/Floor、`GM_Wacom` 以及任何卡牌、背包、敌人材质、美术和 UI 资产都是禁止写集合。

builder 只重建 Debug map 中带 `Wacom.Generated.RunExploration` ownership 的 Actor，保留非生成美术与可复用 host；它确保唯一 Debug Floor Descriptor、8/7/3/6 当前 fixture 结构和 Debug GameMode 引用，并在保存四个 owned Package 前运行同一 Scene validator。六个内容 Host 由 `FindContentHost` 按 Floor typed payload 对应的 Definition 查找并复用，属于人工摆放实例：不得带 generated ownership，builder 只刷新其 `RunMapNodeBinding`，不覆盖 Blueprint class、transform、Definition、交互配置或绑定的 Viewpoint/SceneEnemyHost。内容人员移动这些 Host 后必须同时检查触发范围、点击命中体、Viewpoint staging 和 traversal spline 避让。

连续两次构建要求逻辑身份、计数和引用相同；Debug 生成 Actor GUID 与 owned 二进制 hash 不属于稳定合同。`Wacom.Editor.RunExplorationDebugAssets` 还锁定六个手工 Host 的 NodeId、NodeType、Blueprint class、transform 和非 generated ownership，并用 SHA-256 与 dirty-state 守卫正式 map、Authoring 数据、`GM_Wacom`、Player BP 和三个共享 Run Path BP。Debug map 可以为测试表现独立调整 Anchor/Path/Branch 世界几何；它与正式图共享身份和 validator 合同，不共享 transform 真值。

Run Map UI 资产由独立命令构建，不修改关卡或 Floor 数据：

```powershell
& 'E:\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe' 'D:\UE_Project\5.7\Wacom\Wacom.uproject' -run=WacomBuildRunMapUIAssets -NoSplash -Unattended
```

它幂等生成并编译 `/Game/Wacom/UI/Map/WBP_RunMapNode` 与 `WBP_RunMapScreen`。UI builder 不修改关卡或 Floor 数据，也不依赖 Debug builder 迁移 Authoring 文案；两个 builder 各自要求连续运行两次结果稳定。

## §5 生成内容自动化

| 测试入口 | 目的 |
|---|---|
| `Wacom.Data.GeneratedContent.DefinitionAssetValidation` | 读取生成角色、卡牌、Snake 敌人、Snake Behavior、部位、单蛇 Encounter，并统一跑 DataAsset validator |
| `Wacom.Data.BattleStarterContent.StarterPackAssetValidation` | 检查 starter pack 核心字段、BugGirl 初始牌组排除关系、测试卡不进入 StarterDeck 和 Snake Behavior intent set |
| `Wacom.Data.BattleStarterContent.BadgeDisplayTestCardAssetValidation` | 检查 badge 测试卡、DebugSnake 商店 0 金币出售和 CardPresentationBuilder badge view |
| `Wacom.Data.Shop.DebugSnakeAssetValidation` | 验证 DebugSnake 商店能通过 validator |
| `Wacom.Data.Shop.DebugSnakeAsset` | 验证商品顺序和价格，避免内容生成漂移 |
| `Wacom.Data.RunEvent.DebugSnakeGiftAsset` | 验证蛇巢遗物事件节点、选项、条件、效果和毒牙引用 |
| `Wacom.Data.RunEvent.DebugFlagRewardAsset` | 验证 RunFlag、金币门槛 / 扣费、奖励和 reset flags 样例 |
| `Wacom.Battle.GeneratedStarterContent` | 使用真实生成资产进入 `UBattleSession`，验证核心卡牌、辅助卡和 Snake 意图能产生预期 Snapshot/Event |

这些测试证明“生成资产字段仍符合当前合同”，不是正式内容平衡验收。

<a id="battle-rule-content-authoring-matrix"></a>
## §6 Battle Rule Content Authoring Matrix

卡牌和敌人意图的内容制作以 `FWacomBattleRuleContentContract`、本矩阵、validator 和 runtime fixture 为准。Gameplay tag 已声明不代表可写入正式 DataAsset。

`Wacom.Battle.RuleContentMatrix` 使用 transient Definition：每个代表配置先通过 validator，再提交到 `UBattleSession` 验证 Snapshot/Event。`Reserved` / `EventOnly` 条目只记录制作状态，不代表可制作。

`FCardEffect` 当前字段：

```cpp
USTRUCT(BlueprintType)
struct FCardEffect
{
    FGameplayTag EffectType;
    int32 Magnitude = 0;
    FGameplayTag Target;
    FGameplayTag TargetZone;
    int32 Duration = 0;
    FGameplayTag MagnitudeSource;
    FEffectCondition Condition;
    TArray<FMagnitudeModifier> MagnitudeModifiers;
    bool bMagnitudeFromRuntimeCost = false; // 旧资产兼容，新增资产应使用 MagnitudeSource
};

USTRUCT(BlueprintType)
struct FMagnitudeModifier
{
    FEffectCondition Condition;
    EMagnitudeModOp Op = EMagnitudeModOp::Add;
    int32 Value = 0;
};
```

Magnitude 计算顺序：

1. `MagnitudeSource` 有效时按 source handler 计算。
2. `MagnitudeSource` 为空且 `bMagnitudeFromRuntimeCost=true` 时，用当前 RuntimeCost 兼容旧资产。
3. 否则使用 `Magnitude`。
4. 按数组顺序应用 `MagnitudeModifiers`。

`FCardEffect` 保持兼容性的宽资产 schema，但战斗执行不会把 `TargetZone` 继续作为通用 MetaTag 透传。Private Effect Semantics 在 decode seam 按 EffectType 将它转换为 DrawSource、HandZone、Keyword 或 Status typed 参数；`Magnitude.Source.TargetStatusStacks` 的 Status 读取参数单独进入 magnitude plan。制作校验、正式执行和 Target Preview 均读取同一份 semantic definition。

`FIntentEffect` 额外包含窄类型 `FHandAfflictionDelivery`：

```cpp
USTRUCT(BlueprintType)
struct FHandAfflictionDelivery
{
    EHandAfflictionSelection Selection = Default;
    int32 TargetCardCount = 1;
};
```

它只允许用于 `Target.Player` 的 Slow / Freeze / Twilight。`Magnitude=y` 表示每张卡的状态强度；Slow / Freeze 的 `TargetCardCount=x` 表示随机不重复目标数；Twilight 固定 `AllCurrentHandCards`。不要复用 `Duration` 表示目标数。

卡牌效果矩阵：

| EffectType | Magnitude 语义 | Target 典型值 | TargetZone | Duration | MagnitudeSource | 备注 |
|---|---|---|---|---|---|---|
| `Effect.Damage` | 伤害值 | SingleEnemyPart / AllEnemyParts / Player | - | - | Literal / RuntimeCost / TargetStatusStacks | 部位 HP 归零立即破坏 |
| `Effect.Heal` | 治疗量 | Self(->Player) / Player | - | - | Literal | 恢复玩家 HP，并按规则移除中毒 |
| `Effect.ApplyStatus.Poison` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | - | Literal / RuntimeCost | 层数模型 |
| `Effect.ApplyStatus.Slow` | 强度 | Player / SingleEnemyPart / AllEnemyParts | - | 0 | Literal | 敌方立即延迟当前意图；玩家创建 Pending Hand Affliction |
| `Effect.ApplyStatus.Freeze` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | 0 | Literal | 敌方拦截后续卡牌推进；玩家创建 Pending Hand Affliction |
| `Effect.ApplyStatus.Twilight` | 层数 | Player / SingleEnemyPart / AllEnemyParts | - | 0 | Literal | 敌方作用于下一意图；玩家污染下回合整手牌 |
| `Status.Shield` | 护盾值 | Player / Self(->Player) | - | - | Literal | 写 Player Shield；敌方自身护盾只在 Enemy Intent 合同中使用 |
| `Effect.Shuffle.Random` | - | RandomHandCard | - | - | - | 从手牌随机选普通卡腾挪 |
| `Effect.Shuffle.FromBothToOther` | - | ZoneHandCard | HandZone.Both | - | - | 从双手区挑一张腾挪到左 / 右 |
| `Effect.Shuffle.ToRandomZone` | - | Self(本卡) | - | - | - | 把本卡腾挪到随机区域 |
| `Effect.Card.AddCost` | Modifier 增量 | Self / LastShuffledCard / SelectedHandCard | - | - | Literal | 修改 RuntimeCostModifier |
| `Effect.Card.ReduceCost` | Modifier 减量 | 同上 | - | - | Literal | 计算时下限 clamp 到 0 |
| `Effect.Card.DiscardSelected` | 建议填 1 | SelectedHandCard | - | - | Literal | 指定普通手牌进弃牌堆，触发目标卡 `OnDiscard` |
| `Effect.Card.ExhaustSelected` | 建议填 1 | SelectedHandCard | - | - | Literal | 指定普通手牌进消耗牌堆，不触发 `OnDiscard` |
| `Effect.Draw` | 张数 | Self / Player | CardLocation.Draw / Discard / Exhaust | - | Literal / RuntimeCost | `TargetZone` 在 decode seam 转换为 DrawSource |
| `Effect.Discard` | 张数 | Self / Player | - | - | Literal | 随机弃掉普通手牌，不弃锚点 |
| `Effect.ExhaustSelf` | - | Self(本卡) | - | - | - | 通过临时 `Card.Keyword.Exhaust` 交给打出后去向阶段 |
| `Effect.GainKeyword` | - | LastShuffledCard / SelectedHandCard | Card.Keyword.* | - | - | `TargetZone` 在 decode seam 转换为 Keyword 参数 |
| `Effect.RemoveStatus` | 层数 | Player / SingleEnemyPart | Status.Poison / Freeze / Twilight / Stunned | - | Literal / TargetStatusStacks | 不支持 Shield；敌方 Slow 是即时操作，没有可移除层数 |
| `Effect.ModifyInitiative` | 先机增量 | SingleEnemyPart | - | - | Literal / TargetStatusStacks | 正数增加，负数减少 |

`Magnitude.Source.RuntimeCost` 当前允许用于 `Effect.Damage`、`Effect.ApplyStatus.Poison` 和 `Effect.Draw`。用于 `Effect.Draw` 时，抽牌数量等于本卡打出时的 RuntimeCost，例如基础费用 2 的测试卡默认抽 2 张，费用被改变后按改变后的费用抽。`Magnitude.Source.TargetStatusStacks` 当前借用 `TargetZone` 指定要读取的 Combatant 状态，只允许 Poison / Freeze / Twilight / Stunned；敌方 Slow 不保留层数，玩家卡牌 Slow 也不通过 actor-target magnitude 读取。`Status.Shield` 不在 `StatusStacks` 中。

敌人 Intent 效果矩阵：

| EffectType | Target | Magnitude | Duration | 备注 |
|---|---|---:|---:|---|
| `Effect.Damage` | `Target.Player` | > 0 | 0 | 对玩家造成伤害 |
| `Effect.ApplyStatus.Poison` | `Target.Player` 或 `Target.Self` | > 0 | >= 0 | 持久 Combatant 层数 |
| `Effect.ApplyStatus.Slow / Freeze` | `Target.Player` | `y > 0` | 0 | `HandAffliction.TargetCardCount=x>0`，默认 RandomUnique |
| `Effect.ApplyStatus.Twilight` | `Target.Player` | `y > 0` | 0 | 固定作用于下回合抽牌后的当前整手牌 |
| `Effect.ApplyStatus.Slow / Freeze / Twilight` | `Target.Self` | > 0 | 0 | Self 表示行动部位；HandAffliction 必须保持默认 |
| `Status.Shield` | `Target.Self` | > 0 | 0 | 当前敌人 Intent 不给玩家加护盾 |

敌人 Intent 当前不支持手牌目标、全体敌方部位目标、伤害自身、给玩家加护盾、抽弃牌、卡牌专用效果、治疗、移除状态或修改先机。不要依赖运行时静默忽略非法目标。

## §7 Target 与 HandCard 筛选

Target 解析速查：

| Target | 解析为 | TargetInstanceId 来源 | 是否需要 TargetZone |
|---|---|---|---|
| `Target.Self` | Player 或本卡，视 EffectType | 本卡 InstanceId 或 Invalid | 否 |
| `Target.Player` | Player | Invalid | 否 |
| `Target.SingleEnemyPart` | EnemyPart | 调用方选中的部位 | 否 |
| `Target.AllEnemyParts` | EnemyPart，循环展开 | 自动遍历存活部位 | 否 |
| `Target.RandomHandCard` | HandCard | HandZoneService 自选 | 否 |
| `Target.ZoneHandCard` | 延迟选择的 HandCard | Adapter 放行后由对应 Shuffle handler 自选；当前 FromBoth 固定从 Both 选择 | 是 |
| `Target.LastShuffledCard` | HandCard | 当前 Effect Chain 的 `LastShuffledCard` scratch | 否 |
| `Target.SelectedHandCard` | HandCard | `PlayCard` 命令的 `TargetCardInstanceId` | 否 |
| `Target.Adjacent.Right` | EnemyPart | 未实现 | 否 |

`Target.Self` 消歧：

- `Effect.Shuffle.ToRandomZone`、`Effect.Card.AddCost`、`Effect.Card.ReduceCost` 指向本卡。
- `Effect.Card.DiscardSelected`、`Effect.Card.ExhaustSelected` 必须使用 `Target.SelectedHandCard`。
- Damage / Heal / ApplyStatus / Shield 等常规数值效果指向玩家。

HandCard 目标筛选建议：

- AddCost / ReduceCost 作用到 `Target.SelectedHandCard` 时，通常允许普通手牌和左右手锚点。
- DiscardSelected / ExhaustSelected 作用到 `Target.SelectedHandCard` 时，通常只允许普通手牌，拒绝左右手锚点。
- “只作用伙伴”使用 `RequiredTargetKeywords=Card.Keyword.Companion`。
- “不能作用武器”使用 `BlockedTargetKeywords=Card.Keyword.Weapon`。
- 新资产建议显式填写 `HandCardTargetFilter`，避免组合效果变复杂后依赖兼容推断。

## §8 Condition / ZoneHook / Passive

`FEffectCondition`：

```cpp
USTRUCT(BlueprintType)
struct FEffectCondition
{
    FGameplayTag ConditionType;
    FGameplayTag ParamTag;
    int32 ParamInt = 0;
    bool bNegate = false;
};
```

| ConditionType | 语义 | ParamTag | ParamInt |
|---|---|---|---|
| Invalid | 永真 | - | - |
| `Condition.Self.InZone` | 本卡当前在指定区域 | `HandZone.*` | - |
| `Condition.Target.HasStatus` | 目标部位含指定持久状态 | `Status.Poison / Freeze / Twilight / Stunned` | - |

`FCardZoneHook`：

```cpp
USTRUCT(BlueprintType)
struct FCardZoneHook
{
    FGameplayTag Zone;
    FGameplayTag Trigger;
    TArray<FCardEffect> ExtraEffects;
};
```

当前制作触发点是 `ZoneHook.Trigger.OnPlay` 和 `ZoneHook.Trigger.OnPerfectReleaseHit`。

`FCardPassive`：

```cpp
USTRUCT(BlueprintType)
struct FCardPassive
{
    FGameplayTag Trigger;
    FText DisplayText;
    TArray<FCardEffect> Effects;
    FEffectCondition Condition;
    int32 TriggerThreshold = 0;
};
```

| Trigger | 当前制作状态 | 使用 TriggerThreshold? | Effects 是否执行 |
|---|---|---|---|
| `Passive.Trigger.AfterPlayed` | 可执行 | 否 | 是 |
| `Passive.Trigger.OnDiscard` | 可执行 | 否 | 是 |
| `Passive.Trigger.OnCompanionCount` | 特殊回手触发 | 是 | 否 |
| `Passive.Trigger.OnTwilightTriggered` | EventOnly / 展示占位 | 否 | 否 |
| `Passive.Trigger.OnTurnStart` | Reserved | 否 | 否 |
| `Passive.Trigger.OnTurnEnd` | Reserved | 否 | 否 |
| `Passive.Trigger.OnDraw` | Reserved | 否 | 否 |

`DisplayText` 是旧展示文本，不再进入正式卡牌详情面板。被动详情由 `Trigger / Effects / Condition / TriggerThreshold` 通过 WacomApp explanation template 生成；`OnCompanionCount` 的回手结果由 `PassiveOutcomeTemplates` 展示，`Passive.Effects` 仍不会执行。无结构化详情的功能卡可以通过 `UCardDefinition::Description` 获得普通正文回退，但该回退不解析 `{Effect.0}`，也不是规则真相。战斗规则仍只读取结构化字段。

## §9 扩展制作矩阵时的检查点

新增 Effect、Target、MagnitudeSource、Condition 或 Passive trigger 时，同步完成：

1. 运行时 resolver / dispatcher / service 接入。
2. `FWacomBattleRuleContentContract` 放开制作范围。
3. Data validator 错误 / warning 更新。
4. `Wacom.Battle.RuleContentMatrix` transient fixture 覆盖。
5. 真实生成资产 smoke 按需补充。
6. WacomApp UI presentation / explanation template 覆盖；新增 `Magnitude.Source.*` 时同步 `MagnitudeSourceTemplates`。
7. [WacomGameplayTags.md](./WacomGameplayTags.md) 和本文矩阵同步。

如果 validator 允许但 resolver 静默无效，应优先收紧合同或补 resolver，再放开正式制作。
