import unreal

E = unreal.EditorAssetLibrary

BP_PATH = "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_3D"
OLD_PREFIX = "/SatisfactoryReverse/building/Meshes/"
NEW_PREFIX = "/SatisfactoryBingo/Bingo/Meshes/"

print("=== REMAP BINGO MESHES ===")

bp = E.load_asset(BP_PATH)
if not bp:
    raise RuntimeError("Blueprint Bingo 3D introuvable")

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

replaced = 0
already_bingo = 0
missing = 0
errors = 0

for handle in handles:
    try:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)

        if not isinstance(obj, unreal.StaticMeshComponent):
            continue

        mesh = obj.get_editor_property("static_mesh")
        if not mesh:
            continue

        old_path = mesh.get_path_name().split(".")[0]

        if old_path.startswith(NEW_PREFIX):
            already_bingo += 1
            continue

        if not old_path.startswith(OLD_PREFIX):
            continue

        new_path = NEW_PREFIX + old_path[len(OLD_PREFIX):]
        new_mesh = E.load_asset(new_path)

        if not new_mesh:
            print("MESH BINGO ABSENT:", new_path)
            missing += 1
            continue

        obj.set_editor_property("static_mesh", new_mesh)
        print("REMPLACE:", old_path, "=>", new_path)
        replaced += 1

    except Exception as ex:
        print("ERREUR COMPOSANT:", str(ex))
        errors += 1

if replaced > 0:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    E.save_asset(BP_PATH)

print("REMPLACES:", replaced)
print("DEJA_BINGO:", already_bingo)
print("MANQUANTS:", missing)
print("ERREURS:", errors)
print("=== FIN REMAP ===")