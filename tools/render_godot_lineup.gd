extends SceneTree
## Renders both posed heroes side by side: xvfb-run -a godot --path . --rendering-driver opengl3 --rendering-method gl_compatibility --script res://tools/render_godot_lineup.gd
## Writes Art/Previews/godot_lineup.png. Heroes face -Z, camera looks from -Z (viewer's left = +X), so they face the camera.

func _init() -> void:
	var vp := SubViewport.new()
	vp.size = Vector2i(1600, 800)
	vp.render_target_update_mode = SubViewport.UPDATE_ALWAYS
	root.add_child(vp)
	var env := Environment.new()
	var sky_mat := ProceduralSkyMaterial.new()
	sky_mat.sky_top_color = Color(0.55, 0.65, 0.8)
	sky_mat.sky_horizon_color = Color(0.8, 0.82, 0.86)
	sky_mat.ground_horizon_color = Color(0.6, 0.6, 0.62)
	sky_mat.ground_bottom_color = Color(0.35, 0.36, 0.4)
	var sky := Sky.new()
	sky.sky_material = sky_mat
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.ambient_light_energy = 1.0
	env.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
	var we := WorldEnvironment.new()
	we.environment = env
	vp.add_child(we)
	var sun := DirectionalLight3D.new()
	sun.rotation_degrees = Vector3(-40, 150, 0)
	sun.light_energy = 1.1
	vp.add_child(sun)
	var floor_mesh := MeshInstance3D.new()
	var pm := PlaneMesh.new()
	pm.size = Vector2(12, 8)
	floor_mesh.mesh = pm
	var fm := StandardMaterial3D.new()
	fm.albedo_color = Color(0.35, 0.37, 0.4)
	floor_mesh.material_override = fm
	vp.add_child(floor_mesh)
	var c := (load("res://assets/heroes/clavus/clavus_posed.glb") as PackedScene).instantiate() as Node3D
	c.position = Vector3(1.4, 0, 0)
	vp.add_child(c)
	var m := (load("res://assets/heroes/malleus/malleus_posed.glb") as PackedScene).instantiate() as Node3D
	m.position = Vector3(-1.4, 0, 0)
	vp.add_child(m)
	var cam := Camera3D.new()
	cam.fov = 24
	vp.add_child(cam)
	cam.look_at_from_position(Vector3(0, 1.1, -7.5), Vector3(0, 1.0, 0))
	cam.current = true
	for i in 6:
		await process_frame
	var img := vp.get_texture().get_image()
	img.save_png("res://Art/Previews/godot_lineup.png")
	print("saved ", img.get_size())
	quit()
