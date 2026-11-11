class_name BOverlap
extends RefCounted


static func _dist_sq_to_segment(p: Vector3, a: Vector3, b: Vector3) -> float:
	var ab: Vector3 = b - a
	var len_sq: float = ab.length_squared()
	var t: float = (p - a).dot(ab) / len_sq if len_sq > 1e-16 else 0.0
	t = clampf(t, 0.0, 1.0)
	return (p - (a + ab * t)).length_squared()


## True if the sphere (center, radius) touches any of the capsules.
static func sphere_overlaps_capsules(center: Vector3, radius: float, shapes: Array[BCapsule]) -> bool:
	for c in shapes:
		var reach: float = radius + c.radius
		if _dist_sq_to_segment(center, c.a, c.b) <= reach * reach:
			return true
	return false
