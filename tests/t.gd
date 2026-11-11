extends RefCounted
## Tiny assert helper. Tests call T.approx(...) etc.; failures are recorded with "file::test".

static var current: String = ""
static var failures: Array[String] = []
static var checks: int = 0


static func _fail(msg: String) -> void:
	failures.append("%s: %s" % [current, msg])


static func approx(actual: float, expected: float, eps: float = 1e-4, label: String = "") -> void:
	checks += 1
	var tol: float = eps * maxf(1.0, absf(expected))
	if not (absf(actual - expected) <= tol):
		_fail("%s expected %s ~ %s (eps %s)" % [label, actual, expected, eps])


static func vec(actual: Vector3, expected: Vector3, eps: float = 1e-4, label: String = "") -> void:
	checks += 1
	var tol: float = eps * maxf(1.0, expected.length())
	if not ((actual - expected).length() <= tol):
		_fail("%s expected %s ~ %s (eps %s)" % [label, actual, expected, eps])


static func eq(actual: Variant, expected: Variant, label: String = "") -> void:
	checks += 1
	if actual != expected:
		_fail("%s expected %s == %s" % [label, str(actual), str(expected)])


static func is_true(cond: bool, label: String = "") -> void:
	checks += 1
	if not cond:
		_fail("%s expected true" % label)


static func is_false(cond: bool, label: String = "") -> void:
	checks += 1
	if cond:
		_fail("%s expected false" % label)


static func is_null(v: Variant, label: String = "") -> void:
	checks += 1
	if v != null:
		_fail("%s expected null, got %s" % [label, str(v)])


static func not_null(v: Variant, label: String = "") -> bool:
	checks += 1
	if v == null:
		_fail("%s expected non-null" % label)
		return false
	return true
