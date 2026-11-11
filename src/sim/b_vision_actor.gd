class_name BVisionActor
extends RefCounted

var id: int = 0
var team: int = 0
var pos: Vector3 = Vector3.ZERO
var sight_radius: float = 0.0
var observer: bool = false  # heroes and structures give vision
var hidden: bool = false  # invisible (Nemo): revealed only up close


func _init(p_id: int = 0, p_team: int = 0, p_pos: Vector3 = Vector3.ZERO, p_sight_radius: float = 0.0, p_observer: bool = false, p_hidden: bool = false) -> void:
	id = p_id
	team = p_team
	pos = p_pos
	sight_radius = p_sight_radius
	observer = p_observer
	hidden = p_hidden
