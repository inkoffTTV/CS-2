class_name BPauseMenu
extends PanelContainer
## Esc menu inside a match: resume, leave to the main menu, quit.


func _ready() -> void:
	set_anchors_preset(Control.PRESET_CENTER)
	grow_horizontal = Control.GROW_DIRECTION_BOTH
	grow_vertical = Control.GROW_DIRECTION_BOTH
	custom_minimum_size = Vector2(320, 0)
	var box := VBoxContainer.new()
	box.add_theme_constant_override("separation", 10)
	add_child(box)
	var title := Label.new()
	title.text = "BRUTUS"
	title.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	title.add_theme_font_size_override("font_size", 28)
	box.add_child(title)
	_button(box, "Resume", _resume)
	_button(box, "Leave to menu", func() -> void: Session.back_to_menu())
	_button(box, "Quit", func() -> void: get_tree().quit())


func _button(parent: Control, text: String, action: Callable) -> void:
	var b := Button.new()
	b.text = text
	b.custom_minimum_size = Vector2(0, 40)
	b.pressed.connect(action)
	parent.add_child(b)


func _resume() -> void:
	visible = false
	Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
