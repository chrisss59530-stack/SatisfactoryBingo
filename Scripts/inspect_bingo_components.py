import unreal

E = unreal.EditorAssetLibrary
R = unreal.AssetRegistryHelpers.get_asset_registry()

paths = [
    "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_Buildable",
    "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_3D",
]

for path in paths:
    print("=== BLUEPRINT:", path)

    asset = E.load_asset(path)
    if not asset:
        print("ERREUR: asset introuvable")
        continue

    try:
        print("PARENT:", asset.get_editor_property("parent_class").get_path_name())
    except Exception as ex:
        print("PARENT ERREUR:", ex)

    try:
        deps = R.get_dependencies(
            unreal.Name(path),
            unreal.AssetRegistryDependencyOptions(
                include_soft_package_references=True,
                include_hard_package_references=True,
                include_searchable_names=False,
                include_soft_management_references=False,
                include_hard_management_references=False
            )
        ) or []

        for dep in deps:
            if str(dep).startswith("/SatisfactoryReverse/"):
                print("DEPENDANCE:", dep)
    except Exception as ex:
        print("DEPENDANCES ERREUR:", ex)

    try:
        scs = asset.get_editor_property("simple_construction_script")
        nodes = scs.get_all_nodes()

        for node in nodes:
            template = node.get_editor_property("component_template")
            if not template:
                continue

            print("COMPOSANT:", node.get_variable_name(), template.get_class().get_name())

            if isinstance(template, unreal.StaticMeshComponent):
                mesh = template.get_editor_property("static_mesh")
                print("MESH:", mesh.get_path_name() if mesh else "AUCUN")

    except Exception as ex:
        print("COMPOSANTS ERREUR:", ex)

print("=== FIN ===")