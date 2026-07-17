---
type: devlog
scope: wacom-materials
status: active
updated: 2026-05-24
tags:
  - wacom/devlog
  - wacom/materials
  - dreamshader
---

# DreamShader 材质试写

本次先按 DreamShader 官方文档和项目现有 `DShader` 约定，建立材质源码分类，并新增两个 UI 动态材质样例：

- 源文件：`DShader/Material/UI/M_Wacom_SimplePulse_UI.dsm`
- 生成目标：`/Game/DreamMaterials/M_Wacom_SimplePulse_UI.M_Wacom_SimplePulse_UI`
- 用途：作为后续卡牌、面板或按钮背景的 DreamShader 写法样例。
- 源文件：`DShader/Material/UI/M_UI_BloodBar.dsm`
- 生成目标：`/Game/DreamMaterials/UI/M_UI_BloodBar.M_UI_BloodBar`
- 用途：UI 血条材质，使用 `/Game/Asset/Texture/Noise01` 和 `/Game/Asset/Texture/noise03` 做血液噪声、流动纹理和低血量脉冲。

约定：

- DreamShader 源文件按用途放在 `DShader/Material/UI`、`DShader/Material/Card`、`DShader/Material/Procedural` 等子目录。
- 新增材质生成资产默认放到 `/Game/DreamMaterials/<Category>`，例如 UI 材质放 `/Game/DreamMaterials/UI`。
- 已有 `.dsm` 只移动源文件做分类，暂不修改 `Shader(Name="DreamMaterials/...")` 的生成路径，避免复制出一批重复 `uasset` 或破坏现有引用。
- 材质参数使用 `VectorParameter` / `ScalarParameter`，并写 `Group`、`SortPriority`、`Description`，方便后续美术调参。
- 优先在 `Graph` 中使用 `UE.*` / `UE.Expression(...)` 描述 Unreal 材质节点；只有 DreamShader 图语法不适合表达的复杂 HLSL 逻辑才拆到 `DShader/Shared/*.dsh` 或局部 `Function`。
- `M_UI_BloodBar` 已改成 Graph-first 写法，显式使用 `UE.Panner`、`UE.Expression(Class="TextureSample")`、`UE.Expression(Class="SmoothStep")`、`UE.Expression(Class="Sine")` 等节点。
- 从 `M_CosmicBlob` 拆出可复用能量母题：`DShader/Shared/CosmicEnergy.dsh` 输出 `distortedUV`、`energyMask`、`sourceColor`，示例材质 `DShader/Material/Procedural/M_CosmicEnergy_Motif.dsm` 负责上色和噪声细节。
- 基于 `CosmicEnergy` 新增卡面弱流光材质：`DShader/Material/Card/M_CardSurface_CosmicFoil.dsm`，生成到 `/Game/DreamMaterials/Card/M_CardSurface_CosmicFoil.M_CardSurface_CosmicFoil`，供 `UWacomCardView::SurfaceFoilOverlay` 作为卡牌表面装饰层使用；`SurfaceFoilOverlay` 必须由 WBP 显式绑定，未绑定时不会运行时创建覆盖层。
- 历史试验曾使用 `M_FirstPersonCard_FeedbackEdge.dsm + InteractionFeedbackImage` 承载 pressed / confirm / commit / deny。该路径现已收口删除：Pressed/Commit 只使用 Slot Motion 与 Card Depth，规则确认前不再播放 Confirm，formal-release Deny 使用阻尼 shake 与 CardView Slate 四角硬像素刻线。不要从这条开发记录恢复旧 DreamShader、生成材质或 WBP Image。

血条材质参数口径：

- `HealthPercent`：当前血量比例，0 到 1。
- `DamagePreviewPercent`：血量后方的受击预览段宽度，0 到 1。
- `SoftNoise` / `VeinNoise`：默认引用项目噪声贴图，通过 `TextureSample` 节点采样，可在实例中替换为其他 `/Game/Asset/Texture` 噪声。
- `FlowSpeed`、`NoiseScale`、`VeinScale`、`GlowIntensity`、`PulseStrength`：控制流动速度、噪声尺度、血量边缘高光和低血量脉冲。

CosmicEnergy 母题口径：

- `CosmicEnergy_Field`：保留 `CosmicBlob` 的迭代坐标扰动和能量累积，但把结果拆成可复用输出。
- `distortedUV`：用于扰动其他贴图、UI 图案或卡牌底纹。
- `energyMask`：用于重新上色、控制透明度、驱动边缘高光或状态特效强度。
- `sourceColor`：保留原始 CosmicBlob 的彩色变化，示例材质中只少量混入，避免不同用途看起来都像同一个星云材质。
- 示例材质默认生成到 `/Game/DreamMaterials/Procedural/M_CosmicEnergy_Motif.M_CosmicEnergy_Motif`。
