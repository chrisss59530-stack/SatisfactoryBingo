import unreal

mesh_root = "/SatisfactoryBingo/Bingo/Meshes"
material_root = "/SatisfactoryBingo/Bingo/Materials"

registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path(mesh_root, recursive=True)

fixed = 0
errors = 0

for data in assets:
    mesh = unreal.EditorAssetLibrary.load_asset(str(data.package_name))

    if not isinstance(mesh, unreal.StaticMesh):
        continue

    changed = False
    materials = mesh.get_editor_property("static_materials")

    for slot in materials:
        old = slot.get_editor_property("material_interface")

        if not old:
            continue

        name = old.get_name()

        if not name.startswith("M_CT_Terminal_"):
            continue

        new_path = material_root + "/" + name
        replacement = unreal.EditorAssetLibrary.load_asset(new_path)

        if not replacement:
            unreal.log_warning("[BINGO] Materiau absent : " + new_path)
            errors += 1
            continue

        if old.get_path_name() != replacement.get_path_name():
            slot.set_editor_property("material_interface", replacement)
            changed = True

    if changed:
        mesh.set_editor_property("static_materials", materials)
        unreal.EditorAssetLibrary.save_loaded_asset(mesh)
        fixed += 1
        unreal.log("[BINGO] Mesh corrige : " + mesh.get_name())

unreal.log("=== REPARATION BINGO TERMINEE ===")
unreal.log("Meshes corriges : " + str(fixed))
unreal.log("Materiaux introuvables : " + str(errors))