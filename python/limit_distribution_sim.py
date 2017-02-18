WORLD_STATES = ["green", "red", "red", "green", "green"]
BELIEF_STATE = [1.0 / len(WORLD_STATES)] * len(WORLD_STATES)

SIMULATE_LIM = False

## simulate the limit distribution (ie. start with all probability
## centered on one cell); this collapses to a uniform distribution
## after a small number of iterations which then remains stationary
## (a.k.a. the distr. of absolute least information)
##
## for any X_i (eg. X_4), in the limit it holds that P(X_i) =
##     0.8 * P(X_{i-(U+1)} +   // i=4, U+1=2 --> (P(X_2))
##     0.1 * P(X_{i-(U+2)} +   // i=4, U+2=3 --> (P(X_1))
##     0.1 * P(X_{i-(U  )}     // i=4, U  =1 --> (P(X_3))
## where
##     0.8 is pr. that moving right by U cells from cell X_i gets us to cell X_{i+U  }
##     0.1 is pr. that moving right by U cells from cell X_i gets us to cell X_{i+U-1} (undershoot)
##     0.1 is pr. that moving right by U cells from cell X_i gets us to cell X_{i+U+1} (overshoot)
if (SIMULATE_LIM):
	BELIEF_STATE = [1.0, 0.0, 0.0, 0.0, 0.0]


## simulated sensor and motion inputs over time
SENSOR_INPUTS = ["red", "red"]
MOTION_INPUTS = [1, 1]

## sensor and motion probabilities (accuracies)
P_SENSOR = (0.6, 0.2)
P_MOTION = (0.8, 0.1, 0.1)

assert(len(MOTION_INPUTS) == len(SENSOR_INPUTS))

def SensorInput1D(p, Z):
	q = [0.0] * len(p)

	for i in xrange(len(p)):
		hit = (Z == WORLD_STATES[i])
		q[i] = p[i]
		q[i] *= ((0 + hit) * P_SENSOR[0]  +  (1 - hit) * P_SENSOR[1])

	## normalize new belief
	s = sum(q)

	for i in xrange(len(q)):
		q[i] /= s

	return q



def MotionInput1D(p, U):
	q = [0.0] * len(p)

	for i in xrange(len(p)):
		s = 0.0
		s += P_MOTION[0] * p[(i - U    ) % len(p)]
		s += P_MOTION[1] * p[(i - U - 1) % len(p)]
		s += P_MOTION[2] * p[(i - U + 1) % len(p)]
		q[i] = s

	## normalize new belief
	s = sum(q)

	for i in xrange(len(q)):
		q[i] /= s

	return q

if (SIMULATE_LIM):
	for i in xrange(1000):
		BELIEF_STATE = MotionInput1D(BELIEF_STATE, 1)
else:
	for k in xrange(len(SENSOR_INPUTS)):
		BELIEF_STATE = SensorInput1D(BELIEF_STATE, SENSOR_INPUTS[k])
		BELIEF_STATE = MotionInput1D(BELIEF_STATE, MOTION_INPUTS[k])

print(BELIEF_STATE)

