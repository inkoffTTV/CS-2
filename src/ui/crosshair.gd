class_name BCrosshair
extends Control
## Simple four-line crosshair with a centre dot, and a hit marker (an X; red for a headshot,
## bigger on a kill) that fades out after a confirmed hit.

const MARKER_LIFE: float = 0.22

var _marker: float = 0.0
var _marker_head: bool = false
var _marker_kill: bool = false


func _ready() -> void:
	set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	resized.connect(queue_redraw)


func show_hit(head: bool, killed: bool) -> void:
	_marker = MARKER_LIFE * (1.6 if killed else 1.0)
	_marker_head = head
	_marker_kill = killed
	queue_redraw()


func _process(delta: float) -> void:
	if _marker > 0.0:
		_marker = maxf(0.0, _marker - delta)
		queue_redraw()


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
	if _marker > 0.0:
		_draw_marker(c)


func _draw_marker(c: Vector2) -> void:
	var a: float = clampf(_marker / MARKER_LIFE, 0.0, 1.0)
	var mc: Color = Color(1.0, 0.18, 0.15, a) if _marker_head else Color(1, 1, 1, a)
	var inner: float = 7.0
	var outer: float = 17.0 if _marker_kill else 13.0
	for d: Vector2 in [Vector2(1, 1), Vector2(-1, 1), Vector2(1, -1), Vector2(-1, -1)]:
		var n: Vector2 = d.normalized()
		draw_line(c + n * inner, c + n * outer, Color(0, 0, 0, 0.6 * a), 5.0)
		draw_line(c + n * inner, c + n * outer, mc, 3.0)
