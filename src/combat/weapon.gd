class_name BWeapon
extends Node
## Generic hitscan weapon of a hero (child "Weapon" of the player on every peer).
## Owning peer: while the trigger is held, fires at cfg.fire_rate. It aims from the camera (what is
## under the crosshair), shoots from the eyes toward that point, draws the tracer at once and sends
## the shot with the server time it saw the other heroes at. Server: validates (finite values,
## alive, token-bucket rate limit, origin near the eyes) and hands the shot to BCombat; confirms hits
## to the shooter and shows the tracer to everyone else.

signal hit_confirmed(head: bool, damage: float, killed: bool)

const TRACER_LOCAL := Color(1.0, 0.85, 0.35)
const TRACER_REMOTE := Color(1.0, 0.55, 0.25)
const RPC_CHANNEL: int = 1

var cfg := BWeaponConfig.new()

var _player: BPlayer
var _cooldown: float = 0.0
var _bucket: BTokenBucket  # server


func _ready() -> void:
	_player = get_parent() as BPlayer
	process_priority = 10  # after the camera has followed this frame's look
	_bucket = BTokenBucket.new(cfg.fire_rate, cfg.burst())


func _process(delta: float) -> void:
	if not _player.is_local():
		return
	if _player.is_down() or not _player.input.fire_held():
		_cooldown = maxf(0.0, _cooldown - delta)  # no banking shots while idle
		return
	_cooldown -= delta
	while _cooldown <= 0.0:
		_fire_once()
		_cooldown += cfg.interval()


func _fire_once() -> void:
	var eye: Vector3 = _player.eye_position()
	var aim: Vector3 = _aim_point(eye)
	var dir: Vector3 = (aim - eye).normalized()
	if not dir.is_finite() or dir.is_zero_approx():
		return
	var end: Vector3 = _trace(eye, eye + dir * cfg.max_range, BPlayer.WORLD_MASK | BPlayer.PLAYER_LAYER)
	BTracer.spawn(_effects_root(), _player.muzzle_position(), end, TRACER_LOCAL)
	var view_time: float = BCombatRules.view_time(NetworkTime.tick, NetworkTime.tick_factor, NetworkTime.ticktime,
		0.0 if multiplayer.is_server() else NetworkTimeSynchronizer.rtt)
	if multiplayer.is_server():
		_server_shot(eye, dir, view_time)
	else:
		_shot_rpc.rpc_id(1, eye, dir, view_time)


## What the crosshair points at: the camera ray, started level with the eyes so nothing between
## the camera and the hero is hit, against the world and the other heroes.
func _aim_point(eye: Vector3) -> Vector3:
	var cam: Camera3D = get_viewport().get_camera_3d()
	if cam == null:
		return eye + _player.look_dir() * cfg.max_range
	var o: Vector3 = cam.global_position
	var d: Vector3 = -cam.global_basis.z.normalized()
	var start: Vector3 = o + d * maxf(0.0, (eye - o).dot(d))
	return _trace(start, o + d * (cfg.max_range + (eye - o).length()), BPlayer.WORLD_MASK | BPlayer.PLAYER_LAYER)


func _trace(from: Vector3, to: Vector3, mask: int) -> Vector3:
	var q := PhysicsRayQueryParameters3D.create(from, to, mask, [_player.get_rid()])
	var res: Dictionary = _player.get_world_3d().direct_space_state.intersect_ray(q)
	return res["position"] if not res.is_empty() else to


func _effects_root() -> Node:
	return _player.get_parent()


# --- server --------------------------------------------------------------------------------

@rpc("any_peer", "call_remote", "reliable", RPC_CHANNEL)
func _shot_rpc(origin: Vector3, dir: Vector3, view_time: float) -> void:
	if not multiplayer.is_server() or multiplayer.get_remote_sender_id() != _player.peer_id:
		return
	_server_shot(origin, dir, view_time)


func _server_shot(origin: Vector3, dir: Vector3, view_time: float) -> void:
	var reason: String = _reject_reason(origin, dir, view_time)
	if not reason.is_empty():
		BLog.info(multiplayer, "REJECT peer=%d reason=%s" % [_player.peer_id, reason])
		return
	var combat: BCombat = BCombat.find(get_tree())
	if combat == null:
		return
	var r: BCombat.ShotResult = combat.resolve_shot(_player, origin, dir.normalized(), view_time, cfg)
	if r.target != null:
		if _player.peer_id == multiplayer.get_unique_id():
			_on_hit(r.head, r.damage, r.killed, r.end)
		else:
			_hit_rpc.rpc_id(_player.peer_id, r.head, r.damage, r.killed, r.end)
	for peer in multiplayer.get_peers():
		if peer != _player.peer_id:
			_tracer_rpc.rpc_id(peer, r.end)
	if not Session.dedicated and _player.peer_id != multiplayer.get_unique_id():
		_tracer_rpc(r.end)  # the host sees other heroes' shots too


func _reject_reason(origin: Vector3, dir: Vector3, view_time: float) -> String:
	if not origin.is_finite() or not dir.is_finite() or not is_finite(view_time):
		return "non_finite"
	if dir.length() < 0.5 or dir.length() > 1.5:
		return "bad_dir"
	if _player.is_down():
		return "down"
	if not _bucket.try_take(float(Time.get_ticks_usec()) / 1e6):
		return "rate"
	if origin.distance_to(_player.eye_position()) > cfg.max_origin_error:
		return "origin"
	return ""


# --- effects on clients --------------------------------------------------------------------

@rpc("authority", "call_remote", "reliable", RPC_CHANNEL)
func _hit_rpc(head: bool, damage: float, killed: bool, point: Vector3) -> void:
	_on_hit(head, damage, killed, point)


func _on_hit(head: bool, damage: float, killed: bool, point: Vector3) -> void:
	BDamageNumber.spawn(_effects_root(), point, damage, head)
	hit_confirmed.emit(head, damage, killed)


@rpc("authority", "call_remote", "unreliable_ordered", RPC_CHANNEL)
func _tracer_rpc(end: Vector3) -> void:
	BTracer.spawn(_effects_root(), _player.muzzle_position(), end, TRACER_REMOTE)
