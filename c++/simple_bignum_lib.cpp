#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include <algorithm>
#include <limits>

// set this value to e.g. 2 or 10 to (inefficiently)
// emulate base-2 or base-10 arithmetic respectively
// leave it at <= 1 for maximally-filled limbs
#define BIGNUM_ARITHMETIC_BASE 10lu

// need this in lieu of exponentiation operators
// #define private public

static const char* LIMB_FORMAT_STRS[] = {
	" %3u" , // uint8
	" %5u" , // uint16
	" %10u", // uint32
	" %20u", // uint64
};


// NOTE: does not support variable-size operations
template<typename t_limb_type, size_t c_num_limbs> struct t_bignum {
public:
	enum {
		BIGNUM_COND_OFLOW_FLAG = 0,
		BIGNUM_COND_UFLOW_FLAG = 1,
		BIGNUM_CONDITION_FLAGS = 2,
	};


	static t_limb_type limb_max_val() {
		#if (BIGNUM_ARITHMETIC_BASE >= 2)
		return (std::min(BIGNUM_ARITHMETIC_BASE - 1, static_cast<size_t>(std::numeric_limits<t_limb_type>::max())));
		#else
		// in this case radix simply equals type::max + 1
		//
		// not usable, always overflows limb-type
		// return ((1u << (sizeof(t_limb_type) << 3u)) - 1u);
		//
		// this does not (except for size_t) but is unreadable
		// return (((((1u << ((sizeof(t_limb_type) << 3u) - 1u))) - 1u) << 1) + 1);
		//
		return (std::numeric_limits<t_limb_type>::max());
		#endif
	}

	static size_t limb_radix() {
		return (static_cast<size_t>(t_bignum::limb_max_val()) + 1);
	}

	static size_t raw_size() {
		return (sizeof(t_limb_type) * c_num_limbs);
	}

	static size_t limb_format_str_idx() {
		switch (sizeof(t_limb_type)) {
			case 1: { return 0; } break;
			case 2: { return 1; } break;
			case 4: { return 2; } break;
			case 8: { return 3; } break;
		}

		return 0;
	}


	static const t_bignum& zero_value() { static t_bignum zero_bignum(0, 0); return zero_bignum; }
	static const t_bignum& unit_value() { static t_bignum unit_bignum(0, 1); return unit_bignum; }



	t_bignum(t_limb_type msb = 0, t_limb_type lsb = 0) {
		// temporary results need to be stored in types that
		// are twice as wide as t_limb_type, which would not
		// be possible (without native 128-bit type support)
		// if t_limb_type is itself 8 bytes
		assert(sizeof(t_limb_type) <= 4);
		assert(sizeof(size_t) == 8);

		memset(&m_limbs[0], 0, t_bignum::raw_size());
		memset(&m_flags[0], 0, sizeof(bool) * BIGNUM_CONDITION_FLAGS);

		m_limbs[              0] = msb % t_bignum::limb_radix();
		m_limbs[c_num_limbs - 1] = lsb % t_bignum::limb_radix();
	}

	t_bignum(const t_limb_type* limbs) {
		assert(&m_limbs[0] != &limbs[0]);
		memcpy(&m_limbs[0], &limbs[0], t_bignum::raw_size());
		memset(&m_flags[0], 0, sizeof(bool) * BIGNUM_CONDITION_FLAGS);
	}

	t_bignum(const t_bignum& bn) {
		*this = bn;
	}

	// copy-ctor
	//
	// memcpy behavior is undefined when src and dst
	// are the same, so make sure <bn> is a different
	// instance
	// unlike with the ref-ops there is no real point
	// to using self-assignment so always disallow it
	t_bignum& operator = (const t_bignum& bn) {
		assert(&bn != this);
		memcpy(&m_limbs[0], bn.get_raw_limbs(), t_bignum::raw_size());
		memcpy(&m_flags[0], bn.get_raw_flags(), sizeof(bool) * BIGNUM_CONDITION_FLAGS);
		return (*this);
	}


	t_bignum operator + (const t_bignum& bn) const { return (add_bn(*this, bn));        }
	t_bignum operator - (const t_bignum& bn) const { return (sub_bn(*this, bn));        }
	t_bignum operator * (const t_bignum& bn) const { return (mul_bn(*this, bn));        }
	t_bignum operator / (const t_bignum& bn) const { return (div_bn(*this, bn)).first;  }
//	t_bignum operator ? (const t_bignum& bn) const { return (exp_bn(*this, bn));        }
//	t_bignum operator % (const t_bignum& bn) const { return (mod_bn(*this, bn));        }
	t_bignum operator % (const t_bignum& bn) const { return (div_bn(*this, bn)).second; }

	// limbwise binary ops
	t_bignum operator  & (const t_bignum& bn) const { return (and_bn(*this, bn)); }
	t_bignum operator  | (const t_bignum& bn) const { return ( or_bn(*this, bn)); }
	t_bignum operator  ^ (const t_bignum& bn) const { return (xor_bn(*this, bn)); }

	t_bignum operator << (size_t value) const { return (shl_val(*this, value)); }
	t_bignum operator >> (size_t value) const { return (shr_val(*this, value)); }

	// special-case limbwise unary op
	t_bignum operator ~ () const { return (not_bn(*this)); }


	// NOTE:
	//   all reference-ops make copies (of *this) such that situations like "bn += bn"
	//   are safe, since the functions that implement them work in-place and otherwise
	//   would always need to verify that (&bn != this)
	//
	t_bignum& operator += (const t_bignum& bn) { return ((*this) = add_bn(*this, bn));        }
	t_bignum& operator -= (const t_bignum& bn) { return ((*this) = sub_bn(*this, bn));        }
	t_bignum& operator *= (const t_bignum& bn) { return ((*this) = mul_bn(*this, bn));        }
	t_bignum& operator /= (const t_bignum& bn) { return ((*this) = div_bn(*this, bn)).first;  }
//	t_bignum& operator ?= (const t_bignum& bn) { return ((*this) = exp_bn(*this, bn));        }
//	t_bignum& operator %= (const t_bignum& bn) { return ((*this) = mod_bn(*this, bn));        }
	t_bignum& operator %= (const t_bignum& bn) { return ((*this) = div_bn(*this, bn)).second; }

	t_bignum& operator &= (const t_bignum& bn) { return ((*this) = and_bn(*this, bn)); }
	t_bignum& operator |= (const t_bignum& bn) { return ((*this) =  or_bn(*this, bn)); }
	t_bignum& operator ^= (const t_bignum& bn) { return ((*this) = xor_bn(*this, bn)); }

	t_bignum& operator <<= (size_t shl) { return ((*this) = shl_val(*this, shl)); }
	t_bignum& operator >>= (size_t shr) { return ((*this) = shr_val(*this, shr)); }


	// relational binary ops
	bool operator == (const t_bignum& bn) const {
		return (memcmp(&m_limbs[0], bn.get_raw_limbs(), t_bignum::raw_size()) == 0);
	}
	bool operator != (const t_bignum& bn) const {
		return (!(bn == (*this)));
	}

	bool operator < (const t_bignum& bn) const { return (lt_bn(*this, bn)); }
	bool operator > (const t_bignum& bn) const { return (gt_bn(*this, bn)); }

	bool operator <= (const t_bignum& bn) const { return (((*this) == bn) || ((*this) < bn)); }
	bool operator >= (const t_bignum& bn) const { return (((*this) == bn) || ((*this) > bn)); }


	bool get_flag(size_t flag) const { return m_flags[flag]; }
	void clear_flag(size_t flag) { m_flags[flag] = false; }

	bool is_odd() const { return ((m_limbs[c_num_limbs - 1] & 1) == 1); }
	bool is_even() const { return ((m_limbs[c_num_limbs - 1] & 1) == 0); }

	bool is_valid() const {
		bool r = true;

		// limbs are unsigned, check only the overflow case
		// note: always valid if limb::max equals radix - 1
		for (size_t n = 0; n < c_num_limbs; n++) {
			r &= (m_limbs[n] < t_bignum::limb_radix());
		}

		return r;
	}

	// accessors
	const t_limb_type* get_raw_limbs() const { return &m_limbs[0]; }
	      t_limb_type* get_raw_limbs()       { return &m_limbs[0]; }

	const bool* get_raw_flags() const { return &m_flags[0]; }
	      bool* get_raw_flags()       { return &m_flags[0]; }


	void print() {
		for (size_t n = 0; n < c_num_limbs; n++) {
			printf(LIMB_FORMAT_STRS[t_bignum::limb_format_str_idx()], m_limbs[n]);
		}
		printf("\n");
	}

private:
	t_limb_type  operator [] (size_t idx) const { return m_limbs[idx]; }
	t_limb_type& operator [] (size_t idx)       { return m_limbs[idx]; }


	static t_bignum add_bn(const t_bignum& a, const t_bignum& b) {
		t_bignum c;

		for (size_t n = 0; n < c_num_limbs; n++) {
			c.add_limbs(a, b, n);
		}

		return c;
	}

	static t_bignum sub_bn(const t_bignum& a, const t_bignum& b) {
		t_bignum c;

		for (size_t n = 0; n < c_num_limbs; n++) {
			c.sub_limbs(a, b, n);
		}

		return c;
	}

	static t_bignum mul_bn(const t_bignum& a, const t_bignum& b) {
		t_bignum c;

		for (size_t i = 0; i < c_num_limbs; i++) {
			for (size_t j = 0; j < c_num_limbs; j++) {
				c.mul_limbs(a, b, i, j);
			}
		}

		return c;
	}

	static std::pair<t_bignum, t_bignum> div_bn(const t_bignum& a, const t_bignum& b) {
		assert(b != t_bignum::zero_value());

		// trivial cases: a/1=a and a/a=1
		if (b == t_bignum::unit_value())
			return (std::make_pair<t_bignum, t_bignum>(a, t_bignum::zero_value()));
		if (b == a)
			return (std::make_pair<t_bignum, t_bignum>(t_bignum::unit_value(), t_bignum::zero_value()));

		t_bignum q; // quotient
		t_bignum r; // remainder

		for (size_t i = 0; i < c_num_limbs; i++) {
			// shift remainder (r) one limb to the left
			r = r << 1;

			// bring down next limb of numerator (a) to new LSB
			r[c_num_limbs - 1] = a[i];

			while (r >= b) {
				// find greatest multiple of denominator (b) < quotient
				// new remainder is quotient modulo the denominator (b)
				r -= b;
				q[i] += 1;
			}
		}

		return (std::make_pair<t_bignum, t_bignum>(q, r));
	}

	// NOTE: redundant since div() also returns the remainder
	static t_bignum mod_bn(const t_bignum& a, const t_bignum& b) {
		assert(b != t_bignum::zero_value());

		// trivial case: a%1=0
		if (b == t_bignum::unit_value())
			return (t_bignum::zero_value());

		t_bignum c;

		c = a / b; // C =   (A/B)
		c = c * b; // C =   (A/B)*B
		c = a - c; // C = A-(A/B)*B

		return c;
	}

	static t_bignum exp_bn(const t_bignum& a, const t_bignum& b) {
		// trivial cases: a^0=1 and a^1=a
		if (b == t_bignum::zero_value())
			return t_bignum::unit_value();
		if (b == t_bignum::unit_value())
			return a;

		t_bignum c;

		if (b.is_odd()) {
			// r=x*(x^(n-1))
			c = exp_bn(a, b - t_bignum::unit_value());
			c = a * c;
		} else {
			// r=(x^(n/2))^2
			c = exp_bn(a, b / (t_bignum::unit_value() + t_bignum::unit_value()));
			c = c * c;
		}

		return c;
	}

	static t_bignum gcd_bn(const t_bignum& a, const t_bignum& b) {
		assert(gt_bn(a, b) || (a == b));

		// initialize to <b> in case <a % b> equals 0
		t_bignum c = b;

		while (mod_bn(a, b) != t_bignum::zero_value()) {
			c = mod_bn(a, b);
			a = b;
			b = c;
		}

		return c;
	}



	static t_bignum and_bn(const t_bignum& a, const t_bignum& b) {
		t_bignum res;

		for (size_t n = 0; n < c_num_limbs; n++) {
			res[n] = (a[n] & b[n]) % t_bignum::limb_radix();
		}

		return res;
	}

	static t_bignum or_bn(const t_bignum& a, const t_bignum& b) {
		t_bignum res;

		for (size_t n = 0; n < c_num_limbs; n++) {
			res[n] = (a[n] | b[n]) % t_bignum::limb_radix();
		}

		return res;
	}

	static t_bignum xor_bn(const t_bignum& a, const t_bignum& b) {
		t_bignum res;

		for (size_t n = 0; n < c_num_limbs; n++) {
			res[n] = (a[n] ^ b[n]) % t_bignum::limb_radix();
		}

		return res;
	}

	static t_bignum not_bn(const t_bignum& bn) {
		t_bignum res;

		for (size_t n = 0; n < c_num_limbs; n++) {
			res[n] = t_bignum::limb_max_val() - bn[n];
		}

		return res;
	}


	static t_bignum shl_val(const t_bignum& bn, size_t shl) {
		t_bignum res;

		if (shl > 0 && shl <= c_num_limbs) {
			// iterate rightward from MSB to LSB
			for (size_t n = shl; n < c_num_limbs; n++) {
				res[n - shl] = bn[n];
			}
		} else {
			// support zero-shifts
			res = bn;
		}

		return res;
	}

	static t_bignum shr_val(const t_bignum& bn, size_t shr) {
		t_bignum res;

		if (shr > 0 && shr <= c_num_limbs) {
			// iterate leftward from LSB to MSB
			for (size_t n = c_num_limbs - 1; n >= shr; n--) {
				res[n] = bn[n - shr];
			}
		} else {
			// support zero-shifts
			res = bn;
		}

		return res;
	}




	static bool lt_bn(const t_bignum& a, const t_bignum& b) {
		for (size_t n = 0; n < c_num_limbs; n++) {
			if (a[n] < b[n]) { return  true; }
			if (a[n] > b[n]) { return false; }
		}

		// all limbs are equal
		assert(a == b);
		return false;
	}

	static bool gt_bn(const t_bignum& a, const t_bignum& b) {
		for (size_t n = 0; n < c_num_limbs; n++) {
			if (a[n] > b[n]) { return  true; }
			if (a[n] < b[n]) { return false; }
		}

		// all limbs are equal
		assert(a == b);
		return false;
	}




	void add_limbs(const t_bignum& a_limbs, const t_bignum& b_limbs, size_t ab_limb_order) {
		// MSB of <C> is at index 0, but the limbs are
		// numbered in increasing order from the *L*SB
		const size_t limb_idx = c_num_limbs - ab_limb_order - 1;

		// get carry from last limb-addition (if any)
		const size_t prev_limb_carry = m_limbs[limb_idx];

		// due to the carry it is not possible to reliably detect
		// limb-type overflow without casting them to larger type
		//
		// e.g. a rule like (a + b + c) % N < min(a, b) will fail
		// for ((a=9 + b=9 + c=1) % N=10) == 9  >=  min(a=9, b=9)
		//
		#define LIMB_SUM(a, b, c) (static_cast<size_t>(a) + static_cast<size_t>(b) + c)
		const size_t limb_sum = LIMB_SUM(a_limbs[limb_idx], b_limbs[limb_idx], prev_limb_carry);
		#undef LIMB_SUM

		// set new carry for next limb-addition (if any)
		const size_t next_limb_carry = (limb_sum > t_bignum::limb_max_val());

		// NOTE:
		//   the sum of operands can be larger than radix but smaller than
		//   type::max, which means we need to reduce it modulo the radix
		//
		//   (a + b + c) is always reduced modulo M = type::max (N = radix)
		//
		//   ((a + b + c) + N) % N ==      (a + b + c) when (a + b + c) <  N
		//   ((a + b + c) + N) % N == -N + (a + b + c) when (a + b + c) >= N
		//
		m_limbs[limb_idx] = (limb_sum + t_bignum::limb_radix()) % t_bignum::limb_radix();

		if (limb_idx == 0) {
			// last limb, no wrap-around carry
			m_flags[BIGNUM_COND_OFLOW_FLAG] |= (next_limb_carry == 1);
			return;
		}

		assert(m_limbs[limb_idx - 1] == 0);

		// propagate carry to next limb-addition
		m_limbs[limb_idx - 1] = next_limb_carry;
	}

	void sub_limbs(const t_bignum& a_limbs, const t_bignum& b_limbs, size_t ab_limb_order) {
		const size_t limb_idx = c_num_limbs - ab_limb_order - 1;

		// get carry from last limb-subtraction (if any)
		const size_t prev_limb_carry = m_limbs[limb_idx];

		// due to the carry it is not possible to reliably detect
		// limb-type underflow without casting them to larger type
		//
		// e.g. a rule like (a - b - c) % N > max(a, b) will fail
		// for ((a=0 - b=9 - c=0) % N=10) == 1  <=  max(a=0, b=9)
		//
		#define LIMB_DIF(a, b, c) (static_cast<size_t>(a) - static_cast<size_t>(b) - c)
		const size_t limb_dif = LIMB_DIF(a_limbs[limb_idx], b_limbs[limb_idx], prev_limb_carry);
		#undef LIMB_DIF

		// set new carry for next limb-subtraction (if any)
		const size_t next_limb_carry = (limb_dif > t_bignum::limb_max_val());

		// NOTE:
		//   (a - b - c) is always reduced modulo M = type::max (N = radix)
		//
		//   ((a - b - c) + N) % N ==      (a - b - c)  when (a - b - c) >= 0
		//   ((a - b - c) + N) % N == N - |(a - b - c)| when (a - b - c) <  0
		//
		m_limbs[limb_idx] = (limb_dif + t_bignum::limb_radix()) % t_bignum::limb_radix();

		if (limb_idx == 0) {
			// last limb, no wrap-around carry
			m_flags[BIGNUM_COND_UFLOW_FLAG] |= (next_limb_carry == 1);
			return;
		}

		assert(m_limbs[limb_idx - 1] == 0);

		// propagate carry to next limb-subtraction
		m_limbs[limb_idx - 1] = next_limb_carry;
	}

	void mul_limbs(const t_bignum& a_limbs, const t_bignum& b_limbs, size_t a_limb_order, size_t b_limb_order) {
		const size_t limb_idx_a = c_num_limbs - a_limb_order - 1;
		const size_t limb_idx_b = c_num_limbs - b_limb_order - 1;
		const size_t limb_idx_c = c_num_limbs - (a_limb_order + b_limb_order) - 1;

		// NOTE:
		//   this requires a type twice as wide as t_limb_type
		//   otherwise we only get the result modulo type::max
		//   (i.e. the low-order part)
		const size_t limb_mul = static_cast<size_t>(a_limbs[limb_idx_a]) * static_cast<size_t>(b_limbs[limb_idx_b]);

		if (limb_mul == 0)
			return;

		const t_limb_type limb_lo_val = limb_mul % t_bignum::limb_radix();
		const t_limb_type limb_hi_val = limb_mul / t_bignum::limb_radix();

		bool limb_carry = false;

		// overflow if this is the MSB and it can not hold limb_mul
		m_flags[BIGNUM_COND_OFLOW_FLAG] |= (limb_idx_c == 0 && limb_hi_val != 0);

		// limb_idx_c can underflow, in which case it represents a
		// negative index (meaning the result does not have enough
		// limbs to store limb_mul in its entirety)
		if (limb_idx_c <  c_num_limbs) { m_limbs[limb_idx_c    ] += limb_lo_val; limb_carry |= (m_limbs[limb_idx_c    ] > t_bignum::limb_max_val()); }
		if (limb_idx_c >=           1) { m_limbs[limb_idx_c - 1] += limb_hi_val; limb_carry |= (m_limbs[limb_idx_c - 1] > t_bignum::limb_max_val()); }

		if (!limb_carry)
			return;

		// propagate carry across limbs
		for (size_t n = limb_idx_c; n > 0; n -= 1) {
			if (m_limbs[n] >= t_bignum::limb_max_val()) {
				assert((m_limbs[n] / t_bignum::limb_max_val()) == 1);

				m_limbs[n - 1] += 1;
				m_limbs[n    ] -= t_bignum::limb_max_val();
			}

			// terminate propagation early if possible
			if (m_limbs[n - 1] < t_bignum::limb_max_val()) {
				break;
			}
		}

		// handle last limb (MSB), no wrap-around
		m_limbs[0] %= t_bignum::limb_radix();
	}


private:
	// least-significant limb is [num_limbs-1]
	// all limbs together represent the value
	//
	//   limbs[N-1] * (M ^ (N-1)) +
	//   limbs[N-2] * (M ^ (N-2)) +
	//   limbs[N-k] * (M ^ (N-k)) +
	//   limbs[  0] * (M ^ (  0)) =
	//
	//   SUM_{i=0:N-1} (limbs[i] * (M^i))
	//
	// where N is the number of limbs and M is
	// equal to the radix (aka the number-base)
	t_limb_type m_limbs[c_num_limbs];

	// flags to indicate special conditions during ops
	bool m_flags[BIGNUM_CONDITION_FLAGS];
};



typedef t_bignum<unsigned  char,  4> t_bignum_uc4;
typedef t_bignum<unsigned  char, 32> t_bignum_uc32;
typedef t_bignum<unsigned short,  8> t_bignum_us8;
typedef t_bignum<unsigned   int, 16> t_bignum_ui16;



int main() {
	const unsigned char a_limbs[4] = {9, 3, 3, 3};
	const unsigned char b_limbs[4] = {9, 7, 0, 5};

	t_bignum_uc4 a(a_limbs);
	t_bignum_uc4 b(b_limbs);
	t_bignum_uc4 c;

	assert(!(a == b));
	assert( (a != b));
	assert( (a <  b));
	assert(!(a >  b));
	assert( (a <= b));
	assert(!(a >= b));

	c = a +  b; c.print();
	c = a -  b; c.print();
	c = a *  b; c.print();
	c = a /  b; c.print();
	c = a %  b; c.print();
	c = a &  b; c.print();
	c = a |  b; c.print();
	c = a ^  b; c.print();
	c = a >> 2; c.print();
	c = b << 2; c.print();

	return 0;
}

