"""Create or update only the Battle pile-selection outline material instance.

Run after DreamShader generates M_WacomBattleCardPileSelectionOutline. This
script intentionally does not save Widget Blueprints, player Blueprints, or any
other card/enemy assets.
"""

import unreal


MATERIAL_DIR = "/Game/DreamMaterials/UI"
BASE_MATERIAL_PATH = MATERIAL_DIR + "/M_WacomBattleCardPileSelectionOutline"
MATERIAL_INSTANCE_PATH = (
    MATERIAL_DIR + "/MI_WacomBattleCardPileSelectionOutline_Default"
)


def load_required(asset_path, label):
    asset = unreal.load_asset(asset_path)
    if not asset:
        raise RuntimeError(f"{label} is missing: {asset_path}")
    return asset


base_material = load_required(
    BASE_MATERIAL_PATH, "Battle pile selection outline DreamShader material"
)
material_instance = unreal.load_asset(MATERIAL_INSTANCE_PATH)
if not material_instance:
    factory = unreal.MaterialInstanceConstantFactoryNew()
    material_instance = unreal.AssetToolsHelpers.get_asset_tools().create_asset(
        "MI_WacomBattleCardPileSelectionOutline_Default",
        MATERIAL_DIR,
        unreal.MaterialInstanceConstant,
        factory,
    )
if not material_instance:
    raise RuntimeError("Failed to create Battle pile selection outline MI")

unreal.MaterialEditingLibrary.set_material_instance_parent(
    material_instance, base_material
)
scalar_values = {
    "OutlineLineWidth": 0.012,
    "OutlineFeatherWidth": 0.0025,
    "OutlineFlowWidth": 0.13,
    "OutlineFlowSpeed": 0.12,
    "OutlineGlowStrength": 0.44,
}
vector_values = {
    "OutlinePrimaryColor": unreal.LinearColor(0.50, 0.82, 1.0, 1.0),
    "OutlineAccentColor": unreal.LinearColor(0.96, 0.82, 0.42, 1.0),
}
for parameter_name, value in scalar_values.items():
    unreal.MaterialEditingLibrary.set_material_instance_scalar_parameter_value(
        material_instance, parameter_name, value
    )
for parameter_name, value in vector_values.items():
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(
        material_instance, parameter_name, value
    )
unreal.EditorAssetLibrary.save_loaded_asset(material_instance)
unreal.log("Battle pile selection outline MI configured")
