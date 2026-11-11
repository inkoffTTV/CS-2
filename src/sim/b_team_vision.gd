class_name BTeamVision
extends RefCounted
## Server-side team vision: which enemies each team currently sees.

var _config: BVisionConfig
var _last_update: float = 0.0
var _actors: Dictionary = {}  # id -> Vector2i(team, hidden ? 1 : 0)
var _last_seen: Dictionary = {}  # target id -> { team -> time }


func _init(config: BVisionConfig = null) -> void:
	_config = config if config != null else BVisionConfig.new()


## has_line_of_sight: Callable(observer: BVisionActor, target: BVisionActor) -> bool
func update(now: float, actors: Array[BVisionActor], has_line_of_sight: Callable) -> void:
	_last_update = now
	_actors.clear()
	for a in actors:
		_actors[a.id] = Vector2i(a.team, 1 if a.hidden else 0)

	# Forget ids that are gone so a re-appearing id gets no inherited grace.
	for target_id in _last_seen.keys():
		if not _actors.has(target_id):
			_last_seen.erase(target_id)

	for target in actors:
		for obs in actors:
			if not obs.observer or obs.team == target.team:
				continue
			var vision_range: float = _config.hidden_reveal_radius if target.hidden else obs.sight_radius
			if (target.pos - obs.pos).length_squared() > vision_range * vision_range:
				continue
			if not bool(has_line_of_sight.call(obs, target)):
				continue
			if not _last_seen.has(target.id):
				_last_seen[target.id] = {}
			(_last_seen[target.id] as Dictionary)[obs.team] = now


func is_visible_to_team(target_id: int, team: int) -> bool:
	if not _actors.has(target_id):
		return false
	var known: Vector2i = _actors[target_id]
	if known.x == team:
		return true
	if not _last_seen.has(target_id):
		return false
	var per_team: Dictionary = _last_seen[target_id]
	if not per_team.has(team):
		return false
	var grace: float = 0.0 if known.y == 1 else _config.grace_seconds
	return _last_update - float(per_team[team]) <= grace
