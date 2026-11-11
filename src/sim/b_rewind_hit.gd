class_name BRewindHit
extends RefCounted
## A rewound trace hit: which hero (peer id), the capsule hit and the impact point.

var peer: int = 0
var hit: BRayHit
var point: Vector3 = Vector3.ZERO


func _init(p_peer: int = 0, p_hit: BRayHit = null, p_point: Vector3 = Vector3.ZERO) -> void:
	peer = p_peer
	hit = p_hit
	point = p_point
