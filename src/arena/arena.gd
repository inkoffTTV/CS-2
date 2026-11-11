class_name BArena
extends Node3D
## Greybox arena in metres (port of BrutusGreyboxArena.cpp, Unreal cm/Z-up -> Godot m/Y-up:
## x = UE.X / 100, y = UE.Z / 100, z = UE.Y / 100). Optimates (team 0) hold the -X half,
## Populares (team 1) the +X half; the centre wall is "the Rubicon".

const GROUP: StringName = &"arena"

const SIZE: float = 60.0
const WALL_HEIGHT: float = 4.0
const COVER_SPOTS: Array[Vector2] = [
	Vector2(-15, -15), Vector2(-15, 15), Vector2(15, -15), Vector2(15, 15),
	Vector2(-22, 0), Vector2(22, 0), Vector2(-8, 22), Vector2(8, -22)]
const PLATFORM_CENTER := Vector3(-25, 3, 25)
const ZIPLINE_A := Vector3(-25, 8.5, 25)
const ZIPLINE_B := Vector3(24, 4.5, -24)

## Spawn transforms per team: origin at the feet, yaw faces the Rubicon.
var spawns: Array[Array] = [[], []]
var ziplines: Array[BZiplineSegment] = []

var _geo: Node3D


func _ready() -> void:
	add_to_group(GROUP)
	_geo = Node3D.new()
	_geo.name = "Geometry"
	add_child(_geo)
	_build_layout()
	_build_spawns()
	BArenaDressing.build(self)
	BArenaEnvironment.build(self)


static func find(tree: SceneTree) -> BArena:
	return tree.get_first_node_in_group(GROUP) as BArena


func get_spawn(team: int, index: int) -> Transform3D:
	var list: Array = spawns[clampi(team, 0, 1)]
	return list[BTeams.spawn_index(index, list.size())] as Transform3D


func _build_layout() -> void:
	var half: float = SIZE * 0.5
	BArenaKit.box(_geo, "Floor", Vector3(0, -0.5, 0), Vector3(SIZE, 1, SIZE), BArenaMaterials.get_material("floor"))

	var wall: Material = BArenaMaterials.get_material("wall")
	var h: float = WALL_HEIGHT
	BArenaKit.box(_geo, "WallNorth", Vector3(half, h * 0.5, 0), Vector3(1, h, SIZE), wall)
	BArenaKit.box(_geo, "WallSouth", Vector3(-half, h * 0.5, 0), Vector3(1, h, SIZE), wall)
	BArenaKit.box(_geo, "WallEast", Vector3(0, h * 0.5, half), Vector3(SIZE, h, 1), wall)
	BArenaKit.box(_geo, "WallWest", Vector3(0, h * 0.5, -half), Vector3(SIZE, h, 1), wall)

	# The Rubicon: centre wall with two 6 m passages (z in [-12, -6] and [6, 12]).
	var rubicon: Material = BArenaMaterials.get_material("rubicon")
	BArenaKit.box(_geo, "RubiconA", Vector3(0, h * 0.5, -21), Vector3(0.5, h, 18), rubicon)
	BArenaKit.box(_geo, "RubiconB", Vector3(0, h * 0.5, 0), Vector3(0.5, h, 12), rubicon)
	BArenaKit.box(_geo, "RubiconC", Vector3(0, h * 0.5, 21), Vector3(0.5, h, 18), rubicon)

	# Covers 2 x 1.5 x 2 m, coloured by the half they stand in.
	for i in COVER_SPOTS.size():
		var spot: Vector2 = COVER_SPOTS[i]
		var team: int = BTeams.OPTIMATES if spot.x < 0.0 else BTeams.POPULARES
		BArenaKit.box(_geo, "Cover%d" % i, Vector3(spot.x, 0.75, spot.y), Vector3(2, 1.5, 2), BArenaMaterials.team(team))

	# Slide ramps as solid wedges: 20 degrees free-standing (rises toward +X), 30 degrees up to the platform.
	var ramp: Material = BArenaMaterials.get_material("ramp")
	var len20: float = 15.0
	var run20: float = len20 * cos(deg_to_rad(20.0))
	var rise20: float = len20 * sin(deg_to_rad(20.0))
	BArenaKit.wedge(_geo, "Ramp20", Vector3(-15, rise20 * 0.5, -26), Vector3(run20, rise20, 4), ramp, true)

	var platform_h: float = 6.0
	var run30: float = platform_h / tan(deg_to_rad(30.0))
	var platform_edge_x: float = PLATFORM_CENTER.x + 3.0
	BArenaKit.wedge(_geo, "Ramp30", Vector3(platform_edge_x + run30 * 0.5, platform_h * 0.5, 25), Vector3(run30, platform_h, 4),
		ramp, false)
	BArenaKit.box(_geo, "Platform", PLATFORM_CENTER, Vector3(6, platform_h, 6), BArenaMaterials.team(BTeams.OPTIMATES))

	ziplines.append(BZiplineSegment.new(ZIPLINE_A, ZIPLINE_B))


func _build_spawns() -> void:
	for team in 2:
		var sign_x: float = -1.0 if team == BTeams.OPTIMATES else 1.0
		var yaw: float = sign_x * PI * 0.5  # face the Rubicon (team 0 looks toward +X)
		for z: float in [-5.0, 5.0]:
			spawns[team].append(Transform3D(Basis(Vector3.UP, yaw), Vector3(sign_x * 20.0, 0.05, z)))
