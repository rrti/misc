#include <cassert>
#include <cstdio>
#include <complex>
#include <vector>

template<typename type>
struct t_complex_num {
public:
	t_complex_num(type real = type(0), type imag = type(0)): m_real(real), m_imag(imag) {
	}

	t_complex_num(const std::complex<type>& c): m_real(c.real()), m_imag(c.imag()) {
	}

	t_complex_num conjugate() const { return (t_complex_num(m_real, -m_imag)); }

	t_complex_num  operator *  (const type& s) const { return (t_complex_num(m_real * s, m_imag * s)); }
	t_complex_num  operator /  (const type& s) const { return (t_complex_num(m_real / s, m_imag / s)); }
	t_complex_num& operator *= (const type& s)       { return ((*this) = (*this) * s); }
	t_complex_num& operator /= (const type& s)       { return ((*this) = (*this) / s); }

	t_complex_num  operator *  (const t_complex_num& c) const { return (t_complex_num(real() * c.real() - imag() * c.imag(), real() * c.imag() + imag() * c.real())); }
	t_complex_num  operator +  (const t_complex_num& c) const { return (t_complex_num(real() + c.real(), imag() + c.imag())); }
	t_complex_num  operator -  (const t_complex_num& c) const { return (t_complex_num(real() - c.real(), imag() - c.imag())); }
	t_complex_num& operator += (const t_complex_num& c) { return ((*this) = (*this) + c); }

	const type& real() const { return m_real; }
	const type& imag() const { return m_imag; }

	type& real() { return m_real; }
	type& imag() { return m_imag; }

private:
	type m_real;
	type m_imag;
};

typedef t_complex_num< float> t_complex32f;
typedef t_complex_num<double> t_complex64f;



static bool is_power_of_two(size_t i) {
	return ((i & (i - 1)) == 0);
}

template<typename type>
static void print_complex_vector(const std::vector< t_complex_num<type> >& v) {
	printf("[%s]\n", __FUNCTION__);

	for (size_t i = 0; i < v.size(); i++) {
		printf("\tv[%lu]=<r=%f,i=%f>\n", i, v[i].real(), v[i].imag());
	}
}



template<typename type>
std::vector< t_complex_num<type> > calc_roots(size_t N) {
	std::vector< t_complex_num<type> > roots(N * 2);

	// inverse roots
	for (size_t i = 0; i < N; i++) {
		const type t = (M_PI * 2 *  1 * i) / N;
		const auto c = t_complex_num<type>(std::cos(t), std::sin(t));

		roots[i] = c;
	}
	// forward roots
	for (size_t i = 0; i < N; i++) {
		const type t = (M_PI * 2 *  -1 * i) / N;
		const auto c = t_complex_num<type>(std::cos(t), std::sin(t));

		roots[i + N] = c;
	}

	return roots;
}

template<typename type, int sign>
void calc_fft(std::vector< t_complex_num<type> >& elems, const std::vector< t_complex_num<type> >& roots) {
	size_t N = elems.size();
	size_t j = 0;

	// negative sign means *forward* FFT for which we need the *second* <N> roots
	const t_complex_num<type>* root = &roots[0] + (N * (sign < 0));

	for (size_t i = 0; i < (N - 1); ++i) {
		size_t m = N >> 1;

		if (i < j) 
			std::swap(elems[i], elems[j]);

		j ^= m;

		while ((j & m) == 0) {
			m >>= 1; j ^= m;
		}
	}

	// note: this executes a power-of-two non-recursive FFT
	for (size_t j = 1; j < N; j *= 2) {
		for (size_t m = 0; m < j; ++m) {
			const t_complex_num<type> w = root[N * m / (j * 2)];

			for (unsigned int i = m; i < N; i += (j * 2)) {
				const t_complex_num<type> zi = elems[i];
				const t_complex_num<type>  t = w * elems[i + j];

				elems[i    ] = zi + t;
				elems[i + j] = zi - t;
			}
		}
	}
}



template<typename type>
void fft_forward(std::vector< t_complex_num<type> >& elems, const std::vector< t_complex_num<type> >& roots) {
	calc_fft<type, -1>(elems, roots);
}

template<typename type>
void fft_inverse(std::vector< t_complex_num<type> >& elems, const std::vector< t_complex_num<type> >& roots) {
	calc_fft<type,  1>(elems, roots);
}



template<typename type>
std::vector< t_complex_num<type> > fft_multiply(
	std::vector< t_complex_num<type> >& p,
	std::vector< t_complex_num<type> >& q
) {
	assert(p.size() == q.size());
	assert(is_power_of_two(p.size()));
	assert(is_power_of_two(q.size()));
	assert(is_power_of_two(p.size() + q.size()));

	const std::vector< t_complex_num<type> >& v = calc_roots<type>(p.size() + q.size());
	      std::vector< t_complex_num<type> > r;

	r.resize(p.size() + q.size());

	// pad p and q with zero-terms
	for (size_t i = p.size(); i < r.size(); i++) {
		p.push_back(t_complex_num<type>());
		q.push_back(t_complex_num<type>());
	}

	// compute DFT's
	fft_forward<type>(p, v);
	fft_forward<type>(q, v);

	// pointwise-multiply in the frequency domain
	for (size_t k = 0; k < r.size(); k++) {
		r[k] = p[k] * q[k];
	}

	fft_inverse<type>(r, v);

	// normalize the result terms
	for (size_t k = 0; k < r.size(); k++) {
		r[k] /= r.size();
	}

	return r;
}



int main() {
	std::vector<t_complex32f> p = {t_complex32f(4), t_complex32f(3), t_complex32f(2), t_complex32f(1)};
	std::vector<t_complex32f> q = {t_complex32f(1), t_complex32f(2), t_complex32f(3), t_complex32f(4)};

	print_complex_vector(p);
	print_complex_vector(q);
	print_complex_vector(fft_multiply<float>(p, q));
	return 0;
}

