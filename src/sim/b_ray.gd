class_name BRay
extends RefCounted

var origin: Vector3 = Vector3.ZERO
var dir: Vector3 = Vector3.FORWARD  # normalized


func _init(p_origin: Vector3 = Vector3.ZERO, p_dir: Vector3 = Vector3.FORWARD) -> void:
	origin = p_origin
	dir = p_dir
