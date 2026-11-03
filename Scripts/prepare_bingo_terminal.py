import unreal

E = unreal.EditorAssetLibrary
SRC = "/SatisfactoryReverse/building"
DST = "/SatisfactoryBingo/Bingo"

created = 0
existing = 0
failed = 0

def copy_asset(source, destination):
    global created, existing, failed

    if E.does_asset_exist(destination):
        existing += 1
        return

    if not E.does_asset_exist(source):
        print("SOURCE ABSENTE:", source)
        failed += 1
        return

    E.make_directory(destination.rsplit("/", 1)[0])

    if E.duplicate_asset(source, destination):
        E.save_asset(destination)
        print("CREE:", destination)
        created += 1
    else:
        print("ECHEC:", source)
        failed += 1

print("=== PREPARATION TERMINAL BINGO ===")

# Copie locale des meshes 3D
for path in E.list_assets(SRC + "/Meshes", recursive=True, include_folder=False):
    source = path.split(".")[0]
    name = source.rsplit("/", 1)[-1]

    if name.startswith("SM_Contractory_Terminal_V4_3_"):
        copy_asset(source, DST + "/Meshes/" + name)

# Icones
for name in ["Terminal_256", "Terminal_512"]:
    copy_asset(
        SRC + "/Icons/" + name,
        DST + "/Icons/" + name
    )

# Materiaux propres au terminal, si presents
for path in E.list_assets(SRC, recursive=True, include_folder=False):
    source = path.split(".")[0]
    name = source.rsplit("/", 1)[-1]

    if name.startswith(("M_CT_Terminal_", "MI_CT_Terminal_")):
        copy_asset(source, DST + "/Materials/" + name)

print("CREES:", created)
print("EXISTANTS:", existing)
print("ECHECS:", failed)
print("=== FIN PREPARATION BINGO ===")