extends Node
## Autoload "Session": what the player asked for (host / join / dedicated server, nickname) and
## the command line. The game scene reads it on load and starts networking.
##
## Command line (after "--" when running from the editor binary):
##   --server            headless dedicated server (no local player)
##   --host              listen server; the host also plays
##   --connect IP[:PORT] join a server
##   --port N            port for --server / --host (default 7777)
##   --name NICK         nickname
##   --bot [SCENARIO]    scripted inputs: moves (default) | zipline | ramp | shooter | target;
##                       implies --connect if no mode given
##   --quit-after SEC    quit after SEC seconds (tests)
##   --screenshot PATH   host locally, spawn, save a PNG to PATH after a few frames, quit
##   --shot-view NAME    screenshot camera: player (default) | overview | base0 | base1 | zipline |
##                       combat (player camera, waits for a kill in the feed and a hit marker)
##   --shot-delay SEC    seconds between the local spawn and the screenshot (default 1.5)

enum Mode { NONE, HOST, JOIN, DEDICATED }

const DEFAULT_PORT: int = 7777
const MAX_CLIENTS: int = 16
const NICK_MAX: int = 16

var mode: Mode = Mode.NONE
var dedicated: bool = false
var address: String = "127.0.0.1"
var port: int = DEFAULT_PORT
var nickname: String = ""
var bot: bool = false
var bot_scenario: String = "moves"
var quit_after: float = 0.0
var screenshot_path: String = ""
var shot_view: String = "player"
var shot_delay: float = 1.5
var last_error: String = ""


func _ready() -> void:
	nickname = default_nickname()
	_parse_cli()


static func default_nickname() -> String:
	return "Legionary%d" % (randi() % 900 + 100)


static func sanitize_nick(raw: String) -> String:
	var s: String = raw.strip_edges().replace("\n", " ")
	if s.is_empty():
		s = "Nameless"
	return s.substr(0, NICK_MAX)


## Parses "host", "host:port" or ":port" into [address, port].
static func parse_endpoint(text: String, fallback_port: int = DEFAULT_PORT) -> Array:
	var t: String = text.strip_edges()
	var addr: String = t
	var p: int = fallback_port
	var colon: int = t.rfind(":")
	if colon >= 0:
		addr = t.substr(0, colon)
		var port_text: String = t.substr(colon + 1)
		if port_text.is_valid_int():
			p = port_text.to_int()
	if addr.is_empty():
		addr = "127.0.0.1"
	return [addr, p]


func has_cli_mode() -> bool:
	return mode != Mode.NONE


func _parse_cli() -> void:
	var args: PackedStringArray = OS.get_cmdline_user_args()
	if args.is_empty():
		args = OS.get_cmdline_args()
	var i: int = 0
	while i < args.size():
		var a: String = args[i]
		var next: String = args[i + 1] if i + 1 < args.size() else ""
		match a:
			"--server":
				mode = Mode.DEDICATED
				dedicated = true
			"--host":
				mode = Mode.HOST
			"--connect":
				mode = Mode.JOIN
				var ep: Array = parse_endpoint(next, port)
				address = ep[0]
				port = ep[1]
				i += 1
			"--port":
				port = next.to_int()
				i += 1
			"--name":
				nickname = sanitize_nick(next)
				i += 1
			"--bot":
				bot = true
				if not next.is_empty() and not next.begins_with("--"):
					bot_scenario = next
					i += 1
			"--quit-after":
				quit_after = next.to_float()
				i += 1
			"--screenshot":
				screenshot_path = next
				i += 1
			"--shot-view":
				shot_view = next
				i += 1
			"--shot-delay":
				shot_delay = next.to_float()
				i += 1
		i += 1
	if not screenshot_path.is_empty() and mode == Mode.NONE:
		mode = Mode.HOST
		port = 7790 + randi() % 100
	if bot and mode == Mode.NONE:
		mode = Mode.JOIN
	if bot and nickname.begins_with("Legionary"):
		nickname = "Bot%d" % (randi() % 900 + 100)


## Starts networking for the current mode. Returns OK or an error (last_error has the text).
func start_network(mp: MultiplayerAPI) -> Error:
	var peer := ENetMultiplayerPeer.new()
	var err: Error
	if mode == Mode.JOIN:
		err = peer.create_client(address, port)
		if err != OK:
			last_error = "Cannot connect to %s:%d (%s)" % [address, port, error_string(err)]
			return err
	else:
		err = peer.create_server(port, MAX_CLIENTS)
		if err != OK:
			last_error = "Cannot host on port %d (%s)" % [port, error_string(err)]
			return err
	mp.multiplayer_peer = peer
	return OK


func stop_network(mp: MultiplayerAPI) -> void:
	if mp.has_multiplayer_peer():
		mp.multiplayer_peer.close()
	mp.multiplayer_peer = OfflineMultiplayerPeer.new()


## Back to the main menu (from a game), optionally with an error to show.
func back_to_menu(error_text: String = "") -> void:
	last_error = error_text
	mode = Mode.NONE
	dedicated = false
	stop_network(get_tree().get_multiplayer())
	get_tree().change_scene_to_file.call_deferred("res://scenes/main_menu.tscn")
