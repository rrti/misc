import sys
from math import pi as PI
from math import ceil
from math import log
from math import sin
from math import cos

PI2 = PI * 2.0

## returns the base-<b> logarithm of <n>
def blog(b, n):
	return (log(n) / log(b))

## lists all <n> n-th roots of unity
def enumerate_unity_roots(n):
	print("[enumerate_unity_roots][n=%d]" % n)

	roots = []

	for j in xrange(0, n):
		cr = complex(cos((PI2 * j) / n), sin((PI2 * j) / n))
		cn = cr ** n

		roots += [(cr, cn)]

		scr = str(cr)
		scn = str(cn)

		print("\t%s -- %s" % (scr, scn))

	return roots


## input: P, vector of real coefficients
## output: PT, vector of complex coefficients
def fft_fwd(P, tabs):
	n = len(P)
	m = n / 2

	if (n == 1):
		return P

	Pe = [0] * m   ## U, "even-degree" coefficients
	Po = [0] * m   ## V, "odd-degree" coefficients

	for j in xrange(0, m):
		Pe[j] = P[(j * 2)    ]
		Po[j] = P[(j * 2) + 1]

	PTe = fft_fwd(Pe, tabs + '\t')   ## U* = FFT(U)
	PTo = fft_fwd(Po, tabs + '\t')   ## V* = FFT(V)
	PT = [None] * n

	wn = complex(cos(PI2 / n), sin(PI2 / n))
	w1 = 1

	for j in xrange(0, m):
		PT[j    ] = PTe[j] + (w1 * PTo[j])
		PT[j + m] = PTe[j] - (w1 * PTo[j])
		w1 *= wn

	return PT

## input: RT, vector of complex coefficients
## output: R, vector of real coefficients
def fft_inv(RT, tabs):
	n = len(RT)
	m = n / 2

	if (n == 1):
		return RT

	RTe = [None] * m   ## U*, "even-degree" coefficients
	RTo = [None] * m   ## V*, "odd-degree" coefficients

	for j in xrange(0, m):
		RTe[j] = RT[(j * 2)    ]
		RTo[j] = RT[(j * 2) + 1]

	Re = fft_inv(RTe, tabs + '\t')   ## U = FFT^{-1}(U*)
	Ro = fft_inv(RTo, tabs + '\t')   ## V = FFT^{-1}(V*)
	R = [None] * n

	wn = complex(cos(PI2 / n), -sin(PI2 / n))
	w1 = 1

	for j in xrange(0, m):
		R[j    ] = 2 * (Re[j] + (w1 * Ro[j]))
		R[j + m] = 2 * (Re[j] - (w1 * Ro[j]))
		w1 *= wn

	return R



def fft_multiply(P, Q, s = -1):
	p = len(P)   ## equal to degree(P) + 1
	q = len(Q)   ## equal to degree(Q) + 1
	l = int(ceil(blog(2, p + q)))
	k = (2 ** l)
	s = (p + q) * (p + q)

	## pad P and Q with zero-terms
	for j in xrange(p, k): P.append(0)
	for j in xrange(q, k): Q.append(0)

	## compute the DFT's of P and Q
	PT = fft_fwd(P, "");
	QT = fft_fwd(Q, "");
	RT = [None] * k

	## pointwise-multiply DFT(P) and DFT(Q)
	for j in xrange(0, k):
		RT[j] = (PT[j] * QT[j])

	## compute the inverse DFT of (R = DFT(P) * DFT(Q))
	R = fft_inv(RT, "")
	r = [int(R[i].real / s) for i in xrange(len(R))]

	print("[fft_multiply] %s * %s = %s" % (P[0: p], Q[0: q], r))
	return R



def main(argc, argv):
	if (argc > 1):
		for i in xrange(int(argv[1])):
			enumerate_unity_roots(i)

	## P = (1 + 2x)
	## Q = (2 + 3x)
	## R = (2 + 7x + 6x^2)
	##
	fft_multiply([1, 2], [2, 3], 16)

	## P = (4 + 3x + 2x^2 +  x^3)
	## Q = (1 + 2x + 3x^2 + 4x^3)
	## R = (4 + 11x + 20x^2 + 30x^3 + 20x^4 + 11x^5 + 4x^6)
	##
	fft_multiply([4, 3, 2, 1], [1, 2, 3, 4], 64)

	## P = (1 + 2x + 3x^2)
	## Q = (2 + 5x)
	## R = (2 + 9x + 16x^2 + 15x^3)
	##
	fft_multiply([1, 2, 3], [2, 5], 64)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

