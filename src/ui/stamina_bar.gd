class_name BStaminaBar
extends Control
## Stamina charges as pips; the regenerating charge fills up gradually.

var charges: float = 3.0:
	set(v):
		if not is_equal_approx(v, charges):
			charges = v
			queue_redraw()
var max_charges: int = 3
var color: Color = Color.WHITE

const PIP := Vector2(46, 12)
const GAP: float = 8.0


func _ready() -> void:
	custom_minimum_size = Vector2(PIP.x * max_charges + GAP * (max_charges - 1), PIP.y)
	mouse_filter = Control.MOUSE_FILTER_IGNORE


func _draw() -> void:
	for i in max_charges:
		var r := Rect2(Vector2(i * (PIP.x + GAP), 0), PIP)
		draw_rect(r, Color(0, 0, 0, 0.5))
		var fill: float = clampf(charges - float(i), 0.0, 1.0)
		if fill > 0.0:
			var c: Color = color if fill >= 1.0 else color.darkened(0.45)
			draw_rect(Rect2(r.position, Vector2(r.size.x * fill, r.size.y)), c)
		draw_rect(r, Color(1, 1, 1, 0.7), false, 1.5)
