extends SceneTree
## Bird's-eye render of the arena without networking:
## xvfb-run -a godot --path . --rendering-driver opengl3 --rendering-method gl_compatibility --audio-driver Dummy -s res://tools/arena_overview.gd -- <out.png>


func _init() -> void:
	var args: PackedStringArray = OS.get_cmdline_user_args()
	var out: String = args[0] if args.size() > 0 else "user://arena_overview.png"
	var arena := BArena.new()
	root.add_child(arena)
	await process_frame
	var hero := (load("res://assets/heroes/clavus/clavus_posed.glb") as PackedScene).instantiate() as Node3D
	hero.transform = arena.get_spawn(0, 0)
	root.add_child(hero)
	var cam := Camera3D.new()
	cam.fov = 55.0
	root.add_child(cam)
	cam.look_at_from_position(Vector3(-46, 30, 40), Vector3(-2, 0, 2))
	cam.current = true
	for i in 8:
		await process_frame
	var img := root.get_viewport().get_texture().get_image()
	img.save_png(out)
	print("saved ", out, " ", img.get_size())
	quit()
