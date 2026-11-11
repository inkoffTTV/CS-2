class_name BPlayer
extends CharacterBody3D
## A hero driven inside the netfox rollback loop: the owning client predicts its own movement,
## the server simulates authoritatively and corrects it. Everything that affects the next tick
## (transform, velocity, stamina, dash timer, air jumps, mode, zipline, last buttons) is rollback state.
## Health lives in $Health (server-authoritative, not rolled back); a dead hero is frozen and hidden,
## and the respawn teleport happens inside the rollback tick so resimulation replays it.

enum Mode { MOVE = 0, SLIDE = 1, ZIPLINE = 2 }

signal movement_event(kind: StringName)

const STAND_HEIGHT: float = 1.8
const CROUCH_HEIGHT: float = 1.0
const RADIUS: float = 0.4
const CENTER_HEIGHT: float = 0.9  # capsule centre above the feet when standing
const FLOOR_SNAP: float = 0.35
const WORLD_MASK: int = 1
const PLAYER_LAYER: int = 2
const EYE_BELOW_TOP: float = 0.2

static var walk_cfg: BWalkConfig = BWalkConfig.new()
static var move_cfg: BMoveConfig = BMoveConfig.new()
static var stamina_cfg: BStaminaConfig = BStaminaConfig.new()
static var zip_cfg: BZiplineConfig = BZiplineConfig.new()

# Identity (set by the spawner on every peer before the node enters the tree).
var peer_id: int = 0
var team: int = BTeams.NONE
var nick: String = ""

# Rollback state.
var stamina: float = 3.0
var dash_left: float = 0.0
var air_jumps: int = 0
var mode: int = Mode.MOVE
var zip_index: int = -1
var zip_dir: int = 1
var prev_buttons: int = 0
var crouched: bool = false  # short capsule (sliding or crouch-walking)
var respawn_seen: int = -1  # the respawn tick (BHealth.respawn_tick) this state already teleported for

var _pressed: int = 0
var _tick: int = 0
var _emitted: Dictionary[String, int] = {}  # "tick:kind" -> tick, to report each event once
var _shape: CapsuleShape3D
var _shape_node: CollisionShape3D
var _look_reset_for: int = -1
var _local: int = -1  # cached is_local(): -1 unknown, 0 no, 1 yes

@onready var input: BPlayerInput = $Input as BPlayerInput
@onready var rollback: RollbackSynchronizer = $RollbackSynchronizer as RollbackSynchronizer
@onready var health: BHealth = $Health as BHealth
@onready var weapon: BWeapon = $Weapon as BWeapon
@onready var _visual: BPlayerVisual = $Visual as BPlayerVisual


func _ready() -> void:
	collision_layer = PLAYER_LAYER
	collision_mask = WORLD_MASK  # players do not block each other (yet)
	floor_max_angle = deg_to_rad(46.0)
	floor_snap_length = FLOOR_SNAP
	_shape = CapsuleShape3D.new()
	_shape.radius = RADIUS
	_shape_node = CollisionShape3D.new()
	_shape_node.name = "Collision"
	_shape_node.shape = _shape
	add_child(_shape_node)
	_apply_shape(false)
	_visual.setup(self)
	movement_event.connect(_log_event)


func _log_event(kind: StringName) -> void:
	if multiplayer.is_server() or is_local():
		BLog.info(multiplayer, "EVENT %s peer=%d tick=%d pos=%s" % [kind, peer_id, NetworkRollback.tick, global_position])


func is_local() -> bool:
	if _local < 0:
		if not BLog.connected(multiplayer):
			return false
		_local = 1 if peer_id == multiplayer.get_unique_id() else 0
	return _local == 1


func facing() -> Vector3:
	return BWalk.facing_from_yaw(rotation.y)


func is_dashing() -> bool:
	return dash_left > 0.0


func is_crouched() -> bool:
	return crouched


## Dead, or respawned on the server but this state has not reached the respawn tick yet.
func is_down() -> bool:
	return health.is_dead() or respawn_seen < health.respawn_tick


func height() -> float:
	return CROUCH_HEIGHT if crouched else STAND_HEIGHT


func eye_position() -> Vector3:
	return global_position + Vector3(0.0, height() - EYE_BELOW_TOP, 0.0)


## Where tracers start: the gun at the right hip/shoulder of the model.
func muzzle_position() -> Vector3:
	return global_position + global_basis * Vector3(0.32, height() * 0.72, -0.55)


func look_dir() -> Vector3:
	var p: float = input.live_pitch
	return Vector3(-sin(input.live_yaw) * cos(p), sin(p), -cos(input.live_yaw) * cos(p))


func hitboxes() -> Array[BCapsule]:
	return BHitboxes.build(global_position, height(), RADIUS)


func _process(_delta: float) -> void:
	var down: bool = is_down()
	collision_layer = 0 if down else PLAYER_LAYER
	_visual.visible = not down
	if is_local() and input.bot == null and health.respawn_tick > _look_reset_for:
		_look_reset_for = health.respawn_tick  # face the Rubicon again after a respawn
		input.set_look(health.respawn_xform.basis.get_euler().y, 0.0)


func _rollback_tick(dt: float, tick: int, is_fresh: bool) -> void:
	_tick = tick
	if health.respawn_tick > respawn_seen and tick >= health.respawn_tick:
		_respawn()
	if health.is_dead_at(tick):
		return
	rotation = Vector3(0.0, input.look_yaw, 0.0)
	var held: int = input.buttons()
	_pressed = held & ~prev_buttons
	prev_buttons = held
	var st := BStaminaState.new(stamina)
	BStamina.regen(st, stamina_cfg, dt)
	stamina = st.charges

	if mode == Mode.ZIPLINE:
		if _tick_zipline(dt):
			return
	elif (_pressed & BPlayerInput.INTERACT) != 0 and _try_attach_zipline(is_fresh):
		_tick_zipline(dt)
		return
	_tick_move(dt, is_fresh)


# --- walking, dash, jumps, slide ------------------------------------------------------------

func _tick_move(dt: float, is_fresh: bool) -> void:
	_refresh_floor()
	var grounded: bool = is_on_floor()
	if grounded:
		air_jumps = 0
	_apply_shape(mode == Mode.SLIDE or (input.slide and grounded))
	var wish: Vector3 = BWalk.wish_dir(input.move, input.look_yaw)

	if (_pressed & BPlayerInput.DASH) != 0:
		_try_dash(wish, is_fresh)
	if (_pressed & BPlayerInput.JUMP) != 0 and _try_jump(grounded, is_fresh):
		grounded = false
	if mode == Mode.MOVE and input.slide and grounded and not is_dashing() \
			and BMoveRules.can_start_slide(BMath.flatten(velocity).length(), true, move_cfg):
		velocity = BMoveRules.start_slide_velocity(BMath.flatten(velocity), move_cfg)
		mode = Mode.SLIDE
		_apply_shape(true)
		_emit(&"slide", is_fresh)

	if mode == Mode.SLIDE:
		_step_slide(dt)
	else:
		_step_walk(dt, grounded, wish)

	if dash_left > 0.0:
		dash_left = maxf(0.0, dash_left - dt)
		if dash_left <= 0.0 and not is_on_floor():
			velocity = BWalk.end_air_dash(velocity, walk_cfg)


func _try_dash(wish: Vector3, is_fresh: bool) -> void:
	var st := BStaminaState.new(stamina)
	if not BStamina.try_consume(st, stamina_cfg):
		return
	stamina = st.charges
	var v: Vector3 = BMoveRules.compute_dash_velocity(wish, facing(), move_cfg)
	velocity = Vector3(v.x, 0.0, v.z)
	dash_left = walk_cfg.dash_duration
	if mode == Mode.SLIDE:
		mode = Mode.MOVE
	_emit(&"dash", is_fresh)


func _try_jump(grounded: bool, is_fresh: bool) -> bool:
	if grounded or mode == Mode.SLIDE:
		velocity.y = walk_cfg.jump_velocity
		mode = Mode.MOVE
		_emit(&"jump", is_fresh)
		return true
	var st := BStaminaState.new(stamina)
	if air_jumps >= move_cfg.air_jumps or not BStamina.try_consume(st, stamina_cfg):
		return false
	stamina = st.charges
	air_jumps += 1
	velocity.y = maxf(velocity.y, walk_cfg.jump_velocity)  # an air jump always gains height
	_emit(&"air_jump", is_fresh)
	return true


func _step_walk(dt: float, grounded: bool, wish: Vector3) -> void:
	var max_speed: float = walk_cfg.crouch_speed if crouched else walk_cfg.walk_speed
	var h: Vector3 = BMath.flatten(velocity)
	if not is_dashing():  # no friction or steering during a dash
		h = BWalk.step_horizontal(h, wish, dt, grounded, max_speed, walk_cfg)
	var vy: float = velocity.y
	if grounded and vy <= 0.0:
		vy = 0.0
	else:
		vy -= walk_cfg.gravity * dt
	velocity = Vector3(h.x, vy, h.z)
	_move()


func _step_slide(dt: float) -> void:
	var n: Vector3 = get_floor_normal() if is_on_floor() else Vector3.UP
	var slide_v: Vector3 = BMoveRules.step_slide(velocity, n, dt, move_cfg)
	velocity = slide_v
	_move()
	# On the floor move_and_slide() drops the downhill (vertical) part of the velocity; the slide keeps
	# its in-plane speed unless it hit a wall.
	if is_on_floor() and not is_on_wall():
		velocity = slide_v
	if not is_on_floor() or not input.slide or BMoveRules.should_exit_slide(velocity, move_cfg):
		mode = Mode.MOVE


# --- ziplines --------------------------------------------------------------------------------

func _cable_probe() -> Vector3:
	return global_position + Vector3(0.0, CENTER_HEIGHT - zip_cfg.hang_offset_y, 0.0)


func _zipline(index: int) -> BZiplineSegment:
	var arena: BArena = BArena.find(get_tree())
	if arena == null or index < 0 or index >= arena.ziplines.size():
		return null
	return arena.ziplines[index]


func _try_attach_zipline(is_fresh: bool) -> bool:
	var arena: BArena = BArena.find(get_tree())
	if arena == null:
		return false
	var probe: Vector3 = _cable_probe()
	var best: int = -1
	var best_dist: float = zip_cfg.attach_radius
	for i in arena.ziplines.size():
		var d: float = BZipline.distance_to(arena.ziplines[i], probe)
		if d <= best_dist:
			best = i
			best_dist = d
	if best < 0:
		return false
	zip_index = best
	zip_dir = BZipline.choose_direction(arena.ziplines[best], facing())
	mode = Mode.ZIPLINE
	dash_left = 0.0
	air_jumps = 0
	_pressed &= ~BPlayerInput.INTERACT
	_apply_shape(false)
	_emit(&"zipline", is_fresh)
	return true


## Rides the cable for one tick. Returns false when the player let go this tick (the regular
## movement step then runs).
func _tick_zipline(dt: float) -> bool:
	var seg: BZiplineSegment = _zipline(zip_index)
	var release: int = BPlayerInput.INTERACT | BPlayerInput.JUMP
	if seg == null or (_pressed & release) != 0:
		if (_pressed & BPlayerInput.JUMP) != 0:
			velocity.y = maxf(velocity.y, walk_cfg.jump_velocity)
		_pressed &= ~release
		_leave_zipline()
		return false
	_apply_shape(false)
	var t: float = BZipline.project_t(seg, _cable_probe())
	var nt: float = BZipline.advance_t(seg, t, zip_dir, zip_cfg.speed, dt)
	var target: Vector3 = BZipline.point_at(seg, nt) + Vector3(0.0, zip_cfg.hang_offset_y - CENTER_HEIGHT, 0.0)
	var from: Vector3 = global_position
	velocity = (target - from) / dt
	floor_snap_length = 0.0
	_move()
	floor_snap_length = FLOOR_SNAP
	velocity = (global_position - from) / dt
	var reached_end: bool = nt >= 1.0 if zip_dir > 0 else nt <= 0.0
	if reached_end or global_position.distance_to(target) > 0.25:
		_leave_zipline()
	return true


func _leave_zipline() -> void:
	mode = Mode.MOVE
	zip_index = -1


func _respawn() -> void:
	transform = health.respawn_xform
	velocity = Vector3.ZERO
	stamina = float(stamina_cfg.max_charges)
	dash_left = 0.0
	air_jumps = 0
	mode = Mode.MOVE
	zip_index = -1
	prev_buttons = input.buttons()
	respawn_seen = health.respawn_tick
	_apply_shape(false)


# --- helpers ---------------------------------------------------------------------------------

func _move() -> void:
	velocity *= NetworkTime.physics_factor
	move_and_slide()
	velocity /= NetworkTime.physics_factor


## is_on_floor() only updates in move_and_slide(); a zero move refreshes it after a rollback restore.
func _refresh_floor() -> void:
	var saved: Vector3 = velocity
	velocity = Vector3.ZERO
	move_and_slide()
	velocity = saved


func _apply_shape(short: bool) -> void:
	crouched = short
	var h: float = CROUCH_HEIGHT if short else STAND_HEIGHT
	_shape.height = h
	_shape_node.position = Vector3(0.0, h * 0.5, 0.0)


## Reports a movement event once per tick, and only from a tick simulated with the real input:
## the server may first run a tick with the last known (stale) input and re-run it when the input
## arrives, so is_fresh alone would miss events or report ones that never happened.
func _emit(kind: StringName, _is_fresh: bool) -> void:
	if rollback.is_predicting():
		return
	var key: String = "%d:%s" % [_tick, kind]
	if _emitted.has(key):
		return
	_emitted[key] = _tick
	if _emitted.size() > 64:
		for k: String in _emitted.keys():
			if _emitted[k] < _tick - 64:
				_emitted.erase(k)
	movement_event.emit(kind)
