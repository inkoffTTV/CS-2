extends RefCounted
const T := preload("res://tests/t.gd")
const SessionScript := preload("res://src/net/session.gd")


func test_parse_endpoint_with_port() -> void:
	var ep: Array = SessionScript.parse_endpoint("192.168.1.5:7000")
	T.eq(ep[0], "192.168.1.5")
	T.eq(ep[1], 7000)


func test_parse_endpoint_defaults() -> void:
	var ep: Array = SessionScript.parse_endpoint("example.org")
	T.eq(ep[0], "example.org")
	T.eq(ep[1], 7777)
	var only_port: Array = SessionScript.parse_endpoint(":9000")
	T.eq(only_port[0], "127.0.0.1")
	T.eq(only_port[1], 9000)


func test_sanitize_nick() -> void:
	T.eq(SessionScript.sanitize_nick("   "), "Nameless")
	T.eq(SessionScript.sanitize_nick("  Brutus "), "Brutus")
	T.eq(SessionScript.sanitize_nick("abcdefghijklmnopqrstuvwxyz").length(), 16)
