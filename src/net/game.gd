class_name BGame
extends Node3D
## The match scene: arena, players, spawner and HUD. Starts networking from Session on load.

const POS_LOG_INTERVAL: float = 0.5

@onready var spawner: BPlayerSpawner = $PlayerSpawner as BPlayerSpawner
@onready var hud: BHud = $HUD as BHud
@onready var arena: BArena = $Arena as BArena
@onready var combat: BCombat = $Combat as BCombat

var local_player: BPlayer
var _camera: BPlayerCamera
var _spectator: Camera3D
var _first_pos: Dictionary[int, Vector3] = {}
var _max_moved: Dictionary[int, float] = {}


func _ready() -> void:
	add_child(BRollbackCatchUp.new())
	hud.bind_combat(combat)
	spawner.local_player_spawned.connect(_on_local_player)
	multiplayer.server_disconnected.connect(_on_server_lost)
	multiplayer.connection_failed.connect(_on_connection_failed)
	if DisplayServer.get_name() == "headless":
		Engine.max_fps = 60  # headless processes would otherwise spin a core
	if Session.quit_after > 0.0:
		get_tree().create_timer(Session.quit_after).timeout.connect(_quit_game)
	if not Session.screenshot_path.is_empty():
		var director := BScreenshotDirector.new()
		director.name = "ScreenshotDirector"
		add_child(director)
	_spectator = Camera3D.new()
	_spectator.name = "SpectatorCamera"
	_spectator.fov = 60.0
	add_child(_spectator)
	_spectator.look_at_from_position(Vector3(-40, 26, 36), Vector3(0, 0, 0))
	_spectator.current = true
	hud.set_status("Connecting to %s:%d..." % [Session.address, Session.port] if Session.mode == Session.Mode.JOIN
		else "Starting server...")
	if Session.start_network(multiplayer) != OK:
		_fail(Session.last_error)
		return
	if multiplayer.is_server():
		var t := Timer.new()
		t.wait_time = POS_LOG_INTERVAL
		t.autostart = true
		t.timeout.connect(_log_positions)
		add_child(t)
	if Session.dedicated:
		hud.set_status("Dedicated server on port %d" % Session.port)


func _on_local_player(player: BPlayer) -> void:
	local_player = player
	BLog.info(multiplayer, "local player spawned: %s (%s)" % [player.nick, BTeams.team_name(player.team)])
	(player.get_node("Visual") as BPlayerVisual).set_nameplate_visible(false)
	_camera = BPlayerCamera.new()
	_camera.name = "PlayerCamera"
	add_child(_camera)
	_camera.setup(player)
	_spectator.queue_free()
	hud.bind(player)
	hud.set_status("")
	if not Session.bot and Session.screenshot_path.is_empty() and DisplayServer.get_name() != "headless":
		Input.mouse_mode = Input.MOUSE_MODE_CAPTURED


func _log_positions() -> void:
	for node in spawner.players_root().get_children():
		var p := node as BPlayer
		if p == null:
			continue
		if not _first_pos.has(p.peer_id):
			_first_pos[p.peer_id] = p.global_position
		var moved: float = p.global_position.distance_to(_first_pos[p.peer_id])
		_max_moved[p.peer_id] = maxf(moved, _max_moved.get(p.peer_id, 0.0))
		BLog.info(multiplayer, "POS peer=%d pos=(%.2f, %.2f, %.2f) moved=%.2f speed=%.2f mode=%d stamina=%.2f" % [
			p.peer_id, p.global_position.x, p.global_position.y, p.global_position.z, moved, p.velocity.length(), p.mode,
			p.stamina])


func _on_server_lost() -> void:
	_fail("Server closed the connection")


func _on_connection_failed() -> void:
	_fail("Could not connect to %s:%d" % [Session.address, Session.port])


func _fail(text: String) -> void:
	BLog.info(multiplayer, "ERROR " + text)
	if Session.has_cli_mode() and (Session.bot or Session.dedicated or not Session.screenshot_path.is_empty()):
		get_tree().quit(1)
		return
	Session.back_to_menu(text)


func _quit_game() -> void:
	if multiplayer.is_server():
		_log_positions()
		for peer in _max_moved:
			BLog.info(multiplayer, "SUMMARY peer=%d max_moved=%.2f" % [peer, _max_moved[peer]])
	BLog.info(multiplayer, "quit after %.1f s" % Session.quit_after)
	Session.stop_network(multiplayer)
	get_tree().quit(0)
