extends SceneTree
## Run: godot --headless --path . -s res://tests/run_tests.gd
## Discovers tests/test_*.gd; every method named test_* is a test. Exit code 1 on any failure.

const T := preload("res://tests/t.gd")


func _init() -> void:
	var files: Array[String] = []
	var dir := DirAccess.open("res://tests")
	if dir == null:
		printerr("cannot open res://tests")
		quit(1)
		return
	for f in dir.get_files():
		if f.begins_with("test_") and f.ends_with(".gd"):
			files.append(f)
	files.sort()

	var tests_run := 0
	var load_errors := 0
	for f in files:
		var script: Variant = load("res://tests/" + f)
		if script == null or not (script is GDScript) or not (script as GDScript).can_instantiate():
			load_errors += 1
			T.failures.append("%s: failed to load/compile" % f)
			continue
		var inst: Object = (script as GDScript).new()
		var names: Array[String] = []
		for m in inst.get_method_list():
			var n: String = m["name"]
			if n.begins_with("test_"):
				names.append(n)
		names.sort()
		for n in names:
			T.current = "%s::%s" % [f, n]
			tests_run += 1
			inst.call(n)

	for fail in T.failures:
		printerr("FAIL " + fail)
	var failed: int = T.failures.size()
	print("%d tests, %d checks, %d failures (%d files, %d load errors)" % [tests_run, T.checks, failed, files.size(), load_errors])
	quit(1 if failed > 0 else 0)
