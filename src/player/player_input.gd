class_name BPlayerInput
extends BaseNetInput
## Input for one player, gathered by its owning peer before every netfox tick loop.
## Buttons are sent as "held" flags; a tap shorter than a tick is latched until the next gather.
## The player turns presses into one-shot actions by comparing with the previous tick (rollback state).

const JUMP: int = 1
const DASH: int = 2
const SLIDE: int = 4
const INTERACT: int = 8
const FIRE: int = 16

const PITCH_LIMIT: float = deg_to_rad(80.0)

@export var mouse_sensitivity: float = 0.0025

# Input properties (RollbackSynchronizer)
var move: Vector2 = Vector2.ZERO  # x = right, y = forward
var look_yaw: float = 0.0
var look_pitch: float = 0.0
var jump: bool = false
var dash: bool = false
var slide: bool = false
var interact: bool = false
var fire: bool = false

## Live look angles for the local camera (updated every mouse event, not tick-quantized).
var live_yaw: float = 0.0
var live_pitch: float = 0.0
## Scripted inputs instead of the keyboard (--bot).
var bot: BBotBrain = null

var _latched: int = 0


## Replaces BaseNetInput._ready(): its lambda keeps firing for a despawned (out-of-tree) node.
func _ready() -> void:
	NetworkTime.before_tick_loop.connect(_on_before_tick_loop)


func _on_before_tick_loop() -> void:
	if is_inside_tree() and is_multiplayer_authority():
		_gather()


func buttons() -> int:
	return (JUMP if jump else 0) | (DASH if dash else 0) | (SLIDE if slide else 0) \
		| (INTERACT if interact else 0) | (FIRE if fire else 0)


func set_look(yaw: float, pitch: float) -> void:
	look_yaw = yaw
	look_pitch = pitch
	live_yaw = yaw
	live_pitch = pitch


## Trigger held right now (the weapon fires from _process, not per tick).
func fire_held() -> bool:
	if bot != null:
		return fire
	return Input.mouse_mode == Input.MOUSE_MODE_CAPTURED and Input.is_action_pressed("fire")


func is_local() -> bool:
	return is_multiplayer_authority() and multiplayer.has_multiplayer_peer()


func _unhandled_input(event: InputEvent) -> void:
	if bot != null or not is_local():
		return
	var motion := event as InputEventMouseMotion
	if motion != null and Input.mouse_mode == Input.MOUSE_MODE_CAPTURED:
		live_yaw = wrapf(live_yaw - motion.relative.x * mouse_sensitivity, -PI, PI)
		live_pitch = clampf(live_pitch - motion.relative.y * mouse_sensitivity, -PITCH_LIMIT, PITCH_LIMIT)
		return
	if event.is_action_pressed("jump"):
		_latched |= JUMP
	elif event.is_action_pressed("dash"):
		_latched |= DASH
	elif event.is_action_pressed("interact"):
		_latched |= INTERACT
	elif event.is_action_pressed("fire"):
		_latched |= FIRE


func _gather() -> void:
	if bot != null:
		bot.fill(self)
		return
	var captured: bool = Input.mouse_mode == Input.MOUSE_MODE_CAPTURED
	move = Input.get_vector("move_left", "move_right", "move_back", "move_forward") if captured else Vector2.ZERO
	look_yaw = live_yaw
	look_pitch = live_pitch
	jump = captured and (Input.is_action_pressed("jump") or (_latched & JUMP) != 0)
	dash = captured and (Input.is_action_pressed("dash") or (_latched & DASH) != 0)
	slide = captured and Input.is_action_pressed("slide")
	interact = captured and (Input.is_action_pressed("interact") or (_latched & INTERACT) != 0)
	fire = captured and (Input.is_action_pressed("fire") or (_latched & FIRE) != 0)
	_latched = 0
