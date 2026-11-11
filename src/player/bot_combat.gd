class_name BBotCombat
extends RefCounted
## Combat scenarios for headless test bots (see BBotBrain).
##  shooter: walks to a post in front of the +Z passage of the Rubicon on its own half, then strafes
##           left/right and holds fire at the nearest visible enemy, aiming the camera so the
##           crosshair sits on it (with a small wobble, so some shots miss and some hit the head).
##  target:  walks to a lane on its own half in view of that passage and runs back and forth along
##           it; after a respawn it walks back to the lane.

const ARRIVE_DIST: float = 0.8
const STRAFE_PERIOD: float = 1.2
const ENGAGE_RANGE: float = 60.0
const AIM_HEIGHT: float = 1.2
const POST := Vector2(6, 9)  # mirrored in X by the side
const LANE: Array[Vector2] = [Vector2(10, 2), Vector2(10, 16)]

var _scenario: String
var _player: BPlayer
var _side: float = -1.0  # -1: Optimates half (-X), +1: Populares half
var _route: Array[Vector2] = []
var _wp: int = 0
var _planned_for: int = -2  # respawn tick the route was planned for


func _init(scenario: String, player: BPlayer) -> void:
	_scenario = scenario
	_player = player
	_side = -1.0 if player.position.x < 0.0 else 1.0  # not in the tree yet; the players root is at the origin


func fill(input: BPlayerInput, t: float) -> void:
	if t < 0.5 or _player.is_down():
		return
	if _planned_for != _player.health.respawn_tick:
		_planned_for = _player.health.respawn_tick
		_plan()
	if _scenario == "shooter":
		_shooter(input, t)
	else:
		_target(input)


func _plan() -> void:
	_wp = 0
	_route.clear()
	if _scenario == "shooter":
		_route.append(Vector2(POST.x * _side, POST.y))
	else:
		for p in LANE:
			_route.append(Vector2(p.x * _side, p.y))


func _flat_pos() -> Vector2:
	return Vector2(_player.global_position.x, _player.global_position.z)


func _target(input: BPlayerInput) -> void:
	var pos: Vector2 = _flat_pos()
	if pos.distance_to(_route[_wp]) < ARRIVE_DIST:
		_wp = (_wp + 1) % _route.size()
	input.set_look(BBotBrain.yaw_toward(_route[_wp] - pos), 0.0)
	input.move = Vector2(0, 1)


func _shooter(input: BPlayerInput, t: float) -> void:
	var pos: Vector2 = _flat_pos()
	var enemy: BPlayer = _nearest_enemy()
	var move_world := Vector2.ZERO
	var post: Vector2 = _route[_route.size() - 1]
	if _wp < _route.size():
		if pos.distance_to(_route[_wp]) < ARRIVE_DIST:
			_wp += 1
		else:
			move_world = (_route[_wp] - pos).normalized()
	elif pos.distance_to(post) > 2.5:
		move_world = (post - pos).normalized()
	elif enemy != null:
		var to := Vector2(enemy.global_position.x, enemy.global_position.z) - pos
		var strafe_sign: float = 1.0 if int(t / STRAFE_PERIOD) % 2 == 0 else -1.0
		move_world = Vector2(-to.y, to.x).normalized() * strafe_sign

	if enemy != null and _in_sight(enemy):
		_aim(input, enemy, t)
		input.fire = true
	elif move_world != Vector2.ZERO:
		input.set_look(BBotBrain.yaw_toward(move_world), 0.0)
	else:
		input.set_look(BBotBrain.yaw_toward(Vector2(-_side, 0)), 0.0)
	input.move = _to_local(move_world, input.look_yaw)


## Look angles that put the over-the-shoulder camera's crosshair on the enemy (fixed-point
## iteration, since the camera position depends on the look).
func _aim(input: BPlayerInput, enemy: BPlayer, t: float) -> void:
	var point: Vector3 = enemy.global_position + Vector3(0.15 * sin(t * 3.1), AIM_HEIGHT + 0.3 * sin(t * 2.3), 0.0)
	var pivot: Vector3 = _player.global_position + Vector3(0.0, BPlayerCamera.PIVOT_STAND, 0.0)
	var yaw: float = input.look_yaw
	var pitch: float = input.look_pitch
	for i in 4:
		var d: Vector3 = point - (pivot + BPlayerCamera.arm_offset(yaw, pitch))
		yaw = atan2(-d.x, -d.z)
		pitch = atan2(d.y, Vector2(d.x, d.z).length())
	input.set_look(yaw, clampf(pitch, -BPlayerInput.PITCH_LIMIT, BPlayerInput.PITCH_LIMIT))


func _nearest_enemy() -> BPlayer:
	var best: BPlayer = null
	var best_d: float = ENGAGE_RANGE
	for n in _player.get_parent().get_children():
		var p := n as BPlayer
		if p == null or p.team == _player.team or p.is_down():
			continue
		var d: float = p.global_position.distance_to(_player.global_position)
		if d < best_d:
			best = p
			best_d = d
	return best


func _in_sight(enemy: BPlayer) -> bool:
	var to: Vector3 = enemy.global_position + Vector3(0.0, AIM_HEIGHT, 0.0)
	var q := PhysicsRayQueryParameters3D.create(_player.eye_position(), to, BPlayer.WORLD_MASK, [_player.get_rid()])
	return _player.get_world_3d().direct_space_state.intersect_ray(q).is_empty()


## World-space horizontal direction to a move stick (x = right, y = forward) for a yaw.
static func _to_local(dir: Vector2, yaw: float) -> Vector2:
	var forward := Vector2(-sin(yaw), -cos(yaw))
	var right := Vector2(cos(yaw), -sin(yaw))
	return Vector2(dir.dot(right), dir.dot(forward))
