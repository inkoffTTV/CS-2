class_name BArenaDressing
extends RefCounted
## Base dressing (after Tools/create_greybox_map.py): spawn pads, throne, banners, team light,
## wall trim, zipline poles and cable.


static func build(arena: BArena) -> void:
	var root := Node3D.new()
	root.name = "Dressing"
	arena.add_child(root)
	BArenaDecor.build(root)
	for team in 2:
		_build_base(root, arena, team)
	_build_zipline(root, arena)


static func _build_base(root: Node3D, arena: BArena, team: int) -> void:
	var s: float = -1.0 if team == BTeams.OPTIMATES else 1.0
	var side: Material = BArenaMaterials.team(team)
	var glow: Material = BArenaMaterials.team_glow(team)
	var gold: Material = BArenaMaterials.get_material("gold")
	var tag: String = "T%d" % team

	for spawn: Transform3D in arena.spawns[team]:
		BArenaKit.cylinder(root, "SpawnPad_%s" % tag, Vector3(spawn.origin.x, 0.03, spawn.origin.z), 1.1, 0.06, glow)
		BArenaKit.cylinder(root, "SpawnRing_%s" % tag, Vector3(spawn.origin.x, 0.025, spawn.origin.z), 1.35, 0.05, gold)

	# Decorative throne at the back of the base (the real objective comes with the lane milestone).
	var back: float = s * 27.8
	BArenaKit.box(root, "Throne_%s_Dais" % tag, Vector3(back, 0.3, 0), Vector3(3.6, 0.6, 4.2), gold)
	BArenaKit.box(root, "Throne_%s_Seat" % tag, Vector3(back, 1.1, 0), Vector3(2.0, 1.0, 2.4), side)
	BArenaKit.box(root, "Throne_%s_Back" % tag, Vector3(back + s * 0.9, 3.0, 0), Vector3(0.4, 3.8, 2.4), side)
	BArenaKit.box(root, "Throne_%s_Crown" % tag, Vector3(back + s * 0.9, 5.1, 0), Vector3(0.5, 0.4, 1.6), gold, Vector3.ZERO, false)

	# Banners on the back wall.
	for z: float in [-13.0, 13.0]:
		BArenaKit.box(root, "Banner_%s" % tag, Vector3(s * 29.4, 2.3, z), Vector3(0.12, 3.4, 2.6), side, Vector3.ZERO, false)
		BArenaKit.box(root, "BannerBar_%s" % tag, Vector3(s * 29.35, 4.05, z), Vector3(0.16, 0.14, 3.0), gold, Vector3.ZERO, false)

	# Coloured light washing the base.
	var light := OmniLight3D.new()
	light.name = "BaseLight_%s" % tag
	light.light_color = BTeams.team_color(team)
	light.light_energy = 0.9
	light.omni_range = 18.0
	light.omni_attenuation = 1.2
	light.shadow_enabled = false
	light.position = Vector3(s * 23.0, 4.5, 0)
	root.add_child(light)


static func _build_zipline(root: Node3D, arena: BArena) -> void:
	var pole: Material = BArenaMaterials.get_material("pole")
	var cable: Material = BArenaMaterials.get_material("cable")
	for seg in arena.ziplines:
		for end: Vector3 in [seg.a, seg.b]:
			var base_y: float = _ground_height_under(end)
			var top: float = end.y + 0.6
			BArenaKit.cylinder(root, "ZipPole", Vector3(end.x, (base_y + top) * 0.5, end.z), 0.12, top - base_y, pole, true)
			BArenaKit.box(root, "ZipArm", Vector3(end.x, end.y + 0.15, end.z), Vector3(0.6, 0.12, 0.6), pole, Vector3.ZERO, false)
		BArenaKit.rod(root, "ZipCable", seg.a, seg.b, 0.035, cable)


static func _ground_height_under(p: Vector3) -> float:
	# The only raised ground under a zipline end is the platform.
	var c: Vector3 = BArena.PLATFORM_CENTER
	if absf(p.x - c.x) <= 3.0 and absf(p.z - c.z) <= 3.0:
		return c.y + 3.0
	return 0.0
