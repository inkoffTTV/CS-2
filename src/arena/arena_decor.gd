class_name BArenaDecor
extends RefCounted
## Non-gameplay set dressing: wall pilasters and cornice, the colonnade and ground outside the
## arena (so the horizon is not empty), the Rubicon's gold capping and inscription, base floor zones.


static func build(root: Node3D) -> void:
	_walls(root)
	_outside(root)
	_rubicon(root)
	for team in 2:
		_base_zone(root, team)


static func _walls(root: Node3D) -> void:
	var trim: Material = BArenaMaterials.get_material("wall_trim")
	var half: float = BArena.SIZE * 0.5
	var h: float = BArena.WALL_HEIGHT
	var full: float = BArena.SIZE + 1.6
	# Cornice on top and a plinth at the bottom of the inner faces.
	for side in 4:
		var along_x: bool = side >= 2
		var s: float = -1.0 if side % 2 == 0 else 1.0
		var size_top := Vector3(full, 0.3, 1.6) if along_x else Vector3(1.6, 0.3, full)
		var pos_top := Vector3(0, h + 0.15, s * half) if along_x else Vector3(s * half, h + 0.15, 0)
		BArenaKit.box(root, "Cornice", pos_top, size_top, trim, Vector3.ZERO, false)
		var size_plinth := Vector3(BArena.SIZE, 0.5, 1.3) if along_x else Vector3(1.3, 0.5, BArena.SIZE)
		var pos_plinth := Vector3(0, 0.25, s * half) if along_x else Vector3(s * half, 0.25, 0)
		BArenaKit.box(root, "Plinth", pos_plinth, size_plinth, trim, Vector3.ZERO, false)
		# Pilasters every 6 m along the inner face.
		var k: float = -half + 3.0
		while k < half:
			var p := Vector3(k, h * 0.5, s * (half - 0.6)) if along_x else Vector3(s * (half - 0.6), h * 0.5, k)
			var sz := Vector3(0.7, h, 0.35) if along_x else Vector3(0.35, h, 0.7)
			BArenaKit.box(root, "Pilaster", p, sz, trim, Vector3.ZERO, false)
			k += 6.0


static func _outside(root: Node3D) -> void:
	var ground := MeshInstance3D.new()
	ground.name = "OuterGround"
	var plane := PlaneMesh.new()
	plane.size = Vector2(600, 600)
	ground.mesh = plane
	ground.material_override = BArenaMaterials.get_material("outer_ground")
	ground.position = Vector3(0, -0.02, 0)
	root.add_child(ground)
	_cypresses(root)
	# A colonnade around the arena, visible above the walls.
	var stone: Material = BArenaMaterials.get_material("column")
	var gold: Material = BArenaMaterials.get_material("gold")
	var r: float = BArena.SIZE * 0.5 + 4.0
	var step: float = 8.0
	var k: float = -r
	while k <= r + 0.01:
		for p: Vector3 in [Vector3(k, 0, -r), Vector3(k, 0, r), Vector3(-r, 0, k), Vector3(r, 0, k)]:
			BArenaKit.cylinder(root, "Column", p + Vector3(0, 4.5, 0), 0.6, 9.0, stone)
			BArenaKit.box(root, "Capital", p + Vector3(0, 9.2, 0), Vector3(1.7, 0.4, 1.7), stone, Vector3.ZERO, false)
			BArenaKit.box(root, "ColumnBase", p + Vector3(0, 0.25, 0), Vector3(1.6, 0.5, 1.6), stone, Vector3.ZERO, false)
		k += step
	# Architrave linking the column tops.
	for side in 4:
		var along_x: bool = side >= 2
		var s: float = -1.0 if side % 2 == 0 else 1.0
		var size := Vector3(2 * r + 1.7, 0.9, 1.4) if along_x else Vector3(1.4, 0.9, 2 * r + 1.7)
		var pos := Vector3(0, 9.85, s * r) if along_x else Vector3(s * r, 9.85, 0)
		BArenaKit.box(root, "Architrave", pos, size, stone, Vector3.ZERO, false)
		var band := Vector3(size.x + 0.02, 0.18, size.z + 0.02)
		BArenaKit.box(root, "ArchitraveGold", pos + Vector3(0, -0.3, 0), band, gold, Vector3.ZERO, false)


static func _rubicon(root: Node3D) -> void:
	var gold: Material = BArenaMaterials.get_material("gold")
	var h: float = BArena.WALL_HEIGHT
	for seg: Vector2 in [Vector2(-21, 18), Vector2(0, 12), Vector2(21, 18)]:
		BArenaKit.box(root, "RubiconCap", Vector3(0, h + 0.06, seg.x), Vector3(0.7, 0.12, seg.y + 0.2), gold, Vector3.ZERO, false)
		BArenaKit.box(root, "RubiconFoot", Vector3(0, 0.15, seg.x), Vector3(0.8, 0.3, seg.y + 0.2), gold, Vector3.ZERO, false)
	# "ALEA IACTA EST" on both faces of the middle segment (Caesar crossing the Rubicon).
	for s: float in [-1.0, 1.0]:
		var label := Label3D.new()
		label.name = "RubiconInscription"
		label.text = "ALEA IACTA EST"
		label.font_size = 96
		label.pixel_size = 0.006
		label.modulate = Color(0.95, 0.78, 0.40)
		label.outline_size = 0
		label.shaded = true
		label.double_sided = false
		label.position = Vector3(s * 0.27, 2.9, 0)
		label.rotation = Vector3(0, s * PI * 0.5, 0)
		root.add_child(label)


static func _base_zone(root: Node3D, team: int) -> void:
	var s: float = -1.0 if team == BTeams.OPTIMATES else 1.0
	var zone := MeshInstance3D.new()
	zone.name = "BaseZone%d" % team
	var plane := PlaneMesh.new()
	plane.size = Vector2(11.0, 22.0)
	zone.mesh = plane
	zone.material_override = BArenaMaterials.get_material("zone%d" % team)
	zone.position = Vector3(s * 24.0, 0.012, 0)
	zone.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	root.add_child(zone)
	# Gold border lines of the zone.
	var gold: Material = BArenaMaterials.get_material("gold")
	var inner_x: float = s * 18.5
	BArenaKit.box(root, "ZoneEdge", Vector3(inner_x, 0.02, 0), Vector3(0.12, 0.04, 22.0), gold, Vector3.ZERO, false)
	for z: float in [-11.0, 11.0]:
		BArenaKit.box(root, "ZoneEdge", Vector3(s * 24.0, 0.02, z), Vector3(11.0, 0.04, 0.12), gold, Vector3.ZERO, false)


## Dark cypress trees scattered outside the colonnade (deterministic placement).
static func _cypresses(root: Node3D) -> void:
	var rng := RandomNumberGenerator.new()
	rng.seed = 753  # ab urbe condita
	var mat: Material = BArenaMaterials.get_material("cypress")
	var trunk: Material = BArenaMaterials.get_material("pole")
	var placed: int = 0
	while placed < 90:
		var p := Vector3(rng.randf_range(-110.0, 110.0), 0.0, rng.randf_range(-110.0, 110.0))
		if absf(p.x) < 42.0 and absf(p.z) < 42.0:
			continue
		var h: float = rng.randf_range(9.0, 16.0)
		var r: float = h * rng.randf_range(0.11, 0.15)
		var crown := MeshInstance3D.new()
		crown.name = "Cypress"
		var cone := CylinderMesh.new()
		cone.top_radius = 0.0
		cone.bottom_radius = r
		cone.height = h
		cone.radial_segments = 10
		cone.rings = 1
		crown.mesh = cone
		crown.material_override = mat
		crown.position = p + Vector3(0, h * 0.5 + 0.8, 0)
		root.add_child(crown)
		BArenaKit.cylinder(root, "CypressTrunk", p + Vector3(0, 0.5, 0), r * 0.25, 1.0, trunk)
		placed += 1
