---
type: data-authoring-reference
scope: wacom-data-authoring
status: active
updated: 2026-07-19
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
| `BuildSlimeTrioContent()` | 幂等生成 SlimeTrio 规则数据、`DA_Encounter_SlimeTrioSingle` 与 `BP_EnemyHost_SlimeTrio`；只读取独立的已提交 Placeholder |
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

SlimeTrio 复用同一个 manifest-driven 晋升实现，但拥有独立目标包和稳定身份，不引用或修改 Snake Placeholder：

```powershell
# 首次晋升 Slime Idle，并从帧 1 / 2 / 3 生成 Left / Core / Right 终态
-run=WacomBuildEnemyPack -Pack=SlimeTrio -PromotePlaceholderArt

# 日常重建 SlimeTrio 数据、Encounter 与三部位 Host
-run=WacomBuildEnemyPack -Pack=SlimeTrio
```

SlimeTrio 同样拒绝 `-PromoteArt`，`-ForceArtRefresh` 只允许与 `-PromotePlaceholderArt` 同用。日常命令和 `WacomRegenerateContent` 只消费 `/Game/Wacom/Art/Placeholders/Enemies/SlimeTrio`，不读取 ignored `/Game/Art`。三个 Part 暂不配置行动动画或奖励卡；不能用 Idle 冒充攻击动画。

单部位敌人紧凑 UI 使用独立、幂等的 Editor builder，不接入 `WacomRegenerateContent`，也不触碰旧 Enemy WBP：

```powershell
# 创建或更新受合同标记管理的新 WBP、Intent Style 与四张像素图标
-run=WacomBuildEnemyUI -BuildSinglePartCompact

# 只读检查父类、bindings、动画、Style 映射和图标资源
-run=WacomBuildEnemyUI -InspectSinglePartCompact

# 只读检查正式 v2 分段生命条、详情面板与完整 Slate 命中祖先链
-run=WacomBuildEnemyUI -InspectSegmentedVitals
```

正式 v2 WBP 的命中修复只能通过 `UWacomEnemyUIToolset::NormalizeInteractiveHitTestPaths()` 在 Writer Package allowlist 内执行。该工具只接受六个已知 Enemy UI 包，只把交互按钮、动态条目容器及其路径上的 `HitTestInvisible` 改为保留子控件命中的 Visibility；遇到 Hidden / Collapsed 路径会拒绝，不会擅自显示内容或修改布局。

玩家状态栏的敌人行动命中反馈使用独立幂等 builder，同样不接入 `WacomRegenerateContent`：

```powershell
# 幂等构建 PlayerStatusBar V2、HUD 左上角位置和 32px 状态图标
-run=WacomBuildPlayerStatusUI -BuildVitalsV2

# 只读检查父类、bindings、Vitals MI、HUD 位置、状态图标和命中策略
-run=WacomBuildPlayerStatusUI -InspectOnly
```

该 builder 只会把玩家状态条与状态图标子树统一设为 `HitTestInvisible`。保存 `BP_BattleHUD` 时只调整 `PlayerStatusBar` 的 Canvas 位置，禁止重写整棵 HUD 的命中可见性；只读审计还会确认 `CommandBar` 允许子按钮参与 Slate Hit Test，避免 Wait / EndTurn 失去点击。

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
| Snake Host 内容包 | `/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake`；按 Definition 顺序生成 Head / Body / Tail typed Part，每段直接拥有错帧 Slime Flipbook Layer 与独立单帧 Destroyed，占位资产禁止正式出包 |
| SlimeTrio 内容包 | `Enemy.SlimeTrio`，Left / Core / Right 三部位；`BP_EnemyHost_SlimeTrio` 横向复用独立 Slime Placeholder Idle，以错帧、尺寸和 Tint 区分，局部 Destroyed 互不影响；无奖励卡和行动动画，禁止正式出包 |
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

- Card / EnemyPart / Enemy / EnemyBehavior / Character 校验 ID、基础数值、必填引用、数组索引、Gameplay tag 命名空间和当前 battle rule content contract。Card 的局部校验还检查下一段稀有度、同族身份、禁止卡种、结构不漂移和实际数值变化；catalog 校验负责跨资产的 CardId/链唯一性、循环、合流与分叉。它们不校验文案质量、数值平衡、流派构筑、固定卡组数量或生成资产路径。
- Enemy 校验 `PartSlotId` 必填且不重复，并在配置 `DefaultBehavior / BehaviorOverride / InitialIntentSetId` 时检查对应 phase 和 intent set 是否存在。
- Scene Enemy Host 的诊断与写入严格分层，不属于 `WacomData` schema 或 Validator mutation。`WacomApp` 的纯 Authoring Report 只读取 Host 的 typed Part/Layer/Anchor SCS 层级与 `EnemyDefinition`；`WacomEditor` 的显式同步为缺失槽位创建 `UWacomBattleEnemyPartComponent`、默认 `Visual_Main` Flipbook Layer 和 ImpactAnchor，并从 `PartDefinition.PartId` 派生 `PartId`。已有 Component Transform、BoxExtent、Paper2D 属性、Layer 与 Anchor 保留；surplus 不删除，多选共用事务，无变化不 dirty package。
- Host validator 检查重复/未知 PartSlotId、PartId mismatch、缺失视觉、重复 LayerId、Visual/Anchor 非直接子组件、多 Anchor、无效 Style 与多个 terminal clip owner。Validator、Report、Map Validate 都不生成、删除或改写组件。
- Sprite/Flipbook Layer 的 Destroyed 资源是可选制作数据；Validator 棻查资源类型、有限正数 Destroyed PlayRate 与 `[0,1]` 换图时机。运行时在真实 authored Paper2D Component 上原地换图，不创建 VisualLayer 镜像。
- `UWacomBattleEnemyPartAnimationStyle` 可选，不影响未配置 Action 素材的 Host Ready。它必须精确指向同一 Part 下唯一 Flipbook `LayerId`；Intent key、Flipbook、PlayRate 与 Impact marker 必须有效。可选 `EnemyDestroyedClip` 在同一 Host 最多出现一次；同步服务不会猜 Clip、Intent 或 terminal owner。
- Host / Part Action Clip 的 `ImpactNormalizedTime` 必须是有限 `[0,1]`，默认和 TrainingWarrior 正式内容均为 `0.55`。它表示行动动画中应用 Journal 行动后 Combat facts 的语义命中点；Destroyed Clip 不消费该字段。内容人员应按接触帧调整，而不是用动画名称或效果类型推断。
- 普通 Scene Enemy UI 不需要在 EnemyDefinition 增加额外字段。恰好一个有效 PartSlot 自动使用单段 WBP；2–4 个有效 PartSlot 使用 Definition 顺序的等宽连续分段，不按 MaxHP 分配宽度。每段 HP、Shield、Initiative、Intent 和 Buff 都来自已有 Snapshot ViewData；Shield 只改变本段外框与徽章，零值不占布局。超过 4 个部位继续给制作警告并应显式配置未来 Boss WidgetClass。Intent 图标仍由 UI-only Style 精确映射，详情面板不从 Intent effects 自动生成伤害或状态说明。
- 正式 Pixel Impact System 的 Graph 只由 `WacomBuildBattleEnemyPartImpactNiagara` 写入。当前生成合同包含六个 Emitter 和固定 `EffectKind=0/1/2/3` 映射；TargetPreview 分支还必须暴露 `User.PreviewMode` 与 `User.AvailabilityIconSize`，分别控制 Available 中心图标和 Valid/Invalid Hover 框。运行 `-InspectOnly` 只读验证版本、User Parameter、Renderer 和编译结果。`Scripts/SetupBattleEnemyPartImpactAssets.py` 幂等写入 Destroyed Style 数值但不覆盖人工声音引用，也不会在已有 DreamShader 材质 / MI 合同正确时重存它们；`Scripts/SetupBattleEnemyPartTargetPreviewAssets.py` 只定向维护目标预演 MI 参数，不修改 Host/Part Blueprint。
- EnemyBehavior 校验 `BehaviorId`、phase、intent set、intent、selector rule、condition、cooldown authoring 和敌人意图 effect contract；可选传入 owning EnemyDefinition 时，会额外校验 `AppliesToPartSlotId / PartDestroyed` 等部位槽引用。
- Character 会校验 `StarterDeck` 不包含左右手卡。
- Shop 校验 `ShopId`、`Offers`、Offer 卡牌和非负价格；强化服务开启时还要求至少一条 White/Blue/Yellow 当前稀有度价格、每档最多一条且价格非负，并拒绝 Intrinsic/Purple/未知稀有度。未配置的合法稀有度在该 Shop 中不可强化。Validator 不决定价格平衡、随机商品池或具体强化数值。
- RunEvent 校验事件图结构、ID、引用、NextNode、卡牌条件 / 效果、卡牌支付筛选和 ZoneId、事件状态目标、RunFlag、压力 ID，以及 `Automatic / Free / Fixed` Action Point policy。正成本非 terminal choice 是错误；早期由 effect 单独扣减探索预算的做法已删除。金币门槛 / 扣费组合中的 authoring 风险可以给 warning。
- RunPickupDefinition 校验固定单一主奖励配置：`PickupId`、奖励类型、金币数量或卡牌引用；可选 `GrantedCredentialIds` 必须全部非 `None` 且 Definition 内唯一。
- RunWorldCardInteractionDefinition 校验 `InteractionId`、至少一个正向卡牌筛选和有效奖励项。
- Actor 摆放实例的 `PersistentId`、重复 ID、receiver 和 facade 配置属于 map / level validation，见 [WacomWorldInteraction.md](./WacomWorldInteraction.md#2-run-world-interactable-actor)。
- Map Floor 与每个 Node 的 `DisplayName` 必填；`ShortDescription` 可空。`MapPosition` 合法闭区间是 `[0,1920] × [0,1080]`，越界或完全重合为 error，节点中心距离小于 `48 px` 为 warning；这些坐标不参与规则距离或合法性。

不要把 Validator 放进 `WacomData`，否则运行时模块会反向依赖编辑器能力。

### Card Upgrade 链制作

每个强化等级必须是独立 `UCardDefinition` package；不要复制一张卡后保留相同 `CardId`，也不要让运行时修改 DataAsset 的 Rarity/Effect。相邻版本填写同一个显式 `UpgradeFamilyId`，前一版本的 `NextUpgradeDefinition` 只指向下一稀有度，Purple 末端保持空。旧卡不准备进入强化系统时两个新字段都保持空。

单资产 Data Validation 会沿可达链检查合法边和结构稳定性；正式 manifest、定向制作工具或内容测试还必须把整组候选传给 `FWacomCardUpgradeCatalogValidation::Validate()`，以捕获只看单链无法发现的重复 CardId、多个前驱、同族多根、合流或环。Shop 价格表只决定当前商店开放的稀有度档和金币，不应复制卡牌数值。Spec 019 没有创建或保存任何 Card/Shop 资产；首批 Production 链、价格和 WBP 由 Spec 020 单独授权后制作。

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

其中 `DA_Floor_Main_01` 与 `L_Run_Floor_Main_01` 已由独立 Floor 1 Production 场景轮创建；`DA_Journey_Main_01`、Floor 2/3 FloorDefinition 与 world 仍不存在，也不授权通用 builder 或迁移脚本顺带创建。正式三层各 20 Node/21 Edge canonical graph、内容槽、跨层门槛与 Journey 节奏见 [WacomMap](./WacomMap.md#wacommap) §9；不得创建最小空壳图、伪 FloorEntrance 或 terminal Actor 特例绕过 Production readiness gate。

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

Floor 1 的 15 个节点 Definition 已从“只预留职责”升级为完整内容合同。Spec 011 冻结的 38 个核心 DataAsset 与击倒分支合同追加的 8 张 CardDefinition 均已按下表创建，总量为 `38 core + 8 branch reward = 46`：

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

BehaviorId 使用 `SerpentWood.<Archetype>.Behavior`，PartId 使用 `SerpentWood.<Archetype>.<Part>`。所有 11 个正式部位已经清空 deprecated `KnockdownRewardCard`，并按所属 Archetype 显式填写 `AidRewardCard` 与 `DestroyRewardCard`。四组身份为 `Reward.SerpentWood.<Archetype>.Aid/Destroy`；完整规则字段见 [WacomData.md](./WacomData.md) §13。Spec 011 的历史 38 core package 与下列 8 张分支奖励卡共同构成当前 Floor 1 Production manifest：

| CardId | Package path |
|---|---|
| `Reward.SerpentWood.BrushSnake.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/DA_Card_BrushSnake_Aid` |
| `Reward.SerpentWood.BrushSnake.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/BrushSnake/DA_Card_BrushSnake_Destroy` |
| `Reward.SerpentWood.MoltGuard.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/DA_Card_MoltGuard_Aid` |
| `Reward.SerpentWood.MoltGuard.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/MoltGuard/DA_Card_MoltGuard_Destroy` |
| `Reward.SerpentWood.RootStalker.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/DA_Card_RootStalker_Aid` |
| `Reward.SerpentWood.RootStalker.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/RootStalker/DA_Card_RootStalker_Destroy` |
| `Reward.SerpentWood.ShallowGuardian.Aid` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/DA_Card_ShallowGuardian_Aid` |
| `Reward.SerpentWood.ShallowGuardian.Destroy` | `/Game/Wacom/Data/Cards/Rewards/SerpentWood/ShallowGuardian/DA_Card_ShallowGuardian_Destroy` |

Card object name 与 package leaf 相同；内部 CardId 才是稳定规则身份。Aid/Destroy 每个部位只引用所属敌人的卡对，不按部位或节点另建 Definition。Encounter 敌人组合、24 条 Intent、4 张核心新卡、Pickup/Shop/RunEvent 精确字段见 [WacomData.md](./WacomData.md) §13。

EnemyPart 奖励校验分两档：

| Profile | 允许 | 拒绝 |
|---|---|---|
| `General` | 无奖励、纯 legacy、只使用一个或两个显式新字段 | legacy 与任一新字段混填 |
| `FormalProduction` | Aid/Destroy 两个显式字段都存在且 legacy 为空 | 缺任一显式分支、残留 legacy、任何混填 |

TrainingWarrior 与 Snake 的现有二进制资产继续由 General + legacy fallback 读取。两个 builder 的未来写入源码已经改为同时填写 Aid/Destroy 并清空 legacy，但在资产所有者授权迁移前不得运行 builder 或重存资产。删除 legacy 字段必须等待 TrainingWarrior、Snake 和全部正式 Part 完成授权迁移，并由 AssetRegistry/引用审计证明零旧字段依赖。

Wayfarer 允许只读引用三张现有正式 Starter 卡和 `/Game/Wacom/Data/Cards/Rewards/DA_Card_PoisonFang`；该卡 live `CardId=PoisonFang`，不新建主题副本。Production Definition 不得引用 `Debug`、Authoring、`Test.*`、BadgeDisplayTests 或 TrainingWarrior 内容；TrainingWarrior 只作为当前 enemy-pack 制作范式参考。

正式制作入口是 `WacomBuildFormalFloor1Content` commandlet 与 `Wacom.BuildFormalFloor1Content` Editor console command。两者共享 exact 46-entry manifest，并按 `Cards=12`、`EnemyGraph=19`、`NodeDefinitions=15` 分组；默认只读 inspect，只有显式 `SeedMissing` 才创建缺失 package。已有正确 class 的资产永不覆盖、重存或恢复 seed defaults；`CompareSeedDefaults` 只用于首次验收，后续日常验证默认只守稳定 ID、class、引用和有序结构，允许人工继续调 DisplayName、描述和数值。入口没有 Force/replace/regenerate 模式。Starter 与 PoisonFang 始终是只读依赖；不得通过全量 `WacomRegenerateContent` 或 Debug builder 顺带写入地图、Player/Host Blueprint、UI、材质、卡牌表现、背包或其它 Agent 资产。

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

### Floor 2 MoltCavern Production 内容制作合同

Spec 017 已将上述 15 个节点从“只预留职责”升级为完整内容设计合同，Spec 018 已按该合同创建并验证精确 47 个资产：

| Type | Count | Contract |
|---|---:|---|
| Encounter Definition | 7 | `Encounter.MoltCavern.*` 七个节点内容 |
| RunEvent Definition | 3 | CastoffEcho / LostDelver / MoltingRite |
| Pickup Definition | 4 | FungalCache / MineralCache / VenomCrystalCache / MoltSeal |
| Shop Definition | 1 | `Shop.MoltCavern.DeepWayfarer` |
| Card Definition | 12 | 4 固定 Pickup/Run 卡 + 8 Aid/Destroy 卡 |
| Enemy Definition | 4 | ScaleCrawler / StoneScaleGuard / VenomHunter / CavernGuardian |
| Enemy Behavior Definition | 4 | 每敌人一份 `Default` + per-Part `Sequence` |
| Enemy Part Definition | 12 | `2 + 3 + 3 + 4` 个部位 |
| **Production total** | **47** | exact manifest 已创建；首次 strict 验收完成 |

主题路径固定为：

```text
/Game/Wacom/Data/Enemies/MoltCavern/<Archetype>/
/Game/Wacom/Data/Encounters/MoltCavern/
/Game/Wacom/Data/Events/MoltCavern/
/Game/Wacom/Data/Pickups/MoltCavern/
/Game/Wacom/Data/Shops/MoltCavern/
/Game/Wacom/Data/Cards/Rewards/MoltCavern/
/Game/Wacom/Data/Cards/Rewards/MoltCavern/<Archetype>/
/Game/Wacom/Data/Cards/Run/MoltCavern/
```

Enemy 资产 leaf 使用 `DA_Enemy_<Archetype>`、`DA_Behavior_<Archetype>`、`DA_Part_<Archetype>_<Part>`；其它 leaf 使用 `DA_Encounter_<Slot>`、`DA_Event_<Slot>`、`DA_Pickup_<Slot>`、`DA_Shop_DeepWayfarer` 与 `DA_Card_<CardName>`。精确 47-package 表见 Spec 017 的 production manifest 与 Spec 018 的 asset manifest；writer allowlist 必须从该 exact set 明确列出，不能扫描整个主题目录扩大保存范围。

四个 EnemyId 为：

```text
Enemy.MoltCavern.ScaleCrawler
Enemy.MoltCavern.StoneScaleGuard
Enemy.MoltCavern.VenomHunter
Enemy.MoltCavern.CavernGuardian
```

BehaviorId 使用 `MoltCavern.<Archetype>.Behavior`，PartId 使用 `MoltCavern.<Archetype>.<Part>`，IntentSet/Intent 分别追加 `.Sequence` 与 `.<Intent>`。所有 12 个正式 Part 已显式配置同 Archetype 的 `Reward.MoltCavern.<Archetype>.Aid/Destroy` 并清空 deprecated `KnockdownRewardCard`；后续修改仍必须通过 `FormalProduction` profile。

卡牌路径细分：四张固定卡中 GlowcapPoultice、CrystalWard、VenomShard 位于 Rewards/MoltCavern 根，MoltSeal 位于 Cards/Run/MoltCavern；八张分支卡按 Archetype 子目录保存。DeepWayfarer 只读引用以下既有 package，不计入 47，也不得由未来 Floor 2 seeder 保存：

```text
/Game/Wacom/Data/Cards/Rewards/SerpentWood/DA_Card_HerbalPoultice
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_ChitinWard
/Game/Wacom/Data/Cards/BugGirl/StarterPack/DA_Card_Starter_MoltCut
```

Floor 1/2 现共用 `WacomEditor/Private` 的 formal production seed service；主题差异仅由 profile 提供 manifest、稳定身份、初始字段、只读依赖和特有不变量。Floor 2 入口为 commandlet `WacomBuildFormalFloor2Content` 与 Editor command `Wacom.BuildFormalFloor2Content`，参数固定为 `Group=Cards|EnemyGraph|NodeDefinitions|All`、`SeedMissing`、`CompareSeedDefaults` 与 `Report=`。默认 inspect-only；仅 `SeedMissing` 创建缺失 package；错误 class/结构在首次保存前 fail closed；已有正确 class 永不覆盖、重存或恢复 seed defaults；`Force/Replace/Regenerate` 与未知参数被拒绝。首次交付已通过 47/47 load、AssetRegistry allowlist closure、SHA-256、三组第二遍 `0 created / 0 saved`、Git LFS 与 Battle/Run smoke。DisplayName、描述和批准的平衡字段后续允许人工调优；stable ID、class、引用、关键词、TargetMode、effect/condition/choice/slot/intent 有序结构由校验守护。

Production Definition 不得引用 Debug、Authoring、`Test.*`、BadgeDisplayTests、TrainingWarrior、Character、地图、Host、UI、材质或卡牌表现资产。`DA_Character_BugGirl` 的既有 StarterDeck 污染是用户已接受的外部问题：不属于 Floor 2 manifest，不在本轮修改，也不能通过削弱 validator 隐藏或误报为 closure 通过。

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

三层共 46 个节点内容槽。Floor 1 的 15 个槽已经由 Spec 011 冻结并由 Spec 014 创建 46 个支持 DataAsset，Floor/map/Host 灰盒由 Spec 015 独立落地；Floor 2 的 15 个槽已经由 Spec 017 冻结并由 Spec 018 创建 47 个支持 DataAsset；Floor 3 的 16 个槽仍只冻结职责和命名入口。现有 `DA_Event_Debug*`、`DA_Shop_Debug*`、`DA_Pickup_Debug*`、`DA_RunWorldCardInteraction_Debug*` 与 `DA_Card_DebugKey` 只能服务 Debug/测试，不得作为 Production typed payload、蛇印/蜕印或终局占位。

`Pickup.SerpentWood.SerpentSigil` 的正式 Definition 必须以 Card 作为固定主奖励，并在 `GrantedCredentialIds` 中授予 `Credential.Run.SerpentSigil`；`Node.Exit.01` 的 FloorEntrance 只在 `RequiredCredentialIds` 中引用该 Credential，不把 `Card.Run.SerpentSigil` 写回 `OwnedCardRequirements`。两者由同一稳定 ID 对接，但表现卡和资格状态互不推断。

`Pickup.MoltCavern.MoltSeal` 使用相同通用合同：固定主奖励为 `Card.Run.MoltSeal`，`GrantedCredentialIds` 授予 `Credential.Run.MoltSeal`；Floor 2 `Node.Exit.01` 只要求 Credential 并指向 `Floor.Main.03`。不得从表现卡推断、撤销或补算资格。

Floor 3 `Node.Guardian.01` 是无出边的 success terminal，不配置 FloorEntrance payload。Production `DA_Journey_Main_01` 必须配置 `DisplayName` 和 `SuccessTerminalNode={Floor.Main.03, Node.Guardian.01}`。`/Game/Wacom/Data/Map/Production/` 下缺失 terminal 是 validation error；旧 Debug/Authoring Journey 可保持未配置并产生 warning，Runtime 仍允许启动但不会自动成功。已配置 terminal 在 Editor 与 Runtime 都必须位于最后一层、引用存在的 `Encounter + bBoss=true` 节点、从 Entry 可达、无出边，且最后一层不得包含 FloorEntrance。不得用 Actor label、Level Blueprint、EncounterId、legacy `bRunActive` 或伪 TargetFloorId 实现终局。

Map validator 会拒绝空/重复 Credential requirement，以及不存在于入口前置不可绕过固定 Pickup 中的 grant。现有 Debug Pickup 默认 grant 数组为空，不能被晋升或复制成 Production 蛇印入口占位。

蛇印/蜕印凭证门禁、Floor 2/3 图、通用 Journey success、Floor 1 内容/奖励以及 Floor 2 内容均已冻结。Floor 1 的 46 个 Production DataAsset 与 Floor/map/Host 灰盒已完成真实审计；Floor 2 的 47 个 DataAsset 也已完成真实加载、闭包、哈希与幂等审计，但 Floor/map/Host 尚未创建。剩余 Production 阻塞是 Floor 2 场景、Floor 3 内容设计/资产/场景、完整 Production Journey、跨层 world transition、正式美术和平衡验收；这些工作不得反向覆盖已人工调优的 Floor 1/2 资产或迁移 `L_Exploration`。Floor 1 原始图门禁见 Spec 007，通用 Credential 见 Spec 008，图/pacing 见 Spec 009，成功合同见 Spec 010，Floor 1 内容/资产见 Spec 011–015，Floor 2 内容与资产见 Spec 017–018。

Floor 1 Production 场景入口是 `WacomBuildFormalFloor1ProductionScene` commandlet 与 `Wacom.BuildFormalFloor1ProductionScene` Editor console command。二者共享 exact 7-package manifest，分组为 `Floor=1`、`EnemyHosts=4`、`Scene=2`：

```text
/Game/Wacom/Data/Map/Production/DA_Floor_Main_01
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/BP_EnemyHost_BrushSnake_Graybox
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/BP_EnemyHost_MoltGuard_Graybox
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/BP_EnemyHost_RootStalker_Graybox
/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/BP_EnemyHost_ShallowGuardian_Graybox
/Game/Wacom/Run/SceneActors/Graybox/BP_WacomRunFloorEntranceMarker_Graybox
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

默认运行只 inspect；只有显式 `SeedMissing` 才创建缺失 package，`Force` 被拒绝。已有正确 class 的资产不覆盖、不重存、不恢复 seed defaults；`CompareSeedDefaults` 仅用于首次播种和连续第二次 `0 created / 0 saved` 验收。四个 Enemy Host 通过现有 `SyncPartsFromDefinition` 初始生成 11 个 Part，使用受控 Placeholder 只服务灰盒可见性和命中制作验证；正式发布必须替换这些引用。新 map 固定 `1 Descriptor / 20 Anchors / 21 Paths / 4 BranchTargets / 16 content Hosts / 8 enemy Hosts / 11 viewpoints`，内容 Host 绑定真实 Spec 014 Definition，禁止 Debug/Authoring/Test/legacy-map 引用。Exit marker 只有可见 primitive、`RunMapNodeBinding` 与实例 `PersistentId`，没有交互、travel 或 Level Blueprint 规则。

该入口只能通过正式 Unreal MCP writer lease 保存 exact package allowlist；Editor 生命周期内不切 branch、不更新 HEAD、不编译。保存后必须做 AssetRegistry/failed-load、五个 Blueprint compile、Floor/scene validator、SHA-256、Git LFS 和第二次幂等审计。它不是可重复覆盖的关卡 builder；首次播种后世界 transform、Host 摆放和 Blueprint 表现都转为人工权威。

Floor 1 直接关卡 PIE 使用独立 Preview bootstrap，而不修改上述七包 seeder 或创建正式 Journey。控制台入口固定为 `WacomSeedFormalFloor1PreviewBootstrap`，唯一允许保存的两个 Package 是：

```text
/Game/Wacom/Run/Preview/GM_WacomRunFloorPreview
/Game/Wacom/Maps/Run/L_Run_Floor_Main_01
```

Preview GameMode Blueprint 只允许缺失时创建，父类必须是 `AWacomRunFloorPreviewGameMode`；它从 `/Game/Wacom/Core/GameModes/GM_Wacom` 复制 PlayerController、DefaultPawn/PlayerCharacter、Character、BattleHUD、ExplorationHUD 和 JourneySummary 表现配置，并强制 `DefaultJourneyDefinition=null`。若同名资产已存在但父类、编译状态或任一配置不符，命令拒绝覆盖。地图修改面只允许把 World Settings GameMode Override 指向 Preview Blueprint，并在缺失时于唯一 `Node.Entry` Anchor transform 创建一个无 Run 身份的 `PlayerStart_FloorMain01Preview`；任何其它 PlayerStart、未知 GameMode、场景合同漂移或无效 Entry 都 fail closed。

该命令每次运行前后都复用 Spec 015 的严格场景校验，不调用 Spec 014/015/Debug builder，不修改 Floor 图、Anchor、Path、BranchTarget、Host 或人工 transform；保存并重载后必须在同一命令内完成第二遍 `0 created / 0 modified / 0 saved`。它只服务 Editor PIE，地图引用 Preview GameMode 是 release blocker；完整 Production Journey/Floor 2/3 启动接管后必须移除这项依赖。

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
| `Wacom.Data.Enemy.SlimeTrio` | 验证 SlimeTrio 稳定 ID、规则数值、Placeholder 闭包、三部位 Host authoring 合同，以及 Builder 幂等性 |
| `Wacom.Data.BattleStarterContent.StarterPackAssetValidation` | 检查 starter pack 核心字段、BugGirl 初始牌组排除关系、测试卡不进入 StarterDeck 和 Snake Behavior intent set |
| `Wacom.Data.BattleStarterContent.BadgeDisplayTestCardAssetValidation` | 检查 badge 测试卡、DebugSnake 商店 0 金币出售和 CardPresentationBuilder badge view |
| `Wacom.Data.Shop.DebugSnakeAssetValidation` | 验证 DebugSnake 商店能通过 validator |
| `Wacom.Data.Shop.DebugSnakeAsset` | 验证商品顺序和价格，避免内容生成漂移 |
| `Wacom.Data.RunEvent.DebugSnakeGiftAsset` | 验证蛇巢遗物事件节点、选项、条件、效果和毒牙引用 |
| `Wacom.Data.RunEvent.DebugFlagRewardAsset` | 验证 RunFlag、金币门槛 / 扣费、奖励和 reset flags 样例 |
| `Wacom.Battle.GeneratedStarterContent` | 使用真实生成资产进入 `UBattleSession`，验证核心卡牌、辅助卡和 Snake 意图能产生预期 Snapshot/Event |
| `Wacom.Editor.FormalFloor1Content.Manifest` | 对 exact 46-entry manifest、分组、seed-only/no-overwrite、stable/strict comparator 与冻结结构做 transient 验证 |
| `Wacom.Data.FormalFloor1Content` | 真实加载 46 个 SerpentWood Production DataAsset，核对 class/count、稳定 ID/引用、有序结构与首次 seed defaults |
| `Wacom.Editor.FormalProductionContentSeedService` | 验证 Floor 1/2 共用 parser、inspect-only、no-overwrite、comparator 与 profile facade 一致性 |
| `Wacom.Editor.FormalFloor2Content.Manifest` | 对 exact 47-entry manifest、分组、synthetic missing inspect、transient defaults 与 comparator 边界做验证 |
| `Wacom.Data.FormalFloor2Content` | 真实加载 47 个 MoltCavern DataAsset，核对 strict defaults、稳定引用与 AssetRegistry allowlist closure |

这些测试证明“生成/播种资产字段仍符合当前合同”，不是正式内容平衡、卡牌表现或世界场景验收。

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
