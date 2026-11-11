extends RefCounted
const T := preload("res://tests/t.gd")


func test_standing_body_and_head() -> void:
	var s: Array[BCapsule] = BHitboxes.build(Vector3(1, 0, 2), 1.8, 0.4)
	T.eq(s.size(), 2)
	T.eq(s[0].zone, BCapsule.Zone.BODY)
	T.eq(s[1].zone, BCapsule.Zone.HEAD)
	T.vec(s[0].a, Vector3(1, 0.4, 2))
	T.approx(s[0].b.y + s[0].radius, 1.8 - 2.0 * BHitboxes.HEAD_RADIUS, 1e-4, "body top = head bottom")
	T.vec(s[1].a, Vector3(1, 1.8 - BHitboxes.HEAD_RADIUS, 2))
	T.approx(s[1].radius, BHitboxes.HEAD_RADIUS)


func test_shots_hit_expected_zone() -> void:
	var s: Array[BCapsule] = BHitboxes.build(Vector3.ZERO, 1.8, 0.4)
	var head: BRayHit = BRaycast.raycast_capsules(BRay.new(Vector3(0, 1.6, 10), Vector3(0, 0, -1)), s, 100.0)
	if T.not_null(head):
		T.eq(head.zone, BCapsule.Zone.HEAD)
		T.approx(head.distance, 10.0 - BHitboxes.HEAD_RADIUS, 1e-3)
	var chest: BRayHit = BRaycast.raycast_capsules(BRay.new(Vector3(0, 1.0, 10), Vector3(0, 0, -1)), s, 100.0)
	if T.not_null(chest):
		T.eq(chest.zone, BCapsule.Zone.BODY)
		T.approx(chest.distance, 9.6, 0.01)
	T.is_null(BRaycast.raycast_capsules(BRay.new(Vector3(0, 1.9, 10), Vector3(0, 0, -1)), s, 100.0), "over the head")
	T.is_null(BRaycast.raycast_capsules(BRay.new(Vector3(0.5, 1.0, 10), Vector3(0, 0, -1)), s, 100.0), "beside")


func test_crouched_capsule_stays_inside() -> void:
	var s: Array[BCapsule] = BHitboxes.build(Vector3.ZERO, 1.0, 0.4)
	T.approx(s[0].b.y, 0.4, 1e-4, "body collapses to a sphere")
	T.approx(s[1].a.y + s[1].radius, 1.0, 1e-4, "head top = capsule top")
