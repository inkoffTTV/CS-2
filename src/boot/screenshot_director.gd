class_name BScreenshotDirector
extends Node
## --screenshot PATH: waits for the local player, lets the scene settle, picks a camera
## (--shot-view player|overview|base0|base1|zipline), saves a PNG and quits.

const SETTLE_FRAMES: int = 45

var _frames_after_spawn: int = -1


func _ready() -> void:
	var game := get_parent() as BGame
	game.spawner.local_player_spawned.connect(func(_p: BPlayer) -> void: _frames_after_spawn = 0)
	get_tree().create_timer(30.0).timeout.connect(func() -> void:
		push_error("screenshot: no local player after 30 s")
		get_tree().quit(1))


func _process(_delta: float) -> void:
	if _frames_after_spawn < 0:
		return
	_frames_after_spawn += 1
	if _frames_after_spawn == 2:
		_place_camera()
	if _frames_after_spawn == SETTLE_FRAMES:
		_save()


func _place_camera() -> void:
	var view: String = Session.shot_view
	if view == "player":
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
