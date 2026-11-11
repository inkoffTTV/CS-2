class_name BSpread
extends RefCounted
## Shotgun pellet directions, deterministic per seed (own integer PRNG, identical on every platform).

const MASK: int = 0xFFFFFFFF


## SplitMix-style mixer used to seed the xorshift state.
static func _seed_state(seed_value: int) -> int:
	var z: int = ((seed_value & MASK) + 0x9E3779B9) & MASK
	z = (((z ^ (z >> 16)) & MASK) * 0x85EBCA6B) & MASK
	z = (((z ^ (z >> 13)) & MASK) * 0xC2B2AE35) & MASK
	z = (z ^ (z >> 16)) & MASK
	return z if z != 0 else 1


## Returns [next_state, float in [0, 1) with 24 bits of precision].
static func _next(state: int) -> PackedFloat64Array:
	var s: int = state
	s = (s ^ (s << 13)) & MASK
	s = (s ^ (s >> 17)) & MASK
	s = (s ^ (s << 5)) & MASK
	return PackedFloat64Array([float(s), float(s >> 8) / 16777216.0])


## Count unit vectors uniformly distributed (by solid angle) inside a cone of spread_degrees around
## forward. spread <= 0 returns forward unchanged for every pellet.
static func pellet_directions(forward: Vector3, spread_degrees: float, count: int, seed_value: int) -> Array[Vector3]:
	var result: Array[Vector3] = []
	if count <= 0:
		return result
	if spread_degrees <= 0.0:
		for i in count:
			result.append(forward)
		return result

	var f: Vector3 = BMath.safe_normalized(forward)
	# Orthonormal basis around f; pick the world axis least aligned with f so f may be straight up.
	var axis: Vector3 = Vector3(0, 1, 0) if absf(f.y) < 0.9 else Vector3(1, 0, 0)
	var u: Vector3 = BMath.safe_normalized(axis.cross(f))
	var v: Vector3 = f.cross(u)

	var cos_max: float = cos(deg_to_rad(spread_degrees))
	var state: int = _seed_state(seed_value)
	for i in count:
		var r1: PackedFloat64Array = _next(state)
		state = int(r1[0])
		var r2: PackedFloat64Array = _next(state)
		state = int(r2[0])
		var cos_t: float = 1.0 - r1[1] * (1.0 - cos_max)
		var sin_t: float = sqrt(maxf(0.0, 1.0 - cos_t * cos_t))
		var phi: float = r2[1] * TAU
		result.append(BMath.safe_normalized(f * cos_t + (u * cos(phi) + v * sin(phi)) * sin_t))
	return result
