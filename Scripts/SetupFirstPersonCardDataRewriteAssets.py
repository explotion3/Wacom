"""Create the direct CostDigitImage rewrite MI/Style and bind only Anchor fields.

Run after DreamShader generates M_WacomCard_CostDigitRewrite.
This script does not load or save any other card material, pile-transfer Style,
or authored first-person card parameter.
"""

import os

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_WacomCard_CostDigitRewrite"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_WacomCard_CostDigitRewrite_Default"
STYLE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
STYLE_PATH = STYLE_DIR + "/DA_FPCardDataRewriteStyle_Pixel"
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
            "MI_WacomCard_CostDigitRewrite_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not material_instance:
        raise RuntimeError("Failed to create card-data rewrite material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(material_instance, parent)
    scalar_values = {
        "CostRewriteGridColumns": 14.0,
        "CostRewriteEdgeWidth": 0.09,
        "CostRewriteEdgeBrightness": 1.20,
    }
    vector_values = {
        "CostRewriteNeutralPrimaryColor": unreal.LinearColor(0.90, 0.88, 0.72, 1.0),
        "CostRewriteNeutralSecondaryColor": unreal.LinearColor(0.48, 0.68, 0.86, 1.0),
        "CostRewriteBeneficialPrimaryColor": unreal.LinearColor(0.58, 0.82, 1.0, 1.0),
        "CostRewriteBeneficialSecondaryColor": unreal.LinearColor(0.98, 0.82, 0.42, 1.0),
        "CostRewriteDetrimentalPrimaryColor": unreal.LinearColor(0.92, 0.26, 0.68, 1.0),
        "CostRewriteDetrimentalSecondaryColor": unreal.LinearColor(0.10, 0.16, 0.34, 1.0),
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
            unreal.WacomFirstPersonCardDataRewriteStyle,
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardDataRewriteStyle_Pixel",
            STYLE_DIR,
            unreal.WacomFirstPersonCardDataRewriteStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create card-data rewrite Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("digit_rewrite_material_instance", material_instance)
    style.set_editor_property("duration_seconds", 0.34)
    style.set_editor_property("preview_pulse_period_seconds", 0.85)
    style.set_editor_property("preview_minimum_opacity", 0.38)
    style.set_editor_property("preview_maximum_opacity", 0.90)
    style.set_editor_property("preview_peak_brightness", 1.45)
    style.set_editor_property("old_dissolve_end_seconds", 0.10)
    style.set_editor_property("new_reveal_start_seconds", 0.12)
    style.set_editor_property("new_reveal_end_seconds", 0.25)
    style.set_editor_property("minimum_scale", 0.88)
    style.set_editor_property("overshoot_scale", 1.10)
    style.set_editor_property("overshoot_peak_seconds", 0.26)
    style.set_editor_property("sequence_stagger_seconds", 0.045)
    style.set_editor_property("max_sequence_delay_seconds", 0.14)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")

    anchor.set_editor_property("enable_card_data_rewrite", True)
    anchor.set_editor_property("card_data_rewrite_style", style_asset)
    anchor.set_editor_property("reduce_card_data_rewrite_motion", False)
    anchor.set_editor_property("card_data_rewrite_duration_override_seconds", -1.0)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


base_material = load_required(BASE_MATERIAL_PATH, "Card-data rewrite DreamShader material")
default_instance = load_or_create_material_instance(base_material)
default_style = load_or_create_style(default_instance)
if os.environ.get("WACOM_SKIP_CARD_DATA_REWRITE_ANCHOR", "0") != "1":
    assign_style_to_player(default_style)
unreal.log("Wacom first-person card data-rewrite assets configured")
