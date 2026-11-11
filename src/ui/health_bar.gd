class_name BHealthBar
extends Control
## Health bar with the number on it; flashes when health drops.

const SIZE := Vector2(300, 22)

var max_health: float = 250.0
var health: float = 250.0:
	set(v):
		if v < health:
			_flash = 1.0
		if v != health:
			health = v
			queue_redraw()
var color: Color = Color(0.35, 0.9, 0.45)

var _flash: float = 0.0
var _font: Font


func _ready() -> void:
	custom_minimum_size = SIZE
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	_font = get_theme_default_font()


func _process(delta: float) -> void:
	if _flash > 0.0:
		_flash = maxf(0.0, _flash - delta * 4.0)
		queue_redraw()


func _draw() -> void:
	var r := Rect2(Vector2.ZERO, SIZE)
	draw_rect(r, Color(0, 0, 0, 0.55))
	var frac: float = clampf(health / maxf(1.0, max_health), 0.0, 1.0)
	var c: Color = color if frac > 0.3 else Color(0.95, 0.25, 0.2)
	c = c.lerp(Color.WHITE, _flash * 0.6)
	draw_rect(Rect2(r.position, Vector2(r.size.x * frac, r.size.y)), c)
	draw_rect(r, Color(1, 1, 1, 0.75), false, 1.5)
	var text: String = "%d" % ceili(health)
	var fs: int = 17
	var tw: float = _font.get_string_size(text, HORIZONTAL_ALIGNMENT_LEFT, -1, fs).x
	var pos := Vector2((SIZE.x - tw) * 0.5, SIZE.y * 0.5 + fs * 0.36)
	draw_string_outline(_font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, 4, Color(0, 0, 0, 0.8))
	draw_string(_font, pos, text, HORIZONTAL_ALIGNMENT_LEFT, -1, fs, Color.WHITE)
