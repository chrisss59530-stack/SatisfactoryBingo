import unreal

E = unreal.EditorAssetLibrary
PATH = "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_Buildable"

print("=== CONFIGURATION ELECTRIQUE BINGO ===")

bp = E.load_asset(PATH)
if not bp:
    raise RuntimeError("Blueprint Bingo introuvable")

# La classe parente est accessible via les fonctions Python standard.
try:
    cls = bp.generated_class()
    parents = [c.__name__ for c in cls.__mro__]
    print("HERITAGE:", " -> ".join(parents))
except Exception as ex:
    print("HERITAGE ERREUR:", ex)

subsystem = unreal.get_engine_subsystem(unreal.SubobjectDataSubsystem)
handles = subsystem.k2_gather_subobject_data_for_blueprint(bp)

for handle in handles:
    try:
        data = unreal.SubobjectDataBlueprintFunctionLibrary.get_data(handle)
        obj = unreal.SubobjectDataBlueprintFunctionLibrary.get_object(data)

        if not obj:
            continue

        cls_name = obj.get_class().get_name()

        if cls_name not in [
            "FGPowerConnectionComponent",
            "FGPowerInfoComponent"
        ]:
            continue

        print("=== COMPOSANT:", obj.get_name(), "===")

        for name in [
            "m_max_num_connections",
            "mMaxNumConnections",
            "m_power_consumption",
            "mPowerConsumption",
            "m_target_consumption",
            "mTargetConsumption",
            "m_base_production",
            "mBaseProduction",
            "m_is_producing",
            "mIsProducing"
        ]:
            try:
                value = obj.get_editor_property(name)
                print("PROPRIETE:", name, "=", value)
            except Exception:
                pass

        print("CLASSE:", obj.get_class().get_path_name())

    except Exception as ex:
        print("ERREUR COMPOSANT:", ex)

# Etat des references Contractory.
try:
    R = unreal.AssetRegistryHelpers.get_asset_registry()
    opts = unreal.AssetRegistryDependencyOptions(
        include_soft_package_references=True,
        include_hard_package_references=True,
        include_searchable_names=False,
        include_soft_management_references=False,
        include_hard_management_references=False
    )

    deps = R.get_dependencies(unreal.Name(PATH), opts) or []
    remaining = sorted(
        str(dep) for dep in deps
        if str(dep).startswith("/SatisfactoryReverse/")
    )

    print("DEPENDANCES CONTRACTORY:", len(remaining))
    for dep in remaining:
        print("RESTE:", dep)

except Exception as ex:
    print("DEPENDANCES ERREUR:", ex)

print("=== FIN CONFIGURATION ===")