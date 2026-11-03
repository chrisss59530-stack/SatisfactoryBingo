import unreal

E = unreal.EditorAssetLibrary

PATH = "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_Buildable"

bp = E.load_asset(PATH)
if not bp:
    raise RuntimeError("Blueprint Bingo introuvable")

print("=== VERIFICATION BINGO ===")

try:
    cls = bp.generated_class()
    cdo = unreal.get_default_object(cls)

    for name in [
        "m_interact_widget_soft_class",
        "mInteractWidgetSoftClass"
    ]:
        try:
            value = cdo.get_editor_property(name)
            print("WIDGET:", name, str(value))
        except Exception:
            pass

except Exception as ex:
    print("CDO ERREUR:", ex)

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

for handle in handles:
    try:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)

        if not obj:
            continue

        name = obj.get_name()
        cls_name = obj.get_class().get_name()

        if isinstance(obj, unreal.ChildActorComponent):
            child_cls = obj.get_editor_property("child_actor_class")
            print(
                "CHILD ACTOR:",
                name,
                child_cls.get_path_name() if child_cls else "AUCUN"
            )

        elif isinstance(obj, unreal.StaticMeshComponent):
            mesh = obj.get_editor_property("static_mesh")
            print(
                "STATIC MESH:",
                name,
                mesh.get_path_name() if mesh else "AUCUN"
            )

    except Exception as ex:
        print("COMPOSANT ERREUR:", ex)

print("=== FIN VERIFICATION ===")