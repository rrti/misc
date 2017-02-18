import math
import random

m_exp = math.exp
m_log = math.log
m_rng = random.random



def f1(x): return (x*x)
def f2(x): return (5.0*x*x*x - 7.0*x*x + 11.0*x - 47.0)

## MC integration using naive uniform sampling
## rectangle has width (b - a) and height c, f
## is any one-variable function
##
def monte_carlo_integrate(f, n, a, b, c = 1.0):
	hits = 0

	for i in xrange(n):
		x  = a + m_rng() * (b - a) ## sample x from domain
		y  =     m_rng() * (c    ) ## sample y from range
		fx = f(x)

		if (fx >= 0.0):
			hits += (y <= fx)
		else:
			hits += (y >= fx)

	area = c * (b - a)
	prob = hits * (1.0 / n)
	return (prob * area)

print("INT[x1=%f,x2=%f][f2]=%f" % (0.0, 4.0, monte_carlo_integrate(f2,      10, 0.0, 4.0)))
print("INT[x1=%f,x2=%f][f2]=%f" % (0.0, 4.0, monte_carlo_integrate(f2,     100, 0.0, 4.0)))
print("INT[x1=%f,x2=%f][f2]=%f" % (0.0, 4.0, monte_carlo_integrate(f2,    1000, 0.0, 4.0)))
print("INT[x1=%f,x2=%f][f2]=%f" % (0.0, 4.0, monte_carlo_integrate(f2,   10000, 0.0, 4.0)))
print("INT[x1=%f,x2=%f][f2]=%f" % (0.0, 4.0, monte_carlo_integrate(f2,  100000, 0.0, 4.0)))
print("INT[x1=%f,x2=%f][f2]=%f" % (0.0, 4.0, monte_carlo_integrate(f2, 1000000, 0.0, 4.0)))



##
## x-axis: values of random variable X
## y-axis: probability f(x) that X = x
##
def eval_gauss_func(x, mu, sigma, xmin = -10.0, xmax = 10.0):
	assert( x >=  0.0 and  x <=  1.0)
	assert(mu >= xmin and mu <= xmax)

	norm = 1.0 / (sigma * ((math.pi * 2.0) ** 0.5))

	## <x> is a number between 0 and 1 but the domain
	## of the Gaussian is [-inf, +inf] so transform it
	xcur = xmin + (xmax - xmin) * x
	xdev = xcur - mu

	pwr = -((xdev * xdev) / (2.0 * sigma * sigma))
	exp = m_exp(pwr)
	ret = norm * exp

	assert(ret >= 0.0)
	assert(ret <= 1.0)
	return ret

def reject_sample_gauss_func(mu, sigma):
	x = m_rng()
	y = m_rng()

	if (y <= eval_gauss_func(x, mu, sigma)):
		return (x, y)

	## sample rejected
	return (-1.0, -1.0)

def raw_sample_gauss_func(mu = 0.0, sigma = 1.0):
	x = 0.0
	y = 0.0
	s = 1.0

	## Marsaglia polar method for sampling standard
	## (mu=0, sigma=1) normal random variables (x,y)
	while (s >= 1.0):
		x = 2.0 * m_rng() - 1.0
		y = 2.0 * m_rng() - 1.0

		s = x * x + y * y

	t = ((-2.0 * m_log(s)) / s) ** 0.5
	x = mu + x * t * sigma
	y = mu + y * t * sigma

	return (x, y)



"""
for xi in xrange(101):
	x = xi * 0.01
	fx = eval_gauss_func(x, 0.0, 3.0)
	print("%f\t%f" % (x, fx))
"""


num_accepted_samples = 0
num_rejected_samples = 0

for n in xrange(100000):
	random_gauss_sample = reject_sample_gauss_func(0.0, 3.0)
	num_accepted_samples += (random_gauss_sample[1] == -1.0)
	num_rejected_samples += (random_gauss_sample[1] != -1.0)

	if (random_gauss_sample[1] != -1.0):
		print("%f\t%f" % (random_gauss_sample[0], random_gauss_sample[1]))

## print num_accepted_samples, num_rejected_samples

