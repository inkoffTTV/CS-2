class_name BCapsule
extends RefCounted
## A capsule between a and b with radius. A sphere is a capsule with a == b.

enum Zone { BODY, HEAD }

var a: Vector3 = Vector3.ZERO
var b: Vector3 = Vector3.ZERO
var radius: float = 0.0
var zone: Zone = Zone.BODY


func _init(p_a: Vector3 = Vector3.ZERO, p_b: Vector3 = Vector3.ZERO, p_radius: float = 0.0, p_zone: Zone = Zone.BODY) -> void:
	a = p_a
	b = p_b
	radius = p_radius
	zone = p_zone


func copy() -> BCapsule:
	return BCapsule.new(a, b, radius, zone)
