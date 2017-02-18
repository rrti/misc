#include <array>
#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdint>

#define IMPROVED_PERLIN 1
#define RANDOMIZE_GRADS 0
#define REPEATING_NOISE 0

#define PERM_TABLE_SIZE (1 <<  8)
#define GRAD_TABLE_SIZE (1 <<  8)
#define LERP_TABLE_SIZE (1 << 16)

static const uint8_t RAW_PERM_TABLE[PERM_TABLE_SIZE] = {
	151, 160, 137,  91,  90,  15, 131,  13, 201,  95,  96,  53, 194, 233,   7, 225,
	140,  36, 103,  30,  69, 142,   8,  99,  37, 240,  21,  10,  23, 190,   6, 148,
	247, 120, 234,  75,   0,  26, 197,  62,  94, 252, 219, 203, 117,  35,  11,  32,
	 57, 177,  33,  88, 237, 149,  56,  87, 174,  20, 125, 136, 171, 168,  68, 175,
	 74, 165,  71, 134, 139,  48,  27, 166,  77, 146, 158, 231,  83, 111, 229, 122,
	 60, 211, 133, 230, 220, 105,  92,  41,  55,  46, 245,  40, 244, 102, 143,  54,
	 65,  25,  63, 161,   1, 216,  80,  73, 209,  76, 132, 187, 208,  89,  18, 169,
	200, 196, 135, 130, 116, 188, 159,  86, 164, 100, 109, 198, 173, 186,   3,  64,
	 52, 217, 226, 250, 124, 123,   5, 202,  38, 147, 118, 126, 255,  82,  85, 212,
	207, 206,  59, 227,  47,  16,  58,  17, 182, 189,  28,  42, 223, 183, 170, 213,
	119, 248, 152,   2,  44, 154, 163,  70, 221, 153, 101, 155, 167,  43, 172,   9,
	129,  22,  39, 253,  19,  98, 108, 110,  79, 113, 224, 232, 178, 185, 112, 104,
	218, 246,  97, 228, 251,  34, 242, 193, 238, 210, 144,  12, 191, 179, 162, 241,
	 81,  51, 145, 235, 249,  14, 239, 107,  49, 192, 214,  31, 181, 199, 106, 157,
	184,  84, 204, 176, 115, 121,  50,  45, 127,   4, 150, 254, 138, 236, 205,  93,
	222, 114,  67, 29,   24,  72, 243, 141, 128, 195,  78,  66, 215,  61, 156, 180,
};

static uint8_t MOD_PERM_TABLE[PERM_TABLE_SIZE * 2];



static int32_t fastfloor(float x) {
	// also handles negative numbers
	const int32_t i = x;
	const int32_t a = ((x < 0.0f) && ((i - x) != 0.0f));

	return ((i * (1 - a))  +  (i - 1) * (0 + a));
}

static inline float lerp(float p, float q, float a) {
	return (p * (1.0f - a) + q * a);
}



struct t_float2 {
public:
	explicit t_float2(float _x = 0.0f, float _y = 0.0f) {
		m_x = _x;
		m_y = _y;
	}

	t_float2  operator +  (const t_float2 v) const { return (t_float2(x() + v.x(), y() + v.y())); }
	t_float2& operator += (const t_float2 v) { x() += v.x(); y() += v.y(); return *this; }

	t_float2  operator *  (const float s) const { return (t_float2(x() * s, y() * s)); }
	t_float2& operator *= (const float s) { x() *= s; y() *= s; return *this; }

	static float dot(const t_float2 v, const t_float2 w) { return (v.x() * w.x() + v.y() * w.y()); }

	float sqr_len() const { return (dot(*this, *this)); }
	float     len() const { return (std::sqrt(sqr_len())); }

	float  x() const { return m_x; }
	float  y() const { return m_y; }
	float& x()       { return m_x; }
	float& y()       { return m_y; }

private:
	float m_x;
	float m_y;
};



template<uint32_t grid_size = 10> struct t_perlin_noise_gen {
public:
	t_perlin_noise_gen() {
		#if (RANDOMIZE_GRADS == 1 && IMPROVED_PERLIN == 0)
		for (size_t n = 0; n < GRAD_TABLE_SIZE; n++) {
			//
			// precompute gradient vectors (via Monte Carlo rejection)
			// according to the original specification of Perlin noise
			//
			//   in 2D, sample directions distributed around unit 2-sphere
			//   in 3D, sample directions distributed around unit 3-sphere
			//   in 4D, sample directions distributed around unit 4-sphere
			//   etc
			//
			while (m_grad_table[n].sqr_len() > 1.0f) {
				m_grad_table[n].x() = (random() * 1.0f) / RAND_MAX;
				m_grad_table[n].y() = (random() * 1.0f) / RAND_MAX;
			}

			m_grad_table[n] *= (1.0f / m_grad_table[n].len());
		}
		#endif

		// precompute the fade-function over the range [0, 1]
		for (size_t n = 0; n < LERP_TABLE_SIZE; n++) {
			m_lerp_table[n] = fade_func((n * 1.0f) / LERP_TABLE_SIZE);
		}

		for (size_t n = 0; n < PERM_TABLE_SIZE; n++) {
			MOD_PERM_TABLE[n                  ] = RAW_PERM_TABLE[n];
			MOD_PERM_TABLE[n + PERM_TABLE_SIZE] = RAW_PERM_TABLE[n];
		}
	}


	float sample(t_float2 p) const {
		#if (REPEATING_NOISE == 1)
		p.x() = std::fmod(p.x(), grid_size * 1.0f);
		p.y() = std::fmod(p.y(), grid_size * 1.0f);
		#endif

		// calculate grid corner coordinates (top-left)
		int32_t xi = fastfloor(p.x());
		int32_t yi = fastfloor(p.y());

		// cell-relative coordinates (always in [0, 1])
		const float dx = p.x() - xi; assert(dx >= 0.0f && dx < 1.0f);
		const float dy = p.y() - yi; assert(dy >= 0.0f && dy < 1.0f);

		// keep coordinates in permutation-table bounds
		xi &= (PERM_TABLE_SIZE - 1); assert(xi >= 0 && xi < PERM_TABLE_SIZE);
		yi &= (PERM_TABLE_SIZE - 1); assert(yi >= 0 && yi < PERM_TABLE_SIZE);


		// map [0, 1] to [0, LERP_TABLE_SIZE - 1] and look up the blend-factors
		const float fdx = m_lerp_table[static_cast<uint32_t>(dx * LERP_TABLE_SIZE)]; // fade_func(dx)
		const float fdy = m_lerp_table[static_cast<uint32_t>(dy * LERP_TABLE_SIZE)]; // fade_func(dy)

		// look up the gradient at each grid corner
		// and calculate influence of each gradient
		//
		// note: distance-vectors point inward (from corners to <dx, dy>)
		const float n00 = dot_grad(hash_coor(   (xi),    (yi)),  t_float2(dx       , dy       ));
		const float n10 = dot_grad(hash_coor(inc(xi),    (yi)),  t_float2(dx - 1.0f, dy       ));
		const float n01 = dot_grad(hash_coor(   (xi), inc(yi)),  t_float2(dx       , dy - 1.0f));
		const float n11 = dot_grad(hash_coor(inc(xi), inc(yi)),  t_float2(dx - 1.0f, dy - 1.0f));

		// blend contribution from each corner using bilinear interpolation
		const float nx0 = lerp(n00, n10, fdx);
		const float nx1 = lerp(n01, n11, fdx);
		const float nxy = lerp(nx0, nx1, fdy);

		return nxy;
	}

	float sample_octaves(const t_float2 p, uint32_t num_octaves, float amplitude_mult) const {
		float noise_sum = 0.0f;
		float max_value = 0.0f;

		float frequency = 1.0f;
		float amplitude = 1.0f;

		for (uint32_t i = 0; i < num_octaves; i++) {
			noise_sum += (sample(p * frequency) * amplitude);
			max_value += amplitude;

			amplitude *= amplitude_mult;
			frequency *= 2.0f;
		}

		return (noise_sum / max_value);
	}

private:
	float fade_func(float t) const {
		assert(t >= 0.0f);
		assert(t <= 1.0f);

		#if (IMPROVED_PERLIN == 0)
		// original Hermite blending function
		return (t * t * (3.0f - 2.0f * t));
		#else
		return (t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f));
		#endif
	}


	// based on hash-code, (pseudo-randomly) pick a gradient vector
	// and calculate the dot product with relative coordinate vector
	// <vec>
	//
	// equivalent but slower explicit table-lookup implementation:
	//
	//   GRADIENT_VECTORS_2D[] = {
	//     t_float2(1, 0), t_float2(-1,  0),
	//     t_float2(0, 1), t_float2( 0, -1),
	//   }
	//
	//   return (t_float2::dot(GRADIENT_VECTORS_2D[hash & 0x3], vec));
	//
	float dot_grad(uint8_t hash, const t_float2 vec) const {
		#if (RANDOMIZE_GRADS == 1 && IMPROVED_PERLIN == 0)
		return (t_float2::dot(m_grad_table[hash & 0x3], vec));

		#else

		switch (hash & 0x3) {
			case 0x0: { return ( vec.x()); } break;
			case 0x1: { return (-vec.x()); } break;
			case 0x2: { return ( vec.y()); } break;
			case 0x3: { return (-vec.y()); } break;
		}

		return 0.0f;
		#endif
	}

	#if 0
	// same but in three dimensions (12 edge-midpoints of cube)
	// note that one of the coordinate values is always 0 here
	//
	// equivalent but slower explicit table-lookup implementation:
	//
	//   GRADIENT_VECTORS_3D[] = {
	//     t_float3(1, 1, 0), t_float3(-1,  1, 0), t_float3(1, -1,  0), t_float3(-1, -1,  0),
	//     t_float3(1, 0, 1), t_float3(-1,  0, 1), t_float3(1,  0, -1), t_float3(-1,  0, -1),
	//     t_float3(0, 1, 1), t_float3( 0, -1, 1), t_float3(0,  1, -1), t_float3( 0, -1, -1),
	//   };
	//
	//   return (t_float3::dot(GRADIENT_VECTORS_3D[hash & 0xf], vec));
	//
	float dot_grad(uint8_t hash, const t_float3 vec) const {
		#if (RANDOMIZE_GRADS == 1 && IMPROVED_PERLIN == 0)
		return (t_float3::dot(m_grad_table[hash & 0x3], vec));

		#else

		switch (hash & 0xf) {
			case 0x0: { return ( vec.x() + vec.y()); } break;
			case 0x1: { return (-vec.x() + vec.y()); } break;
			case 0x2: { return ( vec.x() - vec.y()); } break;
			case 0x3: { return (-vec.x() - vec.y()); } break;

			case 0x4: { return ( vec.x() + vec.z()); } break;
			case 0x5: { return (-vec.x() + vec.z()); } break;
			case 0x6: { return ( vec.x() - vec.z()); } break;
			case 0x7: { return (-vec.x() - vec.z()); } break;

			case 0x8: { return ( vec.y() + vec.z()); } break;
			case 0x9: { return (-vec.y() + vec.z()); } break;
			case 0xa: { return ( vec.y() - vec.z()); } break;
			case 0xb: { return (-vec.y() - vec.z()); } break;

			// special cases
			case 0xc: { return ( vec.y() + vec.x()); } break;
			case 0xd: { return (-vec.y() + vec.z()); } break;
			case 0xe: { return ( vec.y() - vec.x()); } break;
			case 0xf: { return (-vec.y() - vec.z()); } break;
			default: {} break;
		}

		return 0.0f;
		#endif
	}
	#endif


	uint8_t hash_coor(int32_t xi, int32_t yi) const {
		return MOD_PERM_TABLE[MOD_PERM_TABLE[xi] + yi];
	}

	uint8_t hash_coor(int32_t xi, int32_t yi, int32_t zi) const {
		return MOD_PERM_TABLE[MOD_PERM_TABLE[MOD_PERM_TABLE[xi] + yi] + zi];
	}


	int32_t inc(int32_t i) const {
		#if (REPEATING_NOISE == 0)
		return (i + 1);
		#else
		i += 1;
		i %= grid_size;
		return i;
		#endif
	}

private:
	#if (RANDOMIZE_GRADS == 1 && IMPROVED_PERLIN == 0)
	std::array<t_float2, GRAD_TABLE_SIZE + 1> m_grad_table;
	#endif
	std::array<   float, LERP_TABLE_SIZE + 1> m_lerp_table;
};



int main(int argc, char** argv) {
	srandom((argc > 1)? atoi(argv[1]): time(NULL));

	assert(fastfloor( 1.00f) ==  1);
	assert(fastfloor( 0.02f) ==  0);
	assert(fastfloor( 0.00f) ==  0);
	assert(fastfloor(-0.02f) == -1);
	assert(fastfloor(-1.00f) == -1);

	t_perlin_noise_gen<> png;

	for (size_t y = 0; y < 100; y++) {
		for (size_t x = 0; x < 100; x++) {
			printf("%lu\t%lu\t%f\n", x, y, png.sample_octaves(t_float2(x / 50.0f, y / 40.0f), 4, 0.8f));
		}
	}

	return 0;
}

