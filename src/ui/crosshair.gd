class_name BCrosshair
extends Control
## Simple four-line crosshair with a centre dot.


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	resized.connect(queue_redraw)


func _draw() -> void:
	var c: Vector2 = size * 0.5
	var shadow := Color(0, 0, 0, 0.55)
	var col := Color(1, 1, 1, 0.95)
	for pass_i in 2:
		var w: float = 4.0 if pass_i == 0 else 2.0
		var colr: Color = shadow if pass_i == 0 else col
		draw_line(c + Vector2(-14, 0), c + Vector2(-5, 0), colr, w)
		draw_line(c + Vector2(5, 0), c + Vector2(14, 0), colr, w)
		draw_line(c + Vector2(0, -14), c + Vector2(0, -5), colr, w)
		draw_line(c + Vector2(0, 5), c + Vector2(0, 14), colr, w)
	draw_circle(c, 1.5, col)
