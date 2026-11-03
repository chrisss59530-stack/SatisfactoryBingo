import unreal

REPORT = r"C:/Users/chris/Desktop/Bingo_Audit_Interfaces_V3.txt"

assets = [
    "/SatisfactoryBingo/Bingo/UI/WBP_Bingo_Terminal",
    "/SatisfactoryBingo/Bingo/UI/WBP_Bingo_V3",
    "/SatisfactoryBingo/Bingo/UI/WBP_Bingo_V3_Native",
    "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_Buildable",
    "/SatisfactoryBingo/Bingo/Buildables/BP_Bingo_Terminal_3D"
]

registry = unreal.AssetRegistryHelpers.get_asset_registry()
lines = ["=== AUDIT INTERFACES BINGO V3 ===", ""]

for path in assets:
    lines.append("ASSET : " + path)

    asset = unreal.EditorAssetLibrary.load_asset(path)

    if not asset:
        lines.append("STATUT : INTROUVABLE")
        lines.append("")
        continue

    lines.append("STATUT : PRESENT")
    lines.append("TYPE : " + asset.get_class().get_name())

    try:
        generated = asset.generated_class()
        if generated:
            lines.append("CLASSE GENEREE : " + generated.get_name())
            parent = generated.get_super_class()
            if parent:
                lines.append("PARENT : " + parent.get_name())
    except Exception as e:
        lines.append("PARENT : non disponible (" + str(e) + ")")

    try:
        package = asset.get_outermost().get_name()
        dependencies = registry.get_dependencies(
            package,
            unreal.AssetRegistryDependencyOptions(
                include_soft_package_references=True,
                include_hard_package_references=True,
                include_searchable_names=False,
                include_soft_management_references=False,
                include_hard_management_references=False
            )
        )
        lines.append("DEPENDANCES :")
        for dep in dependencies:
            name = str(dep)
            if "Bingo" in name:
                lines.append("  " + name)
    except Exception as e:
        lines.append("DEPENDANCES : erreur " + str(e))

    try:
        referencers = registry.get_referencers(
            asset.get_outermost().get_name(),
            unreal.AssetRegistryDependencyOptions(
                include_soft_package_references=True,
                include_hard_package_references=True,
                include_searchable_names=False,
                include_soft_management_references=False,
                include_hard_management_references=False
            )
        )
        lines.append("REFERENCE PAR :")
        for ref in referencers:
            lines.append("  " + str(ref))
    except Exception as e:
        lines.append("REFERENCES : erreur " + str(e))

    lines.append("")

with open(REPORT, "w", encoding="utf-8") as f:
    f.write("\n".join(lines))

unreal.log("AUDIT BINGO V3 TERMINE : " + REPORT)