class_name BPlayerVisual
extends Node3D
## Look of a player: the posed hero model, a team-coloured ring under the feet, a nameplate,
## and a lean while sliding or crouching. Purely cosmetic, never rolled back.

const HERO_SCENE: PackedScene = preload("res://assets/heroes/clavus/clavus_posed.glb")

var _player: BPlayer
var _model: Node3D
var _nameplate: Label3D
var _lean: float = 0.0


func setup(player: BPlayer) -> void:
	_player = player
	_model = HERO_SCENE.instantiate() as Node3D
	_model.name = "Model"
	add_child(_model)

	var ring := MeshInstance3D.new()
	ring.name = "TeamRing"
	var torus := TorusMesh.new()
	torus.inner_radius = 0.48
	torus.outer_radius = 0.58
	torus.rings = 32
	ring.mesh = torus
	ring.scale = Vector3(1.0, 0.15, 1.0)
	ring.position = Vector3(0.0, 0.03, 0.0)
	var mat := StandardMaterial3D.new()
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	mat.albedo_color = BTeams.team_color(player.team)
	mat.emission_enabled = true
	mat.emission = BTeams.team_color(player.team)
	mat.emission_energy_multiplier = 1.5
	ring.material_override = mat
	ring.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	add_child(ring)

	_nameplate = Label3D.new()
	_nameplate.name = "Nameplate"
	_nameplate.text = player.nick
	_nameplate.modulate = BTeams.team_color(player.team).lerp(Color.WHITE, 0.3)
	_nameplate.outline_modulate = Color(0, 0, 0, 0.8)
	_nameplate.outline_size = 8
	_nameplate.font_size = 40
	_nameplate.pixel_size = 0.006
	_nameplate.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	_nameplate.no_depth_test = false
	_nameplate.position = Vector3(0.0, 2.35, 0.0)
	add_child(_nameplate)


func set_nameplate_visible(v: bool) -> void:
	if _nameplate != null:
		_nameplate.visible = v


func _process(delta: float) -> void:
	if _player == null or _model == null:
		return
	var target_lean: float = 0.0
	if _player.mode == BPlayer.Mode.SLIDE:
		target_lean = deg_to_rad(45.0)
	elif _player.is_crouched():
		target_lean = deg_to_rad(15.0)
	_lean = lerpf(_lean, target_lean, 1.0 - exp(-delta * 14.0))
	# Lean back around the feet (+X rotation tilts the head toward +Z, i.e. backwards).
	_model.rotation = Vector3(_lean, 0.0, 0.0)
	_model.position = Vector3(0.0, sin(_lean) * 0.25, 0.0)
