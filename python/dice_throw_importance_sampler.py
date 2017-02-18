import random

NUM_DICE_THROWS = 10000000
FAIR_DIE_DISTR = (1.0 / 6.0,  1.0 / 6.0,  1.0 / 6.0,  1.0 / 6.0,  1.0 / 6.0,  1.0 / 6.0)
BIAS_DIE_DISTR = (0.5 / 6.0,  0.5 / 6.0,  0.5 / 6.0,  0.5 / 6.0,  1.0 / 6.0,  3.0 / 6.0)
FAIR_DIE_EXP_V = 0.0
BIAS_DIE_EXP_V = 0.0
FAIR_THROW_SUM = 0.0
BIAS_THROW_SUM = 0.0
FAIR_THROW_BINS = [0.0] * len(FAIR_DIE_DISTR)
BIAS_THROW_BINS = [0.0] * len(BIAS_DIE_DISTR)

UNIFORM_RANDGEN = random.random

FAIR_DIE_FILE = open("fair_die_sample_avg.dat", 'w')
BIAS_DIE_FILE = open("bias_die_sample_avg.dat", 'w')

assert(len(FAIR_DIE_DISTR) == len(BIAS_DIE_DISTR))
## both distributions should sum to 1
assert(((sum(FAIR_DIE_DISTR) - 1.0) ** 2.0) < 0.01)
assert(((sum(BIAS_DIE_DISTR) - 1.0) ** 2.0) < 0.01)


## calculate the a-priori expected values
## (weighted averages) for each 6-sided die
for n in xrange(len(FAIR_DIE_DISTR)):
	FAIR_DIE_EXP_V += (FAIR_DIE_DISTR[n] * (n + 1))
	BIAS_DIE_EXP_V += (BIAS_DIE_DISTR[n] * (n + 1))

for n in xrange(NUM_DICE_THROWS):
	## generate uniform samples
	r = int(UNIFORM_RANDGEN() * len(FAIR_DIE_DISTR))

	assert(r < len(FAIR_DIE_DISTR))

	## die-distributions are discrete, so we have
	## "real" probabilities rather than densities
	pFair = FAIR_DIE_DISTR[r]
	pBias = BIAS_DIE_DISTR[r]

	## expected-value is weighted-average in general,
	## simple unweighted-average if all P's are equal
	##
	## the average over <n> throws should converge to
	## the expected value, but since our "thrower" RNG
	## is uniform we have to correct its simulation of
	## the BIASED die through importance sampling:
	##
	##    if an event is LESS likely to occur in the biased
	##    probability-space than our uniform generator will
	##    sample it (eg. throwing '1'), then (pBias / pFair)
	##    must DECREASE the event's contribution when it is
	##    drawn uniformly
	##
	##    if an event is MORE likely to occur in the biased
	##    probability-space than our uniform generator will
	##    sample it (eg. throwing '6'), then (pBias / pFair)
	##    must INCREASE the event's contribution when it is
	##    drawn uniformly
	FAIR_THROW_SUM += ((r + 1) * (pFair / pFair))
	BIAS_THROW_SUM += ((r + 1) * (pBias / pFair))

	## keep track of the "real" number of times each came
	## was thrown; note this needs rounding for non-integer
	## values of (pBias / pFair)
	FAIR_THROW_BINS[r] += (pFair / pFair)
	BIAS_THROW_BINS[r] += (pBias / pFair)

	## plot the convergence of the "throw-average so far" to
	## the actual a-priori expected value (weighted average)
	## NOTE: VERY SLOW FOR LARGE N
	##
	## FAIR_DIE_FILE.write("%d\t%f\n" % (n, FAIR_THROW_SUM / (n + 1)))
	## BIAS_DIE_FILE.write("%d\t%f\n" % (n, FAIR_THROW_SUM / (n + 1)))

## the sample averages should be near the expected values
FAIR_THROW_S_AVG = FAIR_THROW_SUM / NUM_DICE_THROWS
BIAS_THROW_S_AVG = BIAS_THROW_SUM / NUM_DICE_THROWS

print("FAIR_DIE_EXP_V=%f BIAS_DIE_EXP_V=%f" % (FAIR_DIE_EXP_V, BIAS_DIE_EXP_V))
print("FAIR_THROW_S_AVG=%f BIAS_THROW_S_AVG=%f" % (FAIR_THROW_S_AVG, BIAS_THROW_S_AVG))
print("FAIR_THROW_BINS=%s (sum=%f)" % (FAIR_THROW_BINS, sum(FAIR_THROW_BINS)))
print("BIAS_THROW_BINS=%s (sum=%f)" % (BIAS_THROW_BINS, sum(BIAS_THROW_BINS)))

FAIR_DIE_FILE = FAIR_DIE_FILE.close()
BIAS_DIE_FILE = BIAS_DIE_FILE.close()

