"""Imports BRUTUS hero art from Art/Export into the project.

Run inside the Unreal Editor: Tools -> Execute Python Script... -> pick this file
(needs the "Python Editor Script Plugin", enabled by default in UE 5.x).
Safe to re-run: existing assets are replaced.
"""
import os
import unreal

PROJECT_DIR = unreal.Paths.convert_relative_path_to_full(unreal.Paths.project_dir())
EXPORT_DIR = os.path.join(PROJECT_DIR, "Art", "Export")

# (fbx file, destination folder, is skeletal)
ASSETS = [
    ("SM_Hero_Clavus_Posed.fbx", "/Game/Characters/Clavus", False),
    ("SM_Hero_Malleus_Posed.fbx", "/Game/Characters/Malleus", False),
    ("SK_Hero_Clavus.fbx", "/Game/Characters/Clavus", True),
    ("SK_Hero_Malleus.fbx", "/Game/Characters/Malleus", True),
    ("SM_Wpn_Clavus_Nailgun.fbx", "/Game/Weapons/Clavus", False),
    ("SM_Wpn_Malleus_Hammer.fbx", "/Game/Weapons/Malleus", False),
]


def make_task(fbx_path, destination, is_skeletal):
    task = unreal.AssetImportTask()
    task.filename = fbx_path
    task.destination_path = destination
    task.automated = True
    task.replace_existing = True
    task.save = True

    # Classic FBX options. With the Interchange importer (default in recent versions) these are
    # ignored and Interchange's defaults apply, which detect static vs skeletal on their own.
    options = unreal.FbxImportUI()
    options.import_materials = True
    options.import_textures = False
    options.import_animations = False
    options.import_as_skeletal = is_skeletal
    options.import_mesh = True
    options.mesh_type_to_import = unreal.FBXImportType.FBXIT_SKELETAL_MESH if is_skeletal else unreal.FBXImportType.FBXIT_STATIC_MESH
    options.static_mesh_import_data.set_editor_property("convert_scene", True)
    options.static_mesh_import_data.set_editor_property("combine_meshes", True)
    options.skeletal_mesh_import_data.set_editor_property("convert_scene", True)
    task.options = options
    return task


def main():
    tasks = []
    for name, destination, is_skeletal in ASSETS:
        path = os.path.join(EXPORT_DIR, name)
        if not os.path.isfile(path):
            unreal.log_warning("BRUTUS import: missing " + path)
            continue
        tasks.append(make_task(path, destination, is_skeletal))
    unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)
    for task in tasks:
        unreal.log("BRUTUS import: {} -> {}".format(os.path.basename(task.filename), list(task.imported_object_paths)))


main()
