"""Configure only the Battle retained-card seal MI, Style and Anchor fields.

Set WACOM_SKIP_RETAIN_SEAL_ANCHOR=1 to create/update the assets without saving
BP_WacomPlayerCharacter. This script intentionally does not rebuild other
DreamShader or card-presentation assets.
"""

from __future__ import annotations

import os

import unreal


SURFACE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_SurfaceEffects_RetainSeal"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_SurfaceEffects_RetainSeal_Default"
STYLE_PATH = SURFACE_DIR + "/DA_FPCardRetainSealStyle_Pixel"
PLAYER_BP_PATH = "/Game/Wacom/Core/Player/BP_WacomPlayerCharacter"


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


def load_or_create_material_instance(parent):
    instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
    if not instance:
        factory = unreal.MaterialInstanceConstantFactoryNew()
        instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "MI_FirstPersonCard_SurfaceEffects_RetainSeal_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not instance:
        raise RuntimeError("Failed to create Retain Seal material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    scalar_values = {
        "RetainSealPixelColumns": 96.0,
        "RetainSealCornerExtent": 0.14,
        "RetainSealCornerThickness": 0.025,
        "RetainSealOuterEdgeWidth": 0.010,
        "RetainSealCenterStampExtent": 0.045,
        "RetainSealHeldIntensity": 0.25,
        "RetainSealBrightness": 1.35,
    }
    for name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, name, value
        )

    vector_values = {
        "RetainSealPrimaryColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "RetainSealSecondaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
    }
    for name, value in vector_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
            instance, name, value
        )
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance


def load_or_create_style(material_instance):
    style_asset = unreal.load_asset(STYLE_PATH)
    if not style_asset:
        factory = unreal.DataAssetFactory()
        factory.set_editor_property(
            "data_asset_class", unreal.WacomFirstPersonCardRetainSealStyle
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardRetainSealStyle_Pixel",
            SURFACE_DIR,
            unreal.WacomFirstPersonCardRetainSealStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create Retain Seal Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("surface_effect_material_instance", material_instance)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")

    anchor.set_editor_property("enable_retained_feedback", True)
    anchor.set_editor_property("card_retain_seal_style", style_asset)
    anchor.set_editor_property("reduce_card_retain_seal_motion", False)
    anchor.set_editor_property("retained_feedback_duration", 0.32)
    anchor.set_editor_property("retained_feedback_stagger_seconds", 0.045)
    anchor.set_editor_property("retained_feedback_lift_pixels", 12.0)
    anchor.set_editor_property("retained_feedback_scale", 1.025)
    anchor.set_editor_property("retained_feedback_held_lift_pixels", 5.0)
    anchor.set_editor_property("retained_feedback_held_scale", 1.01)
    anchor.set_editor_property("retained_feedback_release_duration", 0.16)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


base_material = load_required(BASE_MATERIAL_PATH, "Retain Seal DreamShader material")
default_instance = load_or_create_material_instance(base_material)
default_style = load_or_create_style(default_instance)
if os.environ.get("WACOM_SKIP_RETAIN_SEAL_ANCHOR", "0") != "1":
    assign_style_to_player(default_style)
unreal.log("Wacom Battle Retain Seal assets configured")
