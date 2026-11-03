import unreal, os
stage = r"C:\SatisfactoryBingo\SML\Mods\SatisfactoryBingo\_import_ui"
names = ["Fond_Cadre_Sans_Elements","Case_Normale","Case_Active","Case_Terminee","Onglet_Orange","Onglet_Gris","Panneau_Gauche_Fond","Panneau_Droit_Fond"]
tasks = []
for n in names:
    p = os.path.join(stage, n + ".png")
    if not os.path.exists(p):
        print("SAUTE (png absent) " + n); continue
    t = unreal.AssetImportTask()
    t.filename = p; t.destination_path = "/SatisfactoryBingo/Bingo/UI/V3Textures"; t.destination_name = n
    t.replace_existing = True; t.automated = True; t.save = True
    tasks.append(t)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
for t in tasks:
    n = str(t.destination_name)
    a = unreal.EditorAssetLibrary.load_asset("/SatisfactoryBingo/Bingo/UI/V3Textures/" + n)
    if a:
        a.set_editor_property("mip_gen_settings", unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
        a.set_editor_property("lod_group", unreal.TextureGroup.TEXTUREGROUP_UI)
        a.set_editor_property("compression_settings", unreal.TextureCompressionSettings.TC_EDITOR_ICON)
        unreal.EditorAssetLibrary.save_loaded_asset(a); print("OK " + n)
    else:
        print("RATE " + n)
