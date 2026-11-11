extends RefCounted
const T := preload("res://tests/t.gd")


func _target() -> Array[BCapsule]:
	var a: Array[BCapsule] = [
		BCapsule.new(Vector3(0, 0, 0), Vector3(0, 1, 0), 0.3, BCapsule.Zone.BODY),
		BCapsule.new(Vector3(0, 1.5, 0), Vector3(0, 1.5, 0), 0.15, BCapsule.Zone.HEAD),
	]
	return a


func test_sphere_touching_side() -> void:
	T.is_true(BOverlap.sphere_overlaps_capsules(Vector3(1, 0.5, 0), 0.8, _target()))


func test_sphere_clear() -> void:
	T.is_false(BOverlap.sphere_overlaps_capsules(Vector3(2, 0.5, 0), 0.8, _target()))


func test_sphere_near_end_cap() -> void:
	T.is_true(BOverlap.sphere_overlaps_capsules(Vector3(0, -0.2, 0), 0.1, _target()))
	T.is_false(BOverlap.sphere_overlaps_capsules(Vector3(0, -0.2, -1), 0.1, _target()))


func test_sphere_only_head() -> void:
	T.is_true(BOverlap.sphere_overlaps_capsules(Vector3(0, 1.9, 0), 0.3, _target()))
	T.is_false(BOverlap.sphere_overlaps_capsules(Vector3(0, 2.0, 0), 0.3, _target()))


func test_empty_never_overlaps() -> void:
	var none: Array[BCapsule] = []
	T.is_false(BOverlap.sphere_overlaps_capsules(Vector3.ZERO, 10.0, none))
