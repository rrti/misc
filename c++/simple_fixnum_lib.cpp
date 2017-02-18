#include <algorithm>
#include <cassert>
#include <cstdint>
#include <limits>

template<typename t> static int sign(t v) {
	if (v > 0) return  1;
	if (v < 0) return -1;
	return 0;
}

#if 0
template<typename t> static bool is_signed_type(t) { return (sign(t(-1)) != sign(t(1))); }
#else
template<typename t> static bool is_signed_type(t) { return (t(-1) < t(1)); }
#endif



#if 0
template<typename t_integ_type, typename t_fract_type> struct t_fixnum {
public:
	t_fixnum(t_integ_type integ_val = t_integ_type(0), t_fract_type fract_val = t_fract_type(0), int8_t sign_val = 1) {
		assert(!is_signed_type(integ_val));
		assert(!is_signed_type(fract_val));
		assert(fract_val < fract_scale());
		assert(sign_val == -1 || sign_val == 1);

		set_sign_val(sign_val);
		set_integ_val(integ_val);
		set_fract_val(fract_val);
	}

	t_fixnum& operator = (const t_fixnum& fn) {
		set_sign_val(fn.sign_val());
		set_integ_val(fn.integ_val());
		set_fract_val(fn.fract_val());
		return *this;
	}


	bool operator < (const t_fixnum& fn) const {
		if (sign_val() < fn.sign_val()) return true;
		if (sign_val() > fn.sign_val()) return false;

		assert(sign_val() == fn.sign_val());

		const t_integ_type a =    integ_val() *    sign_val();
		const t_integ_type b = fn.integ_val() * fn.sign_val();

		// both operands have equal signs
		if (a < b) return true;
		if (a > b) return false;
		// both operands have equal integer parts
		return (fract_val() < fn.fract_val());
	}

	bool operator > (const t_fixnum& fn) const {
		if (sign_val() < fn.sign_val()) return false;
		if (sign_val() > fn.sign_val()) return true;

		assert(sign_val() == fn.sign_val());

		const t_integ_type a =    integ_val() *    sign_val();
		const t_integ_type b = fn.integ_val() * fn.sign_val();

		// both operands have equal signs
		if (a > b) return true;
		if (a < b) return false;
		// both operands have equal integer parts
		return (fract_val() > fn.fract_val());
	}

	bool operator == (const t_fixnum& fn) const {
		if (integ_val() != fn.integ_val()) return false;
		if (fract_val() != fn.fract_val()) return false;
		if (sign_val() != fn.sign_val()) return false;
		return true;
	}
	bool operator != (const t_fixnum& fn) const { return !((*this) == fn); }
	bool operator <= (const t_fixnum& fn) const { return ((*this) < fn || (*this) == fn); }
	bool operator >= (const t_fixnum& fn) const { return ((*this) > fn || (*this) == fn); }


	t_fixnum operator + (const t_fixnum& fn) const {
		t_fixnum r;

		if (sign_val() == fn.sign_val()) {
			// r = (a + b) *OR* r = (-a + -b) = -(|a| + |b|)
			const uint64_t a =    raw_val();
			const uint64_t b = fn.raw_val();

			r.set_integ_val((a + b) / fract_scale());
			r.set_fract_val((a + b) % fract_scale());
			r.set_sign_val(sign_val());
		} else {
			// r = -a + b = b - |a| if a < 0
			// r =          a - |b| if b < 0
			r = (sign_val() == -1)? (fn - abs()): ((*this) - fn.abs());
		}

		return r;
	}

	t_fixnum operator - (const t_fixnum& fn) const {
		t_fixnum r;

		if (sign_val() == fn.sign_val()) {
			// r = (a - |b|) *OR* r = (-a - -b) = (-a + b) = b - |a|
			const uint64_t a =    raw_val();
			const uint64_t b = fn.raw_val();

			if (abs() >= fn.abs()) {
				r.set_integ_val((a - b) / fract_scale());
				r.set_fract_val((a - b) % fract_scale());
				r.set_sign_val(1 * sign_val());
			} else {
				r.set_integ_val((b - a) / fract_scale());
				r.set_fract_val((b - a) % fract_scale());
				r.set_sign_val(-1 * sign_val());
			}
		} else {
			// r = -a -  b = -(|a| +  b) if a < 0
			// r =  a - -b =    a  + |b| if b < 0
			r = (sign_val() == -1)? ((abs() + fn).neg()): ((*this) + fn.abs());
		}

		return r;
	}

	t_fixnum operator * (const t_fixnum& fn) const {
		t_fixnum r;

		const uint64_t a =    raw_val();
		const uint64_t b = fn.raw_val();

		r.set_integ_val((a * b) / (fract_scale() * fract_scale()));
		r.set_fract_val((a * b) % (fract_scale() * fract_scale()));
		r.set_fract_val(r.fract_val() / fract_scale());
		r.set_sign_val(sign_val() * fn.sign_val());
		return r;
	}

	t_fixnum operator / (const t_fixnum& fn) const {
		#if 0
		t_fixnum r;

		const uint64_t a =    raw_val();
		const uint64_t b = fn.raw_val();

		r.set_integ_val((a / b) / fract_scale());
		r.set_fract_val((a / b) % fract_scale());
		r.set_sign_val(sign_val() / fn.sign_val());
		return r;
		#else
		return ((*this) * fn.inv());
		#endif
	}


	t_fixnum inv() const {
		assert((*this) != zero_val());

		const t_integ_type numer = fract_scale() * fract_scale();
		const t_integ_type denom = raw_val(); // i * S + f

		t_fixnum r;
		r.set_integ_val((numer / denom) / fract_scale());
		r.set_fract_val((numer / denom) % fract_scale());
		r.set_sign_val(sign_val());
		return r;
	}

	t_fixnum abs() const { return (t_fixnum(integ_val(), fract_val(),               1)); }
	t_fixnum neg() const { return (t_fixnum(integ_val(), fract_val(), sign_val() * -1)); }

	t_fixnum min(const t_fixnum& fn) const { return (((*this) <= fn)? (*this): fn); }
	t_fixnum max(const t_fixnum& fn) const { return (((*this) >= fn)? (*this): fn); }


	void set_integ_val(t_integ_type i) { m_integ_val = i; }
	void set_fract_val(t_fract_type f) { m_fract_val = f; }
	void set_sign_val(uint8_t v) { m_sign_val = v; }

	t_integ_type integ_val() const { return m_integ_val; }
	t_fract_type fract_val() const { return m_fract_val; }
	int8_t sign_val() const { return m_sign_val; }

	uint64_t raw_val() const { return (static_cast<uint64_t>(integ_val()) * fract_scale() + fract_val()); }

	bool is_negative() const { return (m_sign_val == -1); }
	bool is_positive() const { return (m_sign_val ==  1); }

	float to_float() const {
		return (m_sign_val * ((integ_val() * 1.0f) + (fract_val() * 1.0f) / fract_scale()));
	}


	static t_fixnum zero_val() { return (t_fixnum(0, 0)); }
	static t_fixnum unit_val() { return (t_fixnum(1, 0)); }

	// scale is limited to floor(sqrt(fract_type::max)) with this representation; see operator*
	static t_fract_type fract_scale() { return (std::sqrt(max_fract_val())); }

	static t_integ_type max_integ_val() { return (std::numeric_limits<t_integ_type>::max()); }
	static t_fract_type max_fract_val() { return (std::numeric_limits<t_fract_type>::max()); }

private:
	// this uses the simplest representation: all operators work directly
	// on the linear sum i * S + f for a number i.f with integer part <i>
	// and fractional part <f> (where S is the desired scaling factor and
	// f is interpreted as an integer 0 < f < S) which loses sqrt(N) bits
	// of precision
	//
	t_integ_type m_integ_val;
	t_fract_type m_fract_val;

	int8_t m_sign_val;
};

typedef t_fixnum<uint16_t, uint16_t> t_fixnum16p16;
typedef t_fixnum<uint32_t, uint32_t> t_fixnum32p32;

#else

template<typename value_type = uint32_t, uint32_t fract_bits = 16> struct t_fixnum {
public:
	t_fixnum(): m_value(0) {
		assert(fract_bits >         0u  );
		assert(fract_bits < total_bits());
	}
	t_fixnum(float f): m_value(0) {
		assert(fract_bits >         0u  );
		assert(fract_bits < total_bits());

		m_value += ((value_type(f) & integ_upper()) << integ_shift());
		m_value += (value_type((f - value_type(f)) * fract_scale()));
	}
	t_fixnum(value_type i, value_type f): m_value(0) {
		assert(fract_bits >         0u  );
		assert(fract_bits < total_bits());

		m_value = ((i & integ_upper()) << integ_shift()) | ((f & fract_upper()) << 0);
	}

	bool operator == (const t_fixnum& fn) const { return (m_value == fn.get_value()); }
	bool operator != (const t_fixnum& fn) const { return (m_value != fn.get_value()); }
	bool operator <  (const t_fixnum& fn) const { return (m_value <  fn.get_value()); }
	bool operator >  (const t_fixnum& fn) const { return (m_value >  fn.get_value()); }
	bool operator <= (const t_fixnum& fn) const { return (m_value <= fn.get_value()); }
	bool operator >= (const t_fixnum& fn) const { return (m_value <= fn.get_value()); }

	t_fixnum& operator = (const t_fixnum& fn) {
		m_value = fn.get_value(); return *this;
	}

	t_fixnum operator + (const t_fixnum& fn) const { t_fixnum r; r.set_value(get_value() + fn.get_value()); return r; }
	t_fixnum operator - (const t_fixnum& fn) const { t_fixnum r; r.set_value(get_value() - fn.get_value()); return r; }

	t_fixnum operator * (const t_fixnum& fn) const {
		const value_type a = get_integ(), c = fn.get_integ();
		const value_type b = get_fract(), d = fn.get_fract();

		// (a.b) * (c.d) = a*c + (a*d)/(S^1) + (b*c)/(S^1) + (b*d)/(S^2)
		// (division by S^1 is already implicit in the fractional terms
		// so it can be crossed out)
		//
		const value_type i = (a * c);
		const value_type f = (a * d) + (b * c) + (b * d) / fract_scale();

		return (t_fixnum(i + f / fract_scale(), f % fract_scale()));
	}

	t_fixnum operator / (const t_fixnum& fn) const {
		return ((*this) * fn.inv());
	}

	t_fixnum inv() const {
		assert((*this) != zero_val());

		// note: this will overflow with more than 32 bits of fractional precision
		const uint64_t numer = (static_cast<uint64_t>(fract_scale()) * static_cast<uint64_t>(fract_scale()));
		const uint64_t denom = (static_cast<uint64_t>(get_integ()) * fract_scale()) | get_fract();

		t_fixnum r;
		r.set_value(numer / denom);
		return r;
	}

	t_fixnum min(const t_fixnum& fn) const { return (((*this) <= fn)? (*this): fn); }
	t_fixnum max(const t_fixnum& fn) const { return (((*this) >= fn)? (*this): fn); }

	t_fixnum floor() const { return (t_fixnum(get_integ(), 0)); }
	t_fixnum ceil() const { return (t_fixnum(get_integ() + (get_fract() != 0), 0)); }

	void set_value(value_type v) { m_value = v; }

	value_type get_value() const { return m_value; }
	value_type get_integ() const { return ((m_value >> integ_shift()) & integ_upper()); }
	value_type get_fract() const { return ((m_value >>             0) & fract_upper()); }

	float to_float() const {
		float f = 0.0f;

		f +=  (get_integ() * 1.0f);
		f += ((get_fract() * 1.0f) / fract_scale());
		return f;
	}


	static t_fixnum zero_val() { return (t_fixnum(0, 0)); }
	static t_fixnum unit_val() { return (t_fixnum(1, 0)); }

	static value_type total_bits() { return (sizeof(value_type) * 8u); }

	static value_type integ_shift() { return (      fract_bits); }
	static value_type fract_scale() { return (1u << fract_bits); }

	static value_type integ_upper() { return ((1u << (total_bits() - fract_bits)) - 1u); }
	static value_type fract_upper() { return ((1u << (               fract_bits)) - 1u); } // fract_scale - 1

private:
	// x:y unsigned fixed-point representation can represent numbers
	// from 0.0 to (2^x - 1).(2^y - 1), where the decimal value maps
	// to the [0, 1> interval
	value_type m_value;
};

typedef t_fixnum<uint16_t,  8u> t_fixnum08p08;
typedef t_fixnum<uint32_t, 16u> t_fixnum16p16;
#endif



int main() {
	assert( is_signed_type(5 ));
	assert(!is_signed_type(5u));
	return 0;
}

