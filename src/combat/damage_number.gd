class_name BDamageNumber
extends Label3D
## Floating damage number at the impact point, seen by the shooter only.

const LIFE: float = 0.7

var _age: float = 0.0


static func spawn(parent: Node, point: Vector3, damage: float, head: bool) -> void:
	if parent == null or DisplayServer.get_name() == "headless":
		return
	var n := BDamageNumber.new()
	n.text = "%d" % roundi(damage)
	n.modulate = Color(1.0, 0.25, 0.2) if head else Color(1.0, 0.95, 0.8)
	n.outline_modulate = Color(0, 0, 0, 0.85)
	n.outline_size = 10
	n.font_size = 56 if head else 44
	n.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	n.no_depth_test = true
	n.fixed_size = true
	n.pixel_size = 0.0009
	n.position = point + Vector3(randf_range(-0.15, 0.15), 0.3, randf_range(-0.15, 0.15))
	parent.add_child(n)


func _process(delta: float) -> void:
	_age += delta
	if _age >= LIFE:
		queue_free()
		return
	position.y += delta * 0.8
	modulate.a = 1.0 - maxf(0.0, _age - LIFE * 0.5) / (LIFE * 0.5)
	outline_modulate.a = modulate.a
