class_name BHud
extends CanvasLayer
## In-game HUD: crosshair, stamina pips, ping, team name in team colour, movement readout,
## player list, connection status and the Esc menu.

var _player: BPlayer
var _crosshair: BCrosshair
var _stamina: BStaminaBar
var _team_label: Label
var _nick_label: Label
var _ping_label: Label
var _move_label: Label
var _status_label: Label
var _list_label: RichTextLabel
var _menu: BPauseMenu
var _list_timer: float = 0.0


func _ready() -> void:
	layer = 10
	var root := Control.new()
	root.name = "Root"
	root.set_anchors_and_offsets_preset(Control.PRESET_FULL_RECT)
	root.mouse_filter = Control.MOUSE_FILTER_IGNORE
	add_child(root)

	_crosshair = BCrosshair.new()
	_crosshair.visible = false
	root.add_child(_crosshair)

	var top_left := VBoxContainer.new()
	top_left.position = Vector2(24, 18)
	top_left.add_theme_constant_override("separation", 2)
	root.add_child(top_left)
	_team_label = _label(top_left, 30)
	_nick_label = _label(top_left, 18)
	_ping_label = _label(top_left, 16)
	_ping_label.modulate = Color(0.85, 0.85, 0.85)

	var bottom := VBoxContainer.new()
	bottom.set_anchors_preset(Control.PRESET_CENTER_BOTTOM)
	bottom.grow_horizontal = Control.GROW_DIRECTION_BOTH
	bottom.grow_vertical = Control.GROW_DIRECTION_BEGIN
	bottom.offset_bottom = -36
	bottom.alignment = BoxContainer.ALIGNMENT_END
	bottom.add_theme_constant_override("separation", 6)
	root.add_child(bottom)
	_move_label = _label(bottom, 16)
	_move_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_stamina = BStaminaBar.new()
	_stamina.size_flags_horizontal = Control.SIZE_SHRINK_CENTER
	_stamina.visible = false
	bottom.add_child(_stamina)

	_list_label = RichTextLabel.new()
	_list_label.bbcode_enabled = true
	_list_label.fit_content = true
	_list_label.scroll_active = false
	_list_label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	_list_label.set_anchors_preset(Control.PRESET_TOP_RIGHT)
	_list_label.grow_horizontal = Control.GROW_DIRECTION_BEGIN
	_list_label.offset_left = -260
	_list_label.offset_right = -20
	_list_label.offset_top = 18
	_list_label.add_theme_font_size_override("normal_font_size", 16)
	_list_label.add_theme_constant_override("outline_size", 4)
	_list_label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
	root.add_child(_list_label)

	_status_label = Label.new()
	_status_label.set_anchors_preset(Control.PRESET_CENTER)
	_status_label.grow_horizontal = Control.GROW_DIRECTION_BOTH
	_status_label.grow_vertical = Control.GROW_DIRECTION_BOTH
	_status_label.horizontal_alignment = HORIZONTAL_ALIGNMENT_CENTER
	_status_label.add_theme_font_size_override("font_size", 26)
	_status_label.add_theme_constant_override("outline_size", 6)
	_status_label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
	root.add_child(_status_label)

	_menu = BPauseMenu.new()
	_menu.visible = false
	root.add_child(_menu)


func _label(parent: Control, font_size: int) -> Label:
	var l := Label.new()
	l.add_theme_font_size_override("font_size", font_size)
	l.add_theme_constant_override("outline_size", 5)
	l.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.75))
	parent.add_child(l)
	return l


func bind(player: BPlayer) -> void:
	_player = player
	_crosshair.visible = true
	_stamina.visible = true
	var c: Color = BTeams.team_color(player.team)
	_team_label.text = BTeams.team_name(player.team).to_upper()
	_team_label.add_theme_color_override("font_color", c.lerp(Color.WHITE, 0.15))
	_nick_label.text = player.nick
	_stamina.color = c.lerp(Color.WHITE, 0.35)
	_stamina.max_charges = BPlayer.stamina_cfg.max_charges


func set_status(text: String) -> void:
	_status_label.text = text


func _process(delta: float) -> void:
	_list_timer -= delta
	if _list_timer <= 0.0:
		_list_timer = 0.5
		_refresh_list()
	if _player == null or not is_instance_valid(_player):
		return
	_stamina.charges = _player.stamina
	var mp: MultiplayerAPI = _player.multiplayer
	if mp.is_server():
		_ping_label.text = "Host" if not Session.dedicated else ""
	else:
		_ping_label.text = "Ping %d ms" % roundi(NetworkTimeSynchronizer.rtt * 1000.0)
	var speed: float = BMath.flatten(_player.velocity).length()
	var state: String = ""
	match _player.mode:
		BPlayer.Mode.SLIDE:
			state = "SLIDE"
		BPlayer.Mode.ZIPLINE:
			state = "ZIPLINE"
	if _player.is_dashing():
		state = "DASH"
	_move_label.text = "%.1f m/s  %s" % [speed, state]


func _refresh_list() -> void:
	var players: Node = get_parent().get_node_or_null("Players")
	if players == null:
		return
	var lines: PackedStringArray = []
	for team in 2:
		for node in players.get_children():
			var p := node as BPlayer
			if p != null and p.team == team:
				lines.append("[right][color=#%s]%s[/color][/right]" % [BTeams.team_color(team).to_html(false), p.nick])
	_list_label.text = "\n".join(lines)


func _unhandled_input(event: InputEvent) -> void:
	if event.is_action_pressed("menu"):
		_menu.visible = not _menu.visible
		if _menu.visible:
			Input.mouse_mode = Input.MOUSE_MODE_VISIBLE
		elif _player != null:
			Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
		get_viewport().set_input_as_handled()
	elif event is InputEventMouseButton and (event as InputEventMouseButton).pressed and not _menu.visible \
			and _player != null and Input.mouse_mode != Input.MOUSE_MODE_CAPTURED:
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
