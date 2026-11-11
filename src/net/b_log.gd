class_name BLog
extends RefCounted
## One-line logs with the peer role, e.g. "[server] EVENT dash peer=123". The smoke test greps these.


## True while mp has a live connection (get_unique_id() / is_server() are valid).
static func connected(mp: MultiplayerAPI) -> bool:
	return mp != null and mp.has_multiplayer_peer() and not mp.multiplayer_peer is OfflineMultiplayerPeer \
		and mp.multiplayer_peer.get_connection_status() == MultiplayerPeer.CONNECTION_CONNECTED


static func role(mp: MultiplayerAPI) -> String:
	if not connected(mp):
		return "offline"
	if mp.is_server():
		return "server" if Session.dedicated else "host"
	return "client %d" % mp.get_unique_id()


static func info(mp: MultiplayerAPI, msg: String) -> void:
	print("[%s] %s" % [role(mp), msg])
