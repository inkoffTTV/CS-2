extends RefCounted
const T := preload("res://tests/t.gd")


func test_reaches_target_after_flight_time() -> void:
	var from := Vector3(1, 0.2, 0.5)
	var to := Vector3(6, 1.5, -3)
	var g := -9.8
	var t := 0.45
	var v := BBallistics.launch_velocity_to_reach(from, to, g, t)
	var p := from + v * t + Vector3(0, 0.5 * g * t * t, 0)
	T.is_true(absf(p.x - to.x) < 0.01)
	T.is_true(absf(p.y - to.y) < 0.01)
	T.is_true(absf(p.z - to.z) < 0.01)


func test_zero_gravity_straight_line() -> void:
	T.vec(BBallistics.launch_velocity_to_reach(Vector3.ZERO, Vector3(1, 0, 0), 0.0, 0.5), Vector3(2, 0, 0))


func test_non_positive_flight_time_is_zero() -> void:
	T.eq(BBallistics.launch_velocity_to_reach(Vector3.ZERO, Vector3(1, 0, 0), -9.8, 0.0), Vector3.ZERO)
