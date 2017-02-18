import math
import sys

sin = math.sin
cos = math.cos

## def accel(t): return (5.0 * sin(t))
## divergent with Euler, correct with all other integrators
def accel(t): return (5.0 * cos(t))
## def accel(t): return (5.0 * sin(t) * t*t)
## def accel(t): return (5.0 * cos(t) * t*t)
def blend(v, w, a): return (v * (1.0 - a) + w * (0.0 + a))


class euler_state:
	def __init__(self, p = 0.0, v = 0.0):
		self.p = p   ## position x(t)
		self.v = v   ## velocity v(t)

	def pos(self): return self.p
	def vel(self): return self.v
	def acc(self, t): return (accel(t))

class euler_integrator:
	## forward difference approximation to the first derivative; f(x+h)-f(x)
	## note: f(t+dt*0.5) is stable, f(t+dt*0) drifts right, f(t+dt*1) drifts left
	def istep(self, ps, ct, dt):
		ss = self.step(ps,  ct, dt)
		ps.p = ss.p
		ps.v = ss.v
		return ps

	def step(self, ps,  ct, dt):
		ss = euler_state()
		a = ps.acc(ct + dt * 0.0)

		## [semi-]implicit forward Euler
		ss.v = ps.v +    a * dt
		ss.p = ps.p + ss.v * dt
		return ss


class verlet_state:
	def __init__(self, p0 = 0.0, p1 = 0.0):
		self.p0 = p0
		self.p1 = p1
		self.v1 = 0.0

	def pos(self): return self.p1
	def vel(self): return self.v1
	def acc(self, t): return (accel(t))

class verlet_integrator:
	## central difference approximation to the second derivative; f(x+h*0.5)-f(x-0.5*h)
	## this is the Stormer-Verlet version, i.e. x(t+1) = 2.0*x(t) - x(t-1) + a(t)*dt*dt
	##   (x(t+1) - 2*x(t) + x(t-1)) / (dt*dt) = a(t)
	##   (x(t+1) - 2*x(t) + x(t-1))           = a(t)*(dt*dt)
	##   (x(t+1)                              = a(t)*(dt*dt) + 2*x(t) - x(t-1)
	## note that we can not simulate a first-order DE with Verlet integration
	def istep(self, ps,  ct, dt):
		ss = self.step(ps,  ct, dt)
		ps.p0 = ss.p0
		ps.p1 = ss.p1
		ps.v1 = ss.v1
		return ps

	def step(self, ps,  ct, dt):
		ss = verlet_state()

		a1 = ps.acc(ct + dt * 0.0)
		##! a2 = ps.acc(ct + dt * 1.0)
		p1 = ps.p0 + (ps.v1 * dt) + (a1 * dt * dt * 0.5)
		p2 = ps.p1 * 2.0 - ps.p0  + (a1 * dt * dt      )

		ss.p0 = blend(ps.p0, ps.p1, ct != 0.0)
		ss.p1 = blend(   p1,    p2, ct != 0.0)
		ss.v1 = (ss.p1 - ps.p0) / (dt * 2.0)

		return ss


class rkutta_state:
	def __init__(self, x = 0.0, v = 0.0):
		self.x = x   ## position x(t)
		self.v = v   ## velocity v(t)
	def __repr__(self):
		return ("<x=%f,v=%f>" % (self.x, self.v))
	def __eq__(self, s):
		return (self.x == s.x and self.v == s.v)
	def __add__(self, d):
		return (rkutta_state(self.x + d.v, self.v + d.a))

	def pos(self): return self.x
	def vel(self): return self.v
	def acc(self, t): return (accel(t))

class rkutta_deriv:
	def __init__(self, v = 0.0, a = 0.0):
		self.v = v   ## velocity v(t); first derivative of x(t)
		self.a = a   ## acceleration a(t); second derivative of x(t)
	def __repr__(self):
		return ("<v=%f,a=%f>" % (self.v, self.a))
	def __eq__(self, d):
		return (self.v == d.v and self.a == d.a)
	def __add__(self, d):
		return (rkutta_deriv(self.v + d.v, self.a + d.a))
	def __mul__(self, s):
		return (rkutta_deriv(self.v * s, self.a * s))

class rkutta_integrator:
	def __init__(self):
		self.a = rkutta_deriv()

	## def sample_state(self, ps, ds,  dt): return (ps + ds * dt)
	## def sample_deriv(self, ps,  ct): return (rkutta_deriv(ps.v, ps.acc(ct)))

	def sample_state_deriv(self, ps, ds,  ct, dt):
		## Euler-step state from s(t) to s(t) + ds*dt; also sample acceleration at ct+dt
		## u = s + ds*dt = (s.x, s.v) + (ds.v, ds.a)*dt = (s.x + ds.v*dt, s.v + ds.a*dt)
		psu = ps + ds * dt
		psa = ps.acc(ct + dt)
		return (rkutta_deriv(psu.v, psa))

	def blend_deriv_samples(self, a, b, c, d, w0 = 0.16666667, w1 = 0.33333334):
		## return ((a * w0) + (b + c) * w1 + (d * w0))
		return ((a + (b + c) * 2.0 + d) * w0)


	def step_euler(self, ps,  ct, dt):
		## d1 = (v = s.v + d0.a*dt, a =       f(ct+dt)) = (      s.v   ,       f(ct+dt)   )
		## s' = (x = s.x + d1.v*dt, v = s.v + d1.a*dt)  = (s.x + s.v*dt, s.v + f(ct+dt)*dt)
		ds0 = self.a
		ds1 = self.sample_state_deriv(ps, ds0,  ct, dt * 0.0)
		assert(ps.v == ds1.v)
		return (ps + ds1 * dt)

	def istep(self, ps,  ct, dt):
		ss = self.step(ps,  ct, dt)
		ps.x = ss.x
		ps.v = ss.v
		return ps
	def step(self, ps,  ct, dt):
		## return (self.step_euler(ps,  ct, dt))

		ds0 = self.a
		ds1 = self.sample_state_deriv(ps, ds0,  ct, dt * 0.0)
		ds2 = self.sample_state_deriv(ps, ds1,  ct, dt * 0.5)
		ds3 = self.sample_state_deriv(ps, ds2,  ct, dt * 0.5)
		ds4 = self.sample_state_deriv(ps, ds3,  ct, dt * 1.0)

		## ds1 = self.sample_deriv(self.sample_state(ps, ds0, dt * 0.0),  ct + dt * 0.0)
		## ds2 = self.sample_deriv(self.sample_state(ps, ds1, dt * 0.5),  ct + dt * 0.5)
		## ds3 = self.sample_deriv(self.sample_state(ps, ds2, dt * 0.5),  ct + dt * 0.5)
		## ds4 = self.sample_deriv(self.sample_state(ps, ds3, dt * 1.0),  ct + dt * 1.0)

		return (ps + self.blend_deriv_samples(ds1, ds2, ds3, ds4) * dt)




def approximate(f,  st, dt,  n):
	v = [None] * n
	w = [None] * n

	## initial values
	ft = f(t)
	se = 0.0

	## def  j(t): return (5.0 * sin(t))
	## def dj(t): return (5.0 * cos(t))

	## numerical derivative (df/dt); forward difference limit
	def dfdt(f, t, h = 0.0000001): return ((f(t + h) - f(t)) / h)

	## piece-wise linearly approximate f by walking along its tangents
	for i in xrange(n):
		v[i] = (ct, ft)
		w[i] = (ct, f(t))

		## step y=ft by dy=df and x=t by dx=dt, track sum-squared error
		ct  = st + i * dt
		df  = dfdt(f, ct) * dt
		ft += df
		se += ((w[i][1] - v[i][1]) ** 2.0)

	return (v, w, se)

def integrate(ps, bi,  st, dt,  n):
	v = [(st, ps)] + [None] * n
	k = 1

	assert(len(v) == (n + 1))
	while (k < len(v)):
		ct = st + (k - 1) * dt
		ps = bi.step(ps,  ct, dt)

		## keep a copy of the state after each integration step; fails with istep()
		v[k] = (ct, ps)

		## this accumulates too much error
		## t += dt
		k += 1

	return v


def integrate_euler(st, dt,  n):
	ps = euler_state()
	ei = euler_integrator()

	return (integrate(ps, ei,  st, dt,  n))

def integrate_verlet(st, dt,  n):
	ps = verlet_state()
	vi = verlet_integrator()

	return (integrate(ps, vi,  st, dt,  n))

def integrate_rkutta(st, dt,  n):
	ps = rkutta_state()
	ri = rkutta_integrator()

	return (integrate(ps, ri,  st, dt,  n))


def run(ifunc, st, dt,  n):
	sdata = ifunc(st, dt,  n)
	pfile = open("%s_p.dat" % (ifunc.__name__), 'w')
	vfile = open("%s_v.dat" % (ifunc.__name__), 'w')

	for (ct, ps) in sdata:
		pfile.write("%f\t%f\n" % (ps.pos(), ct))
		vfile.write("%f\t%f\n" % (ps.vel(), ct))

	pfile.close()
	vfile.close()

def main(argc, argv):
	int_funcs = [integrate_euler, integrate_verlet, integrate_rkutta]

	iter_count = ((argc > 1 and   int(argv[1])) or 1000)
	start_time = ((argc > 2 and float(argv[2])) or 0.0)
	delta_time = ((argc > 3 and float(argv[3])) or 0.1)

	for int_func in int_funcs:
		run(int_func, start_time, delta_time, iter_count)

	return 0

if (__name__ == "__main__"):
	sys.exit(main(len(sys.argv), sys.argv))

