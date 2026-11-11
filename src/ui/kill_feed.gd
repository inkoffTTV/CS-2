class_name BKillFeed
extends VBoxContainer
## "Killer  >  Victim" lines in team colours; each fades out after a few seconds.

const LIFE: float = 6.0
const MAX_LINES: int = 5


func _ready() -> void:
	mouse_filter = Control.MOUSE_FILTER_IGNORE
	alignment = BoxContainer.ALIGNMENT_BEGIN
	add_theme_constant_override("separation", 4)


func add_kill(killer_nick: String, killer_team: int, victim_nick: String, victim_team: int, mine: bool) -> void:
	var panel := PanelContainer.new()
	panel.mouse_filter = Control.MOUSE_FILTER_IGNORE
	panel.size_flags_horizontal = Control.SIZE_SHRINK_END
	var box := StyleBoxFlat.new()
	box.bg_color = Color(0.05, 0.05, 0.08, 0.6)
	if mine:  # involves the local player: a light frame
		box.set_border_width_all(2)
		box.border_color = Color(1.0, 0.95, 0.85, 0.85)
	box.content_margin_left = 10
	box.content_margin_right = 10
	box.content_margin_top = 3
	box.content_margin_bottom = 3
	box.set_corner_radius_all(3)
	panel.add_theme_stylebox_override("panel", box)
	var label := RichTextLabel.new()
	label.bbcode_enabled = true
	label.fit_content = true
	label.autowrap_mode = TextServer.AUTOWRAP_OFF
	label.scroll_active = false
	label.mouse_filter = Control.MOUSE_FILTER_IGNORE
	label.add_theme_font_size_override("normal_font_size", 17)
	label.add_theme_constant_override("outline_size", 4)
	label.add_theme_color_override("font_outline_color", Color(0, 0, 0, 0.8))
	label.text = "[color=#%s]%s[/color]  [color=#dddddd]>[/color]  [color=#%s]%s[/color]" % [
		_html(killer_team), _esc(killer_nick), _html(victim_team), _esc(victim_nick)]
	panel.add_child(label)
	add_child(panel)
	while get_child_count() > MAX_LINES:
		var old: Node = get_child(0)
		remove_child(old)
		old.queue_free()
	var tween: Tween = panel.create_tween()
	tween.tween_interval(LIFE - 0.6)
	tween.tween_property(panel, "modulate:a", 0.0, 0.6)
	tween.tween_callback(panel.queue_free)


static func _html(team: int) -> String:
	return BTeams.team_color(team).lerp(Color.WHITE, 0.25).to_html(false)


static func _esc(s: String) -> String:
	return s.replace("[", "[lb]")
