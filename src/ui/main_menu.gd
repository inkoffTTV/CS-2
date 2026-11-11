class_name BMainMenu
extends Node3D
## Main menu over a slowly orbiting view of the arena: nickname, Host, Join by IP:port, Quit.

const GAME_SCENE: String = "res://scenes/game.tscn"
const SETTINGS_PATH: String = "user://menu.cfg"

var _camera: Camera3D
var _orbit: float = 0.6
var _nick_edit: LineEdit
var _address_edit: LineEdit
var _port_edit: SpinBox
var _status: Label


func _ready() -> void:
	Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
	var arena := BArena.new()
	arena.name = "Arena"
	add_child(arena)
	_camera = Camera3D.new()
	_camera.fov = 60.0
	add_child(_camera)
	_camera.current = true
	_update_camera()
	_build_ui()
	_load_settings()
	if not Session.last_error.is_empty():
		_status.text = Session.last_error
		Session.last_error = ""


func _process(delta: float) -> void:
	_orbit += delta * 0.05
	_update_camera()


func _update_camera() -> void:
	var r: float = 44.0
	_camera.look_at_from_position(Vector3(cos(_orbit) * r, 22.0, sin(_orbit) * r), Vector3(0, 0, 0))


func _build_ui() -> void:
	var layer := CanvasLayer.new()
	add_child(layer)
	var shade := ColorRect.new()
	shade.color = Color(0.05, 0.04, 0.08, 0.55)
	shade.set_anchors_preset(Control.PRESET_LEFT_WIDE)
	shade.offset_right = 520
	layer.add_child(shade)

	var box := VBoxContainer.new()
	box.position = Vector2(64, 110)
	box.custom_minimum_size = Vector2(390, 0)
	box.add_theme_constant_override("separation", 12)
	layer.add_child(box)

	var title := Label.new()
	title.text = "BRUTUS"
	title.add_theme_font_size_override("font_size", 72)
	title.add_theme_color_override("font_color", Color(0.95, 0.80, 0.45))
	box.add_child(title)
	var sub := Label.new()
	sub.text = "Optimates vs Populares"
	sub.add_theme_font_size_override("font_size", 20)
	sub.add_theme_color_override("font_color", Color(0.85, 0.82, 0.78))
	box.add_child(sub)
	box.add_child(_spacer(18))

	box.add_child(_caption("Nickname"))
	_nick_edit = LineEdit.new()
	_nick_edit.max_length = Session.NICK_MAX
	_nick_edit.text = Session.nickname
	box.add_child(_nick_edit)

	box.add_child(_caption("Host a game (port)"))
	var host_row := HBoxContainer.new()
	_port_edit = SpinBox.new()
	_port_edit.min_value = 1024
	_port_edit.max_value = 65535
	_port_edit.value = Session.DEFAULT_PORT
	_port_edit.custom_minimum_size = Vector2(140, 0)
	host_row.add_child(_port_edit)
	var host_btn := _button("Host", _on_host)
	host_btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	host_row.add_child(host_btn)
	box.add_child(host_row)

	box.add_child(_caption("Join by IP:port"))
	var join_row := HBoxContainer.new()
	_address_edit = LineEdit.new()
	_address_edit.text = "127.0.0.1:%d" % Session.DEFAULT_PORT
	_address_edit.size_flags_horizontal = Control.SIZE_EXPAND_FILL
	_address_edit.text_submitted.connect(func(_t: String) -> void: _on_join())
	join_row.add_child(_address_edit)
	join_row.add_child(_button("Join", _on_join))
	box.add_child(join_row)

	box.add_child(_spacer(8))
	box.add_child(_button("Quit", func() -> void: get_tree().quit()))
	_status = Label.new()
	_status.add_theme_color_override("font_color", Color(1.0, 0.55, 0.45))
	_status.autowrap_mode = TextServer.AUTOWRAP_WORD
	box.add_child(_status)

	var help := Label.new()
	help.text = "WASD move  ·  Mouse look  ·  Space jump / double jump  ·  Shift dash  ·  Ctrl/C slide  ·  E zipline  ·  Esc menu"
	help.set_anchors_preset(Control.PRESET_BOTTOM_WIDE)
	help.offset_top = -40
	help.offset_left = 24
	help.add_theme_color_override("font_color", Color(0.95, 0.93, 0.9))
	help.add_theme_constant_override("outline_size", 6)
	help.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.85))
	layer.add_child(help)


func _caption(text: String) -> Label:
	var l := Label.new()
	l.text = text
	l.add_theme_color_override("font_color", Color(0.75, 0.72, 0.7))
	return l


func _spacer(h: float) -> Control:
	var c := Control.new()
	c.custom_minimum_size = Vector2(0, h)
	return c


func _button(text: String, action: Callable) -> Button:
	var b := Button.new()
	b.text = text
	b.custom_minimum_size = Vector2(110, 40)
	b.pressed.connect(action)
	return b


func _on_host() -> void:
	Session.nickname = Session.sanitize_nick(_nick_edit.text)
	Session.mode = Session.Mode.HOST
	Session.dedicated = false
	Session.port = int(_port_edit.value)
	_start()


func _on_join() -> void:
	Session.nickname = Session.sanitize_nick(_nick_edit.text)
	var ep: Array = Session.parse_endpoint(_address_edit.text)
	Session.mode = Session.Mode.JOIN
	Session.dedicated = false
	Session.address = ep[0]
	Session.port = ep[1]
	_start()


func _start() -> void:
	_save_settings()
	get_tree().change_scene_to_file(GAME_SCENE)


func _load_settings() -> void:
	var cfg := ConfigFile.new()
	if cfg.load(SETTINGS_PATH) != OK:
		return
	_nick_edit.text = str(cfg.get_value("menu", "nickname", _nick_edit.text))
	_address_edit.text = str(cfg.get_value("menu", "address", _address_edit.text))
	_port_edit.value = float(cfg.get_value("menu", "port", _port_edit.value))


func _save_settings() -> void:
	var cfg := ConfigFile.new()
	cfg.set_value("menu", "nickname", _nick_edit.text)
	cfg.set_value("menu", "address", _address_edit.text)
	cfg.set_value("menu", "port", int(_port_edit.value))
	cfg.save(SETTINGS_PATH)
