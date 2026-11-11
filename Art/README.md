# Art — temporary hero models (greybox+)

> **Текущие файлы в `Export/` собраны с `--ue-face-y`** — герои смотрят по UE +Y, как UE5 Manny, чтобы анимации Manny перенацеливались без поворота. При подключении в C++ компоненту меша нужен поворот yaw −90° (стандарт для персонажей UE). Превью в `Previews/` сняты с варианта по умолчанию — выглядят так же.


These are stylized low-poly **placeholders** for Clavus and Malleus that replace the grey capsules in early playtests. They are not final art.
One script builds them from scratch with Blender's Python API.

| File | What | Triangles |
|---|---|---|
| `Export/SK_Hero_Clavus.fbx` | skeletal mesh + skeleton (25 bones), 7 material slots | 6 364 |
| `Export/SK_Hero_Malleus.fbx` | skeletal mesh + skeleton (28 bones), 4 material slots | 5 444 |
| `Export/SM_Wpn_Clavus_Nailgun.fbx` | static mesh, pivot at the grip, 5 material slots | 1 928 |
| `Export/SM_Wpn_Malleus_Hammer.fbx` | static mesh, pivot at the grip, 4 material slots | 996 |
| `Export/SM_Hero_Clavus_Posed.fbx` | static mesh: Clavus in the relaxed preview pose with the nailgun in hand, one mesh, 12 material slots | 8 292 |
| `Export/SM_Hero_Malleus_Posed.fbx` | static mesh: Malleus in the relaxed preview pose with the hammer in hand, one mesh, 8 material slots | 6 440 |
| `Previews/*.png` | Workbench renders: front / side / three-quarter per hero, plus `lineup.png` with a 190/210 cm scale bar | |

The budgets are under 15k triangles per hero and under 4k per weapon. The script checks them.

## Regenerate

```bash
pip install bpy==5.3.0 && python3 Art/Blender/build_heroes.py
```

The script is idempotent: it starts from an empty factory scene and overwrites `Export/` and `Previews/`.
It then re-imports every FBX to verify it and writes the results to `Art/_scratch/verify_report.json` (git-ignored).
Each run checks:

- height: Clavus 0–199.5 cm (crown about 190, the right pad sticks out to 199.5); Malleus 0–210 cm
- that the right hand is on the correct side and the toes point forward
- that every vertex is weighted 100 % to exactly one bone
- the triangle budgets

Flags:

- `--no-render` skips the previews.
- `--export-posed-static` writes ONLY the two `SM_Hero_*_Posed.fbx` files (no other FBX, no previews) and verifies them. The default run also writes them, but a default run overwrites the skeletal FBX with the +X-facing variant, so to keep the current `--ue-face-y` skeletal files use this flag alone. The posed meshes exist only for the default orientation (facing UE +X); the flag cannot be combined with `--ue-face-y`.
- `--ue-face-y` builds a variant that faces UE +Y (see *Retargeting* below). It skips the previews and overwrites `Export/` unless you also pass `--export-dir`.
- `--export-dir DIR` writes the FBX files to another folder.

Headless rendering on Linux needs EGL: `apt-get install libegl1 libegl-mesa0 libgl1-mesa-dri`.

## Conventions

- **Units:** 1 Blender unit = 1 cm (scene unit scale 0.01, length cm). The FBX `UnitScaleFactor` is 1.0 (cm), so UE needs no rescale. The re-imported armature has scale 1, so UE gets no ×100 root scale.
- **Origin and size:** feet at the origin, Z up. Both models fit their capsules: Clavus has radius 42 and half-height 96; Malleus has radius 60 and half-height 105. The pads and arms in the T-pose stick out.
- **Facing:** the hero faces **UE +X** and its right side is UE +Y. This matches the C++: `GetMesh()` has no rotation, only `Z = -HalfHeight`. In Blender the hero faces +X and its left is +Y.
- **FBX export settings:**
  - Scale and units: scale 1.0, Apply Unit ✓, Apply Scalings "FBX Units Scale" (`FBX_SCALE_NONE`).
  - Axes: Forward **-Z**, Up **Y**. These are Blender's defaults; UE's *Convert Scene* undoes them, so Blender (x, y, z) arrives as UE (x, −y, z).
  - Skeleton: Add Leaf Bones ✗, primary bone axis Y, secondary X, armature node type Null. The armature object is renamed `Armature` while exporting, so UE adds no extra root bone.
  - Mesh: smoothing "Face"; normals are custom (weighted normals).
  - No animation baked.
- **Rigging:** rigid segments. Every body piece is skinned 100 % to one bone. The pieces are joined into one skeletal mesh with one vertex group per bone, so they deform cleanly with no weight painting. The rest pose is a T-pose with arms along ±Y.
- **Shading:** bevel modifiers are applied, then a weighted-normal pass. Edges sharper than 46° stay hard. There are no textures, only one flat-colour material per colour; UVs are box-projected (1 UV = 1 m) so UE has a channel to work with.
- **Weapons:** modelled with +X forward (barrels) and +Z up, with the pivot at the grip where the fist closes.

## Skeletons

Both heroes use the UE5 Mannequin subset with the same names and hierarchy:

```
root
└─ pelvis
   ├─ spine_01 ─ spine_02 ─ spine_03
   │                         ├─ neck_01 ─ head
   │                         ├─ clavicle_l ─ upperarm_l ─ lowerarm_l ─ hand_l
   │                         └─ clavicle_r ─ upperarm_r ─ lowerarm_r ─ hand_r ─ weapon_r
   ├─ thigh_l ─ calf_l ─ foot_l ─ ball_l
   └─ thigh_r ─ calf_r ─ foot_r ─ ball_r
```

Extra bones:

- **Clavus:** `x_spike_extend` is a child of `head`. It sits at the base of the ram spike on the helmet and points along the spike. Translate it along its length to extend the spike.
- **Malleus:** `x_vent_01`–`x_vent_04` are children of `spine_03`.
  - 01 and 02 are the two exhaust stacks on the upper back.
  - 03 and 04 are the two glowing grille vents below them.
  - Each vent bone points backwards (−X).
- **Both heroes:** `weapon_r` is a child of `hand_r`. It sits at the fist centre with an **identity** rest orientation (the same axes as the mesh component), so a weapon attached there with no offset points forward in the reference pose.

`root` is at the origin with identity orientation.

## Materials

| Hero | Material | Colour | Roughness | Metallic |
|---|---|---|---|---|
| Clavus | `M_Clavus_Coveralls` | #C8642A | 0.9 | 0 |
| Clavus | `M_Clavus_Patch` | #8F4220 | 0.9 | 0 |
| Clavus | `M_Clavus_Steel` | #6E7378 | 0.45 | 1 |
| Clavus | `M_Clavus_Brass` (nail heads) | #D9A441 | 0.35 | 1 |
| Clavus | `M_Clavus_Leather` (boots, gloves, belt) | #3E302A | 0.7 | 0 |
| Clavus | `M_Clavus_Skin` | #C98E6B | 0.6 | 0 |
| Clavus | `M_Clavus_Glass` | #1C2226 | 0.2 | 0 |
| Clavus nailgun | `M_Clavus_Gun_Steel` | #6E7378 | 0.4 | 1 |
| Clavus nailgun | `M_Clavus_Gun_Brass` | #D9A441 | 0.35 | 1 |
| Clavus nailgun | `M_Clavus_Gun_Paint` | #C8642A | 0.6 | 0 |
| Clavus nailgun | `M_Clavus_Gun_Grip` | #3E302A | 0.7 | 0 |
| Clavus nailgun | `M_Clavus_Gun_Dark` | #1A1C1E | 0.5 | 1 |
| Malleus | `M_Malleus_Steel` | #4A4E55 | 0.45 | 1 |
| Malleus | `M_Malleus_Dark` | #26282C | 0.5 | 1 |
| Malleus | `M_Malleus_Leather` | #7A4A2C | 0.7 | 0 |
| Malleus | `M_Malleus_Glow` (vent interiors, eye slit) | #FF7A1A | 0.5 | 0 |
| Malleus hammer | `M_Malleus_Hammer_Steel` | #4A4E55 | 0.4 | 1 |
| Malleus hammer | `M_Malleus_Hammer_Dark` | #26282C | 0.5 | 1 |
| Malleus hammer | `M_Malleus_Hammer_Leather` | #7A4A2C | 0.7 | 0 |
| Malleus hammer | `M_Malleus_Hammer_Glow` | #FF7A1A | 0.5 | 0 |

The `*_Glow` materials carry an emission colour in the FBX. If UE does not pick it up, plug Base Color into Emissive (×4) in the material.

## UE import

### Target paths

| File | Folder | Main asset |
|---|---|---|
| `SK_Hero_Clavus.fbx` | `/Game/Characters/Clavus/` | `/Game/Characters/Clavus/SK_Hero_Clavus` |
| `SK_Hero_Malleus.fbx` | `/Game/Characters/Malleus/` | `/Game/Characters/Malleus/SK_Hero_Malleus` |
| `SM_Wpn_Clavus_Nailgun.fbx` | `/Game/Weapons/Clavus/` | `/Game/Weapons/Clavus/SM_Wpn_Clavus_Nailgun` |
| `SM_Wpn_Malleus_Hammer.fbx` | `/Game/Weapons/Malleus/` | `/Game/Weapons/Malleus/SM_Wpn_Malleus_Hammer` |

Each hero import also creates `SK_Hero_<Hero>_Skeleton` and `SK_Hero_<Hero>_PhysicsAsset` next to the mesh. All `M_*` materials land next to their mesh.

### Skeletal meshes (FBX import dialog)

- **Mesh**
  - Skeletal Mesh ✓
  - Import Mesh ✓
  - **Skeleton: none** (creates a new skeleton; Clavus and Malleus get one each)
  - Import Morph Targets ✗
  - Update Skeleton Reference Pose ✗
  - Use T0 As Ref Pose ✗
  - Create Physics Asset ✓
- **Animation:** Import Animations ✗
- **Transform**
  - Import Translation 0
  - Import Rotation 0
  - **Import Uniform Scale 1.0**
- **Miscellaneous**
  - **Convert Scene ✓**
  - **Force Front XAxis ✗**
  - Convert Scene Unit ✗ (the file is already in cm; ✓ is harmless)
- **Normals**
  - Normal Import Method: **Import Normals**
  - Tangents: MikkTSpace
- **Material**
  - Material Import Method: **Create New Materials**
  - Import Textures ✗ (there are none)

If the editor uses the *Interchange* import dialog (UE 5.5+), use the same values:

- Skeletal Mesh content: Geometry and Skinning Weights
- Skeleton: empty, so a new one is created
- Create Physics Asset ✓
- Import Animations ✗
- Common: Force Front X Axis ✗, Convert Scene ✓, Convert Scene Unit ✓, Import Uniform Scale 1.0
- Materials: Import Materials ✓, Textures ✗
- Normals: Import

### Weapons (static mesh)

- Combine Meshes ✓
- Import Uniform Scale 1.0
- Convert Scene ✓
- Force Front XAxis ✗
- Import Normals
- Create New Materials
- Generate Lightmap UVs: optional
- Auto Generate Collision ✗ (cosmetic only)

### Posed static meshes (in-game placeholders)

`SM_Hero_Clavus_Posed.fbx` and `SM_Hero_Malleus_Posed.fbx` are the heroes frozen in exactly the relaxed pose of the previews, with the weapon merged into the same mesh. They are plain static meshes: no armature, no skin weights, the same `M_*` material names, bevels and weighted normals kept. They face UE +X with the hero's right side at UE +Y (the default orientation, not `--ue-face-y`), pivot at the feet centre, Z up, centimetres, feet at z = 0. Use them as placeholders in game until animation is set up; then switch to the `SK_Hero_*` meshes.

| File | Folder | Main asset |
|---|---|---|
| `SM_Hero_Clavus_Posed.fbx` | `/Game/Characters/Clavus/` | `/Game/Characters/Clavus/SM_Hero_Clavus_Posed` |
| `SM_Hero_Malleus_Posed.fbx` | `/Game/Characters/Malleus/` | `/Game/Characters/Malleus/SM_Hero_Malleus_Posed` |

Import as **Static Mesh** with the same settings as the weapons: Combine Meshes ✓, **Convert Scene ✓**, Force Front XAxis ✗, **Import Uniform Scale 1.0**, Import Normals, **Create New Materials**, Auto Generate Collision ✗ (add a capsule on the character instead). Place the mesh component with the same relative location as the skeletal one (`Z = -HalfHeight`) and no rotation.

Verified after re-import: Clavus z 0–199.3 cm, 8 292 triangles; Malleus z 0–210 cm, 6 440 triangles.

### Wiring the meshes (done in C++ separately)

- Set the skeletal mesh on `GetMesh()`. Keep relative location `(0, 0, -HalfHeight)` (−96 for Clavus, −105 for Malleus) and rotation `(0, 0, 0)`.
- Attach the weapon static mesh to `GetMesh()` at socket/bone `weapon_r` with a zero relative transform. In the T-pose, the nailgun barrels and the face of the hammer head with the shotgun barrels point forward (+X). Fine-tune the hold in the Skeleton editor: add a socket under `weapon_r` with a rotation offset.
- Malleus' rage scales the mesh uniformly. The rigid skinning keeps working at any scale.

## Retargeting UE5 Manny animations (IK Retargeter)

The bone names already match Manny, so the IK Rig chains map one-to-one.

1. Create an **IK Rig** per hero (`IK_Clavus`, `IK_Malleus`) on the new skeletal mesh.
2. Set the **Retarget Root** to `pelvis`.
3. Add these chains:

   | Chain | Bones |
   |---|---|
   | Spine | `spine_01` → `spine_03` |
   | Neck | `neck_01` |
   | Head | `head` |
   | LeftClavicle / RightClavicle | `clavicle_*` |
   | LeftArm / RightArm | `upperarm_*` → `hand_*` |
   | LeftLeg / RightLeg | `thigh_*` → `ball_*` |
   | Root | `root` |

   Optionally add Full Body IK goals on `hand_*` and `foot_*`.
4. Create an **IK Retargeter** with source `IK_Mannequin` (from the Manny content pack) and target `IK_<Hero>`.
5. Use *Auto Map Chains (exact)*.
6. Fix the **retarget pose.** Manny's reference pose is an A-pose, while these meshes are a T-pose. In the target retarget pose, rotate `upperarm_l/r` down about 45° (or use *Auto Align* in UE 5.4+) until the arms match Manny's.
7. Turn on IK for the leg chains. The proportions differ a lot (Clavus has short legs and long arms; Malleus is 210 cm with 1.8-head legs). Keep root translation *Globally Scaled* so the feet don't slide.
8. Export the retargeted animations with *Export Selected Animations*, or retarget at runtime with *Retarget Pose From Mesh*.

**Facing caveat.** Manny faces **+Y**, but these meshes face **+X**, because that is what the current C++ expects. The retargeter applies rotation and root-motion deltas in component space, so a +X target would get motion turned 90°: walks go sideways and leans go to the side. Before you rely on Manny animations, rebuild with

```bash
python3 Art/Blender/build_heroes.py --ue-face-y
```

This variant faces UE +Y like Manny and needs the mesh component at yaw −90, like the UE templates. The weapons and `weapon_r` turn with the rig. For static placeholders, or animations authored on these skeletons, the default +X export is fine.

## Godot

The game now runs in Godot 4.7, so the same models are also exported as binary glTF into the project (`Art/` itself is ignored by Godot via `.gdignore`):

| File | What |
|---|---|
| `assets/heroes/clavus/clavus_posed.glb`, `assets/heroes/malleus/malleus_posed.glb` | static mesh, relaxed preview pose, weapon merged (same as `SM_Hero_*_Posed.fbx`) |
| `assets/heroes/clavus/clavus.glb`, `assets/heroes/malleus/malleus.glb` | skeleton + skinned mesh, T-pose, same rigid-segment rig and UE-Mannequin bone names, no animations |
| `assets/weapons/clavus_nailgun.glb`, `assets/weapons/malleus_hammer.glb` | weapons, pivot at the grip, barrels/face toward -Z |

Run `python3 Art/Blender/build_heroes.py --godot` to write ONLY these files (the FBX files stay untouched). The default run also writes them, but it overwrites the FBX too, so use `--godot` alone to keep the `--ue-face-y` skeletal FBX.

- **Orientation and scale:** metres, Y up, feet at y = 0. The hero faces **-Z** (Godot forward) and its right side is **+X**. The script bakes a x0.01 scale and a +90 deg Z rotation into the data (Blender cm, facing +X -> metres, facing +Y), then the glTF exporter's +Y Up conversion does the rest. Exporter settings: GLB, `export_yup=True`, no extra scale, `export_apply=False`, skins on, animations off, selection only.
- **Materials:** glTF PBR (base colour, metallic, roughness). `*_Glow` also has emission (strength 1.2 in the glTF; the FBX keeps x4, which would clip to yellow in Godot).
- **Verify:** `godot --headless --path . --import`, then `godot --headless --path . --script res://tools/verify_hero_assets.gd` loads every .glb in Godot and checks height, floor, facing (-Z), right side (+X) and the bone names. It exits non-zero on failure.
- **Preview:** `xvfb-run -a godot --path . --rendering-driver opengl3 --rendering-method gl_compatibility --script res://tools/render_godot_lineup.gd` writes `Previews/godot_lineup.png` (both posed heroes seen from the front).
