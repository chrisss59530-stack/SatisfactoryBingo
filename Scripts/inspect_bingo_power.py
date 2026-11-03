import unreal

E = unreal.EditorAssetLibrary

PATH = "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_Buildable"

print("=== BINGO ELECTRICITE ===")

bp = E.load_asset(PATH)

if not bp:
    raise RuntimeError("Blueprint Bingo introuvable")

subsystem = unreal.get_engine_subsystem(
    unreal.SubobjectDataSubsystem
)

handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

print("NOMBRE COMPOSANTS:", len(handles))

for handle in handles:
    try:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)

        if not obj:
            continue

        name = obj.get_name()
        cls = obj.get_class().get_name()

        print("COMPOSANT:", name, "|", cls)

        if (
            "Power" in name
            or "power" in name.lower()
            or "Power" in cls
            or "Connection" in cls
        ):
            print("ELECTRIQUE:", name, "|", cls)

    except Exception as ex:
        print("ERREUR:", str(ex))

try:
    cls = bp.generated_class()
    cdo = unreal.get_default_object(cls)

    print("CLASSE BINGO:", cls.get_path_name())

    for prop in [
        "m_power_consumption",
        "m_power_connection",
        "m_power_connection_component",
        "m_is_powered"
    ]:
        try:
            print("PROPRIETE:", prop, cdo.get_editor_property(prop))
        except Exception:
            pass

except Exception as ex:
    print("CDO ERREUR:", str(ex))

print("=== FIN BINGO ELECTRICITE ===")