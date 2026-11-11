class_name BZipline
extends RefCounted


static func project_t(seg: BZiplineSegment, pos: Vector3) -> float:
	var ab: Vector3 = seg.b - seg.a
	var len_sq: float = ab.length_squared()
	if len_sq < 1e-10:
		return 0.0
	return clampf((pos - seg.a).dot(ab) / len_sq, 0.0, 1.0)


static func point_at(seg: BZiplineSegment, t: float) -> Vector3:
	return seg.a.lerp(seg.b, t)


static func distance_to(seg: BZiplineSegment, pos: Vector3) -> float:
	return (pos - point_at(seg, project_t(seg, pos))).length()


## Moves t along the cable by speed * dt in direction (+1 toward b, -1 toward a), clamped to [0, 1].
## A zero-length cable is finished immediately (returns the end in direction).
static func advance_t(seg: BZiplineSegment, t: float, direction: int, speed: float, dt: float) -> float:
	var length: float = (seg.b - seg.a).length()
	var sign_dir: float = 1.0 if direction >= 0 else -1.0
	if length < 1e-5:
		return 1.0 if sign_dir > 0.0 else 0.0
	return clampf(t + sign_dir * speed * dt / length, 0.0, 1.0)


## +1 when the horizontal facing points toward b, otherwise -1.
static func choose_direction(seg: BZiplineSegment, facing: Vector3) -> int:
	return 1 if BMath.flatten(facing).dot(BMath.flatten(seg.b - seg.a)) >= 0.0 else -1
