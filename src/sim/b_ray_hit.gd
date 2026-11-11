class_name BRayHit
extends RefCounted

var distance: float = 0.0
var zone: BCapsule.Zone = BCapsule.Zone.BODY
var shape_index: int = -1


func _init(p_distance: float = 0.0, p_zone: BCapsule.Zone = BCapsule.Zone.BODY, p_shape_index: int = -1) -> void:
	distance = p_distance
	zone = p_zone
	shape_index = p_shape_index
