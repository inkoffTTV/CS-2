extends SceneTree
## Verifies the hero/weapon .glb files as Godot imports them (run after `godot --headless --path . --import`):
##   godot --headless --path . --script res://tools/verify_hero_assets.gd
## Convention: metres, Y up, feet at y=0, hero faces -Z, its right side is +X.

var failed := false

func _collect(n: Node, out: Array) -> void:
	if n is MeshInstance3D:
		out.append(n)
	for c in n.get_children():
		_collect(c, out)

func _aabb(root: Node3D) -> AABB:
	var meshes: Array = []
	_collect(root, meshes)
	var box := AABB()
	var first := true
	for m: MeshInstance3D in meshes:
		var b: AABB = _rel(root, m) * m.get_aabb()
		box = b if first else box.merge(b)
		first = false
	return box

func _rel(top: Node3D, n: Node3D) -> Transform3D:
	# transform of n in top's space (works outside the scene tree)
	var t := Transform3D.IDENTITY
	var cur: Node = n
	while cur != top and cur is Node3D:
		t = (cur as Node3D).transform * t
		cur = cur.get_parent()
	return t

func _check(label: String, ok: bool, checks: Array) -> void:
	checks.append("%s=%s" % [label, "ok" if ok else "FAIL"])
	if not ok:
		failed = true

func _load(path: String) -> Node3D:
	var ps := load(path) as PackedScene
	if ps == null:
		printerr("[verify] cannot load ", path)
		failed = true
		return null
	var n := ps.instantiate() as Node3D
	return n

func _asset(path: String, hmin: float, hmax: float, facing: String) -> void:
	var n := _load(path)
	if n == null:
		return
	var b := _aabb(n)
	var checks: Array = []
	_check("height", b.size.y >= hmin and b.size.y <= hmax, checks)
	if not facing.ends_with("gun") and not facing.ends_with("hammer"):
		_check("floor", absf(b.position.y) < 0.02, checks)
	var ext_front := -b.position.z      # extent toward -Z (forward)
	var ext_back := b.end.z
	if facing == "clavus":
		# gun muzzle forward of the back; gun/right pad on +X side
		_check("faces-Z", ext_front > ext_back + 0.1, checks)
		_check("right=+X", b.end.x > -b.position.x + 0.02, checks)
	elif facing == "malleus":
		_check("faces-Z", ext_front > ext_back - 0.05, checks)
		_check("right=+X", b.end.x > -b.position.x + 0.02, checks)
	elif facing.ends_with("_rig"):
		pass  # facing is checked on the skeleton below (backpack vents can stick out further than the toes)
	elif facing == "clavus_gun":
		_check("barrel-Z", ext_front > ext_back * 2.0, checks)
	elif facing == "malleus_hammer":
		_check("barrel-Z", ext_front > ext_back * 2.0 or b.size.z > 0.3, checks)
	print("[verify] %s: size=(%.3f, %.3f, %.3f) m  min=(%.3f, %.3f, %.3f) max=(%.3f, %.3f, %.3f)  %s" % [
		path.get_file(), b.size.x, b.size.y, b.size.z, b.position.x, b.position.y, b.position.z,
		b.end.x, b.end.y, b.end.z, " ".join(checks)])
	if facing.ends_with("_rig"):
		var sk := _find_skeleton(n)
		var ok := sk != null and sk.find_bone("pelvis") >= 0 and sk.find_bone("hand_r") >= 0 and sk.find_bone("upperarm_l") >= 0
		print("[verify] %s: skeleton bones=%d UE names=%s" % [path.get_file(), sk.get_bone_count() if sk else 0, "ok" if ok else "FAIL"])
		if not ok:
			failed = true
		else:
			# right hand must be on +X of the left hand (hero faces -Z, right = +X)
			var rel := _rel(n, sk)
			var r := (rel * sk.get_bone_global_pose(sk.find_bone("hand_r"))).origin
			var l := (rel * sk.get_bone_global_pose(sk.find_bone("hand_l"))).origin
			var foot := (rel * sk.get_bone_global_pose(sk.find_bone("foot_r"))).origin
			var ball := (rel * sk.get_bone_global_pose(sk.find_bone("ball_r"))).origin
			var fok := ball.z < foot.z - 0.03
			print("[verify] %s: ball_r.z=%.2f foot_r.z=%.2f toes point -Z %s" % [path.get_file(), ball.z, foot.z, "ok" if fok else "FAIL"])
			if not fok:
				failed = true
			var hok := r.x > l.x
			print("[verify] %s: hand_r.x=%.2f hand_l.x=%.2f right=+X %s" % [path.get_file(), r.x, l.x, "ok" if hok else "FAIL"])
			if not hok:
				failed = true
	n.free()

func _find_skeleton(n: Node) -> Skeleton3D:
	if n is Skeleton3D:
		return n
	for c in n.get_children():
		var s := _find_skeleton(c)
		if s:
			return s
	return null

func _init() -> void:
	_asset("res://assets/heroes/clavus/clavus_posed.glb", 1.95, 2.0, "clavus")
	_asset("res://assets/heroes/malleus/malleus_posed.glb", 2.08, 2.12, "malleus")
	_asset("res://assets/heroes/clavus/clavus.glb", 1.95, 2.0, "clavus_rig")
	_asset("res://assets/heroes/malleus/malleus.glb", 2.08, 2.12, "malleus_rig")
	_asset("res://assets/weapons/clavus_nailgun.glb", 0.2, 1.3, "clavus_gun")
	_asset("res://assets/weapons/malleus_hammer.glb", 0.2, 2.2, "malleus_hammer")
	print("[verify] ", "FAILED" if failed else "ALL OK")
	quit(1 if failed else 0)
