class_name BDamageNumber
extends Label3D
## Floating damage number above the impact, seen by the shooter only. Hits in quick succession add
## up on one number instead of stacking new ones.

const LIFE: float = 0.8
const MERGE_WINDOW: float = 0.45

static var _last: WeakRef = null

var _age: float = 0.0
var _total: float = 0.0
var _head: bool = false


static func spawn(parent: Node, point: Vector3, damage: float, head: bool) -> void:
	if parent == null or DisplayServer.get_name() == "headless":
		return
	var last: BDamageNumber = _last.get_ref() as BDamageNumber if _last != null else null
	if last != null and last._age < MERGE_WINDOW:
		last._add(point, damage, head)
		return
	var n := BDamageNumber.new()
	n.outline_modulate = Color(0, 0, 0, 0.85)
	n.outline_size = 10
	n.font_size = 40
	n.billboard = BaseMaterial3D.BILLBOARD_ENABLED
	n.no_depth_test = true
	n.fixed_size = true
	n.pixel_size = 0.0009
	parent.add_child(n)
	n._add(point, damage, head)
	_last = weakref(n)


func _add(point: Vector3, damage: float, head: bool) -> void:
	_total += damage
	_head = _head or head
	_age = 0.0
	text = "%d" % roundi(_total)
	modulate = Color(1.0, 0.3, 0.25) if _head else Color(1.0, 0.95, 0.8)
	position = point + Vector3(0.0, 0.7, 0.0)


func _process(delta: float) -> void:
	_age += delta
	if _age >= LIFE:
		queue_free()
		return
	position.y += delta * 0.6
	modulate.a = 1.0 - maxf(0.0, _age - LIFE * 0.5) / (LIFE * 0.5)
	outline_modulate.a = modulate.a
