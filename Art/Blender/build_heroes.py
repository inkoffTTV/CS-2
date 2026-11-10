"""BRUTUS - temporary stylized low-poly heroes (Clavus, Malleus) + weapons.

Builds everything from scratch with Blender's Python API, exports FBX for UE5,
renders Workbench previews and re-imports the FBX files to verify size/orientation.

    pip install bpy==5.3.0
    python3 Art/Blender/build_heroes.py            # build + export + previews + verify
    python3 Art/Blender/build_heroes.py --no-render

Conventions (see Art/README.md):
  * 1 Blender unit = 1 cm (scene unit scale 0.01, length cm). Feet at the origin.
  * Character faces Blender +X, Z up, its LEFT is +Y, its RIGHT is -Y.
    UE imports Blender (x, y, z) as UE (x, -y, z), so the hero faces UE +X and its
    right side is UE +Y (matches the C++: mesh component has no rotation).
  * Rigid skinning: every body piece is weighted 100% to exactly one bone.
  * Weapons are separate static meshes: pivot = grip, barrel/forward +X, up +Z.
"""

import json
import math
import sys
from pathlib import Path

import bpy  # noqa: I001  (bpy must be imported before bmesh/mathutils)
import bmesh
from mathutils import Matrix, Vector

ROOT = Path(__file__).resolve().parents[2]
EXPORT_DIR = ROOT / "Art" / "Export"
PREVIEW_DIR = ROOT / "Art" / "Previews"
SCRATCH_DIR = ROOT / "Art" / "_scratch"

SHARP_ANGLE = math.radians(46.0)  # edges sharper than this stay hard (bevel transitions stay soft)

# --------------------------------------------------------------------------------------
# small math helpers
# --------------------------------------------------------------------------------------


def T(x, y, z):
    return Matrix.Translation((x, y, z))


def R(axis, deg):
    return Matrix.Rotation(math.radians(deg), 4, axis)


def seg_matrix(a, b, hint=(1.0, 0.0, 0.0)):
    """Matrix whose local +Z runs from a to b (origin at a); local X follows `hint`."""
    a, b = Vector(a), Vector(b)
    z = (b - a).normalized()
    h = Vector(hint)
    x = h - z * h.dot(z)
    if x.length < 1e-6:
        x = Vector((0.0, 1.0, 0.0)) - z * z.y
    x.normalize()
    y = z.cross(x)
    m = Matrix((x, y, z)).transposed().to_4x4()
    m.translation = a
    return m


def aim_matrix(pos, normal):
    """Local +Z along `normal`, origin at pos."""
    q = Vector(normal).normalized().to_track_quat("Z", "Y")
    m = q.to_matrix().to_4x4()
    m.translation = Vector(pos)
    return m


def seg_len(a, b):
    return (Vector(b) - Vector(a)).length


def srgb_to_linear(c):
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def hex_rgb(h):
    h = h.lstrip("#")
    return tuple(srgb_to_linear(int(h[i:i + 2], 16) / 255.0) for i in (0, 2, 4))


# --------------------------------------------------------------------------------------
# primitive geometry (local space, returns (verts, faces))
# --------------------------------------------------------------------------------------


def g_frustum(bx, by, tx, ty, h, ox=0.0, oy=0.0, z0=0.0):
    """Box from z0 to z0+h; bottom bx*by centred on the axis, top tx*ty shifted by (ox, oy)."""
    v = [(-bx / 2, -by / 2, z0), (bx / 2, -by / 2, z0), (bx / 2, by / 2, z0), (-bx / 2, by / 2, z0),
         (ox - tx / 2, oy - ty / 2, z0 + h), (ox + tx / 2, oy - ty / 2, z0 + h),
         (ox + tx / 2, oy + ty / 2, z0 + h), (ox - tx / 2, oy + ty / 2, z0 + h)]
    f = [(0, 3, 2, 1), (4, 5, 6, 7), (0, 1, 5, 4), (1, 2, 6, 5), (2, 3, 7, 6), (3, 0, 4, 7)]
    return v, f


def g_box(sx, sy, sz):
    """Box centred on the origin."""
    return g_frustum(sx, sy, sx, sy, sz, z0=-sz / 2)


def g_lathe(profile, n=8, phase=None):
    """Surface of revolution around +Z. profile = [(radius, z), ...] bottom to top; r=0 makes a tip."""
    if phase is None:
        phase = math.pi / n
    verts, rings = [], []
    for r, z in profile:
        if r <= 1e-6:
            rings.append([len(verts)])
            verts.append((0.0, 0.0, z))
        else:
            ring = []
            for j in range(n):
                a = phase + 2 * math.pi * j / n
                ring.append(len(verts))
                verts.append((r * math.cos(a), r * math.sin(a), z))
            rings.append(ring)
    faces = []
    for r0, r1 in zip(rings, rings[1:]):
        for j in range(n):
            k = (j + 1) % n
            if len(r0) == 1:
                faces.append((r0[0], r1[k], r1[j]))
            elif len(r1) == 1:
                faces.append((r0[j], r0[k], r1[0]))
            else:
                faces.append((r0[j], r0[k], r1[k], r1[j]))
    if len(rings[0]) > 1:
        faces.append(tuple(reversed(rings[0])))
    if len(rings[-1]) > 1:
        faces.append(tuple(rings[-1]))
    return verts, faces


def g_cyl(r, h, n=8, z0=0.0, phase=None):
    return g_lathe([(r, z0), (r, z0 + h)], n, phase)


# --------------------------------------------------------------------------------------
# materials
# --------------------------------------------------------------------------------------


def make_material(name, hex_color, rough, metal=0.0, emissive=0.0):
    m = bpy.data.materials.get(name) or bpy.data.materials.new(name)
    col = hex_rgb(hex_color)
    m.use_nodes = True
    bsdf = m.node_tree.nodes.get("Principled BSDF")
    if bsdf:
        bsdf.inputs["Base Color"].default_value = (*col, 1.0)
        bsdf.inputs["Roughness"].default_value = rough
        bsdf.inputs["Metallic"].default_value = metal
        if emissive > 0:
            bsdf.inputs["Emission Color"].default_value = (*col, 1.0)
            bsdf.inputs["Emission Strength"].default_value = emissive
    # viewport/Workbench values
    m.diffuse_color = (*col, 1.0)
    m.roughness = rough
    m.metallic = metal * 0.2  # Workbench renders full metallic too dark; FBX uses the BSDF values
    return m


PALETTE = {
    # Clavus: coveralls #C8642A, steel #6E7378, brass nails #D9A441
    "M_Clavus_Coveralls": ("#C8642A", 0.9, 0.0),
    "M_Clavus_Patch": ("#8F4220", 0.9, 0.0),
    "M_Clavus_Steel": ("#6E7378", 0.45, 1.0),
    "M_Clavus_Brass": ("#D9A441", 0.35, 1.0),
    "M_Clavus_Leather": ("#3E302A", 0.7, 0.0),
    "M_Clavus_Skin": ("#C98E6B", 0.6, 0.0),
    "M_Clavus_Glass": ("#1C2226", 0.2, 0.0),
    # Clavus nailgun
    "M_Clavus_Gun_Steel": ("#6E7378", 0.4, 1.0),
    "M_Clavus_Gun_Brass": ("#D9A441", 0.35, 1.0),
    "M_Clavus_Gun_Paint": ("#C8642A", 0.6, 0.0),
    "M_Clavus_Gun_Grip": ("#3E302A", 0.7, 0.0),
    "M_Clavus_Gun_Dark": ("#1A1C1E", 0.5, 1.0),
    # Malleus: forged steel #4A4E55, leather #7A4A2C, hot orange #FF7A1A
    "M_Malleus_Steel": ("#4A4E55", 0.45, 1.0),
    "M_Malleus_Dark": ("#26282C", 0.5, 1.0),
    "M_Malleus_Leather": ("#7A4A2C", 0.7, 0.0),
    "M_Malleus_Glow": ("#FF7A1A", 0.5, 0.0),
    # Malleus hammer
    "M_Malleus_Hammer_Steel": ("#4A4E55", 0.4, 1.0),
    "M_Malleus_Hammer_Dark": ("#26282C", 0.5, 1.0),
    "M_Malleus_Hammer_Leather": ("#7A4A2C", 0.7, 0.0),
    "M_Malleus_Hammer_Glow": ("#FF7A1A", 0.5, 0.0),
}


def material(name):
    if name not in bpy.data.materials:
        hx, rough, metal = PALETTE[name]
        make_material(name, hx, rough, metal, emissive=4.0 if name.endswith("Glow") else 0.0)
    return bpy.data.materials[name]


# --------------------------------------------------------------------------------------
# mesh assembly: rigid pieces -> one mesh with one vertex group per bone
# --------------------------------------------------------------------------------------


class MeshBuilder:
    def __init__(self, name):
        self.name = name
        self.verts, self.faces, self.face_mat, self.vert_bone = [], [], [], []
        self.mats = []

    def add(self, geom, M, mat, bone=None, bevel=0.0, seg=1, ang=40.0):
        verts, faces = geom
        bm = bmesh.new()
        bv = [bm.verts.new(v) for v in verts]
        for f in faces:
            bm.faces.new([bv[i] for i in f])
        bmesh.ops.recalc_face_normals(bm, faces=bm.faces)
        if bevel > 0:
            edges = [e for e in bm.edges if e.is_manifold and e.calc_face_angle(0.0) > math.radians(ang)]
            if edges:
                bmesh.ops.bevel(bm, geom=edges, offset=bevel, offset_type="OFFSET", profile_type="SUPERELLIPSE",
                                segments=seg, profile=0.5, affect="EDGES", clamp_overlap=True)
        bm.transform(M)
        if M.determinant() < 0:
            bmesh.ops.reverse_faces(bm, faces=bm.faces)
        bm.verts.index_update()
        base = len(self.verts)
        if mat not in self.mats:
            self.mats.append(mat)
        mi = self.mats.index(mat)
        self.verts.extend(tuple(v.co) for v in bm.verts)
        self.vert_bone.extend([bone] * len(bm.verts))
        for f in bm.faces:
            self.faces.append(tuple(base + v.index for v in f.verts))
            self.face_mat.append(mi)
        bm.free()

    def build(self, armature=None):
        me = bpy.data.meshes.new(self.name)
        me.from_pydata(self.verts, [], self.faces)
        me.update()
        for m in self.mats:
            me.materials.append(material(m))
        bm = bmesh.new()
        bm.from_mesh(me)
        bm.faces.ensure_lookup_table()
        for f, mi in zip(bm.faces, self.face_mat):
            f.material_index = mi
            f.smooth = True
        for e in bm.edges:
            e.smooth = not (e.is_manifold and e.calc_face_angle(0.0) > SHARP_ANGLE) and e.is_manifold
        # box-projected UVs (no textures, but UE likes a UV channel; 1 UV unit = 1 m)
        uv = bm.loops.layers.uv.new("UVMap")
        for f in bm.faces:
            n = f.normal
            ax = max(range(3), key=lambda i: abs(n[i]))
            u_i, v_i = [(1, 2), (0, 2), (0, 1)][ax]
            for lp in f.loops:
                co = lp.vert.co
                lp[uv].uv = (co[u_i] * 0.01, co[v_i] * 0.01)
        bm.to_mesh(me)
        bm.free()
        obj = bpy.data.objects.new(self.name, me)
        bpy.context.scene.collection.objects.link(obj)
        # weighted normals: big faces stay flat, chunky bevels read smooth
        wn = obj.modifiers.new("WeightedNormal", "WEIGHTED_NORMAL")
        wn.keep_sharp = True
        wn.weight = 50
        wn.mode = "FACE_AREA"
        apply_modifier(obj, wn.name)
        if armature is not None:
            groups = {}
            for i, b in enumerate(self.vert_bone):
                groups.setdefault(b, []).append(i)
            for bone in armature.data.bones:
                vg = obj.vertex_groups.new(name=bone.name)
                if bone.name in groups:
                    vg.add(groups[bone.name], 1.0, "REPLACE")
            missing = set(groups) - {b.name for b in armature.data.bones}
            assert not missing, f"pieces weighted to unknown bones: {missing}"
            obj.parent = armature
            mod = obj.modifiers.new("Armature", "ARMATURE")
            mod.object = armature
        return obj


def apply_modifier(obj, name):
    with bpy.context.temp_override(object=obj, active_object=obj, selected_objects=[obj]):
        bpy.ops.object.modifier_apply(modifier=name)


def tri_count(obj):
    return sum(len(p.vertices) - 2 for p in obj.data.polygons)


def nail(mb, pos, normal, mat, bone, r=1.9):
    """Big nail head (3-4 cm), low poly."""
    mb.add(g_lathe([(r, -0.6), (r, 0.9), (r * 0.62, 1.8)], 6), aim_matrix(pos, normal), mat, bone)


# --------------------------------------------------------------------------------------
# armature
# --------------------------------------------------------------------------------------


def build_armature(name, bones):
    """bones: list of (name, head, tail, parent). Roll 0 everywhere."""
    arm_data = bpy.data.armatures.new(name)
    arm = bpy.data.objects.new(name, arm_data)
    bpy.context.scene.collection.objects.link(arm)
    bpy.context.view_layer.objects.active = arm
    bpy.ops.object.mode_set(mode="EDIT")
    eb = arm_data.edit_bones
    for bname, head, tail, parent in bones:
        b = eb.new(bname)
        b.head, b.tail, b.roll = head, tail, 0.0
        if parent:
            b.parent = eb[parent]
            b.use_connect = False
        b.use_deform = True
    bpy.ops.object.mode_set(mode="OBJECT")
    arm.show_in_front = True
    return arm


def humanoid_bones(p):
    """Mannequin-subset hierarchy from a dict of joint positions."""
    B = [
        ("root", (0, 0, 0), (0, 20, 0), None),  # identity orientation, at the feet
        ("pelvis", p["pelvis"], p["spine_01"], "root"),
        ("spine_01", p["spine_01"], p["spine_02"], "pelvis"),
        ("spine_02", p["spine_02"], p["spine_03"], "spine_01"),
        ("spine_03", p["spine_03"], p["neck_01"], "spine_02"),
        ("neck_01", p["neck_01"], p["head"], "spine_03"),
        ("head", p["head"], p["head_end"], "neck_01"),
    ]
    for s, side in ((1, "l"), (-1, "r")):
        def m(v):
            return (v[0], v[1] * s, v[2])
        B += [
            (f"clavicle_{side}", m(p["clavicle"]), m(p["shoulder"]), "spine_03"),
            (f"upperarm_{side}", m(p["shoulder"]), m(p["elbow"]), f"clavicle_{side}"),
            (f"lowerarm_{side}", m(p["elbow"]), m(p["wrist"]), f"upperarm_{side}"),
            (f"hand_{side}", m(p["wrist"]), m(p["hand_end"]), f"lowerarm_{side}"),
            (f"thigh_{side}", m(p["hip"]), m(p["knee"]), "pelvis"),
            (f"calf_{side}", m(p["knee"]), m(p["ankle"]), f"thigh_{side}"),
            (f"foot_{side}", m(p["ankle"]), m(p["ball"]), f"calf_{side}"),
            (f"ball_{side}", m(p["ball"]), m(p["toe"]), f"foot_{side}"),
        ]
    # weapon socket bone: identity orientation (tail along +Y, roll 0) at the fist centre,
    # so a weapon modelled +X forward / +Z up attaches with zero relative rotation.
    g = Vector(p["grip_r"])
    B.append(("weapon_r", tuple(g), tuple(g + Vector((0, 10, 0))), "hand_r"))
    return B


# --------------------------------------------------------------------------------------
# CLAVUS - 190 cm, 6 heads, top-heavy asymmetric dieselpunk worker
# --------------------------------------------------------------------------------------

CLAVUS_J = dict(
    pelvis=(0, 0, 84), spine_01=(0, 0, 98), spine_02=(0, 0, 114), spine_03=(0, 0, 130),
    neck_01=(0, 0, 151), head=(0, 0, 160), head_end=(0, 0, 186),
    clavicle=(0, 4, 146), shoulder=(0, 27, 146), elbow=(0, 61, 146), wrist=(0, 94, 146), hand_end=(0, 114, 146),
    hip=(0, 12, 82), knee=(0, 12, 45), ankle=(0, 12, 13), ball=(16, 12, 4), toe=(27, 12, 4),
    grip_r=(1, -104, 146),
)


def build_clavus():
    J = CLAVUS_J
    bones = humanoid_bones(J)
    bones.append(("x_spike_extend", (13, 0, 180), (31, 0, 185), "head"))
    arm = build_armature("ARM_Clavus", bones)
    mb = MeshBuilder("SK_Hero_Clavus")
    C, P, S, B, L, K, G = ("M_Clavus_Coveralls", "M_Clavus_Patch", "M_Clavus_Steel", "M_Clavus_Brass",
                           "M_Clavus_Leather", "M_Clavus_Skin", "M_Clavus_Glass")

    for s, side in ((1, "l"), (-1, "r")):
        y = 12 * s
        foot, ball, calf, thigh = f"foot_{side}", f"ball_{side}", f"calf_{side}", f"thigh_{side}"
        # heavy boots (1.5x): sole, upper, steel toe cap, shaft
        mb.add(g_frustum(40, 21, 40, 21, 4.5), T(7, y, 0), L, foot, bevel=1.5)
        mb.add(g_frustum(28, 19, 22, 17, 13, ox=-2), T(1, y, 3.5), L, foot, bevel=3, seg=2)
        mb.add(g_frustum(14, 20, 9, 16, 10.5, ox=-2), T(21, y, 3.5), S, ball, bevel=3.5, seg=2)
        mb.add(g_frustum(21, 20, 20, 19, 12), T(-1, y, 13), L, calf, bevel=2)
        mb.add(g_box(22, 21, 3), T(-1, y, 21), S, calf, bevel=1)
        # trouser leg, flared over the boot
        mb.add(g_frustum(20, 20, 16, 16, 26), T(0, y, 22), C, calf, bevel=2.5, seg=2)
        # knee pad
        mb.add(g_box(6, 14, 15), T(8.5, y, 46), S, calf, bevel=2, seg=2)
        nail(mb, (11.6, y, 46), (1, 0, 0), B, calf)
        # thigh (short legs: 2.6 heads)
        mb.add(g_frustum(16, 16, 19, 19, 42), T(0, y, 44), C, thigh, bevel=3, seg=2)

        # arms: long, forearms 1.4x upper arm, hands 1.3x
        ua, la, hd = f"upperarm_{side}", f"lowerarm_{side}", f"hand_{side}"
        sh, el, wr, he = [Vector((J[k][0], J[k][1] * s, J[k][2])) for k in ("shoulder", "elbow", "wrist", "hand_end")]
        a = Vector((0, 22 * s, 146))
        mb.add(g_frustum(14, 14, 13, 13, seg_len(a, el) + 1), seg_matrix(a, el), C, ua, bevel=3, seg=2)
        mb.add(g_frustum(17, 17, 17, 17, 5), seg_matrix(el - Vector((0, 4 * s, 0)), el), C, ua, bevel=1.5)  # rolled sleeve
        e0 = el - Vector((0, 2 * s, 0))
        mb.add(g_frustum(17, 16, 19, 18, seg_len(e0, wr) - 8), seg_matrix(e0, wr), C, la, bevel=3, seg=2)
        g0 = wr - Vector((0, 11 * s, 0))
        mb.add(g_frustum(21, 20, 23, 21, 12), seg_matrix(g0, wr + Vector((0, 1 * s, 0))), L, la, bevel=2.5, seg=2)
        mb.add(g_frustum(16, 12, 17, 11.5, 15), seg_matrix(wr, wr + Vector((0, 15 * s, 0))), L, hd, bevel=2.5, seg=2)
        mb.add(g_frustum(17, 10.5, 15, 9, 8), seg_matrix(wr + Vector((0, 13 * s, 0)), he + Vector((0, 1 * s, 0))),
               L, hd, bevel=2.5, seg=2)
        mb.add(g_box(6, 6, 10), T(8.5, wr.y + 7 * s, 143) @ R("Z", 25 * s), L, hd, bevel=2)

    # left shoulder: plain deltoid + patch; right shoulder: the giant pad
    mb.add(g_box(17, 17, 16), T(0, 29, 147), C, "clavicle_l", bevel=5, seg=2)
    mb.add(g_box(9, 10, 1.4), T(1, 40, 153.2), P, "upperarm_l", bevel=0.4)
    mb.add(g_box(1.4, 10, 9), T(9.7, 12, 66) @ R("X", 8), P, "thigh_l", bevel=0.4)
    mb.add(g_box(8, 9, 1.4), T(0, -80, 155.6) @ R("Z", 10), P, "lowerarm_r", bevel=0.4)

    # the giant right pad: a chunky wedge rising 0.3 head above the crown, layered and nailed
    pad = "clavicle_r"
    mb.add(g_frustum(50, 34, 48, 32, 6), T(0, -36, 129), S, pad, bevel=2)                          # lower lame
    mb.add(g_frustum(44, 30, 34, 20, 20, oy=2), T(0, -38, 134), S, pad, bevel=5, seg=2)            # pauldron dome
    lo, hi = Vector((0, -52, 148)), Vector((0, -25, 197))                                          # outer slope
    mb.add(g_frustum(52, 33, 42, 9, hi.z - lo.z, oy=11.5), T(0, -35.5, lo.z), S, pad, bevel=3.5, seg=2)
    Ms = seg_matrix(lo, hi)                                                                        # slope frame
    out = (Ms.to_3x3() @ Vector((0, -1, 0))).normalized()
    mb.add(g_frustum(40, 3, 32, 3, 44, z0=5), Ms @ T(0, -1.4, 0), S, pad, bevel=1.2)                # raised plate
    mb.add(g_frustum(43, 12, 41, 11, 3), T(0, -25, 196.5), B, pad, bevel=0.8)                      # brass cap ~199.5
    for t in (0.2, 0.42, 0.64, 0.86):
        for px in (-14, 0, 14):
            p0 = lo.lerp(hi, t) + out * 1.6
            nail(mb, (px * (1 - 0.25 * t), p0.y, p0.z), out, B, pad)
    for px in (-13, 13):
        nail(mb, (px, -42, 154.2), (0, -0.3, 1), B, pad)

    # pelvis (narrow) + belt
    mb.add(g_frustum(26, 27, 27, 32, 24), T(0, 0, 76), C, "pelvis", bevel=3, seg=2)
    mb.add(g_box(30, 36, 6), T(0, 0, 97), L, "pelvis", bevel=1.5)
    mb.add(g_box(2, 9, 7), T(15.6, 0, 97), B, "pelvis", bevel=0.8)
    mb.add(g_box(11, 7, 12), T(4, -19.5, 91), L, "pelvis", bevel=2)      # pouch
    mb.add(g_box(7, 5, 9), T(-5, 19, 92), L, "pelvis", bevel=1.5)
    mb.add(g_cyl(1.6, 22, 6), T(5, 20, 79), S, "pelvis")                    # wrench handle
    mb.add(g_box(6, 2.5, 6), T(5, 20, 79), S, "pelvis", bevel=1)

    # torso: trapezoid, wide on top
    mb.add(g_frustum(26, 32, 28, 40, 20), T(0, 0, 98), C, "spine_01", bevel=3, seg=2)
    mb.add(g_frustum(28, 40, 31, 52, 20), T(0, 0, 114), C, "spine_02", bevel=3, seg=2)
    mb.add(g_frustum(31, 52, 34, 72, 24, ox=1.5), T(0, 0, 130), C, "spine_03", bevel=5, seg=2)
    for sy in (-11, 11):  # suspender straps
        mb.add(g_frustum(1.6, 5, 1.6, 5, 40, ox=2.2), T(15.4, sy, 114), L, "spine_03")
        mb.add(g_box(1.4, 4, 4), T(17.5, sy, 145), B, "spine_03", bevel=0.5)
    mb.add(g_box(1.4, 11, 10), T(16.6, 19, 138) @ R("X", 6), P, "spine_03", bevel=0.4)
    mb.add(g_cyl(4.5, 2.5, 10), aim_matrix((16.4, -20, 140), (1, 0, 0)), B, "spine_03", bevel=0.8, ang=60)  # gauge
    mb.add(g_cyl(3.4, 0.6, 10, z0=2.5), aim_matrix((16.4, -20, 140), (1, 0, 0)), G, "spine_03")
    mb.add(g_lathe([(13, 0), (14, 3), (11.5, 7)], 10), T(0, 0, 148), S, "spine_03", bevel=1, ang=50)  # collar
    for sy in (-9, 9):  # twin air tanks on the back
        mb.add(g_lathe([(5, 0), (7.5, 2.5), (7.5, 30), (5, 33)], 10), T(-24, sy, 116), S, "spine_03", ang=60)
        mb.add(g_cyl(2.2, 5, 6), T(-24, sy, 148), B, "spine_03", bevel=0.6, ang=60)
    mb.add(g_box(3, 30, 4), T(-17, 0, 140), L, "spine_03", bevel=1)

    # neck + head
    mb.add(g_cyl(8, 14, 8), T(0, 0, 149), K, "neck_01")
    mb.add(g_frustum(21, 24, 19, 19, 24), T(1, 0, 156), K, "head", bevel=3, seg=2)       # big square jaw
    mb.add(g_box(7, 15, 9), T(11.5, 0, 163), S, "head", bevel=2, seg=2)                    # respirator
    for sy in (-7.5, 7.5):
        mb.add(g_cyl(3.2, 6, 8), aim_matrix((12, sy, 161), (0.8, 0.6 * (1 if sy > 0 else -1), 0)), B, "head",
               bevel=0.8, ang=60)
        mb.add(g_box(1.2, 4.5, 2.2), T(10.8, sy * 0.7, 169.5), G, "head")                    # eyes
    mb.add(g_lathe([(15.5, 0), (15.5, 6), (14, 12), (10, 16.5), (4.5, 19), (0, 19.5)], 12), T(0, 0, 171), S,
           "head", ang=50)
    mb.add(g_cyl(18, 2.5, 12, z0=0), T(1.5, 0, 170), S, "head", bevel=1, ang=50)              # brim
    for sy in (-6, 6):                                                                       # goggles on the helmet
        mb.add(g_cyl(4.4, 3, 10), aim_matrix((13.6, sy, 177.5), (1, 0, 0.25)), B, "head", bevel=0.8, ang=60)
        mb.add(g_cyl(3.4, 0.8, 10, z0=2.7), aim_matrix((13.6, sy, 177.5), (1, 0, 0.25)), G, "head")
    for k in range(12):
        a = math.radians(30 * k + 15)
        if math.cos(a) > 0.75:
            continue
        nail(mb, (15.6 * math.cos(a), 15.6 * math.sin(a), 175), (math.cos(a), math.sin(a), 0.25), B, "head")
    nail(mb, (0, 0, 190.2), (0, 0, 1), B, "head", r=2.2)
    # ram spike (extends on x_spike_extend)
    sp0, sp1 = Vector((11, 0, 179.5)), Vector((33, 0, 185.5))
    mb.add(g_lathe([(5, 0), (5, 3), (3.3, 4.5), (2.8, 13), (0, 22.5)], 8), seg_matrix(sp0, sp1, (0, 0, 1)), S,
           "x_spike_extend", ang=60)
    mb.add(g_cyl(5.6, 3, 8), seg_matrix(sp0, sp1, (0, 0, 1)), B, "x_spike_extend", bevel=0.8, ang=60)

    obj = mb.build(arm)
    return arm, obj


def build_nailgun():
    """~110 cm jackhammer nailgun. Pivot at the pistol grip, barrels +X, up +Z."""
    mb = MeshBuilder("SM_Wpn_Clavus_Nailgun")
    S, B, P, G, D = ("M_Clavus_Gun_Steel", "M_Clavus_Gun_Brass", "M_Clavus_Gun_Paint", "M_Clavus_Gun_Grip",
                     "M_Clavus_Gun_Dark")
    X = (1, 0, 0)
    ax = lambda x, y=0.0, z=12.0: aim_matrix((x, y, z), X)
    mb.add(g_frustum(10, 6.5, 9, 6, 20), T(0, 0, -15) @ R("Y", -14), G, bevel=2, seg=2)        # pistol grip
    mb.add(g_box(14, 3, 1.5), T(1, 0, -1), S, bevel=0.5)                                         # trigger guard
    mb.add(g_cyl(9.5, 66, 10, z0=0), ax(-26), P, bevel=2, seg=2, ang=50)                         # body
    for x0 in (-27, -4, 18, 37):
        mb.add(g_cyl(10.6, 3.5, 10), ax(x0), S, bevel=0.8, ang=60)                               # bands
    mb.add(g_lathe([(10.5, 0), (10.5, 3), (7, 6)], 10), ax(-33), B, ang=60)                      # rear cap
    mb.add(g_box(5, 5, 12), T(-20, 0, 26), S, bevel=1.2)                                         # D-handle post
    mb.add(g_cyl(2.8, 24, 8, z0=-12), T(-20, 0, 33) @ R("X", 90), G, bevel=0.8, ang=60)          # top handle
    mb.add(g_box(6, 5, 5), T(-20, 0, 33), S, bevel=1)
    mb.add(g_cyl(4.8, 30, 8), ax(-6, 0, 24), S, bevel=1, ang=60)                                 # piston housing
    mb.add(g_box(26, 5, 6), T(9, 0, 20), S, bevel=1.5)
    mb.add(g_cyl(2.2, 22, 8), ax(24, 0, 24), B, ang=60)                                          # piston rod
    mb.add(g_cyl(4, 4, 8), ax(44, 0, 24), S, bevel=1, ang=60)
    for k in range(3):                                                                           # 3-barrel cluster
        a = math.radians(90 + 120 * k)
        by, bz = 5.2 * math.cos(a), 12 + 5.2 * math.sin(a)
        mb.add(g_cyl(3.1, 44, 8), ax(40, by, bz), S, bevel=0.6, ang=60)
        mb.add(g_cyl(1.9, 0.8, 6, z0=44), ax(40, by, bz), D)
    mb.add(g_cyl(10.4, 6, 10), ax(38), B, bevel=1.2, ang=60)                                     # front collar
    mb.add(g_cyl(9.6, 5, 10), ax(72), S, bevel=1.2, ang=60)                                      # muzzle shroud
    # side drum magazine, diameter 40, on the outside (-Y)
    dm = aim_matrix((6, -11, 8), (0, -1, 0))
    mb.add(g_cyl(20, 11, 16), dm, P, bevel=2, seg=2, ang=50)
    mb.add(g_cyl(20.8, 2.2, 16, z0=-0.5), dm, S, bevel=0.6, ang=60)
    mb.add(g_cyl(20.8, 2.2, 16, z0=9.3), dm, S, bevel=0.6, ang=60)
    mb.add(g_cyl(7, 3, 10, z0=11), dm, B, bevel=0.8, ang=60)                                     # hub
    mb.add(g_box(3, 2, 26), T(6, -22.8, 8) @ R("X", 90) @ R("Y", 90), S)             # winding key
    mb.add(g_box(12, 6, 12), T(6, -9, 12), S, bevel=1.5)                                         # feed chute
    obj = mb.build()
    return obj


# --------------------------------------------------------------------------------------
# MALLEUS - 210 cm, 5 heads, square armoured blacksmith tank
# --------------------------------------------------------------------------------------

MALLEUS_J = dict(
    pelvis=(0, 0, 86), spine_01=(0, 0, 100), spine_02=(0, 0, 120), spine_03=(0, 0, 142),
    neck_01=(0, 0, 176), head=(0, 0, 188), head_end=(0, 0, 208),
    clavicle=(0, 6, 160), shoulder=(0, 40, 160), elbow=(0, 76, 160), wrist=(0, 110, 160), hand_end=(0, 134, 160),
    hip=(0, 18, 82), knee=(0, 18, 46), ankle=(0, 18, 15), ball=(20, 18, 5), toe=(31, 18, 5),
    grip_r=(1, -122, 160),
)
MALLEUS_VENTS = [(-36, 17, 164), (-36, -17, 164), (-31, 16, 146), (-31, -16, 146)]


def build_malleus():
    J = MALLEUS_J
    bones = humanoid_bones(J)
    for i, v in enumerate(MALLEUS_VENTS, 1):
        bones.append((f"x_vent_{i:02d}", v, (v[0] - 10, v[1], v[2]), "spine_03"))
    arm = build_armature("ARM_Malleus", bones)
    mb = MeshBuilder("SK_Hero_Malleus")
    S, D, L, O = "M_Malleus_Steel", "M_Malleus_Dark", "M_Malleus_Leather", "M_Malleus_Glow"

    def vent(cx, cy, cz, w, h, normal_axis, bone):
        """Slatted vent with a glowing interior. normal_axis: '+Z', '-X' ..."""
        rot = {"+Z": Matrix.Identity(4), "-X": R("Y", -90), "+X": R("Y", 90)}[normal_axis]
        M = T(cx, cy, cz) @ rot
        mb.add(g_frustum(w + 4, h + 4, w + 2, h + 2, 3.5, z0=-1.5), M, D, bone, bevel=1)
        mb.add(g_box(w, h, 1.0), M @ T(0, 0, 2.2), O, bone)
        n = max(2, int(h // 5))
        for k in range(n):
            yy = -h / 2 + (k + 0.5) * h / n
            mb.add(g_box(w + 1, 1.8, 2.2), M @ T(0, yy, 3.0), S, bone, bevel=0.5)

    for s, side in ((1, "l"), (-1, "r")):
        y = 18 * s
        foot, ball, calf, thigh = f"foot_{side}", f"ball_{side}", f"calf_{side}", f"thigh_{side}"
        # sabatons: 45 cm long
        mb.add(g_frustum(46, 31, 46, 31, 5), T(8, y, 0), D, foot, bevel=1.5)
        mb.add(g_frustum(34, 29, 28, 26, 15, ox=-2), T(2, y, 5), S, foot, bevel=3, seg=2)
        mb.add(g_frustum(15, 29, 9, 25, 12, ox=-2), T(24, y, 5), S, ball, bevel=3, seg=2)
        mb.add(g_frustum(33, 33, 31, 31, 10), T(0, y, 15), L, calf, bevel=2)
        # pillar legs: greave + knee cop, thigh + plate
        mb.add(g_frustum(33, 33, 31, 31, 30), T(0, y, 22), S, calf, bevel=3, seg=2)
        mb.add(g_box(5, 22, 26), T(16.5, y, 36), S, calf, bevel=2)
        mb.add(g_frustum(14, 27, 12, 25, 16), T(15, y, 40), S, calf, bevel=3, seg=2)
        mb.add(g_frustum(29, 29, 33, 33, 42), T(0, y, 48), L, thigh, bevel=3, seg=2)
        mb.add(g_frustum(6, 30, 6, 32, 30), T(17, y, 56), S, thigh, bevel=2.5)          # tasset
        mb.add(g_frustum(30, 6, 32, 6, 26), T(0, y + 17.5 * s, 62), S, thigh, bevel=2.5)  # side tasset

        # massive arms; hands 1.6x
        ua, la, hd, cl = f"upperarm_{side}", f"lowerarm_{side}", f"hand_{side}", f"clavicle_{side}"
        sh, el, wr, he = [Vector((J[k][0], J[k][1] * s, J[k][2])) for k in ("shoulder", "elbow", "wrist", "hand_end")]
        a = Vector((0, 34 * s, 160))
        mb.add(g_frustum(27, 26, 26, 25, seg_len(a, el) + 1), seg_matrix(a, el), L, ua, bevel=3, seg=2)
        mb.add(g_frustum(29, 28, 29, 28, 10), seg_matrix(el - Vector((0, 15 * s, 0)), el - Vector((0, 5 * s, 0))),
               S, ua, bevel=2.5)                                                               # rerebrace band
        mb.add(g_box(14, 14, 14), T(-12, el.y, 160), S, la, bevel=3, seg=2)                    # elbow cop
        e0 = el - Vector((0, 3 * s, 0))
        mb.add(g_frustum(28, 27, 34, 32, seg_len(e0, wr) + 1), seg_matrix(e0, wr), S, la, bevel=3, seg=2)
        mb.add(g_frustum(36, 34, 36, 34, 5), seg_matrix(wr - Vector((0, 5 * s, 0)), wr), D, la, bevel=1.5)
        mb.add(g_frustum(25, 18, 27, 18, 17), seg_matrix(wr, wr + Vector((0, 17 * s, 0))), L, hd, bevel=3, seg=2)
        mb.add(g_frustum(27, 16, 24, 14, 9), seg_matrix(wr + Vector((0, 15 * s, 0)), he + Vector((0, 1 * s, 0))),
               L, hd, bevel=3, seg=2)
        mb.add(g_frustum(26, 6, 24, 5, 14), seg_matrix(wr + Vector((0, 4 * s, 9)), wr + Vector((0, 18 * s, 9))),
               S, hd, bevel=1.5)                                                               # knuckle plate
        mb.add(g_box(9, 8, 12), T(14, wr.y + 8 * s, 156) @ R("Z", 25 * s), L, hd, bevel=2.5)   # thumb

        # pauldrons (~120 cm shoulders) with vents
        mb.add(g_frustum(56, 34, 54, 32, 8), T(0, 45 * s, 143), S, cl, bevel=2)
        mb.add(g_frustum(52, 30, 44, 26, 34, oy=-1 * s), T(0, 44 * s, 150), S, cl, bevel=3, seg=2)
        vent(0, 43 * s, 184, 18, 13, "+Z", cl)

    # pelvis, belt, blacksmith apron
    mb.add(g_frustum(46, 58, 50, 64, 28), T(0, 0, 74), L, "pelvis", bevel=3, seg=2)
    mb.add(g_box(54, 70, 9), T(0, 0, 100), S, "pelvis", bevel=2.5, seg=1)
    mb.add(g_box(3, 16, 13), T(28, 0, 100), D, "pelvis", bevel=1.5)
    mb.add(g_box(2, 9, 6), T(29.5, 0, 100), O, "pelvis")
    mb.add(g_frustum(3, 40, 3, 30, 52, ox=-0.5), T(30, 0, 58), L, "pelvis", bevel=1)

    # torso: barrel / cube
    mb.add(g_frustum(50, 64, 54, 74, 24), T(0, 0, 98), S, "spine_01", bevel=3, seg=2)
    mb.add(g_frustum(56, 76, 60, 84, 24), T(0, 0, 118), S, "spine_02", bevel=3, seg=2)
    mb.add(g_frustum(60, 86, 58, 88, 38), T(0, 0, 140), S, "spine_03", bevel=4, seg=2)
    mb.add(g_box(6, 72, 30), T(30.5, 0, 160), S, "spine_03", bevel=2.5)                      # chest plate
    mb.add(g_box(6, 66, 14), T(29.5, 0, 133), S, "spine_02", bevel=2.5)
    vent(33.5, 0, 160, 24, 12, "+X", "spine_03")                                               # forge grate
    mb.add(g_frustum(66, 92, 66, 92, 4), T(0, 0, 177), D, "spine_03", bevel=1.5)
    # gorget: head sunk into it
    mb.add(g_frustum(56, 60, 48, 54, 18), T(0, 0, 172), S, "spine_03", bevel=3, seg=2)            # gorget
    mb.add(g_frustum(10, 34, 7, 30, 12), T(20, 0, 182) @ R("Y", -14), S, "spine_03", bevel=2.5)
    mb.add(g_frustum(10, 40, 7, 34, 12), T(-21, 0, 186) @ R("Y", 12), S, "spine_03", bevel=2.5)
    # back: two exhaust stacks + two grille vents (x_vent_01..04)
    for i, (vx, vy, vz) in enumerate(MALLEUS_VENTS, 1):
        bn = f"x_vent_{i:02d}"
        if i <= 2:
            mb.add(g_box(10, 14, 16), T(-31, vy, vz + 2), D, bn, bevel=2)
            mb.add(g_lathe([(6.5, 0), (6.5, 25), (8, 26.5), (8, 31)], 8), T(vx, vy, vz), S, bn, bevel=1, ang=60)
            mb.add(g_cyl(5.6, 1.2, 8, z0=30.5), T(vx, vy, vz), O, bn)
        else:
            vent(vx, vy, vz, 20, 12, "-X", bn)

    # tiny head
    mb.add(g_cyl(9, 14, 8), T(0, 0, 176), L, "neck_01")
    mb.add(g_frustum(26, 24, 22, 20, 22), T(0, 0, 184), S, "head", bevel=3, seg=2)
    mb.add(g_box(3, 21, 7.5), T(10.8, 0, 199), D, "head", bevel=1)                              # visor band
    mb.add(g_box(2.4, 18, 3.4), T(12.2, 0, 199.5), O, "head")                                  # eye slit
    mb.add(g_frustum(18, 5, 12, 3, 4), T(-1, 0, 206), S, "head", bevel=1)                      # crest -> 210

    obj = mb.build(arm)
    return arm, obj


def build_hammer():
    """Hammer: head 70 x 45 x 45, handle 140, two 8 cm shotgun barrels. Pivot at the grip."""
    mb = MeshBuilder("SM_Wpn_Malleus_Hammer")
    S, D, L, O = ("M_Malleus_Hammer_Steel", "M_Malleus_Hammer_Dark", "M_Malleus_Hammer_Leather",
                  "M_Malleus_Hammer_Glow")
    mb.add(g_cyl(4, 140, 8, z0=-28), Matrix.Identity(4), D, bevel=1, ang=60)                   # handle 140
    mb.add(g_cyl(5, 34, 8, z0=-14), Matrix.Identity(4), L, bevel=1, ang=60)                     # leather wrap
    for z in (-15, 20):
        mb.add(g_cyl(5.6, 2.5, 8, z0=z), Matrix.Identity(4), S, bevel=0.6, ang=60)
    mb.add(g_frustum(12, 12, 10, 10, 8), T(0, 0, -36), S, bevel=2, seg=2)                      # pommel
    hz = 112 - 22.5                                                                              # head centre
    mb.add(g_box(70, 45, 45), T(0, 0, hz), S, bevel=3, seg=2)                                    # head
    mb.add(g_frustum(42, 42, 38, 38, 5), T(-35, 0, hz) @ R("Y", -90), S, bevel=2.5, seg=2)       # strike face
    for sz in (-14, 14):                                                                         # heat slots
        mb.add(g_box(40, 1.0, 5), T(-2, 22.8, hz + sz), O)
        mb.add(g_box(40, 1.0, 5), T(-2, -22.8, hz + sz), O)
    for sy in (-24, 24):                                                                         # side bands
        mb.add(g_box(14, 3, 49), T(14, sy * 0.96, hz), D, bevel=1)
    mb.add(g_box(16, 16, 10), T(0, 0, hz - 26), D, bevel=2)                                      # collar
    for sy in (-10, 10):                                                                         # shotgun barrels
        mb.add(g_cyl(4, 10, 10), aim_matrix((33, sy, hz), (1, 0, 0)), S, bevel=0.8, ang=60)
        mb.add(g_cyl(2.6, 0.6, 8, z0=9.8), aim_matrix((33, sy, hz), (1, 0, 0)), D)
    mb.add(g_box(3, 32, 12), T(36, 0, hz), D, bevel=1)
    return mb.build()


# --------------------------------------------------------------------------------------
# export / verification
# --------------------------------------------------------------------------------------

FBX_COMMON = dict(
    use_selection=True, global_scale=1.0, apply_unit_scale=True, apply_scale_options="FBX_SCALE_NONE",
    axis_forward="-Z", axis_up="Y", bake_space_transform=False, use_mesh_modifiers=False,
    mesh_smooth_type="FACE", use_tspace=False, use_custom_props=False, path_mode="AUTO", embed_textures=False,
    bake_anim=False, use_triangles=False,
)


def select_only(objs):
    for o in bpy.context.scene.objects:
        o.select_set(False)
    for o in objs:
        o.select_set(True)
    bpy.context.view_layer.objects.active = objs[0]


def export_skeletal(arm, mesh, path):
    old = arm.name
    arm.name = "Armature"  # UE skips an armature node named "Armature" (no extra root bone)
    select_only([arm, mesh])
    bpy.ops.export_scene.fbx(filepath=str(path), object_types={"ARMATURE", "MESH"}, add_leaf_bones=False,
                             primary_bone_axis="Y", secondary_bone_axis="X", armature_nodetype="NULL",
                             use_armature_deform_only=False, **FBX_COMMON)
    arm.name = old


def export_static(obj, path):
    select_only([obj])
    bpy.ops.export_scene.fbx(filepath=str(path), object_types={"MESH"}, **FBX_COMMON)


def fbx_global_settings(path):
    from io_scene_fbx import parse_fbx
    root, _ver = parse_fbx.parse(str(path))
    out = {}
    for el in root.elems:
        if el.id == b"GlobalSettings":
            for sub in el.elems:
                if sub.id == b"Properties70":
                    for p in sub.elems:
                        key = p.props[0].decode()
                        if key in ("UpAxis", "UpAxisSign", "FrontAxis", "FrontAxisSign", "CoordAxis",
                                   "CoordAxisSign", "UnitScaleFactor", "OriginalUnitScaleFactor"):
                            out[key] = p.props[-1]
    return out


def reset_scene():
    bpy.ops.wm.read_factory_settings(use_empty=True)
    sc = bpy.context.scene
    sc.unit_settings.system = "METRIC"
    sc.unit_settings.scale_length = 0.01
    sc.unit_settings.length_unit = "CENTIMETERS"
    return sc


def verify_exports(expect, export_dir, face_y=False):
    """Re-import every FBX into an empty cm scene and measure it."""
    fwd = Vector((0, -1, 0)) if face_y else Vector((1, 0, 0))    # Blender-space forward
    right = Vector((-1, 0, 0)) if face_y else Vector((0, -1, 0))
    report = {}
    for fname, exp in expect.items():
        reset_scene()
        path = export_dir / fname
        bpy.ops.import_scene.fbx(filepath=str(path))
        meshes = [o for o in bpy.context.scene.objects if o.type == "MESH"]
        arms = [o for o in bpy.context.scene.objects if o.type == "ARMATURE"]
        pts = [o.matrix_world @ v.co for o in meshes for v in o.data.vertices]
        mn = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
        mx = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
        r = dict(min=[round(c, 1) for c in mn], max=[round(c, 1) for c in mx],
                 tris=sum(tri_count(o) for o in meshes), materials=sorted({m.name for o in meshes for m in o.data.materials}),
                 header=fbx_global_settings(path))
        if arms:
            a = arms[0]
            r["bones"] = [b.name for b in a.data.bones]
            hw = a.matrix_world
            r["hand_r_head"] = [round(c, 1) for c in hw @ a.data.bones["hand_r"].head_local]
            r["ball_l_head"] = [round(c, 1) for c in hw @ a.data.bones["ball_l"].head_local]
            # UE coordinates are Blender (x, -y, z)
            hr = r["hand_r_head"]
            r["hand_r_in_UE"] = [hr[0], -hr[1], hr[2]]
            assert Vector(hr).dot(right) > 50, "right hand on the wrong side"
            assert Vector(r["ball_l_head"]).dot(fwd) > 10, "toes point the wrong way"
            # weighting sanity: every vertex in exactly one group with weight 1
            for o in meshes:
                for v in o.data.vertices:
                    assert len(v.groups) == 1 and abs(v.groups[0].weight - 1) < 1e-4, "rigid weighting broken"
        if "height" in exp:
            assert abs(mx.z - exp["height"]) < 1.5 and abs(mn.z) < 0.5, f"{fname}: height {mx.z:.1f} != {exp['height']}"
        assert r["tris"] < exp["budget"], f"{fname}: {r['tris']} tris over budget"
        report[fname] = r
        print(f"[verify] {fname}: z {mn.z:.1f}..{mx.z:.1f} cm, x {mn.x:.1f}..{mx.x:.1f}, "
              f"y {mn.y:.1f}..{mx.y:.1f}, tris {r['tris']}, header {r['header']}")
    return report


# --------------------------------------------------------------------------------------
# preview rendering (Workbench)
# --------------------------------------------------------------------------------------


def rot_bone(arm, name, axis, deg):
    pb = arm.pose.bones[name]
    M = pb.matrix.copy()
    h = M.translation.copy()
    pb.matrix = Matrix.Translation(h) @ R(axis, deg) @ Matrix.Translation(-h) @ M
    bpy.context.view_layer.update()


def pose_hero(arm, hero):
    """Relaxed preview pose (export stays in the T-pose rest pose)."""
    for pb in arm.pose.bones:
        pb.matrix_basis = Matrix.Identity(4)
    bpy.context.view_layer.update()
    if hero == "Clavus":
        rot_bone(arm, "upperarm_l", "X", -72)
        rot_bone(arm, "lowerarm_l", "Y", -12)
        rot_bone(arm, "upperarm_r", "X", 76)
        rot_bone(arm, "upperarm_r", "Y", -10)
        rot_bone(arm, "lowerarm_r", "Y", -72)
    else:
        rot_bone(arm, "upperarm_l", "X", -62)
        rot_bone(arm, "lowerarm_l", "Y", -15)
        rot_bone(arm, "upperarm_r", "X", 55)    # hand resting on the planted hammer pommel
        rot_bone(arm, "lowerarm_r", "Y", -45)
    for side in "lr":
        s = 1 if side == "l" else -1
        rot_bone(arm, f"thigh_{side}", "X", 4 * s)
        rot_bone(arm, f"foot_{side}", "X", -4 * s)


def place_weapon(arm, weapon, extra=Matrix.Identity(4), snap_ground=False):
    bpy.context.view_layer.update()
    pb = arm.pose.bones["weapon_r"]
    head = arm.matrix_world @ pb.head
    weapon.matrix_world = Matrix.Translation(head) @ extra
    if snap_ground:  # preview only: rest the planted weapon on the floor (shifts a few cm)
        bpy.context.view_layer.update()
        low = min((weapon.matrix_world @ v.co).z for v in weapon.data.vertices)
        weapon.location.z -= low


def setup_render(scene):
    scene.render.engine = "BLENDER_WORKBENCH"
    scene.render.resolution_x = scene.render.resolution_y = 1024
    scene.render.resolution_percentage = 100
    scene.render.film_transparent = False
    scene.render.image_settings.file_format = "PNG"
    scene.display.render_aa = "8"
    scene.view_settings.view_transform = "Standard"
    scene.view_settings.exposure = 0.45
    sh = scene.display.shading
    sh.light = "STUDIO"
    sh.color_type = "MATERIAL"
    sh.show_cavity = True
    sh.cavity_type = "BOTH"
    sh.cavity_ridge_factor = 1.2
    sh.cavity_valley_factor = 1.0
    sh.curvature_ridge_factor = 1.0
    sh.curvature_valley_factor = 1.0
    sh.show_object_outline = True
    sh.object_outline_color = (0.03, 0.03, 0.035)
    sh.show_specular_highlight = True
    sh.show_shadows = False
    world = bpy.data.worlds.new("Studio")
    world.color = hex_rgb("#A9ADB2")
    scene.world = world


def bbox_of(objs):
    deps = bpy.context.evaluated_depsgraph_get()
    pts = []
    for o in objs:
        ev = o.evaluated_get(deps)
        me = ev.to_mesh()
        pts += [ev.matrix_world @ v.co for v in me.vertices]
        ev.to_mesh_clear()
    mn = Vector((min(p.x for p in pts), min(p.y for p in pts), min(p.z for p in pts)))
    mx = Vector((max(p.x for p in pts), max(p.y for p in pts), max(p.z for p in pts)))
    return mn, mx


def render_view(scene, objs, view, path, margin=1.12):
    mn, mx = bbox_of(objs)
    ctr = (mn + mx) / 2
    size = mx - mn
    cam_data = bpy.data.cameras.new("cam")
    cam = bpy.data.objects.new("cam", cam_data)
    scene.collection.objects.link(cam)
    scene.camera = cam
    if view in ("front", "side", "side_left", "lineup"):
        cam_data.type = "ORTHO"
        if view in ("front", "lineup"):
            d = Vector((1, 0, 0))
            w = size.y
        else:
            d = Vector((0, 1 if view == "side_left" else -1, 0))
            w = size.x
        cam_data.ortho_scale = max(size.z, w) * margin
        cam.location = ctr + d * 600
    else:
        cam_data.type = "PERSP"
        cam_data.lens = 70
        d = Vector((math.cos(math.radians(38)), -math.sin(math.radians(38)), 0.28)).normalized()
        half = max(size.z, size.x, size.y) * margin / 2
        dist = half / math.tan(math.atan(18 / 70)) + max(size.x, size.y) / 2
        cam.location = ctr + d * dist
    cam_data.clip_start, cam_data.clip_end = 1, 5000
    cam.rotation_euler = (-d).to_track_quat("-Z", "Y").to_euler()
    scene.render.filepath = str(path)
    bpy.ops.render.render(write_still=True)
    bpy.data.objects.remove(cam)
    print(f"[render] {path.relative_to(ROOT)}")


def add_text(scene, body, loc, size, color_hex, align="LEFT"):
    cu = bpy.data.curves.new("txt", "FONT")
    cu.body = body
    cu.size = size
    cu.align_x = align
    cu.extrude = 0.2
    ob = bpy.data.objects.new("txt_" + body, cu)
    ob.location = loc
    ob.rotation_euler = (math.radians(90), 0, math.radians(90))  # faces +X, reads along +Y
    mat = make_material("UI_" + color_hex, color_hex, 0.8)
    cu.materials.append(mat)
    scene.collection.objects.link(ob)
    return ob


def add_block(scene, name, size, loc, color_hex):
    v, f = g_box(*size)
    me = bpy.data.meshes.new(name)
    me.from_pydata(v, [], f)
    me.materials.append(make_material("UI_" + color_hex, color_hex, 0.8))
    ob = bpy.data.objects.new(name, me)
    ob.location = loc
    scene.collection.objects.link(ob)
    return ob


def render_previews(heroes):
    scene = bpy.context.scene
    setup_render(scene)
    PREVIEW_DIR.mkdir(parents=True, exist_ok=True)
    all_objs = []
    for name, (arm, mesh, wpn, extra) in heroes.items():
        pose_hero(arm, name)
        place_weapon(arm, wpn, extra, snap_ground=name == "Malleus")
        all_objs += [arm, mesh, wpn]
    for name, (arm, mesh, wpn, extra) in heroes.items():
        for o in all_objs:
            o.hide_render = o not in (mesh, wpn)
        for view in ("front", "side", "threequarter"):
            # Clavus side = his right (pad side); Malleus side = his left (hammer would hide the body)
            v = "side_left" if (view == "side" and name == "Malleus") else view
            render_view(scene, [mesh, wpn], v, PREVIEW_DIR / f"{name}_{view}.png")
    # lineup: side by side, with a 190 / 210 cm scale bar
    for o in all_objs:
        o.hide_render = o.type == "ARMATURE"
    offsets = {"Clavus": -75.0, "Malleus": 95.0}
    for name, (arm, mesh, wpn, extra) in heroes.items():
        arm.location.y = offsets[name]
        place_weapon(arm, wpn, extra, snap_ground=name == "Malleus")
    bar_y = -165
    add_block(scene, "bar", (2, 5, 210), (0, bar_y, 105), "#2B2D30")
    for z in range(0, 211, 10):
        w = 12 if z % 50 == 0 else 6
        add_block(scene, f"tick{z}", (2, w, 1.0), (0, bar_y + w / 2, z), "#2B2D30")
    for z, txt, col in ((190, "190 cm", "#8F4220"), (210, "210 cm", "#B85A10")):
        add_block(scene, f"line{z}", (1, 330, 0.8), (-120, bar_y + 165, z), col)
        add_text(scene, txt, (5, bar_y - 4, z + 1.5), 8, col, "RIGHT")
    add_text(scene, "0", (5, bar_y - 4, 1.5), 8, "#2B2D30", "RIGHT")
    add_text(scene, "100", (5, bar_y - 4, 101.5), 8, "#2B2D30", "RIGHT")
    add_text(scene, "CLAVUS", (60, offsets["Clavus"], -14), 11, "#2B2D30", "CENTER")
    add_text(scene, "MALLEUS", (60, offsets["Malleus"], -14), 11, "#2B2D30", "CENTER")
    vis = [o for o in scene.objects if not o.hide_render and o.type in ("MESH", "FONT")]
    render_view(scene, vis, "lineup", PREVIEW_DIR / "lineup.png", margin=1.06)
    for name, (arm, mesh, wpn, extra) in heroes.items():
        arm.location.y = 0


# --------------------------------------------------------------------------------------


def arg_value(flag, default):
    return sys.argv[sys.argv.index(flag) + 1] if flag in sys.argv else default


def main():
    # --ue-face-y : turn the heroes so they face UE +Y like the UE5 Mannequin (mesh component then needs
    #               yaw -90). Default: face UE +X (matches the current C++, mesh component unrotated).
    # --export-dir DIR : write the FBX files somewhere else (e.g. for a --ue-face-y variant).
    face_y = "--ue-face-y" in sys.argv
    do_render = "--no-render" not in sys.argv and not face_y
    export_dir = Path(arg_value("--export-dir", EXPORT_DIR)).resolve()
    reset_scene()
    export_dir.mkdir(parents=True, exist_ok=True)

    c_arm, c_mesh = build_clavus()
    gun = build_nailgun()
    m_arm, m_mesh = build_malleus()
    hammer = build_hammer()
    if face_y:  # Blender +X -> Blender -Y (= UE +Y); weapons stay +X since weapon_r turns with the rig
        rz = R("Z", -90)
        for arm, mesh in ((c_arm, c_mesh), (m_arm, m_mesh)):
            arm.data.transform(rz)
            mesh.data.transform(rz)

    stats = {o.name: tri_count(o) for o in (c_mesh, gun, m_mesh, hammer)}
    print("[tris]", stats)
    export_skeletal(c_arm, c_mesh, export_dir / "SK_Hero_Clavus.fbx")
    export_skeletal(m_arm, m_mesh, export_dir / "SK_Hero_Malleus.fbx")
    export_static(gun, export_dir / "SM_Wpn_Clavus_Nailgun.fbx")
    export_static(hammer, export_dir / "SM_Wpn_Malleus_Hammer.fbx")

    if do_render:
        render_previews({
            "Clavus": (c_arm, c_mesh, gun, R("Y", -4)),
            "Malleus": (m_arm, m_mesh, hammer, R("X", 180) @ R("Y", 6)),
        })

    report = verify_exports({
        "SK_Hero_Clavus.fbx": dict(height=199.6, budget=15000),   # crown 190, pad cap 199.6
        "SK_Hero_Malleus.fbx": dict(height=210.0, budget=15000),
        "SM_Wpn_Clavus_Nailgun.fbx": dict(budget=4000),
        "SM_Wpn_Malleus_Hammer.fbx": dict(budget=4000),
    }, export_dir, face_y)
    SCRATCH_DIR.mkdir(parents=True, exist_ok=True)
    (SCRATCH_DIR / ("verify_report_face_y.json" if face_y else "verify_report.json")).write_text(json.dumps(report, indent=2, default=str))
    print("[done]")


if __name__ == "__main__":
    main()
