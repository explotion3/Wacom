"""Create the local EffectBadge digit-feedback MI/Style and bind only Anchor fields.

Run after DreamShader generates M_WacomCard_EffectBadgeFeedback. Set
WACOM_SKIP_EFFECT_BADGE_FEEDBACK_ANCHOR=1 to preserve a player Blueprint owned by
another branch. This script never rebuilds other card or pile-transfer assets.
"""

import os

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_WacomCard_EffectBadgeFeedback"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_WacomCard_EffectBadgeFeedback_Default"
STYLE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
STYLE_PATH = STYLE_DIR + "/DA_FPCardEffectBadgeFeedbackStyle_Pixel"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def load_or_create_material_instance(parent):
    instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
    if not instance:
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_WacomCard_EffectBadgeFeedback_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            unreal.MaterialInstanceConstantFactoryNew(),
        )
    if not instance:
        raise RuntimeError("Failed to create EffectBadge feedback material instance")
    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    for name, value in {
        "BadgeGridColumns": 10.0,
        "BadgeEdgeWidth": 0.10,
        "BadgeEdgeBrightness": 1.25,
        "BadgePreviewMinimumOpacity": 0.55,
        "BadgePreviewMaximumOpacity": 0.92,
    }.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, name, value)
    for name, value in {
        "BadgeNeutralPrimaryColor": unreal.LinearColor(0.93, 0.88, 0.68, 1.0),
        "BadgeNeutralSecondaryColor": unreal.LinearColor(0.44, 0.62, 0.76, 1.0),
        "BadgeIncreasePrimaryColor": unreal.LinearColor(0.58, 0.82, 1.0, 1.0),
        "BadgeIncreaseSecondaryColor": unreal.LinearColor(0.98, 0.82, 0.42, 1.0),
        "BadgeDecreasePrimaryColor": unreal.LinearColor(0.92, 0.26, 0.68, 1.0),
        "BadgeDecreaseSecondaryColor": unreal.LinearColor(0.10, 0.16, 0.34, 1.0),
    }.items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


def load_or_create_style(material_instance):
    style_asset = unreal.load_asset(STYLE_PATH)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class",
            unreal.WacomFirstPersonCardEffectBadgeFeedbackStyle,
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardEffectBadgeFeedbackStyle_Pixel",
            STYLE_DIR,
            unreal.WacomFirstPersonCardEffectBadgeFeedbackStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create EffectBadge feedback Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("digit_feedback_material_instance", material_instance)
    style.set_editor_property("preview_enter_seconds", 0.10)
    style.set_editor_property("preview_exit_seconds", 0.08)
    style.set_editor_property("preview_pulse_period_seconds", 0.85)
    style.set_editor_property("skipped_opacity", 0.28)
    style.set_editor_property("value_change_duration_seconds", 0.28)
    style.set_editor_property("added_duration_seconds", 0.22)
    style.set_editor_property("removed_duration_seconds", 0.18)
    style.set_editor_property("reflow_duration_seconds", 0.14)
    style.set_editor_property("sequence_stagger_seconds", 0.035)
    style.set_editor_property("max_sequence_delay_seconds", 0.12)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")
    anchor.set_editor_property("enable_card_effect_badge_feedback", True)
    anchor.set_editor_property("card_effect_badge_feedback_style", style_asset)
    anchor.set_editor_property("reduce_card_effect_badge_feedback_motion", False)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


base_material = load_required(BASE_MATERIAL_PATH, "EffectBadge feedback DreamShader material")
default_instance = load_or_create_material_instance(base_material)
default_style = load_or_create_style(default_instance)
if os.environ.get("WACOM_SKIP_EFFECT_BADGE_FEEDBACK_ANCHOR", "0") != "1":
    assign_style_to_player(default_style)
unreal.log("Wacom first-person EffectBadge feedback assets configured")
