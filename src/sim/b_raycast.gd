class_name BRaycast
extends RefCounted

const NO_HIT: float = -1.0


static func _dist_to_segment_sq(p: Vector3, a: Vector3, b: Vector3) -> float:
	var ab: Vector3 = b - a
	var len_sq: float = ab.length_squared()
	var t: float = clampf((p - a).dot(ab) / len_sq, 0.0, 1.0) if len_sq > 1e-10 else 0.0
	return (p - (a + ab * t)).length_squared()


static func _ray_sphere(r: BRay, center: Vector3, radius: float) -> float:
	var oc: Vector3 = r.origin - center
	var b: float = oc.dot(r.dir)
	var c: float = oc.length_squared() - radius * radius
	var h: float = b * b - c
	if h < 0.0:
		return NO_HIT
	var t: float = -b - sqrt(h)
	return t if t >= 0.0 else NO_HIT


## Side of the finite cylinder between a and b (caps are handled by the end spheres).
static func _ray_cylinder_side(r: BRay, a: Vector3, b: Vector3, radius: float) -> float:
	var ba: Vector3 = b - a
	var oa: Vector3 = r.origin - a
	var ba_ba: float = ba.dot(ba)
	var ba_rd: float = ba.dot(r.dir)
	var ba_oa: float = ba.dot(oa)
	var qa: float = ba_ba - ba_rd * ba_rd
	if ba_ba < 1e-10 or qa < 1e-10:
		return NO_HIT  # sphere or ray parallel to the axis
	var qb: float = ba_ba * r.dir.dot(oa) - ba_oa * ba_rd
	var qc: float = ba_ba * oa.dot(oa) - ba_oa * ba_oa - radius * radius * ba_ba
	var h: float = qb * qb - qa * qc
	if h < 0.0:
		return NO_HIT
	var t: float = (-qb - sqrt(h)) / qa
	var y: float = ba_oa + t * ba_rd
	return t if (t >= 0.0 and y > 0.0 and y < ba_ba) else NO_HIT


static func _ray_capsule(r: BRay, c: BCapsule) -> float:
	if _dist_to_segment_sq(r.origin, c.a, c.b) <= c.radius * c.radius:
		return 0.0
	var best: float = NO_HIT
	var candidates: Array[float] = [
		_ray_cylinder_side(r, c.a, c.b, c.radius),
		_ray_sphere(r, c.a, c.radius),
		_ray_sphere(r, c.b, c.radius),
	]
	for t in candidates:
		if t >= 0.0 and (best < 0.0 or t < best):
			best = t
	return best


## Nearest hit (BRayHit) against the capsules within max_distance, or null.
## A ray starting inside a capsule hits at 0.
static func raycast_capsules(r: BRay, shapes: Array[BCapsule], max_distance: float) -> BRayHit:
	var result: BRayHit = null
	for i in shapes.size():
		var t: float = _ray_capsule(r, shapes[i])
		if t >= 0.0 and t <= max_distance and (result == null or t < result.distance):
			result = BRayHit.new(t, shapes[i].zone, i)
	return result


## Server time to rewind a shot to: server_now minus the shooter's latency, capped at max_rewind.
## Client times in the future are treated as "now"; non-finite times too.
static func clamp_rewind_time(server_now: float, client_shot_time: float, max_rewind: float = 0.25) -> float:
	if not is_finite(client_shot_time):
		return server_now
	return server_now - clampf(server_now - client_shot_time, 0.0, max_rewind)
