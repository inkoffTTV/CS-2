class_name BPlayerSpawner
extends MultiplayerSpawner
## Server-driven player spawning. A client registers its nickname; once its netfox clock is synced
## the server picks a team (smaller side) and a spawn point and spawns the avatar on every peer.

const PLAYER_SCENE: PackedScene = preload("res://scenes/player.tscn")

signal local_player_spawned(player: BPlayer)

var _nicks: Dictionary[int, String] = {}  # server: registered peers
var _team_counts_spawned: Array[int] = [0, 0]  # server: spawns handed out per team (spawn rotation)


func _ready() -> void:
	spawn_function = _spawn_player
	NetworkEvents.on_server_start.connect(_on_server_start)
	NetworkEvents.on_client_start.connect(_on_client_start)
	NetworkEvents.on_peer_leave.connect(_on_peer_leave)


func players_root() -> Node:
	return get_node(spawn_path)


func find_player(peer: int) -> BPlayer:
	return players_root().get_node_or_null("P%d" % peer) as BPlayer


func _on_server_start() -> void:
	BLog.info(multiplayer, "listening on port %d%s" % [Session.port, " (dedicated)" if Session.dedicated else ""])
	if not Session.dedicated:
		_register_peer(1, Session.nickname)


func _on_client_start(id: int) -> void:
	BLog.info(multiplayer, "connected to server as peer %d" % id)
	_register.rpc_id(1, Session.nickname)


@rpc("any_peer", "call_remote", "reliable")
func _register(nickname: String) -> void:
	if not multiplayer.is_server():
		return
	_register_peer(multiplayer.get_remote_sender_id(), nickname)


func _register_peer(peer: int, nickname: String) -> void:
	if _nicks.has(peer):
		return
	_nicks[peer] = Session.sanitize_nick(nickname)
	if peer != 1:
		while not NetworkTime.is_client_synced(peer):
			await NetworkTime.after_client_sync
			if not multiplayer.get_peers().has(peer):
				_nicks.erase(peer)
				return
	var teams: Array[int] = []
	for p in players_root().get_children():
		var bp := p as BPlayer
		if bp != null:
			teams.append(bp.team)
	var team: int = BTeams.pick_team(teams)
	var data: Dictionary = {"peer": peer, "team": team, "nick": _nicks[peer], "spawn": _team_counts_spawned[team]}
	_team_counts_spawned[team] += 1
	var player := spawn(data) as BPlayer
	BLog.info(multiplayer, "spawned peer=%d nick=%s team=%s pos=%s" % [peer, player.nick, BTeams.team_name(team),
		player.global_position])


func _on_peer_leave(peer: int) -> void:
	if not multiplayer.is_server():
		return
	_nicks.erase(peer)
	var player: BPlayer = find_player(peer)
	if player != null:
		BLog.info(multiplayer, "despawn peer=%d nick=%s" % [peer, player.nick])
		player.queue_free()


func _spawn_player(data: Variant) -> Node:
	var d: Dictionary = data
	var peer: int = d["peer"]
	var team: int = d["team"]
	var arena: BArena = BArena.find(get_tree())
	var player := PLAYER_SCENE.instantiate() as BPlayer
	player.name = "P%d" % peer
	player.peer_id = peer
	player.team = team
	player.nick = d["nick"]
	player.transform = arena.get_spawn(team, d["spawn"])
	player.set_multiplayer_authority(1)
	var input := player.get_node("Input") as BPlayerInput
	input.set_multiplayer_authority(peer)
	input.set_look(player.rotation.y, 0.0)
	if peer == multiplayer.get_unique_id():
		if Session.bot:
			input.bot = BBotBrain.new(Session.bot_scenario, player)
		local_player_spawned.emit.call_deferred(player)
	return player
