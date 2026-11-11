class_name BTeams
extends RefCounted
## The two sides of BRUTUS (docs/design.md, "Стороны").

const OPTIMATES: int = 0
const POPULARES: int = 1
const NONE: int = -1

const COLOR_OPTIMATES := Color(0.42, 0.36, 1.0)
const COLOR_POPULARES := Color(1.0, 0.45, 0.1)


static func team_name(team: int) -> String:
	match team:
		OPTIMATES:
			return "Optimates"
		POPULARES:
			return "Populares"
	return "None"


static func team_color(team: int) -> Color:
	match team:
		OPTIMATES:
			return COLOR_OPTIMATES
		POPULARES:
			return COLOR_POPULARES
	return Color.WHITE


## Team for a new player given the current member teams: the smaller side, Optimates on a tie.
static func pick_team(member_teams: Array[int]) -> int:
	var counts: Array[int] = [0, 0]
	for t in member_teams:
		if t == OPTIMATES or t == POPULARES:
			counts[t] += 1
	return POPULARES if counts[POPULARES] < counts[OPTIMATES] else OPTIMATES


## Index of the spawn point to use for a team member: rotates through the team's points.
static func spawn_index(members_already_in_team: int, spawn_count: int) -> int:
	if spawn_count <= 0:
		return -1
	return members_already_in_team % spawn_count
