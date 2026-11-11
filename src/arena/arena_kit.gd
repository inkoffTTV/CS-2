class_name BArenaKit
extends RefCounted
## Builders for greybox pieces: solid boxes and wedges (with collision) and decor meshes.

const WORLD_LAYER: int = 1


static func box(parent: Node3D, label: String, center: Vector3, size: Vector3, material: Material,
		rotation_deg: Vector3 = Vector3.ZERO, collide: bool = true) -> Node3D:
	var mesh := BoxMesh.new()
	mesh.size = size
	var shape: Shape3D = null
	if collide:
		var bs := BoxShape3D.new()
		bs.size = size
		shape = bs
	return _place(parent, label, center, rotation_deg, mesh, material, shape)


## Solid right-triangle prism: the slope rises toward +X when rise_toward_plus_x, else toward -X.
## size = (run, rise, width).
static func wedge(parent: Node3D, label: String, center: Vector3, size: Vector3, material: Material,
		rise_toward_plus_x: bool) -> Node3D:
	var mesh := PrismMesh.new()
	mesh.size = size
	mesh.left_to_right = 1.0 if rise_toward_plus_x else 0.0
	var shape: Shape3D = mesh.create_convex_shape(true, false)
	return _place(parent, label, center, Vector3.ZERO, mesh, material, shape)


static func cylinder(parent: Node3D, label: String, center: Vector3, radius: float, height: float,
		material: Material, collide: bool = false) -> Node3D:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = height
	mesh.radial_segments = 32
	var shape: Shape3D = null
	if collide:
		var cs := CylinderShape3D.new()
		cs.radius = radius
		cs.height = height
		shape = cs
	return _place(parent, label, center, Vector3.ZERO, mesh, material, shape)


## Thin rod (cable) between two points; no collision.
static func rod(parent: Node3D, label: String, a: Vector3, b: Vector3, radius: float, material: Material) -> Node3D:
	var mesh := CylinderMesh.new()
	mesh.top_radius = radius
	mesh.bottom_radius = radius
	mesh.height = a.distance_to(b)
	mesh.radial_segments = 8
	var mi := MeshInstance3D.new()
	mi.name = label
	mi.mesh = mesh
	mi.material_override = material
	parent.add_child(mi)
	var mid: Vector3 = (a + b) * 0.5
	var dir: Vector3 = (b - a).normalized()
	# Cylinder axis is +Y: build a basis whose Y is the cable direction.
	var x_axis: Vector3 = dir.cross(Vector3.UP if absf(dir.y) < 0.99 else Vector3.RIGHT).normalized()
	var z_axis: Vector3 = x_axis.cross(dir).normalized()
	mi.transform = Transform3D(Basis(x_axis, dir, z_axis), mid)
	return mi


static func _place(parent: Node3D, label: String, center: Vector3, rotation_deg: Vector3, mesh: Mesh,
		material: Material, shape: Shape3D) -> Node3D:
	var root: Node3D
	if shape != null:
		var body := StaticBody3D.new()
		body.collision_layer = WORLD_LAYER
		body.collision_mask = 0
		var cs := CollisionShape3D.new()
		cs.shape = shape
		body.add_child(cs)
		root = body
	else:
		root = Node3D.new()
	root.name = label
	var mi := MeshInstance3D.new()
	mi.mesh = mesh
	mi.material_override = material
	root.add_child(mi)
	parent.add_child(root)
	root.position = center
	root.rotation_degrees = rotation_deg
	return root
