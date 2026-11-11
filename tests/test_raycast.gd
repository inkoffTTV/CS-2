extends RefCounted
const T := preload("res://tests/t.gd")


func _target() -> Array[BCapsule]:
	var a: Array[BCapsule] = [
		BCapsule.new(Vector3(0, 0, 0), Vector3(0, 1, 0), 0.3, BCapsule.Zone.BODY),
		BCapsule.new(Vector3(0, 1.5, 0), Vector3(0, 1.5, 0), 0.15, BCapsule.Zone.HEAD),
	]
	return a


func _ray(o: Vector3, d: Vector3) -> BRay:
	return BRay.new(o, d)


func test_hits_body_side() -> void:
	var hit := BRaycast.raycast_capsules(_ray(Vector3(-5, 0.5, 0), Vector3(1, 0, 0)), _target(), 100.0)
	if T.not_null(hit):
		T.eq(hit.zone, BCapsule.Zone.BODY)
		T.eq(hit.shape_index, 0)
		T.approx(hit.distance, 4.7)


func test_hits_head_sphere() -> void:
	var hit := BRaycast.raycast_capsules(_ray(Vector3(-5, 1.5, 0), Vector3(1, 0, 0)), _target(), 100.0)
	if T.not_null(hit):
		T.eq(hit.zone, BCapsule.Zone.HEAD)
		T.approx(hit.distance, 4.85)


func test_hits_rounded_end_cap() -> void:
	var hit := BRaycast.raycast_capsules(_ray(Vector3(-5, 1.2, 0), Vector3(1, 0, 0)), _target(), 100.0)
	if T.not_null(hit):
		T.eq(hit.zone, BCapsule.Zone.BODY)
		T.approx(hit.distance, 5.0 - 0.223607, 1e-3)


func test_ray_above_misses() -> void:
	T.is_null(BRaycast.raycast_capsules(_ray(Vector3(-5, 2.0, 0), Vector3(1, 0, 0)), _target(), 100.0))


func test_max_distance_limits() -> void:
	T.is_null(BRaycast.raycast_capsules(_ray(Vector3(-5, 0.5, 0), Vector3(1, 0, 0)), _target(), 4.0))


func test_target_behind_not_hit() -> void:
	T.is_null(BRaycast.raycast_capsules(_ray(Vector3(-5, 0.5, 0), Vector3(-1, 0, 0)), _target(), 100.0))


func test_ray_inside_hits_at_zero() -> void:
	var hit := BRaycast.raycast_capsules(_ray(Vector3(0, 0.5, 0), Vector3(1, 0, 0)), _target(), 100.0)
	if T.not_null(hit):
		T.approx(hit.distance, 0.0)


func test_rewind_time_clamped() -> void:
	T.approx(BRaycast.clamp_rewind_time(10.0, 9.9), 9.9)
	T.approx(BRaycast.clamp_rewind_time(10.0, 9.5), 9.75)
	T.approx(BRaycast.clamp_rewind_time(10.0, 10.2), 10.0)


func test_non_finite_shot_time_is_now() -> void:
	T.approx(BRaycast.clamp_rewind_time(10.0, NAN), 10.0)
	T.approx(BRaycast.clamp_rewind_time(10.0, -INF), 10.0)
	T.approx(BRaycast.clamp_rewind_time(10.0, INF), 10.0)
