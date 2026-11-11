class_name BArenaEnvironment
extends RefCounted
## Late-afternoon sky, warm sun with shadows, light fog and glow.


static func build(arena: Node3D) -> void:
	var sky_mat := ProceduralSkyMaterial.new()
	sky_mat.sky_top_color = Color(0.12, 0.30, 0.66)
	sky_mat.sky_horizon_color = Color(0.80, 0.72, 0.62)
	sky_mat.ground_horizon_color = Color(0.50, 0.47, 0.38)
	sky_mat.ground_bottom_color = Color(0.20, 0.20, 0.17)
	sky_mat.sun_angle_max = 30.0
	var sky := Sky.new()
	sky.sky_material = sky_mat

	var env := Environment.new()
	env.background_mode = Environment.BG_SKY
	env.sky = sky
	env.ambient_light_source = Environment.AMBIENT_SOURCE_SKY
	env.ambient_light_energy = 0.5
	env.ambient_light_color = Color(0.72, 0.66, 0.60)
	env.ambient_light_sky_contribution = 0.35
	env.reflected_light_source = Environment.REFLECTION_SOURCE_SKY
	env.tonemap_mode = Environment.TONE_MAPPER_FILMIC
	env.tonemap_exposure = 1.0
	env.fog_enabled = true
	env.fog_light_color = Color(0.76, 0.72, 0.68)
	env.fog_density = 0.0035
	env.fog_sky_affect = 0.15
	env.fog_aerial_perspective = 0.4
	env.glow_enabled = true
	env.glow_intensity = 0.7
	env.glow_bloom = 0.05
	env.glow_hdr_threshold = 1.0
	env.ssao_enabled = true
	env.ssao_intensity = 1.5

	var we := WorldEnvironment.new()
	we.name = "WorldEnvironment"
	we.environment = env
	arena.add_child(we)

	var sun := DirectionalLight3D.new()
	sun.name = "Sun"
	sun.light_color = Color(1.0, 0.89, 0.77)
	sun.light_energy = 1.5
	sun.rotation_degrees = Vector3(-40.0, -155.0, 0.0)
	sun.shadow_enabled = true
	sun.directional_shadow_max_distance = 120.0
	sun.directional_shadow_mode = DirectionalLight3D.SHADOW_PARALLEL_4_SPLITS
	arena.add_child(sun)
