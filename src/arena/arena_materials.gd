class_name BArenaMaterials
extends RefCounted
## Arena palette (after Tools/create_greybox_map.py on the m2-heroes branch), cached per run.

static var _cache: Dictionary[String, Material] = {}


static func get_material(key: String) -> Material:
	if _cache.has(key):
		return _cache[key]
	var m: Material = _make(key)
	_cache[key] = m
	return m


static func _flat(color: Color, roughness: float, metallic: float = 0.0) -> StandardMaterial3D:
	var m := StandardMaterial3D.new()
	m.albedo_color = color
	m.roughness = roughness
	m.metallic = metallic
	return m


static func _glow(color: Color, energy: float) -> StandardMaterial3D:
	var m := _flat(color, 0.5)
	m.emission_enabled = true
	m.emission = color
	m.emission_energy_multiplier = energy
	return m


static func _zone(color: Color) -> StandardMaterial3D:
	return _flat(Color(0.50, 0.47, 0.43).lerp(color, 0.35), 0.85)


static func _make(key: String) -> Material:
	match key:
		"floor":
			var sm := ShaderMaterial.new()
			sm.shader = load("res://src/arena/shaders/checker_floor.gdshader") as Shader
			return sm
		"wall":
			return _flat(Color(0.62, 0.56, 0.48), 0.9)
		"wall_trim":
			return _flat(Color(0.62, 0.56, 0.49), 0.85)
		"rubicon":
			return _flat(Color(0.36, 0.22, 0.12), 0.38, 1.0)
		"ramp":
			return _flat(Color(0.70, 0.69, 0.72), 0.75)
		"gold":
			return _flat(Color(0.95, 0.72, 0.30), 0.4, 0.35)
		"column":
			return _flat(Color(0.74, 0.70, 0.64), 0.85)
		"outer_ground":
			return _flat(Color(0.25, 0.29, 0.17), 1.0)
		"zone0":
			return _zone(BTeams.COLOR_OPTIMATES)
		"zone1":
			return _zone(BTeams.COLOR_POPULARES)
		"cable":
			return _flat(Color(0.20, 0.18, 0.16), 0.4, 0.8)
		"cypress":
			return _flat(Color(0.10, 0.20, 0.10), 1.0)
		"pole":
			return _flat(Color(0.30, 0.27, 0.24), 0.5, 0.6)
		"team0":
			return _flat(BTeams.COLOR_OPTIMATES.lerp(Color(0.2, 0.2, 0.3), 0.15), 0.6)
		"team1":
			return _flat(BTeams.COLOR_POPULARES.lerp(Color(0.3, 0.2, 0.15), 0.1), 0.6)
		"glow0":
			return _glow(BTeams.COLOR_OPTIMATES, 1.4)
		"glow1":
			return _glow(BTeams.COLOR_POPULARES, 1.4)
	push_error("unknown arena material " + key)
	return _flat(Color.MAGENTA, 1.0)


static func team(team_id: int) -> Material:
	return get_material("team%d" % team_id)


static func team_glow(team_id: int) -> Material:
	return get_material("glow%d" % team_id)
