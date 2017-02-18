## n-th order polynomial: a*x^{n} + b*x^{n-1} + ... + z
##
##    a*x^4 + b*x^3 + c*x^2 + d*x^1 + e
##            a*x^3 + b*x^2 + c*x^1 + d
##                    a*x^2 + b*x^1 + c
##                            a*x^1 + b
##                                    a
##
## we can represent a polynomial P of order <n> by a
## list of <n+1> coefficients [a, b, c, d, e, ..., z]
## and calculate the derivative polynomial dP of order
## <n-1> by discarding the last element (which denotes
## a constant)
##
## n = 4   [        a,       b,     c,   d, e]   <-->     a*x^4     b*x^3     c*x^2    d*x^1
## n = 3   [      4*a,     3*b,   2*c, 1*d, 0]   <-->    4a*x^3    3b*x^2    2c*x^1    d
## n = 2   [    3*4*a,   2*3*b, 1*2*c,   0, 0]   <-->   12a*x^2    6b*x^1    2c        0
## n = 1   [  2*3*4*a, 1*2*3*b,     0,   0, 0]   <-->   24a*x^1    6b        0         0
## n = 0   [1*2*3*4*a,       0,     0,   0, 0]   <-->   24a        0         0         0
##
## finding local optima: look at function-value triplets
## <f(x - dx), f(x), f(x + dx)> along an interval [a, b]
## -->  <+, -, +> or <-, +, -> patterns indicate peaks
## (assuming we want to search for them numerically) or
## just adjust boundaries based on slopes



def   fx_a(x): return ((x*x*x*x) - (5.0*x*x*x) + (3.0*x*x) - (7.0*x) - 11.0)
def d_fx_a(x): return ((4.0*x*x*x) - (15.0*x*x) + (6.0*x) - 7.0)

def   fx_b(x): return (x*x*x)
def   fx_c(x): return ((4.0*x*x) - (7.0*x) + 5.0)
def   fx_d(x): return ((x*x) - 612.0)

def   fx_e(x): return ((x*x*x) + (10.0*x*x))
def d_fx_e(x): return ((3.0*x*x) + (20.0*x))

def plot(f, a, b, dx, name):
	h = open(name, 'w')
	x = a

	while (x <= b):
		s = "%f\t%f\n" % (x, f(x))
		x += dx
		h.write(s)

	h.close()


def find_root_x(f, x, dx, eps):
	fx = f(x)
	n = 0

	## Newton-Raphson iteration applied to <f/df>
	while (abs(fx) > eps):
		dfx = (f(x + dx) - fx) / dx
		x = x - (fx / dfx)
		fx = f(x)
		n += 1

	return x

def find_optimum_x(f, a, b, dx, eps):
	x = a

	while (abs(b - a) > eps):
		x = (a + b) * 0.5
		## the slope might not be constant (when f is a second- or
		## higher-order function), so we need more than one sample
		##
		## if sa and sb are both negative (eg. sa = -137, sb = -136)
		## then sb > sa (which normally indicates increasing slope)
		## but we want to move the LEFT boundary to the RIGHT (and
		## vice versa)
		##
		## NOTE: binary search oscillates, which means it can miss
		## (jump over) a peak if x starts in its vicinity and [a, b]
		## is wide ==> how does Newton-Raphson avoid this? ==> N-R
		## finds ROOTS (which do not always exist), *not* PEAKS
		##
		## however, "Newton's method can also be used to find a
		## minimum or maximum of a function. The derivative is
		## zero at a minimum or maximum, so minima and maxima
		## can be found by applying it to the derivative..."
		##
		sa = abs(f(x     ) - f(x - dx)) / dx
		sb = abs(f(x + dx) - f(x     )) / dx

		if (sb > sa): b = x
		elif (sa > sb): a = x
		else: break

	return x



def get_polynomial_string(exponShift, polyCoeffs):
	poly_degree = len(polyCoeffs) - 1
	poly_string = ""
	term_count  = 0

	## iterate over the non-constant terms
	for n in xrange(poly_degree):
		if (polyCoeffs[n] != 0):
			termExpon = max(0, poly_degree - n - exponShift)

			if (term_count > 0):
				poly_string += " + "

			if (termExpon > 0):
				poly_string += ("%d*x^%d" % (polyCoeffs[n], termExpon))
			else:
				poly_string += ("%d" % (polyCoeffs[n]))

			term_count += 1

	## add the constant term
	if (polyCoeffs[-1] != 0):
		if (term_count > 0):
			poly_string += " + "

		poly_string += ("%d" % polyCoeffs[-1])

	return poly_string

## get the <steps>-th derivative of the polynomial function of
## degree <n=len(coeffs)-1> represented by <coeffs> (coeffs[0]
## = a, coeffs[1] = b, ..., coeffs[-1] = constant term)
##
## the more generic way of doing this would be to symbolically
## process expressions composed of {u, bi}nary opera{nds, tors}
## recursively using the rules of calculus
##
def find_polynomial_derivative(num_derivative_steps, poly_coeffs):
	print("[find_polynomial_derivative][pre] poly_coeffs=%s" % poly_coeffs)

	## constant term does not count toward degree
	poly_degree = len(poly_coeffs) - 1

	for n in xrange(num_derivative_steps):
		poly_string = get_polynomial_string(n, poly_coeffs)

		## take the derivative of all terms (including the constant)
		for i in xrange(len(poly_coeffs)):
			termExponent = max(0, poly_degree - i)
			poly_coeffs[i] *= termExponent

		## after each iteration, the coefficients shift by
		## one term since the last is a constant (eg. the
		## 'a' in a*x^4 becomes the 'a' in a*x^3)
		poly_degree -= 1

		## poly_string = get_polynomial_string(n + 1, poly_coeffs)
		print("\t[d/dx]^%u f(x)=%s" % (n, poly_string))

	print("[find_polynomial_derivative][pst] poly_coeffs=%s" % poly_coeffs)
	return poly_coeffs

def analyze_polynomials():
	f   =   fx_a
	df  = d_fx_a
	dx  =   0.01
	a   = -40.0
	b   =  75.0
	eps =   0.01

	## binary-search over <f> and Newton-Raphson
	## applied to <df> are not guaranteed to find
	## the same x-coordinate, but do in this case
	x = find_optimum_x(f, a, b, dx, eps)
	print("[analyze_polynomials][fx_a] x=%f" % x)
	print("\tf(x - dx)=%f" % f(x - dx))
	print("\tf(x     )=%f" % f(x     ))
	print("\tf(x + dx)=%f" % f(x + dx))
	print

	x = find_root_x(df, (a + b) * 0.5, dx, eps)
	print("[analyze_polynomials][d_fx_a] x=%f" % x)
	print("\tf(x - dx)=%f" % f(x - dx))
	print("\tf(x     )=%f" % f(x     ))
	print("\tf(x + dx)=%f" % f(x + dx))
	print

	f = fx_d
	x = find_root_x(f, 10.0, dx, eps)
	print("[analyze_polynomials][fx_d] x=%f" % x) ## sqrt(612.0)
	print("\tf(x - dx)=%f" % f(x - dx))
	print("\tf(x     )=%f" % f(x     ))
	print("\tf(x + dx)=%f" % f(x + dx))
	print

	f  =   fx_e
	df = d_fx_e
	a  = -10.0
	b  =  10.0

	print("[analyze_polynomials][fx_e]")
	plot( f, a, b, dx,   "fx_e.dat")
	plot(df, a, b, dx, "d_fx_e.dat")

	coeffs = [-44, 10, 6, 5, 2]
	coeffs = find_polynomial_derivative(len(coeffs), coeffs)

analyze_polynomials()

