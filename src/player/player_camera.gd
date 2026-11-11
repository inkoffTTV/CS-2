class_name BPlayerCamera
extends Node3D
## Over-the-shoulder third-person camera for the local player only. Follows the (interpolated)
## player position and the live mouse look, so aiming is not quantized to network ticks.

const PIVOT_STAND: float = 1.8
const PIVOT_LOW: float = 1.05
const ARM_LENGTH: float = 3.4
const SHOULDER: float = 0.85

var _player: BPlayer
var _pitch_node: Node3D
var _arm: SpringArm3D
var _camera: Camera3D
var _pivot_h: float = PIVOT_STAND


func setup(player: BPlayer) -> void:
	_player = player
	top_level = true
	_pitch_node = Node3D.new()
	add_child(_pitch_node)
	_arm = SpringArm3D.new()
	_arm.spring_length = ARM_LENGTH
	_arm.collision_mask = BPlayer.WORLD_MASK
	_arm.margin = 0.15
	var probe := SphereShape3D.new()
	probe.radius = 0.2
	_arm.shape = probe
	_arm.position = Vector3(SHOULDER, 0.0, 0.0)
	_pitch_node.add_child(_arm)
	_camera = Camera3D.new()
	_camera.fov = 80.0
	_camera.near = 0.05
	_camera.far = 400.0
	_arm.add_child(_camera)
	_camera.current = true
	_arm.add_excluded_object(player.get_rid())
	_follow(0.0)


func get_camera() -> Camera3D:
	return _camera


func _process(delta: float) -> void:
	_follow(delta)


func _follow(delta: float) -> void:
	if _player == null or not is_instance_valid(_player):
		return
	var low: bool = _player.is_crouched() or _player.mode == BPlayer.Mode.SLIDE
	var target_h: float = PIVOT_LOW if low else PIVOT_STAND
	_pivot_h = target_h if delta <= 0.0 else lerpf(_pivot_h, target_h, 1.0 - exp(-delta * 10.0))
	global_position = _player.global_position + Vector3(0.0, _pivot_h, 0.0)
	rotation = Vector3(0.0, _player.input.live_yaw, 0.0)
	_pitch_node.rotation = Vector3(_player.input.live_pitch, 0.0, 0.0)
