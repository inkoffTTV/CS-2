class_name BTracer
extends MeshInstance3D
## A bright streak from the muzzle to the impact that fades out quickly. Cosmetic only.

const LIFE: float = 0.12
const WIDTH: float = 0.035

static var _mesh: BoxMesh

var _age: float = 0.0
var _mat: StandardMaterial3D


static func spawn(parent: Node, from: Vector3, to: Vector3, color: Color) -> void:
	if parent == null or DisplayServer.get_name() == "headless" or from.distance_to(to) < 0.05:
		return
	var t := BTracer.new()
	t._setup(from, to, color)
	parent.add_child(t)


func _setup(from: Vector3, to: Vector3, color: Color) -> void:
	if _mesh == null:
		_mesh = BoxMesh.new()
		_mesh.size = Vector3(1.0, 1.0, 1.0)
	mesh = _mesh
	cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
	_mat = StandardMaterial3D.new()
	_mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	_mat.transparency = BaseMaterial3D.TRANSPARENCY_ALPHA
	_mat.blend_mode = BaseMaterial3D.BLEND_MODE_ADD
	_mat.albedo_color = color
	material_override = _mat
	var length: float = from.distance_to(to)
	var up: Vector3 = Vector3.UP if absf((to - from).normalized().y) < 0.99 else Vector3.RIGHT
	var basis_look := Basis.looking_at(to - from, up)
	transform = Transform3D(basis_look.scaled_local(Vector3(WIDTH, WIDTH, length)), (from + to) * 0.5)


func _process(delta: float) -> void:
	_age += delta
	if _age >= LIFE:
		queue_free()
		return
	var c: Color = _mat.albedo_color
	c.a = 1.0 - _age / LIFE
	_mat.albedo_color = c
