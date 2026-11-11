extends RefCounted
const T := preload("res://tests/t.gd")


func _hero(x: float) -> Array[BCapsule]:
	return BHitboxes.build(Vector3(x, 0, 0), 1.8, 0.4)


func _ray() -> BRay:
	return BRay.new(Vector3(0, 1.0, 10), Vector3(0, 0, -1))  # straight down -Z at x = 0


func test_rewind_hits_where_the_target_was() -> void:
	var lag := BLagCompensation.new()
	for i in 11:
		lag.record(7, float(i) * 0.1, _hero(float(i)))  # runs +X at 10 m/s
	var past: BRewindHit = lag.trace(_ray(), 100.0, 0.0, [7])
	if T.not_null(past):
		T.eq(past.peer, 7)
		T.approx(past.point.z, 0.4, 0.01)
	T.is_null(lag.trace(_ray(), 100.0, 1.0, [7]), "now the target is 10 m away")
	T.is_null(lag.trace(_ray(), 100.0, 0.0, [8]), "not a candidate")


func test_nearest_of_two() -> void:
	var lag := BLagCompensation.new()
	lag.record(1, 0.0, BHitboxes.build(Vector3(0, 0, -5), 1.8, 0.4))
	lag.record(2, 0.0, BHitboxes.build(Vector3(0, 0, 0), 1.8, 0.4))
	var hit: BRewindHit = lag.trace(_ray(), 100.0, 0.0, [1, 2])
	if T.not_null(hit):
		T.eq(hit.peer, 2)


func test_respawn_is_a_teleport_and_old_life_misses() -> void:
	var lag := BLagCompensation.new()
	lag.record(5, 0.0, _hero(0.0))
	lag.record(5, 0.1, _hero(0.0))
	lag.mark_gap(5)  # died
	T.is_null(lag.sample(5, 0.1), "down: no pose")
	lag.record(5, 5.1, _hero(40.0))  # respawned far away
	T.is_null(lag.trace(_ray(), 100.0, 0.05, [5]), "a shot rewound into the previous life misses")
	T.is_null(lag.sample(5, 3.0), "the dead time is not interpolated either")
	var now: Variant = lag.sample(5, 5.1)
	if T.not_null(now):
		T.approx((now as Array)[0].a.x, 40.0)
	lag.record(5, 5.2, _hero(41.0))
	var mid: Variant = lag.sample(5, 5.15)
	if T.not_null(mid):
		T.approx((mid as Array)[0].a.x, 40.5, 1e-3, "interpolates within the new life")


func test_prune_and_forget() -> void:
	var lag := BLagCompensation.new()
	lag.record(3, 0.0, _hero(0.0))
	lag.record(3, 4.0, _hero(0.0))
	lag.prune(4.0)
	T.is_null(lag.sample(3, 0.5), "older than 3 s is gone")
	lag.forget(3)
	T.eq(lag.peers().size(), 0)
