"""Create a local BattleWarrior-backed enemy Host setup for PIE validation.

The generated assets live below /Game/Art, which is provided by the worktree's
D-drive local asset junction and is intentionally ignored by Git. The script never
modifies the source Debug Snake Host or any formal /Game/Wacom asset.

Generated assets:
- /Game/Art/WacomPIE/EnemyHostAnimation/DA_EnemyHostAnimation_BattleWarrior_PIE
- /Game/Art/WacomPIE/EnemyHostAnimation/BP_SnakeHost_BattleWarrior_PIE

Run with UnrealEditor-Cmd -ExecutePythonScript=... while this worktree's
interactive editor is closed.
The operation is idempotent: existing generated assets are updated in place.
"""

import unreal


LOCAL_ROOT = "/Game/Art/WacomPIE/EnemyHostAnimation"
STYLE_NAME = "DA_EnemyHostAnimation_BattleWarrior_PIE"
STYLE_PATH = f"{LOCAL_ROOT}/{STYLE_NAME}"
HOST_NAME = "BP_SnakeHost_BattleWarrior_PIE"
HOST_PATH = f"{LOCAL_ROOT}/{HOST_NAME}"

SOURCE_HOST_PATH = "/Game/Wacom/Core/Enemy/BP_SnakeHost_Debug"
BATTLE_WARRIOR_ROOT = "/Game/Art/PaperAssets/Party/BattleWarrior"
IDLE_PATH = f"{BATTLE_WARRIOR_ROOT}/BattleWarrior__Idle"
ATTACK_PATH = f"{BATTLE_WARRIOR_ROOT}/BattleWarrior__Attack"
BLOCK_PATH = f"{BATTLE_WARRIOR_ROOT}/BattleWarrior__Block"
CLEAVE_PATH = f"{BATTLE_WARRIOR_ROOT}/BattleWarrior__Cleave"
DOWNED_PATH = f"{BATTLE_WARRIOR_ROOT}/BattleWarrior__Downed"

# Explicit authoring choices for the current Debug Snake behavior. Runtime code still
# performs exact IntentId lookup and never infers a clip from an Intent or asset name.
BLOCK_INTENT_IDS = (
    "Snake.Head.CoiledGuard",
    "Snake.Body.Harden",
    "Snake.Tail.Brace",
)
CLEAVE_INTENT_IDS = (
    "Snake.Body.Slam",
    "Snake.Body.VenomMist",
    "Snake.Tail.Sweep",
    "Snake.Tail.Tangle",
)


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def require_paper_flipbook(asset_path, label):
    flipbook = load_required(asset_path, label)
    if not isinstance(flipbook, unreal.PaperFlipbook):
        raise RuntimeError(
            f"{label} must be a PaperFlipbook, got {flipbook.get_class().get_name()}: "
            f"{asset_path}"
        )
    return flipbook


def make_clip(flipbook, play_rate):
    clip = unreal.WacomBattleEnemyHostAnimationClip()
    clip.set_editor_property("flipbook", flipbook)
    clip.set_editor_property("play_rate", play_rate)
    return clip


def load_or_create_style(attack, block, cleave, downed):
    style = unreal.load_asset(STYLE_PATH)
    if not style:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class", unreal.WacomBattleEnemyHostAnimationStyle)
        style = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            STYLE_NAME,
            LOCAL_ROOT,
            unreal.WacomBattleEnemyHostAnimationStyle,
            factory,
        )
    if not style:
        raise RuntimeError(f"Failed to create enemy Host Animation Style: {STYLE_PATH}")

    style.set_editor_property("default_action_clip", make_clip(attack, 0.75))
    style.set_editor_property("destroyed_clip", make_clip(downed, 0.75))

    intent_clips = {}
    for intent_id in BLOCK_INTENT_IDS:
        intent_clips[unreal.Name(intent_id)] = make_clip(block, 1.0)
    for intent_id in CLEAVE_INTENT_IDS:
        intent_clips[unreal.Name(intent_id)] = make_clip(cleave, 0.75)
    style.set_editor_property("action_clips_by_intent_id", intent_clips)
    unreal.EditorAssetLibrary.save_loaded_asset(style)
    return style


def load_or_create_local_host(style, idle):
    host_blueprint = unreal.load_asset(HOST_PATH)
    if not host_blueprint:
        source_blueprint = load_required(SOURCE_HOST_PATH, "Debug Snake Host source")
        host_blueprint = unreal.EditorAssetLibrary.duplicate_asset(
            SOURCE_HOST_PATH, HOST_PATH)
        if not host_blueprint:
            raise RuntimeError(
                f"Failed to duplicate {source_blueprint.get_name()} to {HOST_PATH}")

    generated_class = host_blueprint.generated_class()
    if not generated_class:
        raise RuntimeError(f"Generated local Host has no generated class: {HOST_PATH}")
    host_cdo = unreal.get_default_object(generated_class)
    host_cdo.set_editor_property(
        "host_authoring_mode",
        unreal.WacomBattleEnemyHostAuthoringMode.SIMPLE_HOST_VISUAL,
    )
    host_cdo.set_editor_property(
        "host_visual_mode",
        unreal.WacomBattleEnemyHostVisualMode.FLIPBOOK,
    )
    host_cdo.set_editor_property("host_flipbook", idle)
    host_cdo.set_editor_property("host_animation_style", style)
    host_cdo.set_editor_property("host_flipbook_play_rate", 1.0)
    host_cdo.set_editor_property("loop_host_flipbook", True)
    host_cdo.set_editor_property("auto_play_host_flipbook", True)
    host_cdo.set_editor_property("host_visual_visible", True)
    unreal.EditorAssetLibrary.save_loaded_asset(host_blueprint)
    return host_blueprint, host_cdo


def validate_generated_setup(style, host_blueprint, host_cdo, idle, attack, downed):
    if not unreal.EditorAssetLibrary.does_asset_exist(STYLE_PATH):
        raise RuntimeError(f"Generated Style was not saved: {STYLE_PATH}")
    if not unreal.EditorAssetLibrary.does_asset_exist(HOST_PATH):
        raise RuntimeError(f"Generated Host was not saved: {HOST_PATH}")

    default_clip = style.get_editor_property("default_action_clip")
    destroyed_clip = style.get_editor_property("destroyed_clip")
    intent_clips = style.get_editor_property("action_clips_by_intent_id")
    if default_clip.get_editor_property("flipbook") != attack:
        raise RuntimeError("Generated Style default Action clip does not match BattleWarrior Attack")
    if destroyed_clip.get_editor_property("flipbook") != downed:
        raise RuntimeError("Generated Style Destroyed clip does not match BattleWarrior Downed")
    if len(intent_clips) != len(BLOCK_INTENT_IDS) + len(CLEAVE_INTENT_IDS):
        raise RuntimeError("Generated Style explicit Intent map has an unexpected entry count")
    if host_cdo.get_editor_property("host_flipbook") != idle:
        raise RuntimeError("Generated Host Idle does not match BattleWarrior Idle")
    if host_cdo.get_editor_property("host_animation_style") != style:
        raise RuntimeError("Generated Host does not reference the generated Animation Style")

    unreal.log(
        "WACOM_ENEMY_HOST_ANIMATION_SETUP_OK "
        f"Style={style.get_path_name()} Host={host_blueprint.get_path_name()} "
        f"ExplicitIntentClips={len(intent_clips)}"
    )


unreal.EditorAssetLibrary.make_directory(LOCAL_ROOT)
idle_flipbook = require_paper_flipbook(IDLE_PATH, "BattleWarrior Idle")
attack_flipbook = require_paper_flipbook(ATTACK_PATH, "BattleWarrior Attack")
block_flipbook = require_paper_flipbook(BLOCK_PATH, "BattleWarrior Block")
cleave_flipbook = require_paper_flipbook(CLEAVE_PATH, "BattleWarrior Cleave")
downed_flipbook = require_paper_flipbook(DOWNED_PATH, "BattleWarrior Downed")

animation_style = load_or_create_style(
    attack_flipbook,
    block_flipbook,
    cleave_flipbook,
    downed_flipbook,
)
local_host_blueprint, local_host_cdo = load_or_create_local_host(
    animation_style, idle_flipbook)
validate_generated_setup(
    animation_style,
    local_host_blueprint,
    local_host_cdo,
    idle_flipbook,
    attack_flipbook,
    downed_flipbook,
)
