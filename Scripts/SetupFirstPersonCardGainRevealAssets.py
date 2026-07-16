"""Configure only the Battle Gained pixel-crystal MI, Style and Anchor fields.

Set WACOM_SKIP_GAIN_REVEAL_ANCHOR=1 to create/update the assets without saving
BP_WacomPlayerCharacter. The script intentionally does not rebuild any other
DreamShader or card-presentation asset.
"""

from __future__ import annotations

import os

import unreal


SURFACE_DIR = "/Game/Wacom/UI/Card/SurfaceEffects"
MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_FirstPersonCard_SurfaceEffects_GainReveal"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_FirstPersonCard_SurfaceEffects_GainReveal_Default"
STYLE_PATH = SURFACE_DIR + "/DA_FPCardGainRevealStyle_PixelCrystal"
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
            "MI_FirstPersonCard_SurfaceEffects_GainReveal_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not instance:
        raise RuntimeError("Failed to create Gain Reveal material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    scalar_values = {
        "GainRevealPixelColumns": 96.0,
        "GainRevealClusterDensity": 0.18,
        "GainRevealClusterTravelPixels": 12.0,
        "GainRevealCrystalBrightness": 1.25,
        "GainRevealEdgeBrightness": 1.45,
    }
    for name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, name, value
        )

    vector_values = {
        "GainRevealCrystalPrimaryColor": unreal.LinearColor(0.58, 0.80, 1.0, 1.0),
        "GainRevealCrystalSecondaryColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
        "GainRevealNeutralEdgeColor": unreal.LinearColor(0.78, 0.82, 0.86, 1.0),
        "GainRevealWhiteEdgeColor": unreal.LinearColor(0.98, 0.90, 0.68, 1.0),
        "GainRevealBlueEdgeColor": unreal.LinearColor(0.42, 0.82, 1.0, 1.0),
        "GainRevealYellowEdgeColor": unreal.LinearColor(1.0, 0.72, 0.20, 1.0),
        "GainRevealPurpleEdgeColor": unreal.LinearColor(0.92, 0.34, 0.92, 1.0),
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
            "data_asset_class", unreal.WacomFirstPersonCardGainRevealStyle
        )
        style_asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
            "DA_FPCardGainRevealStyle_PixelCrystal",
            SURFACE_DIR,
            unreal.WacomFirstPersonCardGainRevealStyle,
            factory,
        )
    if not style_asset:
        raise RuntimeError("Failed to create Gain Reveal Style")

    style = style_asset.get_editor_property("style")
    style.set_editor_property("surface_effect_material_instance", material_instance)
    style.set_editor_property("seed_establish_end_progress", 0.12)
    style.set_editor_property("assembly_end_progress", 0.62)
    style.set_editor_property("rarity_edge_peak_progress", 0.70)
    style.set_editor_property("settle_end_progress", 0.84)
    style.set_editor_property("reduced_cross_fade_start_progress", 0.25)
    style.set_editor_property("reduced_cross_fade_end_progress", 0.65)
    style_asset.set_editor_property("style", style)
    unreal.EditorAssetLibrary.save_loaded_asset(style_asset)
    return style_asset


def assign_style_to_player(style_asset):
    player_bp = load_required(PLAYER_BP_PATH, "BP_WacomPlayerCharacter")
    player_cdo = unreal.get_default_object(player_bp.generated_class())
    anchor = player_cdo.get_editor_property("first_person_card_anchor_component")
    if not anchor:
        raise RuntimeError("FirstPersonCardAnchorComponent is missing on player CDO")

    anchor.set_editor_property("enable_card_gain_reveal", True)
    anchor.set_editor_property("card_gain_reveal_style", style_asset)
    anchor.set_editor_property("reduce_card_gain_reveal_motion", False)
    unreal.EditorAssetLibrary.save_loaded_asset(player_bp)


base_material = load_required(BASE_MATERIAL_PATH, "Gain Reveal DreamShader material")
default_instance = load_or_create_material_instance(base_material)
default_style = load_or_create_style(default_instance)
if os.environ.get("WACOM_SKIP_GAIN_REVEAL_ANCHOR", "0") != "1":
    assign_style_to_player(default_style)
unreal.log("Wacom Battle Gain Reveal assets configured")
