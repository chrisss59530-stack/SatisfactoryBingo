import unreal

E = unreal.EditorAssetLibrary
BP_PATH = "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_Buildable"

bp = E.load_asset(BP_PATH)
if not bp:
    raise RuntimeError("Blueprint Bingo introuvable")

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

removed = 0
errors = 0

for handle in handles:
    try:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)

        if not obj:
            continue

        if obj.get_name() != "ContractoryPowerSocketMesh":
            continue

        params = unreal.RemoveSubobjectParams()
        params.set_editor_property("context_handle", handles[0])
        params.set_editor_property("subobject_handle", handle)

        result = subsystem.delete_subobject(params)

        print("SUPPRESSION:", obj.get_name(), result)
        removed += 1

    except Exception as ex:
        print("ERREUR:", ex)
        errors += 1

if removed > 0 and errors == 0:
    unreal.BlueprintEditorLibrary.compile_blueprint(bp)
    E.save_asset(BP_PATH)

print("SUPPRESSIONS:", removed)
print("ERREURS:", errors)
print("=== FIN ===")