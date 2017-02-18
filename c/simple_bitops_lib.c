static inline bool is_pot(unsigned int i) {
	return ((i & (i - 1)) == 0);
}
static inline bool is_bit_set(unsigned int n, unsigned int b) {
	return ((n % (b << 1)) > (b - 1));
}

static inline unsigned int next_pot(unsigned int x) {
	x -= 1;
	x |= (x >>  1);
	x |= (x >>  2);
	x |= (x >>  4);
	x |= (x >>  8);
	x |= (x >> 16);
	x += 1;
	return x;
}

// undefined behavior (assumes two's-complement representation)
// static inline unsigned int least_significant_bit_set(int x) { return (x & (-x)); }
static inline unsigned int lsb_set(int x) { return (x & (~x + 1)); }


static inline unsigned int reverse_bytewise(unsigned int x) {
	unsigned int r = 0;
	r |= (((x >>  0) & 255) << 24);
	r |= (((x >>  8) & 255) << 16);
	r |= (((x >> 16) & 255) <<  8);
	r |= (((x >> 24) & 255) <<  0);
	return r;
}

// reverses the bits with positions p to q (inclusive) in x
static inline unsigned int reverse_bitwise(unsigned int x, unsigned int p, unsigned int q) {
	const unsigned int mask = ((1 << (q + 1)) - 1) ^ ((1 << p) - 1);
	const unsigned int lhs = x & (~mask);
	const unsigned int rhs = reverse_bytewise((x & mask) >> p); // >> (31 - q)?
	return (lhs | (rhs >> (31 - q)));
}




// works for all numbers
static void swap_args_add(int* a, int* b) {
	*a = (*a) + (*b);
	*b = (*a) - (*b);
	*a = (*a) - (*b);
}

// only works for powers of two
static void swap_args_xor(int* a, int* b) {
	*a = (*a) | (*b);
	*b = (*a) ^ (*b);
	*a = (*a) ^ (*b);
}


// emulates a "branchless select" hardware instr.
// if a >= 0, return x, else y (without branching)
//
// can be used to replace the ternary operator: first
// transform any expressions like "(a >= b)? c: d" to
// "((a - b) >= 0)? c: d", then to "isel(c, d, a - b)"
//
// remember that we also have
//   x∧y =       x*y
//   x∨y = x+y − x*y
//   ¬x  = 1 − x
//
//   an arithmetic right-shift is used to smear out the sign bit
//   mask = (a >> 31) is 0xFFFFFFFF if (a < 0) and 0x0 otherwise
//
//   if mask is 0xFFFFFFFF, then x + ((y – x) & mask) == y
//   if mask is 0x00000000, then x + ((y – x) & mask) == x
//
// (note the similarity to LRP'ing)
//
// inline int isel(int x, int y, int a) { return (x + ((y - x) & (a >> 31))); }
// inline int isel(int x, int y, int a) { return ((x & (~a)) + (y & a)); }
//
// these versions are simpler (but require MUL's) and expect <a> to be 0 or 1
inline   int isel(  int x,   int y,   int a) { return (x + ((y - x) * a)); }
inline float fsel(float x, float y, float a) { return (x + ((y - x) * a)); }



float fast_recip_sqrt(float x) {
	float xh = x * 0.5f;
	int ix = *(int*) &x;

	// "magic number" first guess
	ix = 0x5f375a86 - (ix >> 1);
	x = *(float*) &ix;

	// Newton's method, two iterations
	x = x * (1.5f - xh * (x * x));
	x = x * (1.5f - xh * (x * x));
	return x;
}

