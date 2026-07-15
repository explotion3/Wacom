"""Create/update the core card-surface parallax MI and assign it to card-face WBPs.

Run after DreamShader generates M_WacomCardSurfaceComposite. This script does
not edit the WBP widget tree and does not touch first-person surface-effect or
pile-transfer assets.
"""

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/Card"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_WacomCardSurfaceComposite"
MATERIAL_INSTANCE_PATH = MATERIAL_DIR + "/MI_WacomCardSurfaceComposite_Default"
FRAME_TEXTURE_PATH = "/Game/Asset/Card_Luo/Card62/Card_Frame"
CARD_VIEW_BP_PATHS = (
    "/Game/Wacom/UI/Card/WBP_FirstPersonCardView",
    "/Game/Wacom/UI/Card/WBP_CardView",
)


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
            "MI_WacomCardSurfaceComposite_Default",
            MATERIAL_DIR,
            unreal.MaterialInstanceConstant,
            factory,
        )
    if not instance:
        raise RuntimeError("Failed to create card-surface perspective material instance")

    unreal.MaterialEditingLibrary.set_material_instance_parent(instance, parent)
    scalar_values = {
        "BackColorScale": 0.96,
        "ReferenceTiltDegrees": 9.0,
        "ArtDepthPixels": -2.0,
        "FrameDepthPixels": 0.0,
        "RarityDepthPixels": 1.5,
        "ArtOverscan": 0.035,
        "PixelQuantization": 1.0,
        "ArtReflectionEnabled": 0.0,
        "ArtReflectionStrength": 0.06,
        "FrameReflectionEnabled": 0.0,
        "FrameReflectionStrength": 0.10,
        "FrameMetalContrast": 1.10,
        "RarityReflectionEnabled": 1.0,
        "RarityBevelStrength": 0.16,
        "FoilStrength": 0.08,
        "IridescenceStrength": 0.05,
        "GlowStrength": 0.04,
    }
    for parameter_name, value in scalar_values.items():
        unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
            instance, parameter_name, value)

    frame_texture = load_required(FRAME_TEXTURE_PATH, "Card frame texture")
    unreal.MaterialEditingLibrary.set_material_instance_texture_parameter_value(
        instance, "FrameTexture", frame_texture)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    return instance, frame_texture


def assign_to_card_view(card_view_path, material_instance, frame_texture):
    card_view_bp = load_required(card_view_path, "Card view Widget Blueprint")
    card_view_cdo = unreal.get_default_object(card_view_bp.generated_class())
    card_view_cdo.set_editor_property("card_surface_material", material_instance)
    card_view_cdo.set_editor_property("card_surface_frame_texture", frame_texture)
    unreal.EditorAssetLibrary.save_loaded_asset(card_view_bp)


base_material = load_required(BASE_MATERIAL_PATH, "Card-surface DreamShader material")
default_instance, default_frame = load_or_create_material_instance(base_material)
for card_view_path in CARD_VIEW_BP_PATHS:
    assign_to_card_view(card_view_path, default_instance, default_frame)
unreal.log("Wacom card-surface perspective material instance configured")
