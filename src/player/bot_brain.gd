class_name BBotBrain
extends RefCounted
## Scripted inputs for headless test clients (--bot [moves|zipline]).
##  moves:   run toward the Rubicon, slide, turn, dash, jump + double jump, run on.
##  zipline: walk to the 30 degree ramp, climb onto the platform, grab the zipline, ride it to the end.

const ARRIVE_DIST: float = 0.8

var scenario: String = "moves"
var _player: BPlayer
var _spawn_yaw: float = 0.0
var _start_ms: int = -1
var _waypoints: Array[Vector2] = []
var _wp: int = 0
var _arrived_at: float = -1.0
var _grabbed: bool = false


func _init(p_scenario: String, player: BPlayer) -> void:
	scenario = p_scenario
	_player = player
	_spawn_yaw = player.rotation.y
	if player.position.x > 0.0:  # not in the tree yet; the players root sits at the origin
		_waypoints.append(Vector2(5, 9))  # Populares cross the Rubicon through the +Z passage
		_waypoints.append(Vector2(-5, 9))
	_waypoints.append_array([Vector2(-6, 25), Vector2(-21, 25), Vector2(-25, 25)])


func elapsed() -> float:
	return 0.0 if _start_ms < 0 else float(Time.get_ticks_msec() - _start_ms) / 1000.0


func fill(input: BPlayerInput) -> void:
	if _start_ms < 0:
		_start_ms = Time.get_ticks_msec()
	input.move = Vector2.ZERO
	input.jump = false
	input.dash = false
	input.slide = false
	input.interact = false
	input.fire = false
	if scenario == "zipline":
		_zipline(input, elapsed())
	else:
		_moves(input, elapsed())


func _moves(input: BPlayerInput, t: float) -> void:
	var yaw: float = _spawn_yaw
	if t >= 1.0 and t < 2.6:
		input.move = Vector2(0, 1)
		input.slide = t >= 1.8
	elif t >= 2.6 and t < 5.3:
		yaw = _spawn_yaw + PI * 0.5
		input.move = Vector2(0, 1)
		input.dash = t >= 3.0 and t < 3.1
		input.jump = (t >= 3.6 and t < 3.7) or (t >= 3.95 and t < 4.05)
	elif t >= 5.3 and t < 7.0:
		input.move = Vector2(0, 1)
	input.set_look(yaw, 0.0)


func _zipline(input: BPlayerInput, t: float) -> void:
	var pos := Vector2(_player.global_position.x, _player.global_position.z)
	if t < 0.5 or _player.mode == BPlayer.Mode.ZIPLINE:
		input.set_look(input.look_yaw, 0.0)
		return
	if _wp < _waypoints.size():
		var target: Vector2 = _waypoints[_wp]
		if pos.distance_to(target) < ARRIVE_DIST:
			_wp += 1
		input.set_look(_yaw_toward(target - pos), 0.0)
		input.move = Vector2(0, 1)
		return
	if _grabbed:
		return
	if _arrived_at < 0.0:
		_arrived_at = t
	var to_end: Vector3 = BArena.ZIPLINE_B - _player.global_position
	input.set_look(_yaw_toward(Vector2(to_end.x, to_end.z)), 0.0)
	if t - _arrived_at > 0.3:
		input.interact = true
		_grabbed = true


static func _yaw_toward(dir: Vector2) -> float:
	return atan2(-dir.x, -dir.y)
