class_name BZiplineSegment
extends RefCounted
## A straight zipline cable from a to b in world space. t = 0 at a, t = 1 at b.

var a: Vector3 = Vector3.ZERO
var b: Vector3 = Vector3.ZERO


func _init(p_a: Vector3 = Vector3.ZERO, p_b: Vector3 = Vector3.ZERO) -> void:
	a = p_a
	b = p_b
