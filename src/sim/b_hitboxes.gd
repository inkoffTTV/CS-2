class_name BHitboxes
extends RefCounted
## Hitbox capsules of a hero built from its collision capsule (feet at the origin, Y up):
## a body capsule from the feet to the shoulders and a head sphere at the top.

const HEAD_RADIUS: float = 0.22


## [body, head] for a standing collision capsule of the given height and radius at feet.
static func build(feet: Vector3, height: float, radius: float, head_radius: float = HEAD_RADIUS) -> Array[BCapsule]:
	var head_r: float = minf(head_radius, height * 0.5)
	var head_c: Vector3 = feet + Vector3(0.0, height - head_r, 0.0)
	var a: Vector3 = feet + Vector3(0.0, radius, 0.0)
	# The body ends where the head starts (top of the body capsule = bottom of the head sphere).
	var top_center: float = maxf(radius, height - 2.0 * head_r - radius)
	var b: Vector3 = feet + Vector3(0.0, top_center, 0.0)
	var out: Array[BCapsule] = [
		BCapsule.new(a, b, radius, BCapsule.Zone.BODY),
		BCapsule.new(head_c, head_c, head_r, BCapsule.Zone.HEAD),
	]
	return out
