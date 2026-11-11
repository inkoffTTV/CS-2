extends RefCounted
const T := preload("res://tests/t.gd")
# Old (x, y, z) cm Z-up -> new (x, z, -y) m Y-up.


func test_dash_horizontal_along_wish() -> void:
	var cfg := BMoveConfig.new()
	T.vec(BMoveRules.compute_dash_velocity(Vector3(1, 0.5, 0), Vector3(0, 0, -1), cfg), Vector3(15, 0, 0), 1e-3)


func test_dash_no_input_uses_facing() -> void:
	var cfg := BMoveConfig.new()
	T.vec(BMoveRules.compute_dash_velocity(Vector3.ZERO, Vector3(0, 0, -1), cfg), Vector3(0, 0, -15), 1e-3)


func test_air_jumps_limited_and_reset() -> void:
	var cfg := BMoveConfig.new()
	var s := BAirJumpState.new()
	T.is_true(BMoveRules.try_air_jump(s, cfg))
	T.is_false(BMoveRules.try_air_jump(s, cfg))
	BMoveRules.reset_air_jumps(s)
	T.is_true(BMoveRules.try_air_jump(s, cfg))


func test_slide_needs_speed_and_ground() -> void:
	var cfg := BMoveConfig.new()
	T.is_false(BMoveRules.can_start_slide(4.99, true, cfg))
	T.is_false(BMoveRules.can_start_slide(6.0, false, cfg))
	T.is_true(BMoveRules.can_start_slide(6.0, true, cfg))


func test_slide_start_boost_and_clamp() -> void:
	var cfg := BMoveConfig.new()
	T.vec(BMoveRules.start_slide_velocity(Vector3(10, 0, 0), cfg), Vector3(12.5, 0, 0), 1e-3)
	T.vec(BMoveRules.start_slide_velocity(Vector3(17, 0, 0), cfg), Vector3(18, 0, 0), 1e-3)


func test_slide_decelerates_on_flat() -> void:
	var cfg := BMoveConfig.new()
	T.vec(BMoveRules.step_slide(Vector3(10, 0, 0), Vector3(0, 1, 0), 0.5, cfg), Vector3(8, 0, 0), 1e-3)


func test_slide_accelerates_downhill_on_slope_plane() -> void:
	var cfg := BMoveConfig.new()
	var n := Vector3(0.5, sqrt(3.0) / 2.0, 0.0)  # 30 degrees, downhill toward +X
	var v := BMoveRules.step_slide(Vector3(10, 0, 0), n, 1.0, cfg)
	T.approx(v.length(), 10.9, 1e-3)
	T.is_true(absf(v.dot(n)) < 1e-4)


func test_slide_friction_never_reverses() -> void:
	var cfg := BMoveConfig.new()
	T.vec(BMoveRules.step_slide(Vector3(1, 0, 0), Vector3(0, 1, 0), 1.0, cfg), Vector3.ZERO, 1e-3)


func test_slide_exits_below_exit_speed() -> void:
	var cfg := BMoveConfig.new()
	T.is_true(BMoveRules.should_exit_slide(Vector3(2.99, 0, 0), cfg))
	T.is_false(BMoveRules.should_exit_slide(Vector3(3.01, 0, 0), cfg))


func test_defaults_in_metres() -> void:
	var cfg := BMoveConfig.new()
	T.approx(cfg.dash_speed, 15.0)
	T.approx(cfg.gravity, 9.8)
	T.approx(cfg.slide_max_speed, 18.0)
