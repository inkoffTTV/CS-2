class_name BCombat
extends Node
## Match-wide combat on the server: records every hero's hitboxes each network tick for lag
## compensation, resolves shots (rewind, wall occlusion, damage, kills), respawns the dead after
## BCombatRules.RESPAWN_SECONDS at a team spawn. Broadcasts kills to every peer for the kill feed.

const GROUP: StringName = &"combat"

## killer / victim: peer ids (killer 0 = nobody).
signal kill(killer: int, killer_nick: String, killer_team: int, victim: int, victim_nick: String, victim_team: int)


class ShotResult:
	var target: BPlayer = null
	var head: bool = false
	var damage: float = 0.0
	var killed: bool = false
	var end: Vector3 = Vector3.ZERO
	var rewind: float = 0.0


var lag := BLagCompensation.new()
var _respawn_rotation: Array[int] = [0, 0]


func _ready() -> void:
	add_to_group(GROUP)
	NetworkTime.after_tick_loop.connect(_after_tick_loop)


static func find(tree: SceneTree) -> BCombat:
	return tree.get_first_node_in_group(GROUP) as BCombat


func _players() -> Array[BPlayer]:
	var out: Array[BPlayer] = []
	var root: Node = get_parent().get_node_or_null("Players")
	if root != null:
		for n in root.get_children():
			var p := n as BPlayer
			if p != null and not p.is_queued_for_deletion():
				out.append(p)
	return out


## Server, after every tick loop: the display state is now the state of NetworkTime.tick.
func _after_tick_loop() -> void:
	if not multiplayer.has_multiplayer_peer() or not multiplayer.is_server():
		return
	var now: float = NetworkTime.time
	var seen: Dictionary[int, bool] = {}
	for p in _players():
		seen[p.peer_id] = true
		if BCombatRules.respawn_due(NetworkTime.tick, p.health.death_tick, p.health.respawn_tick, NetworkTime.tickrate):
			_respawn(p)
		if p.is_down():
			lag.mark_gap(p.peer_id)
		else:
			lag.record(p.peer_id, now, p.hitboxes())
	lag.prune(now)
	for peer in lag.peers():
		if not seen.has(peer):
			lag.forget(peer)


func _respawn(p: BPlayer) -> void:
	var arena: BArena = BArena.find(get_tree())
	var team: int = clampi(p.team, 0, 1)
	var xform: Transform3D = arena.get_spawn(team, _respawn_rotation[team]) if arena != null else p.transform
	_respawn_rotation[team] += 1
	p.health.respawn(xform)
	BLog.info(multiplayer, "RESPAWN peer=%d nick=%s tick=%d pos=%s" % [p.peer_id, p.nick, NetworkTime.tick, xform.origin])


## Server: a validated shot of shooter from origin along dir (unit), as the shooter saw the world
## at view_time. Rewinds the enemies, checks walls, applies damage.
func resolve_shot(shooter: BPlayer, origin: Vector3, dir: Vector3, view_time: float, cfg: BWeaponConfig) -> ShotResult:
	var r := ShotResult.new()
	var now: float = NetworkTime.time
	var rewind_time: float = BRaycast.clamp_rewind_time(now, view_time, cfg.max_rewind)
	r.rewind = now - rewind_time
	var ray := BRay.new(origin, dir)
	var by_peer: Dictionary[int, BPlayer] = {}
	var candidates: Array[int] = []
	for p in _players():
		if p != shooter and p.team != shooter.team and not p.is_down():  # no friendly fire, teammates don't block
			by_peer[p.peer_id] = p
			candidates.append(p.peer_id)
	var wall_end: Vector3 = _world_trace(origin, origin + dir * cfg.max_range, shooter)
	r.end = wall_end
	var hit: BRewindHit = lag.trace(ray, cfg.max_range, rewind_time, candidates)
	if hit != null and origin.distance_to(hit.point) > origin.distance_to(wall_end) + 1e-3:
		hit = null  # walls do not move: occlusion against the current static world
	var unrewound: BRewindHit = lag.trace(ray, cfg.max_range, now, candidates)
	if hit == null:
		BLog.info(multiplayer, "SHOT peer=%d miss rewind_ms=%.1f" % [shooter.peer_id, r.rewind * 1000.0])
		return r
	r.target = by_peer[hit.peer]
	r.head = hit.hit.zone == BCapsule.Zone.HEAD
	r.damage = BCombatRules.zone_damage(cfg.damage, hit.hit.zone, cfg.head_multiplier)
	r.end = hit.point
	r.killed = r.target.health.apply_damage(r.damage, shooter.peer_id)
	BLog.info(multiplayer, "HIT shooter=%d target=%d zone=%s dmg=%.0f hp=%.0f rewind_ms=%.1f dist=%.1f unrewound=%s" % [
		shooter.peer_id, r.target.peer_id, "head" if r.head else "body", r.damage, r.target.health.health,
		r.rewind * 1000.0, hit.hit.distance, "hit" if unrewound != null and unrewound.peer == hit.peer else "miss"])
	if r.killed:
		BLog.info(multiplayer, "KILL killer=%d victim=%d tick=%d" % [shooter.peer_id, r.target.peer_id, NetworkTime.tick])
		_kill_rpc.rpc(shooter.peer_id, shooter.nick, shooter.team, r.target.peer_id, r.target.nick, r.target.team)
	return r


## First static-world hit between from and to (or to).
func _world_trace(from: Vector3, to: Vector3, exclude: CollisionObject3D) -> Vector3:
	var q := PhysicsRayQueryParameters3D.create(from, to, BPlayer.WORLD_MASK, [exclude.get_rid()])
	var res: Dictionary = exclude.get_world_3d().direct_space_state.intersect_ray(q)
	return res["position"] if not res.is_empty() else to


@rpc("authority", "call_local", "reliable")
func _kill_rpc(killer: int, killer_nick: String, killer_team: int, victim: int, victim_nick: String, victim_team: int) -> void:
	kill.emit(killer, killer_nick, killer_team, victim, victim_nick, victim_team)
