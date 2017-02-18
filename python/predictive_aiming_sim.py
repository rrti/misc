"""
implements a simulation of predictive aiming
positions and velocities are specified in R3

	let target position be pos_tgt
	let target velocity be vel_tgt (assumed constant)

	let parameterized position of target at time t be P(t) = pos_tgt + vel_tgt * t

	let attacker position be pos_att
	let attacker velocity be vel_att (assumed constant)
	let attacker projectile speed be spd_pro (assumed constant)

	attacker can hit target if trajectory of target
	intersects with a sphere of radius (t * spd_pro)
	TWICE

	assume without any loss of generality that pos_att and vel_att are
	zero-vectors (if not, we can convert them: pos_tgt -= pos_att and
	vel_tgt -= vel_att), then the distance to the parameterized target
	position P(t) is sqrt(dot(P, P)) which must equal (spd_pro * t)

	i.e.
		dot_pp = dot(pos_tgt, pos_tgt)
		dot_pv = dot(pos_tgt, vel_tgt)
		dot_vv = dot(vel_tgt, vel_tgt)

		dot(P(t), P(t)) = P(t) * P(t)

		dot(P(t), P(t)) =
			(pos_tgt.x + vel_tgt.x*t) * (pos_tgt.x + vel_tgt.x*t) +
			(pos_tgt.y + vel_tgt.y*t) * (pos_tgt.y + vel_tgt.y*t) +
			(pos_tgt.z + vel_tgt.z*t) * (pos_tgt.z + vel_tgt.z*t)
		dot(P(t), P(t)) =
			(pos_tgt.x*pos_tgt.x + pos_tgt.x*vel_tgt.x*2.0*t + vel_tgt.x*vel_tgt.x*t*t) +
			(pos_tgt.x*pos_tgt.x + pos_tgt.x*vel_tgt.x*2.0*t + vel_tgt.x*vel_tgt.x*t*t) +
			(pos_tgt.x*pos_tgt.x + pos_tgt.x*vel_tgt.x*2.0*t + vel_tgt.x*vel_tgt.x*t*t)
		dot(P(t), P(t)) =
			dot_pp + dot_pv*2.0*t + dot_vv*t*t

		now solve
			dot_pp + dot_pv*2.0*t +  dot_vv                    *t*t  ==  spd_pro*spd_pro*t*t
			dot_pp + dot_pv*2.0*t +  dot_vv                    *t*t  -   spd_pro*spd_pro*t*t  == 0
			dot_pp + dot_pv*2.0*t + (dot_vv - spd_pro*spd_pro) *t*t  -                        == 0

		this is a simple quadratic, so let
			A = dot_vv - (spd_pro*spd_pro)
			B = dot_pv * 2.0
			C = dot_pp
		then the ABC-formula gives us
			D = (B * B) - (4 * A * C)
			t = (B +/- sqrt(D)) / (A + A)

		in the special case that (vel_tgt DOT vel_tgt) equals (spd_pro*spd_pro),
		A will be zero and we fall back to solving the linear polynomial for t:

			dot_pp + dot_pv*2.0*t  ==  0
		             dot_pv*2.0*t  == -dot_pp
		                        t  == -dot_pp / (dot_pv*2.0)
		                        t  == -C / B

		eg. if target starts at <3, 3, 0> and moves with velocity <-1, 0, 0>,
		and attacker starts at <0, 0, 0> shooting projectiles with speed 1.0
		vertically, there will be an intersection at <0, 3, 0> for t = 3
"""



import sys

def vec_dot(v, w): return ((v[0] * w[0]) + (v[1] * w[1]) + (v[2] * w[2]))
def vec_add(v, w): return ((v[0] + w[0]), (v[1] + w[1]), (v[2] + w[2]))
def vec_mul(v, s): return (v[0] * s, v[1] * s, v[2] * s)
def vec_len(v): return (vec_dot(v, v) ** 0.5)
def vec_dst(v, w): z = (v[0] - w[0], v[1] - w[1], v[2] - w[2]); return vec_len(z)
def vec_nrm(v): s = vec_len(v); return vec_mul(v, 1.0 / s)



def solve_quadratic_eq(a, b, c):
	d = b*b - (4.0*a*c)
	t0 = -1.0
	t1 = -1.0

	if (a != 0.0):
		if (d > 0.0):
			rd = d ** 0.5
			t0 = (-b - rd) / (a + a)
			t1 = (-b + rd) / (a + a)
		elif (d == 0.0):
			t0 = -b / (a + a)
	else:
		## special case
		if (b != 0.0):
			t0 = -c / b

	return (t0, t1)


def aiming_sim(pos_tgt, vel_tgt,  pos_pro, vel_pro,  time_max, delta_time,  verbose):
	dist_min = 1e30
	time_min = 1e30
	time_cur = 0.0

	## linearly advance target and projectile time to find
	## the point in time at which the distance to target is
	## minimized
	while (time_cur < time_max):
		## p = v*dt
		pos_pro_t = vec_add(pos_pro, vec_mul(vel_pro, time_cur))
		pos_tgt_t = vec_add(pos_tgt, vec_mul(vel_tgt, time_cur))
		dst_tgt_t = vec_dst(pos_pro_t, pos_tgt_t)

		if (dst_tgt_t < dist_min):
			dist_min = dst_tgt_t
			time_min = time_cur

		time_cur += delta_time

	if (verbose):
		print("[aiming_sim] dist_min=%.2f time_min=%.2f time_max=%f" % (dist_min, time_min, time_max))

	return (dist_min, dist_min)

def find_firing_solution(params):
	verbose = params["verbose"]

	## target speed and attacker's PROJECTILE speed
	## (both assumed to remain constant over time!)
	spd_tgt = params["spd_tgt"]
	spd_pro = params["spd_pro"]

	## target position/direction
	pos_tgt = params["pos_tgt"]
	dir_tgt = params["dir_tgt"]

	## attacker position/velocity
	pos_att = params["pos_att"]
	vel_att = params["vel_att"]

	## target velocity
	vel_tgt = vec_mul(dir_tgt, spd_tgt)

	A = vec_dot(vel_tgt, vel_tgt) - (spd_pro * spd_pro)
	B = vec_dot(pos_tgt, vel_tgt) * 2.0
	C = vec_dot(pos_tgt, pos_tgt)
	D = (B * B) - (4.0 * A * C)

	## positions to aim at
	aim_pos_min = (-1.0, -1.0, -1.0); t0 = -1.0
	aim_pos_max = (-1.0, -1.0, -1.0); t1 = -1.0

	## find t's at which target trajectory
	## intersects the sphere of opportunity
	(tt0, tt1) = solve_quadratic_eq(A, B, C)

	if (verbose):
		print("[find_firing_solution] tt0=%f tt1=%f" % (tt0, tt1))

	## bail early if no solution possible
	if (tt0 < 0.0 and tt1 < 0.0):
		return (tt0, tt1)

	aim_pos_min = vec_add(pos_tgt, vec_mul(vel_tgt, tt0))
	aim_pos_max = vec_add(pos_tgt, vec_mul(vel_tgt, tt1))



	if (False):
		if (D > 0.0):
			if (A != 0.0):
				rD = D ** 0.5
				t0 = (-B - rD) / (A + A)
				t1 = (-B + rD) / (A + A)
			else:
				## special case, actually only ONE solution
				t0 = -(C / B)

			aim_pos_min = vec_add(pos_tgt, vec_mul(vel_tgt, t0))
			aim_pos_max = vec_add(pos_tgt, vec_mul(vel_tgt, t1))

			if (verbose):
				print("[find_firing_solution] two targeting solutions (t0=%f,t1=%f)" % (t0, t1))
		elif (D == 0.0):
			t0 = -B / (A + A)
			## t1 = -B / (A + A)

			aim_pos_min = vec_add(pos_tgt, vec_mul(vel_tgt, t0))
			## aim_pos_max = vec_add(pos_tgt, vec_mul(vel_tgt, t1))

			if (verbose):
				print("[find_firing_solution] one targeting solution (t0=%f)" % (t0))
		else:
			if (verbose):
				print("[find_firing_solution] no targeting solutions")

			return (-1.0, -1.0)

		assert(tt0 == t0)
		assert(tt1 == t1)



	## figure out how far the target will be from us at
	## the points of intersection (with the opportunity
	## sphere)
	dst_aim_pos_min = vec_len(aim_pos_min)
	dst_aim_pos_max = vec_len(aim_pos_max)
	## normalize to get direction vectors to target positions
	dir_att0 = vec_mul(aim_pos_min, 1.0 / dst_aim_pos_min)
	dir_att1 = vec_mul(aim_pos_max, 1.0 / dst_aim_pos_max)

	## now derive the projectile transit-times to aim_pos_{min,max}
	##
	## (the projectile should be able to arrive at these positions
	## either BEFORE or JUST AS the target does, so that t{0,1}pro
	## <= t{0,1}: numerical accuracy means we can not assert this)
	pro_t0 = dst_aim_pos_min / spd_pro
	pro_t1 = dst_aim_pos_max / spd_pro

	## assert((t0 >= 0.0 and t0pro <= t0) or t0 < 0.0)
	## assert((t1 >= 0.0 and t1pro <= t1) or t1 < 0.0)

	## verify the results numerically
	if (tt0 >= 0.0): aiming_sim(pos_tgt, vel_tgt,  pos_att, vec_mul(dir_att0, spd_pro),  max(tt0, pro_t0), 0.01,  verbose)
	if (tt1 >= 0.0): aiming_sim(pos_tgt, vel_tgt,  pos_att, vec_mul(dir_att1, spd_pro),  max(tt1, pro_t1), 0.01,  verbose)

	return (tt0, tt1)

def main(argc, argv):
	params = {}

	params["verbose"] = True

	params["spd_tgt"] = 1.0
	params["spd_pro"] = 1.0

	params["pos_tgt"] = ( 3, 3, 0)
	params["dir_tgt"] = (-1, 0, 0)

	params["pos_att"] = (0, 0, 0)
	params["vel_att"] = (0, 0, 0)

	find_firing_solution(params)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

