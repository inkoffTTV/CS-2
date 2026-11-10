"""Creates the dressed greybox map /Game/Maps/M_Greybox for BRUTUS milestone tests.

Run inside the Unreal Editor: Tools -> Execute Python Script... -> pick this file.
Headless (after building the editor):
  UnrealEditor-Cmd.exe <path>/Brutus.uproject -ExecutePythonScript="<path>/Tools/create_greybox_map.py" -unattended -nosplash

Safe to re-run: the map and the arena materials are rebuilt from scratch.

Layout (see Source/Brutus/Arena/BrutusGreyboxArena.cpp): Optimates (team 0) hold the -X half,
Populares (team 1) the +X half; the centre wall is "the Rubicon".
"""
import unreal

MAP_PATH = "/Game/Maps/M_Greybox"
MATERIAL_DIR = "/Game/Arena/Materials"
ARENA_CLASS = "/Script/Brutus.BrutusGreyboxArena"

# Team colours, kept in sync with Source/Brutus/Core/BrutusTeams.h
OPTIMATES = (0.42, 0.36, 1.0)
POPULARES = (1.0, 0.45, 0.1)

asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
mel = unreal.MaterialEditingLibrary
actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
level_editor = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)


# ---------------------------------------------------------------------------
# Materials

def _fresh_material(name):
    path = "{}/{}".format(MATERIAL_DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        material = unreal.EditorAssetLibrary.load_asset(path)
        mel.delete_all_material_expressions(material)
    else:
        material = asset_tools.create_asset(name, MATERIAL_DIR, unreal.Material, unreal.MaterialFactoryNew())
    return material


def _const3(material, color, x, y):
    node = mel.create_material_expression(material, unreal.MaterialExpressionConstant3Vector, x, y)
    node.set_editor_property("constant", unreal.LinearColor(color[0], color[1], color[2], 1.0))
    return node


def _const(material, value, x, y):
    node = mel.create_material_expression(material, unreal.MaterialExpressionConstant, x, y)
    node.set_editor_property("r", value)
    return node


def _finish(material):
    mel.recompile_material(material)
    unreal.EditorAssetLibrary.save_loaded_asset(material)
    return material


def make_flat(name, color, roughness=0.8, metallic=0.0, emissive=0.0):
    material = _fresh_material(name)
    mel.connect_material_property(_const3(material, color, -400, 0), "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_const(material, roughness, -400, 150), "", unreal.MaterialProperty.MP_ROUGHNESS)
    mel.connect_material_property(_const(material, metallic, -400, 250), "", unreal.MaterialProperty.MP_METALLIC)
    if emissive > 0.0:
        glow = tuple(c * emissive for c in color)
        mel.connect_material_property(_const3(material, glow, -400, 350), "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    return _finish(material)


def make_checker(name, color_a, color_b, tile_cm=400.0, roughness=0.85):
    """Large world-aligned checker: makes distances and speed readable."""
    material = _fresh_material(name)
    world = mel.create_material_expression(material, unreal.MaterialExpressionWorldPosition, -1200, 0)
    divide = mel.create_material_expression(material, unreal.MaterialExpressionDivide, -1000, 0)
    mel.connect_material_expressions(world, "", divide, "A")
    mel.connect_material_expressions(_const(material, tile_cm, -1200, 120), "", divide, "B")
    floor = mel.create_material_expression(material, unreal.MaterialExpressionFloor, -850, 0)
    mel.connect_material_expressions(divide, "", floor, "")

    mask_x = mel.create_material_expression(material, unreal.MaterialExpressionComponentMask, -700, -60)
    mask_x.set_editor_property("r", True)
    mel.connect_material_expressions(floor, "", mask_x, "")
    mask_y = mel.create_material_expression(material, unreal.MaterialExpressionComponentMask, -700, 60)
    mask_y.set_editor_property("g", True)
    mel.connect_material_expressions(floor, "", mask_y, "")

    add = mel.create_material_expression(material, unreal.MaterialExpressionAdd, -550, 0)
    mel.connect_material_expressions(mask_x, "", add, "A")
    mel.connect_material_expressions(mask_y, "", add, "B")
    fmod = mel.create_material_expression(material, unreal.MaterialExpressionFmod, -420, 0)
    mel.connect_material_expressions(add, "", fmod, "A")
    mel.connect_material_expressions(_const(material, 2.0, -550, 120), "", fmod, "B")
    alpha = mel.create_material_expression(material, unreal.MaterialExpressionAbs, -300, 0)
    mel.connect_material_expressions(fmod, "", alpha, "")

    lerp = mel.create_material_expression(material, unreal.MaterialExpressionLinearInterpolate, -150, 0)
    mel.connect_material_expressions(_const3(material, color_a, -300, -150), "", lerp, "A")
    mel.connect_material_expressions(_const3(material, color_b, -300, 150), "", lerp, "B")
    mel.connect_material_expressions(alpha, "", lerp, "Alpha")
    mel.connect_material_property(lerp, "", unreal.MaterialProperty.MP_BASE_COLOR)
    mel.connect_material_property(_const(material, roughness, -150, 250), "", unreal.MaterialProperty.MP_ROUGHNESS)
    return _finish(material)


def build_materials():
    return {
        "floor": make_checker("M_Arena_Floor", (0.42, 0.40, 0.37), (0.30, 0.29, 0.28)),
        "wall": make_flat("M_Arena_Wall", (0.55, 0.50, 0.44), roughness=0.9),
        "rubicon": make_flat("M_Arena_Rubicon", (0.20, 0.12, 0.06), roughness=0.4, metallic=1.0),
        "ramp": make_flat("M_Arena_Ramp", (0.48, 0.47, 0.50), roughness=0.75),
        "gold": make_flat("M_Arena_Gold", (0.85, 0.62, 0.20), roughness=0.3, metallic=1.0),
        "optimates": make_flat("M_Arena_Optimates", OPTIMATES, roughness=0.6),
        "populares": make_flat("M_Arena_Populares", POPULARES, roughness=0.6),
        "glow_optimates": make_flat("M_Arena_Glow_Optimates", OPTIMATES, roughness=0.5, emissive=6.0),
        "glow_populares": make_flat("M_Arena_Glow_Populares", POPULARES, roughness=0.5, emissive=6.0),
    }


# ---------------------------------------------------------------------------
# Level

CUBE = "/Engine/BasicShapes/Cube.Cube"
CYLINDER = "/Engine/BasicShapes/Cylinder.Cylinder"


def spawn(cls, location, rotation=(0.0, 0.0, 0.0), label=None):
    # rotation = (roll, pitch, yaw), the argument order of unreal.Rotator
    actor = actors.spawn_actor_from_class(cls, unreal.Vector(*location), unreal.Rotator(*rotation))
    if label:
        actor.set_actor_label(label)
    return actor


def spawn_block(label, mesh_path, center, size_cm, material, rotation=(0.0, 0.0, 0.0), collide=True):
    """Static mesh decor sized in centimetres (engine basic shapes are 100 cm)."""
    actor = spawn(unreal.StaticMeshActor, center, rotation, label)
    component = actor.static_mesh_component
    component.set_static_mesh(unreal.EditorAssetLibrary.load_asset(mesh_path))
    actor.set_actor_scale3d(unreal.Vector(size_cm[0] / 100.0, size_cm[1] / 100.0, size_cm[2] / 100.0))
    component.set_material(0, material)
    if not collide:
        component.set_collision_profile_name("NoCollision")
    return actor


def dress_arena(arena, mats):
    cover_side = {  # Cover index -> x of its centre (from BrutusGreyboxArena.cpp)
        0: -1500, 1: -1500, 2: 1500, 3: 1500, 4: -2200, 5: 2200, 6: -800, 7: 800,
    }
    for component in arena.get_components_by_class(unreal.StaticMeshComponent):
        name = component.get_name()
        if name == "Floor":
            material = mats["floor"]
        elif name.startswith("Wall"):
            material = mats["wall"]
        elif name.startswith("MidWall"):
            material = mats["rubicon"]
        elif name.startswith("Cover"):
            index = int(name[len("Cover"):])
            material = mats["optimates"] if cover_side.get(index, 0) < 0 else mats["populares"]
        elif name.startswith("Ramp"):
            material = mats["ramp"]
        elif name == "Platform":
            material = mats["optimates"]
        else:
            continue  # zipline cable keeps its own look
        component.set_material(0, material)


def build_base(sign, team_tag, side_mat, glow_mat, light_color, mats):
    """One team's base on the x = sign * ... half of the arena."""
    # Player starts and their glowing spawn pads.
    for y in (-500.0, 500.0):
        start = spawn(unreal.PlayerStart, (sign * 2000.0, y, 150.0), (0.0, 0.0 if sign < 0 else 180.0, 0.0),
                      "PlayerStart_{}_{}".format(team_tag, "A" if y < 0 else "B"))
        start.set_editor_property("player_start_tag", team_tag)
        spawn_block("SpawnPad_{}_{}".format(team_tag, "A" if y < 0 else "B"), CYLINDER,
                    (sign * 2000.0, y, 3.0), (220.0, 220.0, 6.0), glow_mat, collide=False)

    # Decorative throne at the back of the base (the real objective comes with the lane milestone).
    back = sign * 2780.0
    face_yaw = 0.0 if sign < 0 else 180.0
    spawn_block("Throne_{}_Dais".format(team_tag), CUBE, (back, 0.0, 30.0), (360.0, 420.0, 60.0), mats["gold"])
    spawn_block("Throne_{}_Seat".format(team_tag), CUBE, (back, 0.0, 110.0), (200.0, 240.0, 100.0), side_mat)
    spawn_block("Throne_{}_Back".format(team_tag), CUBE, (back + sign * 90.0, 0.0, 300.0), (40.0, 240.0, 380.0), side_mat)
    spawn_block("Throne_{}_Crown".format(team_tag), CUBE, (back + sign * 90.0, 0.0, 510.0), (50.0, 160.0, 40.0), mats["gold"])

    # Banners on the back wall.
    for y in (-1300.0, 1300.0):
        spawn_block("Banner_{}_{}".format(team_tag, int(y)), CUBE, (sign * 2940.0, y, 230.0), (12.0, 260.0, 340.0),
                    side_mat, collide=False)
        spawn_block("BannerBar_{}_{}".format(team_tag, int(y)), CUBE, (sign * 2935.0, y, 405.0), (16.0, 300.0, 14.0),
                    mats["gold"], collide=False)

    # Coloured light washing the base.
    light = spawn(unreal.PointLight, (sign * 2300.0, 0.0, 450.0), label="BaseLight_{}".format(team_tag))
    component = light.get_component_by_class(unreal.PointLightComponent)
    component.set_intensity(20000.0)
    component.set_attenuation_radius(2200.0)
    component.set_light_color(unreal.LinearColor(light_color[0], light_color[1], light_color[2], 1.0))
    component.set_cast_shadows(False)


def build_sky():
    sun = spawn(unreal.DirectionalLight, (0.0, 0.0, 2000.0), (0.0, -32.0, -35.0), "Sun")
    sun_light = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sun_light.set_intensity(8.0)
    sun_light.set_light_color(unreal.LinearColor(1.0, 0.89, 0.77, 1.0))  # late-afternoon warmth
    sun_light.set_editor_property("atmosphere_sun_light", True)

    spawn(unreal.SkyAtmosphere, (0.0, 0.0, 0.0), label="SkyAtmosphere")
    sky = spawn(unreal.SkyLight, (0.0, 0.0, 1500.0), label="SkyLight")
    sky.get_component_by_class(unreal.SkyLightComponent).set_editor_property("real_time_capture", True)

    fog = spawn(unreal.ExponentialHeightFog, (0.0, 0.0, -100.0), label="HeightFog")
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density", 0.008)

    post = spawn(unreal.PostProcessVolume, (0.0, 0.0, 0.0), label="PostProcess")
    post.set_editor_property("unbound", True)
    settings = post.get_editor_property("settings")
    for prop, value in (("bloom_intensity", 0.6), ("vignette_intensity", 0.35)):
        settings.set_editor_property("override_" + prop, True)
        settings.set_editor_property(prop, value)
    post.set_editor_property("settings", settings)


def main():
    mats = build_materials()

    if not level_editor.new_level(MAP_PATH):
        raise RuntimeError("Could not create " + MAP_PATH)

    arena_class = unreal.load_class(None, ARENA_CLASS)
    if not arena_class:
        raise RuntimeError("BrutusGreyboxArena not found: build the Brutus editor module first")
    arena = spawn(arena_class, (0.0, 0.0, 0.0), label="GreyboxArena")
    dress_arena(arena, mats)

    build_base(-1.0, "Team0", mats["optimates"], mats["glow_optimates"], OPTIMATES, mats)
    build_base(1.0, "Team1", mats["populares"], mats["glow_populares"], POPULARES, mats)
    build_sky()

    level_editor.save_current_level()
    unreal.log("BRUTUS: created and saved " + MAP_PATH)


main()
