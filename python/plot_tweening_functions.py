import math

def clamp(x, xmin, xmax):
	return (max(xmin, min(xmax, x)))

def clamped_sigmoid(x, xscale):
	x = clamp(x, -xscale, xscale)
	y = (1.0 / (1.0 + (math.e ** -x)))
	return (clamp(y, 0.0, 1.0))

def scaled_sigmoid(x, xscale = 6.0):
	x = clamp(x, 0.0, 1.0) * xscale * 2.0 - xscale
	y = clamped_sigmoid(x, xscale)
	return y



##
## note: these are really all functions of alpha
##
def mix_parabolic(xmin, xmax, alpha, degree = 1.0):
	return (xmin * (1.0 - pow(alpha, degree)) + xmax * (0.0 + pow(alpha, degree)))

def mix_sigmoidal(xmin, xmax, alpha):
	return (xmin * scaled_sigmoid(1.0 - alpha) + xmax * (0.0 + scaled_sigmoid(alpha)))

## sinoid (a will be in [-1, 1], must rescale to [0, 1])
## this means we start at half of xmin + half of xmax (!)
def mix_sineoidal(xmin, xmax, alpha):
	a = math.sin((                alpha) * math.pi * 2.0) * 0.5 + 0.5
	b = math.sin((math.pi * 2.0 - alpha) * math.pi * 2.0) * 0.5 + 0.5
	return (xmin * (1.0 - a) + xmax * (0.0 + a))



X_MIN = 20
X_MAX = 60
X_MUL = 10
FILES = [open("f%d.dat" % x, 'w') for x in xrange(7)]

for x in xrange(X_MIN * X_MUL, X_MAX * X_MUL + 1):
	alpha = ((x - X_MIN * X_MUL) * 1.0) / (X_MAX * X_MUL - X_MIN * X_MUL)
	## uncomment to interpolate from "right to left" instead
	## alpha = 1.0 - alpha

	FILES[0].write("%f\t%f\n" % (x, mix_parabolic(X_MIN, X_MAX, alpha, 1.0)))
	FILES[1].write("%f\t%f\n" % (x, mix_parabolic(X_MIN, X_MAX, alpha, 2.0)))
	FILES[2].write("%f\t%f\n" % (x, mix_parabolic(X_MIN, X_MAX, alpha, 3.0)))
	FILES[3].write("%f\t%f\n" % (x, mix_parabolic(X_MIN, X_MAX, alpha, 4.0)))
	FILES[4].write("%f\t%f\n" % (x, mix_parabolic(X_MIN, X_MAX, alpha, 0.5)))
	FILES[5].write("%f\t%f\n" % (x, mix_sigmoidal(X_MIN, X_MAX, alpha)))
	FILES[6].write("%f\t%f\n" % (x, mix_sineoidal(X_MIN, X_MAX, alpha)))

## gnuplot: plot 'f0.dat' with lines, 'f1.dat' with lines, 'f2.dat' with lines, 'f3.dat' with lines, 'f4.dat' with lines, 'f5.dat' with lines, 'f6.dat' with lines
for f in FILES:
	f.close()

