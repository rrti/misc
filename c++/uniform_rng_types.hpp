#ifndef UNIFORM_RNG_TYPES_HDR
#define UNIFORM_RNG_TYPES_HDR

#include <chrono>
#include <random>

namespace rng {
	template<uint32_t MUL = 214013u> struct t_basic_lcg32 {
	public:
		typedef uint16_t res_type;
		typedef uint32_t val_type;

		t_basic_lcg32(const val_type _val = def_val, const val_type _seq = def_seq) { seed(_val, _seq); }
		t_basic_lcg32(const t_basic_lcg32& rng) { *this = rng; }

		void seed(const val_type init_val = def_val, const val_type init_seq = def_seq) {
			val = init_val;
			seq = init_seq;
		}

		res_type operator ()(                    ) { return ( next(     )); }
		res_type operator ()(const res_type bound) { return (bnext(bound)); }

		res_type min() const { return min_res; }
		res_type max() const { return max_res; }

		res_type  next(                    ) { return (((val = (val * MUL + seq)) >> 16) & max_res); }
		res_type bnext(const res_type bound) { return (next() % bound); }

	public:
		static constexpr res_type min_res = 0;
		static constexpr res_type max_res = 0x7fff;
		static constexpr val_type def_val = 0;
		static constexpr val_type def_seq = 2531011u;

	private:
		val_type val = 0;
		val_type seq = 0;
	};


	template<uint64_t MUL = 6364136223846793005ull> struct t_xshrr_pcg64 {
	public:
		typedef uint32_t res_type;
		typedef uint64_t val_type;

		t_xshrr_pcg64(const val_type _val = def_val, const val_type _seq = def_seq) { seed(_val, _seq); }
		t_xshrr_pcg64(const t_xshrr_pcg64& rng) { *this = rng; }

		void seed(const val_type init_val = def_val, const val_type init_seq = def_seq) {
			val = 0u;
			seq = (init_seq << 1u) | 1u;

			next();
			val += init_val;
			next();
		}

		res_type operator ()(                    ) { return ( next(     )); }
		res_type operator ()(const res_type bound) { return (bnext(bound)); }

		res_type min() const { return min_res; }
		res_type max() const { return max_res; }
		res_type next() {
			const val_type prev_val = val;

			// advance internal state
			val = prev_val * MUL + seq;

			// xsh_rr_64_32; calculate output using old state to maximize ILP
			const res_type xsh = ((prev_val >> 18u) ^ prev_val) >> 27u;
			const res_type rot =   prev_val >> 59u;
			return ((xsh >> rot) | (xsh << ((-rot) & 31)));
		}

		res_type bnext(const res_type bound) {
			const res_type n = -bound % bound;
			res_type r = 0;

			// xsh_rr_64_32_boundedrand; output a uniformly
			// distributed number <r> where 0 <= r < bound
			// need to make the range of the RNG a multiple
			// of bound to avoid bias, so drop outputs less
			// than a threshold
			for (r = next(); r < n; r = next());

			return (r % bound);
		}

	public:
		static constexpr res_type min_res = std::numeric_limits<res_type>::min();
		static constexpr res_type max_res = std::numeric_limits<res_type>::max();
		static constexpr val_type def_val = 0x853c49e6748fea9bULL;
		static constexpr val_type def_seq = 0xda3e39cb94b95bdbULL;

	private:
		val_type val = 0;
		val_type seq = 0;
	};
};



template<typename t_real = double> struct t_uniform_real_rng {
public:
	typedef  rng::t_xshrr_pcg64<>  m_sampler_type;
	// typedef  std::mt19937  m_sampler_type;
	// typedef  std::default_random_engine  m_sampler_type;

	typedef  typename std::uniform_real_distribution<t_real>  m_distrib_type;
	typedef  typename m_distrib_type::param_type  m_distrib_param_type;

	t_uniform_real_rng(uint64_t seed = 0) { set_seed(seed); }

	void set_seed(uint64_t seed) {
		if (seed == 0) {
			const std::chrono::system_clock::time_point cur_time = std::chrono::system_clock::now();
			const std::chrono::nanoseconds run_time = std::chrono::duration_cast<std::chrono::nanoseconds>(cur_time.time_since_epoch());

			m_sampler.seed(run_time.count());
		} else {
			m_sampler.seed(seed);
		}
	}

	void set_range(t_real min, t_real max) {
		// range is [min := min(b - a), max := max(b - a)]
		m_distrib.reset();
		m_distrib.param(m_distrib_param_type{min, max});
	}

	// same as std::generate, but with implicit generator
	template<typename t_iter> void gen_samples(t_iter first, t_iter last) {
		while (first != last) {
			(*first)++ = (*this)();
		}
	}

	t_real operator ()() { return (m_distrib(m_sampler)); }

	t_real min_range() const { return (m_distrib.a()); }
	t_real max_range() const { return (m_distrib.b()); }
	t_real val_range() const { return (max_range() - min_range()); }

private:
	m_sampler_type m_sampler;
	m_distrib_type m_distrib;
};



template<typename t_int = uint64_t> struct t_uniform_int_rng {
public:
	typedef  rng::t_xshrr_pcg64<>  m_sampler_type;
	// typedef  std::mt19937  m_sampler_type;
	// typedef  std::default_random_engine  m_sampler_type;

	typedef  typename std::uniform_int_distribution<t_int>  m_distrib_type;
	typedef  typename m_distrib_type::param_type  m_distrib_param_type;

	t_uniform_int_rng(uint64_t seed = 0) { set_seed(seed); }

	void set_seed(uint64_t seed) {
		if (seed == 0) {
			const std::chrono::system_clock::time_point cur_time = std::chrono::system_clock::now();
			const std::chrono::nanoseconds run_time = std::chrono::duration_cast<std::chrono::nanoseconds>(cur_time.time_since_epoch());

			m_sampler.seed(run_time.count());
		} else {
			m_sampler.seed(seed);
		}
	}

	void set_range(t_int min, t_int max) {
		m_distrib.reset();
		m_distrib.param(m_distrib_param_type{min, max});
	}

	template<typename t_iter> void gen_samples(t_iter first, t_iter last) {
		while (first != last) {
			(*first)++ = (*this)();
		}
	}

	t_int operator ()() { return (m_distrib(m_sampler)); }

	t_int min_range() const { return (m_distrib.a()); }
	t_int max_range() const { return (m_distrib.b()); }
	t_int val_range() const { return (max_range() - min_range()); }

private:
	m_sampler_type m_sampler;
	m_distrib_type m_distrib;
};


typedef t_uniform_real_rng< float> t_uniform_f32_rng;
typedef t_uniform_real_rng<double> t_uniform_f64_rng;

typedef t_uniform_int_rng<uint32_t> t_uniform_u32_rng;
typedef t_uniform_int_rng<uint64_t> t_uniform_u64_rng;

#endif

