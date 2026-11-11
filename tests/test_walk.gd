extends RefCounted
const T := preload("res://tests/t.gd")


func test_facing_yaw_zero_is_minus_z() -> void:
	T.vec(BWalk.facing_from_yaw(0.0), Vector3(0, 0, -1))
	T.vec(BWalk.facing_from_yaw(-PI / 2.0), Vector3(1, 0, 0), 1e-4, "yaw -90 faces +X")


func test_wish_dir_forward_and_right() -> void:
	T.vec(BWalk.wish_dir(Vector2(0, 1), 0.0), Vector3(0, 0, -1))
	T.vec(BWalk.wish_dir(Vector2(1, 0), 0.0), Vector3(1, 0, 0))
	T.approx(BWalk.wish_dir(Vector2(1, 1), 0.3).length(), 1.0, 1e-4, "diagonal clamped")


func test_ground_accelerates_to_walk_speed() -> void:
	var cfg := BWalkConfig.new()
	var v := Vector3.ZERO
	for i in 30:
		v = BWalk.step_horizontal(v, Vector3(0, 0, -1), 1.0 / 30.0, true, cfg.walk_speed, cfg)
	T.vec(v, Vector3(0, 0, -cfg.walk_speed))


func test_ground_brakes_without_input() -> void:
	var cfg := BWalkConfig.new()
	var v := BWalk.step_horizontal(Vector3(6.5, 0, 0), Vector3.ZERO, 0.1, true, cfg.walk_speed, cfg)
	T.approx(v.x, 6.5 - cfg.braking * 0.1)
	v = BWalk.step_horizontal(v, Vector3.ZERO, 1.0, true, cfg.walk_speed, cfg)
	T.vec(v, Vector3.ZERO)


func test_ground_overspeed_brakes_down_to_cap() -> void:
	var cfg := BWalkConfig.new()
	var v := BWalk.step_horizontal(Vector3(15, 0, 0), Vector3(1, 0, 0), 0.1, true, cfg.walk_speed, cfg)
	T.approx(v.x, 15.0 - cfg.braking * 0.1)


func test_air_keeps_momentum_without_input() -> void:
	var cfg := BWalkConfig.new()
	var v := BWalk.step_horizontal(Vector3(12, 0, 0), Vector3.ZERO, 0.5, false, cfg.walk_speed, cfg)
	T.vec(v, Vector3(12, 0, 0))


func test_air_control_is_weaker_and_never_exceeds_cap() -> void:
	var cfg := BWalkConfig.new()
	var v := BWalk.step_horizontal(Vector3.ZERO, Vector3(1, 0, 0), 0.1, false, cfg.walk_speed, cfg)
	T.approx(v.x, cfg.ground_accel * cfg.air_control * 0.1)
	var fast := BWalk.step_horizontal(Vector3(10, 0, 0), Vector3(0, 0, 1), 0.1, false, cfg.walk_speed, cfg)
	T.approx(fast.length(), 10.0, 1e-4, "steering in air keeps speed, does not add")


func test_end_air_dash_caps_horizontal_only() -> void:
	var cfg := BWalkConfig.new()
	var v := BWalk.end_air_dash(Vector3(15, -3, 0), cfg)
	T.approx(v.x, cfg.walk_speed * cfg.air_dash_exit_factor)
	T.approx(v.y, -3.0)
