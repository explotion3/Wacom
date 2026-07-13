"""Create the independent hand-target impact MI/Style and bind it to the player Anchor.

Run after DreamShader generates M_FirstPersonCard_SurfaceEffects_HandTargetImpact.
This script intentionally does not load, save, or rebuild the pile-transfer Style.
"""

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_SurfaceEffects_HandTargetImpact"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_SurfaceEffects_HandTargetImpact_Default"
STYLE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
STYLE_PATH = STYLE_DIR + "/DA_FPCardHandTargetImpactStyle_PixelStamp"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def load_or_create_material_instance(parent):
    material_instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
    if not material_instance:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        material_instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_FirstPersonCard_SurfaceEffects_HandTargetImpact_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not material_instance:
        raise RuntimeError("Failed to create hand-target impact material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance, parent)
    scalar_values = {
        "TargetImpactGridColumns": 72.0,
        "TargetImpactLineWidth": 0.009,
        "TargetImpactFragmentDensity": 0.16,
        "TargetImpactGlowStrength": 1.20,
        "TargetImpactPreviewStrength": 0.30,
    }
    vector_values = {
        "TargetImpactPrimaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
        "TargetImpactSecondaryColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "TargetImpactAccentColor": unreal.LinearColor(0.88, 0.30, 0.72, 1.0),
    }
    for parameter_name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            material_instance, parameter_name, value)
    for parameter_name, value in vector_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            material_instance, parameter_name, value)
    unreal.EditorAssetLibrary.save_loaded_asset(material_instance)
    return material_instance


def load_or_create_style(material_instance):
    style_asset = unreal.load_asset(STYLE_PATH)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class",
            unreal.WacomFirstPersonCardHandTargetImpactStyle,
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardHandTargetImpactStyle_PixelStamp",
            STYLE_DIR,
            unreal.WacomFirstPersonCardHandTargetImpactStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create hand-target impact Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("surface_effect_material_instance", material_instance)
    style.set_editor_property("preview_fade_in_seconds", 0.10)
    style.set_editor_property("preview_period_seconds", 0.90)
    style.set_editor_property("commit_delay_seconds", 0.07)
    style.set_editor_property("departure_gate_seconds", 0.11)
    style.set_editor_property("rebound_peak_seconds", 0.16)
    style.set_editor_property("commit_duration_seconds", 0.29)
    style.set_editor_property("compression_scale", 0.96)
    style.set_editor_property("compression_translation_pixels", 4.0)
    style.set_editor_property("rebound_scale", 1.05)
    style.set_editor_property("rebound_lift_pixels", 5.0)
    style.set_editor_property("z_order_boost", 900)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")

    anchor.set_editor_property("enable_card_hand_target_impact", True)
    anchor.set_editor_property("card_hand_target_impact_style", style_asset)
    anchor.set_editor_property("reduce_card_hand_target_impact_motion", False)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


base_material = load_required(BASE_MATERIAL_PATH, "Hand-target impact DreamShader material")
default_instance = load_or_create_material_instance(base_material)
default_style = load_or_create_style(default_instance)
assign_style_to_player(default_style)
unreal.log("Wacom hand-target impact material instance and Style configured")
