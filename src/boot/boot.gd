extends Node
## Main scene: command-line modes go straight into a match, otherwise the main menu.


func _ready() -> void:
	var target: String = "res://scenes/game.tscn" if Session.has_cli_mode() else "res://scenes/main_menu.tscn"
	get_tree().change_scene_to_file.call_deferred(target)
