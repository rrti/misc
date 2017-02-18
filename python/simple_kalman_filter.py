"""
when designing a Kalman Filter we need two things
	1) state transition function (matrix)
	2) measurement function (matrix)

	simple exampe for one-dimensional motion:
		(x', v')^T <-- F * (x, v)
		(z     )   <-- H * (x, v)

	where F is a 2x2 matrix (we know that x' = x + 1*v and v' = v)
		(1, 1)
		(0, 1)
	or more generally (given a time-step interval dt such that x' = x + v*dt)
		(1, dt)
		(0,  1)
	and H is a 1x2 matrix (we only observe position x, not velocity v)
		(1, 0)

	general (LA) equations are:
		x  := STATE (not just position) at time-step t
		x' := STATE (not just position) at time-step t+1
		F  := state-transition matrix
		P  := uncertainty covariance matrix
		u  := motion-vector
		z  := measurement at time-step t
		y  := error between measurement and prediction, compares measurement to prediction
		H  := measurement-transition matrix (?), maps STATES to MEASUREMENTS
		S  := uncertainty (mapped into measurement-space)
		R  := measurement noise
		K  := Kalman Gain
		I  := identity matrix

		prediction update step
			x' = F * x + u
			P' = F * P * F^T
		measurement update step
			y = z - H * x
			S = H * P * H^T + R
			K = P * H^T * S^-1

			x' = x + K * y
			P' = (I - K * H) * P

	for motion in two dimensions (state=<<px,py>, <vx,vy>>)
	with time-step interval dt we have
        (px')    (1 0 dt  0)   (px)
        (py')    (0 1  0 dt)   (py)
              =  (         ) * (  )
        (vx')    (0 0  1  0)   (vx)
        (vy')    (0 0  0  1)   (vy)

	interesting idea: when parts of the state are independent
	(eg. velocity in x-direction and in y-direction) we can use
	two 2D filters instead of one 4D filter

	higher-order derivatives of state variables can be incorporated into
	filter as well, eg. acceleration derived from velocity derived from
	position
		x' = 1.0 * x  +  d_t * v_x  +  0.5 * d_t^2 * a_x
		v' = 0.0 * x  +  1.0 * v_x  +        d_t   * a_x
		a' = 0.0 * x  +  0.0 * v_x  +  1.0         * a_x

	if the equations of motion (or whatever is being simulated) are
	non-linear, an Extended Kalman Filter must be used to model them
	(the F-matrix can ONLY capture linear relations), otherwise only
	approximations can be made eg. by assuming constant acceleration
"""


import sys
import math


class matrix:
	def __init__(self, array):
		self.array = array
		self.dimx = len(array)
		self.dimy = len(array[0])

		if (array == [[]]):
			self.dimx = 0

	def __repr__(self):
		s = "\n"
		for i in xrange(self.dimx):
			s += ("\t%s\n" % self.array[i])
		return s


	def make_zero_matrix(self, dimx, dimy):
		assert(dimx >= 1 and dimy >= 1)
		return (matrix( [[0 for row in xrange(dimy)] for col in xrange(dimx)] ))

	def make_unit_matrix(self, dim):
		assert(dim >= 1)
		return (matrix( [[1 for row in xrange(dim)] for col in xrange(dim)] ))


	def __add__(self, other):
		assert(self.dimx == other.dimx and self.dimy == other.dimy)
		res = self.make_zero_matrix(self.dimx, self.dimy)

		for i in xrange(self.dimx):
			for j in xrange(self.dimy):
				res.array[i][j] = self.array[i][j] + other.array[i][j]

		return res

	def __sub__(self, other):
		assert(self.dimx == other.dimx and self.dimy == other.dimy)
		res = self.make_zero_matrix(self.dimx, self.dimy)

		for i in xrange(self.dimx):
			for j in xrange(self.dimy):
				res.array[i][j] = self.array[i][j] - other.array[i][j]

		return res

	def __mul__(self, other):
		assert(self.dimy == other.dimx)
		res = self.make_zero_matrix(self.dimx, other.dimy)

		for i in xrange(self.dimx):
			for j in xrange(other.dimy):
				for k in xrange(self.dimy):
					res.array[i][j] += self.array[i][k] * other.array[k][j]

		return res

	def transpose(self):
		res = self.make_zero_matrix(self.dimy, self.dimx)

		for i in xrange(self.dimx):
			for j in xrange(self.dimy):
				res.array[j][i] = self.array[i][j]

		return res

	def Cholesky(self, ztol = 1.0e-5):
		## computes the upper triangular Cholesky factorization of a positive definite matrix
		res = self.make_zero_matrix(self.dimx, self.dimx)

		for i in xrange(self.dimx):
			S = sum([(res.array[k][i]) ** 2 for k in xrange(i)])
			d = self.array[i][i] - S

			if (abs(d) < ztol):
				res.array[i][i] = 0.0
			else:
				## matrix must be positive-definite
				assert(d >= 0.0)
				res.array[i][i] = d ** 0.5

			for j in xrange(i + 1, self.dimx):
				S = sum([res.array[k][i] * res.array[k][j] for k in xrange(self.dimx)])

				if (abs(S) < ztol):
					S = 0.0

				res.array[i][j] = (self.array[i][j] - S) / res.array[i][i]

		return res

	def CholeskyInverse(self):
		## computes inverse of matrix given its Cholesky upper Triangular decomposition
		res = self.make_zero_matrix(self.dimx, self.dimx)

		## backward step
		for j in reversed(xrange(self.dimx)):
			tjj = self.array[j][j]
			S = sum([self.array[j][k] * res.array[j][k] for k in xrange(j + 1, self.dimx)])
			res.array[j][j] = 1.0 / (tjj ** 2.0) - S / tjj

			for i in reversed(xrange(j)):
				v = -sum([self.array[i][k] * res.array[k][j] for k in xrange(i + 1, self.dimx)]) / self.array[i][i]
				res.array[j][i] = v
				res.array[i][j] = v

		return res

	def inverse(self):
		aux = self.Cholesky()
		res = aux.CholeskyInverse()
		return res




def kalman_filter_1d(M, X, P, U, F, H, R, I):
	print("[kalman_filter_1d]")

	## constants
	HT = H.transpose()
	FT = F.transpose()

	## apply filter for each measurement
	for n in xrange(len(M)):
		## z is a scalar but must be boxed
		## to a 1x1 matrix to calculate y
		z = M[n]
		Z = matrix([[ z ]])

		## measurement update
		##   y = z - H * x
		##   S = H * P * H^T + R
		##   K = P * H^T * S^-1
		##   x' = x + K * y
		##   P' = (I - K * H) * P
		y = Z - (H * X)
		S = (H * P * HT) + R
		K = P * HT * S.inverse()
		P = (I - (K * H)) * P
		X = X + (K * y)

		## prediction update
		##   X' = F * X + U
		##   P' = F * P * F^T
		X = (F * X) + U
		P = F * P * FT

		print("\t[n=%d] x=%s" % (n, X))
		print("\t[n=%d] P=%s" % (n, P))

def kalman_filter_2d(M, X, P, U, F, H, R, I):
	print("[kalman_filter_2d]")

	## constants
	HT = H.transpose()
	FT = F.transpose()

	## apply filter for each measurement
	for n in xrange(len(M)):
		Z = matrix([ M[n] ])

		## prediction update
		X = (F * X) + U
		P = F * P * FT

		## measurement update
		y = Z.transpose() - (H * X)
		S = (H * P * HT) + R
		K = P * HT * S.inverse()
		P = (I - (K * H)) * P
		X = X + (K * y)

		print("\t[n=%d] x=%s" % (n, X))
		print("\t[n=%d] P=%s" % (n, P))



def test_kalman_filter_1d(dt = 1.0):
	M = [1, 2, 3]                              ## measurements

	X = matrix([[0.0], [0.0]])                 ## initial state (location and velocity)
	P = matrix([[1000.0, 0.0], [0.0, 1000.0]]) ## initial uncertainty
	U = matrix([[0.0], [0.0]])                 ## external motion
	F = matrix([[1.0, 1.0], [0.0, 1.0]])       ## next-state function
	H = matrix([[1.0, 0.0]])                   ## measurement function
	R = matrix([[1.0]])                        ## measurement uncertainty
	I = matrix([[1.0, 0.0], [0.0, 1.0]])       ## identity matrix

	kalman_filter_1d(M, X, P, U, F, H, R, I)

def test_kalman_filter_2d(dt = 0.1):
	M = [
		[ 5.0, 10.0],
		[ 6.0,  8.0],
		[ 7.0,  6.0],
		[ 8.0,  4.0],
		[ 9.0,  2.0],
		[10.0,  0.0]
	]
	XY = [4.0, 12.0]


	M = [
		[ 1.0,  4.0],
		[ 6.0,  0.0],
		[11.0, -4.0],
		[16.0, -8.0]
	]
	XY = [-4.0, 8.0]


	M = [
		[1.0, 17.0],
		[1.0, 15.0],
		[1.0, 13.0],
		[1.0, 11.0]
	]
	XY = [1.0, 19.0]


	# initial state (location and velocity), external motion
	X = matrix([[XY[0]], [XY[1]], [0.0], [0.0]])
	U = matrix([[0.0], [0.0], [0.0], [0.0]])


	## initial uncertainty
	P = matrix([
		[0.0, 0.0,    0.0,    0.0],
		[0.0, 0.0,    0.0,    0.0],
		[0.0, 0.0, 1000.0,    0.0],
		[0.0, 0.0,    0.0, 1000.0],
	])

	## next-state function
	F = matrix([
		[1.0, 0.0,  dt, 0.0],
		[0.0, 1.0, 0.0,  dt],
		[0.0, 0.0, 1.0, 0.0],
		[0.0, 0.0, 0.0, 1.0],
	])

	## measurement function
	H = matrix([
		[1.0, 0.0, 0.0, 0.0],
		[0.0, 1.0, 0.0, 0.0],
	])

	## measurement uncertainty
	R = matrix([
		[0.1, 0.0],
		[0.0, 0.1],
	])

	## identity matrix
	I = matrix([
		[1.0, 0.0, 0.0, 0.0],
		[0.0, 1.0, 0.0, 0.0],
		[0.0, 0.0, 1.0, 0.0],
		[0.0, 0.0, 0.0, 1.0],
	])

	kalman_filter_2d(M, X, P, U, F, H, R, I)


def main(argc, argv):
	test_kalman_filter_1d()
	test_kalman_filter_2d()
	return 0

sys.exit(main(len(sys.argv), sys.argv))

