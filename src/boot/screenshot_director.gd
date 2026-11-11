class_name BScreenshotDirector
extends Node
## --screenshot PATH: waits for the local player, lets the scene settle, picks a camera
## (--shot-view player|overview|base0|base1|zipline|combat), waits --shot-delay seconds, saves a PNG,
## quits. With --bot the local player follows a bot scenario first (e.g. ride the zipline).
## combat: the player camera; after the delay it waits (up to COMBAT_WAIT s) for a moment with a
## kill in the feed, a fresh hit marker and the local hero alive (and preferably hurt).

const MIN_FRAMES: int = 10
const COMBAT_WAIT: float = 40.0

var _frames_after_spawn: int = -1
var _spawned_ms: int = 0


func _ready() -> void:
	# Software rendering under Xvfb is slow; let netfox catch up on all ticks so bots move in real time.
	NetworkTime._max_ticks_per_frame = 120
	var game := get_parent() as BGame
	game.spawner.local_player_spawned.connect(_on_spawned)
	get_tree().create_timer(60.0 + Session.shot_delay + COMBAT_WAIT).timeout.connect(func() -> void:
		push_error("screenshot: no local player after 30 s")
		get_tree().quit(1))


func _on_spawned(_p: BPlayer) -> void:
	_frames_after_spawn = 0
	_spawned_ms = Time.get_ticks_msec()


func _process(_delta: float) -> void:
	if _frames_after_spawn < 0:
		return
	_frames_after_spawn += 1
	if _frames_after_spawn == 2:
		_place_camera()
	var waited: float = float(Time.get_ticks_msec() - _spawned_ms) / 1000.0
	if _frames_after_spawn >= MIN_FRAMES and waited >= Session.shot_delay and _moment_ok(waited):
		_frames_after_spawn = -1
		_save()


func _moment_ok(waited: float) -> bool:
	if Session.shot_view != "combat" or waited > Session.shot_delay + COMBAT_WAIT:
		return true
	var game := get_parent() as BGame
	var p: BPlayer = game.local_player
	if p == null or p.is_down() or not game.hud.is_combat_moment():
		return false
	# Prefer a moment with some damage taken (shows the health bar working) for the first half.
	return p.health.health < p.health.max_health or waited > Session.shot_delay + COMBAT_WAIT * 0.5


func _place_camera() -> void:
	var view: String = Session.shot_view
	if view == "player" or view == "combat":
		return
	var cam := Camera3D.new()
	cam.name = "ShotCamera"
	cam.fov = 60.0
	get_parent().add_child(cam)
	match view:
		"overview":
			cam.look_at_from_position(Vector3(-44, 30, 40), Vector3(-1, 0, 1))
		"base0":
			cam.look_at_from_position(Vector3(-7, 5, -10), Vector3(-25, 1.5, 1))
		"base1":
			cam.look_at_from_position(Vector3(7, 5, 10), Vector3(25, 1.5, -1))
		"zipline":
			cam.look_at_from_position(Vector3(-14, 9, 12), Vector3(5, 4, -6))
		_:
			push_warning("unknown --shot-view " + view)
	cam.current = true


func _save() -> void:
	var img: Image = get_viewport().get_texture().get_image()
	var path: String = Session.screenshot_path
	var abs_path: String = path if path.is_absolute_path() else ProjectSettings.globalize_path("res://").path_join(path)
	DirAccess.make_dir_recursive_absolute(abs_path.get_base_dir())
	var err: Error = img.save_png(abs_path)
	print("[screenshot] saved %s (%dx%d) err=%d" % [abs_path, img.get_width(), img.get_height(), err])
	Session.stop_network(multiplayer)
	get_tree().quit(0 if err == OK else 1)
