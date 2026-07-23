"""Apply the formal enemy interaction outline and sprite-collision asset contract.

The caller must hold the enemy writer lease with the exact package allowlist declared by
this script. The outline material must already be generated from its DreamShader source.
Headless callers must open /Engine/Maps/Entry so Blueprint template reconstruction cannot
touch or reinstance placed actors in a project map.
Pass -WacomOutlineMaterialOnly to update only the target-preview Style reference. No map,
flipbook, definition, rule, or unrelated UI asset is saved here.
"""

import unreal


OUTLINE_MATERIAL = "/Game/DreamMaterials/World/M_WacomBattleEnemyPartInteractionOutline"
TARGET_PREVIEW_STYLE = (
    "/Game/Wacom/UI/Battle/WorldImpact/"
    "DA_BattleEnemyPartTargetPreviewStyle_PixelLock"
)
STABLE_SPRITES = [
    "/Game/Wacom/Art/Enemies/TrainingWarrior/Sprites/"
    "SPR_Enemy_TrainingWarrior_Idle_00",
    "/Game/Wacom/Art/Placeholders/Enemies/Snake/Sprites/"
    "SPR_Enemy_SnakePlaceholder_Idle_00",
    "/Game/Wacom/Art/Placeholders/Enemies/SlimeTrio/Sprites/"
    "SPR_Enemy_SlimeTrioPlaceholder_Idle_00",
]
HOST_BLUEPRINTS = [
    "/Game/Wacom/Core/Enemy/BP_EnemyHost_TrainingWarrior",
    "/Game/Wacom/Core/Enemy/BP_EnemyHost_Snake",
    "/Game/Wacom/Core/Enemy/BP_EnemyHost_SlimeTrio",
    "/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug",
    "/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/BrushSnake/"
    "BP_EnemyHost_BrushSnake_Graybox",
    "/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/MoltGuard/"
    "BP_EnemyHost_MoltGuard_Graybox",
    "/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/RootStalker/"
    "BP_EnemyHost_RootStalker_Graybox",
    "/Game/Wacom/Run/SceneActors/Enemies/SerpentWood/ShallowGuardian/"
    "BP_EnemyHost_ShallowGuardian_Graybox",
]


def require_asset(path, label):
    asset = unreal.load_asset(path)
    if not asset:
        raise RuntimeError(f"Missing {label}: {path}")
    return asset


def configure_style(outline_material):
    style = require_asset(TARGET_PREVIEW_STYLE, "target preview Style")
    values = {
        "outline_material": outline_material,
        "selectable_outline_color": unreal.LinearColor(0.45, 0.16, 0.015, 1.0),
        "selectable_outline_thickness_source_pixels": 1.0,
        "selectable_outline_alpha": 0.55,
        "hovered_outline_color": unreal.LinearColor(0.80, 0.34, 0.025, 1.0),
        "hovered_outline_thickness_source_pixels": 2.0,
        "hovered_outline_alpha": 0.95,
        "outline_outer_color_multiplier": 0.45,
        "outline_outer_alpha_multiplier": 0.70,
    }
    for name, value in values.items():
        style.set_editor_property(name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(style)


def configure_stable_sprites():
    for path in STABLE_SPRITES:
        sprite = require_asset(path, "stable interaction Sprite")
        unreal.WacomEnemyInteractionAuthoringLibrary.configure_stable_interaction_sprite(
            sprite)
        unreal.EditorAssetLibrary.save_loaded_asset(sprite)


def configure_hosts():
    for path in HOST_BLUEPRINTS:
        blueprint = require_asset(path, "formal enemy Host Blueprint")
        result = (
            unreal.WacomEnemyInteractionAuthoringLibrary
            .apply_interaction_layer_contract_to_host_blueprint(blueprint)
        )
        if result < 0:
            raise RuntimeError(
                f"Host must contain exactly one direct typed visual per Part: {path}")
        unreal.EditorAssetLibrary.save_loaded_asset(blueprint)


material = require_asset(OUTLINE_MATERIAL, "DreamShader-generated outline material")
if "-wacomoutlinematerialonly" in unreal.SystemLibrary.get_command_line().lower():
    configure_style(material)
    unreal.log("Wacom enemy interaction outline Style configured: style=1")
else:
    configure_style(material)
    configure_stable_sprites()
    configure_hosts()
    unreal.log(
        "Wacom formal enemy interaction assets configured: "
        f"material=1 style=1 sprites={len(STABLE_SPRITES)} hosts={len(HOST_BLUEPRINTS)}"
    )
