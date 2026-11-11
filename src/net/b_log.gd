class_name BLog
extends RefCounted
## One-line logs with the peer role, e.g. "[server] EVENT dash peer=123". The smoke test greps these.


static func role(mp: MultiplayerAPI) -> String:
	if mp == null or not mp.has_multiplayer_peer() or mp.multiplayer_peer is OfflineMultiplayerPeer:
		return "offline"
	if mp.is_server():
		return "server" if Session.dedicated else "host"
	return "client %d" % mp.get_unique_id()


static func info(mp: MultiplayerAPI, msg: String) -> void:
	print("[%s] %s" % [role(mp), msg])
