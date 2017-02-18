#include <cassert>
#include <cmath>
#include <cstdlib>
#include <cstdio>

#if (USE_OMP == 1)
#include <omp.h>
#endif

#include <algorithm>
#include <limits>
#include <vector>

#include <boost/bind.hpp>
#include <boost/thread.hpp>



// typedef  float real32_t;
typedef double real64_t;

// material types
enum t_mat_type {
	MAT_TYPE_DIFF = 0,
	MAT_TYPE_SPEC = 1,
	MAT_TYPE_REFR = 2,
};

enum t_axis_idx {
	AXIS_IDX_X = 0,
	AXIS_IDX_Y = 1,
	AXIS_IDX_Z = 2,
};

static const real64_t MIN_COOR_VAL = -std::numeric_limits<real64_t>::max();
static const real64_t MAX_COOR_VAL =  std::numeric_limits<real64_t>::max();

static const uint64_t PRIMES[61] = {
	  2,   3,   5,   7,  11,  13,  17,  19,  23,  29,  31,  37,  41,  43,  47,  53,  59,  61,  67,  71, 73, 79,
	 83,  89,  97, 101, 103, 107, 109, 113, 127, 131, 137, 139, 149, 151, 157, 163, 167, 173, 179, 181,
	191, 193, 197, 199, 211, 223, 227, 229, 233, 239, 241, 251, 257, 263, 269, 271, 277, 281, 283
};

static const uint64_t IMAGE_SIZE_X = 1024;
static const uint64_t IMAGE_SIZE_Y =  768;

static const uint64_t NUM_AA_RAYS_X = 2;
static const uint64_t NUM_AA_RAYS_Y = 2;

static const uint64_t MAX_RAYTRACE_DEPTH = 4;



template<typename type> type clamp(const type v, const type vmin, const type vmax) {
	return (std::max(vmin, std::min(v, vmax)));
}
template<typename type> type norm(const type v, const type vmin, const type vmax) {
	return ((v - vmin) / (vmax - vmin));
}
template<typename type> type lerp(const type a, const type b, const real64_t s) {
	return (a * (0.0 + s) + b * (1.0 - s));
}

inline uint8_t tone_map(real64_t x) {
	return (pow(1.0 - exp(-x), 1.0 / 2.2) * 255 + 0.5);
}

inline uint64_t log2(uint64_t x) {
	uint64_t n = 0;
	for (n = 0; x > 1; x >>= 1, ++n) {}
	return n;
}

// calculate the median value of a number <n>
// if n is a power of 2, this will just be n/2
inline uint64_t calc_median_value(uint64_t n) {
	const uint64_t s1 =   1 << log2(n); // s1 = 2^h = n (if n is pow2)
	const uint64_t s2 =  s1 >>       1; // s2 = (2^h)/2 = 2^(h-1)
	const uint64_t  d =         n - s1; // difference (0 if n is pow2)

	assert(n != 0);
	assert(n >= s1);

	if (s2 != 0 && d >= s2)
		return (s1 - 1);

	return (s2 + d);
}

// simple Halton sequence generator
real64_t halton_seq(uint64_t p, uint64_t n) {
	real64_t r = 0.0;
	real64_t f = 1.0;

	// <p> is supposed to be prime
	p = PRIMES[p % (sizeof(PRIMES) / sizeof(PRIMES[0]))];

	while (n > 0) {
		f /= p;
		r += (f * (n % p));
		n /= p;
	}

	assert(r >= 0.0 && r <= 1.0);
	return r;
}



template<typename type> struct t_vec_xyz {
public:
	t_vec_xyz(type x_ = type(0), type y_ = type(0), type z_ = type(0)) {
		m_x = x_;
		m_y = y_;
		m_z = z_;
	}

	t_vec_xyz& operator = (const t_vec_xyz& v) {
		m_x = v.x();
		m_y = v.y();
		m_z = v.z();
		return *this;
	}

	t_vec_xyz& operator += (const t_vec_xyz& v) {
		x() += v.x();
		y() += v.y();
		z() += v.z();
		return *this;
	}

	bool operator != (const t_vec_xyz& v) const {
		return (x() != v.x() || y() != v.y() || z() != v.z());
	}

	t_vec_xyz operator + (const t_vec_xyz& v) const { return (t_vec_xyz(x() + v.x(), y() + v.y(), z() + v.z())); }
	t_vec_xyz operator - (const t_vec_xyz& v) const { return (t_vec_xyz(x() - v.x(), y() - v.y(), z() - v.z())); }
	t_vec_xyz operator % (const t_vec_xyz& v) const { return (t_vec_xyz(y() * v.z() - z() * v.y(), z() * v.x() - x() * v.z(), x() * v.y() - y() * v.x())); }
	t_vec_xyz operator * (type s) const { return (t_vec_xyz(x() * s, y() * s, z() * s)); }
	t_vec_xyz operator / (type s) const { return (t_vec_xyz(x() / s, y() / s, z() / s)); }

	t_vec_xyz mul(const t_vec_xyz& v) const { return (t_vec_xyz(x() * v.x(), y() * v.y(), z() * v.z())); }
	t_vec_xyz abs() const { return (t_vec_xyz(std::abs(x()), std::abs(y()), std::abs(z()))); }
	t_vec_xyz norm() const { return ((*this) * (type(1) / std::sqrt(len_sq()))); }

	type len_sq() const { return (inner(*this)); }
	type inner(const t_vec_xyz& v) const { return (x() * v.x() + y() * v.y() + z() * v.z()); }

	type min() const { return (std::min(x(), std::min(y(), z()))); }
	type max() const { return (std::max(x(), std::min(y(), z()))); }

	type  operator [] (const uint64_t idx) const { assert(idx < 3); return (*(&m_x + idx)); }
	type& operator [] (const uint64_t idx)       { assert(idx < 3); return (*(&m_x + idx)); }

	type  x() const { return m_x; }
	type  y() const { return m_y; }
	type  z() const { return m_z; }
	type& x()       { return m_x; }
	type& y()       { return m_y; }
	type& z()       { return m_z; }

private:
	type m_x;
	type m_y;
	type m_z;
};

typedef t_vec_xyz<real64_t> t_vec64f;
typedef t_vec_xyz<uint64_t> t_vec64u;



struct t_ray {
public:
	t_ray() {}
	t_ray(const t_vec64f& pos, const t_vec64f& dir): m_pos(pos), m_dir(dir) {}

	const t_vec64f& pos() const { return m_pos; }
	const t_vec64f& dir() const { return m_dir; }

	t_vec64f& pos(const t_vec64f& pos) { return (m_pos = pos); }
	t_vec64f& dir(const t_vec64f& dir) { return (m_dir = dir); }

private:
	t_vec64f m_pos; // origin
	t_vec64f m_dir; // direction
};

struct t_ray_intersection {
public:
	uint64_t id() const { return m_object_id; }
	uint64_t& id(uint64_t id) { return (m_object_id = id); }

	const t_vec64f& pos() const { return m_int_pos; }
	const t_vec64f& nrm() const { return m_int_nrm; }

	t_vec64f& pos(const t_vec64f& pos) { return (m_int_pos = pos); }
	t_vec64f& nrm(const t_vec64f& nrm) { return (m_int_nrm = nrm); }

private:
	uint64_t m_object_id;

	t_vec64f m_int_pos; // intersection position
	t_vec64f m_int_nrm; // surface normal at INTP
};

// for non-diffuse Fresnel interactions
struct t_ray_interaction {
public:
	const t_ray& reflection_ray() const { return m_refl_ray; }
	const t_ray& refraction_ray() const { return m_refr_ray; }

	t_ray& reflection_ray(const t_ray& r) { return (m_refl_ray = r); }
	t_ray& refraction_ray(const t_ray& r) { return (m_refr_ray = r); }


	real64_t reflection_angle() const { return m_refl_angle; }
	real64_t reflection_coeff() const { return m_refl_coeff; }

	real64_t& reflection_angle(real64_t a) { return (m_refl_angle = a); }
	real64_t& reflection_coeff(real64_t c) { return (m_refl_coeff = c); }

	bool internal_reflection() const { return (reflection_angle() <= 0.0); }

private:
	t_ray m_refl_ray;
	t_ray m_refr_ray;

	real64_t m_refl_angle;
	real64_t m_refl_coeff;
};



struct t_aabb {
public:
	t_aabb(): m_mins(MAX_COOR_VAL, MAX_COOR_VAL, MAX_COOR_VAL), m_maxs(MIN_COOR_VAL, MIN_COOR_VAL, MIN_COOR_VAL) {
	}

	void fit(const t_vec64f& p) {
		m_mins.x() = std::min(p.x(), m_mins.x());
		m_mins.y() = std::min(p.y(), m_mins.y());
		m_mins.z() = std::min(p.z(), m_mins.z());

		m_maxs.x() = std::max(p.x(), m_maxs.x());
		m_maxs.y() = std::max(p.y(), m_maxs.y());
		m_maxs.z() = std::max(p.z(), m_maxs.z());
	}

	const t_vec64f& mins() const { return m_mins; }
	const t_vec64f& maxs() const { return m_maxs; }

private:
	t_vec64f m_mins;
	t_vec64f m_maxs;
};



struct t_material {
public:
	t_material(t_mat_type type = MAT_TYPE_DIFF): m_type(type) {
	}

	t_vec64f sample_diffuse_reflection_dir(const t_vec64f int_nrm, const uint64_t rnd_seed, const uint64_t photon_id) const {
		// sample hemisphere reflection angles
		const real64_t r1 = 2.0 * M_PI * halton_seq(rnd_seed - 1, photon_id);
		const real64_t r2 = halton_seq(rnd_seed + 0, photon_id);
		const real64_t r2s = std::sqrt(r2);

		// set surface coordinate-basis
		const t_vec64f w = int_nrm;
		const t_vec64f u = ((fabs(w.x()) > 0.1? t_vec64f(0, 1, 0): t_vec64f(1, 0, 0)) % w).norm();
		const t_vec64f v = w % u;

		// set reflection direction
		return (u * std::cos(r1) * r2s  +  v * std::sin(r1) * r2s  +  w * std::sqrt(1.0 - r2));
	}

	t_ray_interaction calc_fresnel_interaction_dirs(
		const t_vec64f ray_dir,
		const t_vec64f int_pos,
		const t_vec64f int_nrm,
		real64_t nc,
		real64_t nt
	) const {
		t_ray_interaction ri;

		// if ray direction and normal are anti-parallel, ray is
		// outside object going in and sign will be positive (+1)
		// otherwise ray is inside object going out, which causes
		// a negative sign (-1) and inverts the normal
		const real64_t sgn = (ray_dir.inner(int_nrm) < 0.0) * 2.0 - 1.0;

		const real64_t nnt = lerp((nc / nt), (nt / nc), sgn > 0.0);
		const real64_t ddn = ray_dir.inner(int_nrm * sgn);
		const real64_t c2t = clamp(1.0 - nnt * nnt * (1.0 - ddn * ddn), 0.0, 1.0);

		ri.reflection_ray(t_ray(int_pos, ray_dir - (int_nrm * sgn) * 2.0 * int_nrm.inner(ray_dir)));
		ri.refraction_ray(t_ray(int_pos, (ray_dir * nnt - (int_nrm * sgn) * ((ddn * nnt + std::sqrt(c2t)))).norm()));

		// Fresnel terms
		const real64_t f0 = ((nt - nc) * (nt - nc)) / ((nt + nc) * (nt + nc));
		const real64_t f1 = 1.0 - lerp(-ddn, (ri.refraction_ray().dir()).inner(int_nrm), sgn > 0.0);

		ri.reflection_angle(c2t);
		ri.reflection_coeff(f0 + (1.0 - f0) * std::pow(f1, 5.0));
		return ri;
	}

	t_mat_type type() const { return m_type; }

private:
	t_mat_type m_type;
};

struct t_sphere {
public:
	t_sphere(const t_vec64f pos, const t_vec64f clr, const t_material& mat, real64_t rad) {
		m_pos = pos;
		m_clr = clr;
		m_mat = mat;
		m_rad = rad;
	}

	real64_t intersect(const t_ray& r) const {
		const t_vec64f dif_vec = m_pos - r.pos();

		// project dif onto dir; difference vector minus
		// this projection is the orthogonal separation
		// (which must be smaller than radius to count)
		const real64_t ort_dst = dif_vec.inner(r.dir());
		const real64_t det_sqr = ort_dst * ort_dst - dif_vec.inner(dif_vec) + m_rad * m_rad;

		if (det_sqr < 0.0)
			return MAX_COOR_VAL;

		const real64_t det = std::sqrt(det_sqr);

		if ((ort_dst - det) > 1e-4) return (ort_dst - det);
		if ((ort_dst + det) > 1e-4) return (ort_dst + det);
		return MAX_COOR_VAL;
	}

	const t_vec64f& pos() const { return m_pos; }
	const t_vec64f& clr() const { return m_clr; }

	const t_material& mat() const { return m_mat; }

private:
	t_vec64f m_pos; // position
	t_vec64f m_clr; // RGB color (diffuse albedo)
	t_material m_mat;

	real64_t m_rad;
};



struct t_photon {
public:
	t_photon() {}
	t_photon(const t_vec64f& pos, const t_vec64f& pwr) {
		m_pos = pos;
		m_pwr = pwr;
	}

	const t_vec64f& pos() const { return m_pos; }
	const t_vec64f& pwr() const { return m_pwr; }
	      t_vec64f& pos()       { return m_pos; }
	      t_vec64f& pwr()       { return m_pwr; }

private:
	t_vec64f m_pos;
	t_vec64f m_pwr; // current power
};

bool cmpx(const t_photon& a, const t_photon& b) { return ((a.pos()).x() < (b.pos()).x()); }
bool cmpy(const t_photon& a, const t_photon& b) { return ((a.pos()).y() < (b.pos()).y()); }
bool cmpz(const t_photon& a, const t_photon& b) { return ((a.pos()).z() < (b.pos()).z()); }



struct t_kd_tree {
public:
	// traverse tree-structure to collect the <dsts.size()> nearest-neighbors of <pos> in <ngbs>
	bool get_nearest_ngbs(std::vector<t_photon*>& ngbs, std::vector<real64_t>& dsts, const t_vec64f& pos) const {
		assert(!array.empty());
		assert(ngbs.size() == dsts.size());
		traverse(ngbs, dsts, pos, 0);
		return (ngbs[0] != NULL);
	}

	// recursively construct a tree-structure over the photons
	// nodes will need to contain O(log N) entries on average
	void build(std::vector<t_photon>& buf) {
		array.resize(buf.size(), NULL);
		nodes.resize(buf.size(), AXIS_IDX_X);

		build_rec(&buf[0], -1, 0, buf.size());
	}

	void build_rec(t_photon* buf, uint8_t axis_idx, uint64_t node_idx, uint64_t num_nodes) {
		switch (num_nodes) {
			case 0: {                                                         } break;
			case 1: { assert(node_idx < array.size()); array[node_idx] = buf; } break;
			default: {
				// const uint64_t median_val = num_nodes >> 1;
				const uint64_t median_val = calc_median_value(num_nodes);

				assert(num_nodes > median_val);
				assert(node_idx < nodes.size());
				assert(nodes[node_idx] <= AXIS_IDX_Z);

				// sort photons along axis of largest separation
				// but no need to sort along the same axis twice
				if ((nodes[node_idx] = sep_axis_index(buf, num_nodes)) != axis_idx) {
					switch (nodes[node_idx]) {
						case AXIS_IDX_X: { std::sort(buf, buf + num_nodes, cmpx); } break;
						case AXIS_IDX_Y: { std::sort(buf, buf + num_nodes, cmpy); } break;
						case AXIS_IDX_Z: { std::sort(buf, buf + num_nodes, cmpz); } break;
						default: { assert(false); } break;
					}
				}

				// median-value node becomes the root for both subtrees
				array[node_idx] = &buf[median_val];
				assert(nodes[node_idx] <= AXIS_IDX_Z);

				// recurse until we have a single photon left
				build_rec(buf,                  nodes[node_idx], node_idx * 2 + 1,             median_val    );
				build_rec(buf + median_val + 1, nodes[node_idx], node_idx * 2 + 2, num_nodes - median_val - 1);
			} break;
		}
	}

private:
	uint8_t sep_axis_index(const t_photon* photons, uint64_t n) {
		t_aabb aabb;

		// calculate the bounds for this set of photons
		for (uint64_t i = 0; i < n; ++i)
			aabb.fit(photons[i].pos());

		const t_vec64f w = aabb.maxs() - aabb.mins();

		// return dimension of largest difference
		if (w.x() > w.y()) {
			if (w.x() > w.z()) return AXIS_IDX_X;
			if (w.y() > w.z()) return AXIS_IDX_Y;
			return AXIS_IDX_Z;
		}

		if (w.y() > w.z()) return AXIS_IDX_Y;
		if (w.x() > w.z()) return AXIS_IDX_X;
		return AXIS_IDX_Z;
	}

	void traverse(std::vector<t_photon*>& ngbs, std::vector<real64_t>& dsts, const t_vec64f& pos, uint64_t node_idx) const {
		if (node_idx >= array.size())
			return;

		if ((node_idx * 2 + 1) < array.size()) {
			assert(node_idx < nodes.size());
			assert(nodes[node_idx] <= AXIS_IDX_Z);

			const  uint8_t axis_idx     = nodes[node_idx];
			const real64_t axis_dist    = pos[axis_idx] - array[node_idx]->pos()[axis_idx];
			const real64_t axis_dist_sq = axis_dist * axis_dist;

			if (axis_dist < 0.0) {
				// negative extent, visit left subtree
				traverse(ngbs, dsts, pos, node_idx * 2 + 1);

				// dsts[n - 1] is the current largest distance of any of the
				// dsts.size() photons to (the center of) our nearest-neigbor
				// search, and this can change (decrease) during traversal
				//
				if (axis_dist_sq < dsts[dsts.size() - 1]) {
					traverse(ngbs, dsts, pos, node_idx * 2 + 2);
				}
			} else {
				// positive extent, visit right subtree
				traverse(ngbs, dsts, pos, node_idx * 2 + 2);

				if (axis_dist_sq < dsts[dsts.size() - 1]) {
					traverse(ngbs, dsts, pos, node_idx * 2 + 1);
				}
			}
		} /*else*/ {
			// distance between current photon (node) and
			// (the center of) our nearest-neigbor search
			const real64_t dist_sq = (pos - array[node_idx]->pos()).len_sq();

			if (dist_sq >= dsts[dsts.size() - 1])
				return;

			uint64_t lft_idx = 0;
			uint64_t rgt_idx = dsts.size();

			// find the insertion-position for the new photon
			// (use binary-search to do this in O(log n) time)
			while (lft_idx < rgt_idx) {
				const uint64_t mid_idx = (lft_idx + rgt_idx) >> 1;

				if (dist_sq < dsts[mid_idx]) {
					rgt_idx = mid_idx;
				} else {
					lft_idx = mid_idx + 1;
				}
			}

			// make room for the photon to be inserted
			// (distances are in ascending sorted order)
			for (uint64_t j = dsts.size(); --j > lft_idx; ) {
				assert(j < ngbs.size());

				ngbs[j] = ngbs[j - 1];
				dsts[j] = dsts[j - 1];
			}

			ngbs[lft_idx] = array[node_idx];
			dsts[lft_idx] = dist_sq;
		}
	}

private:
	std::vector<t_photon*> array;
	std::vector<uint8_t> nodes;
};



struct t_light {
public:
	const t_vec64f& pos() const { return m_pos; }
	const t_vec64f& pwr() const { return m_pwr; }

	t_vec64f& pos(const t_vec64f& pos) { return (m_pos = pos); }
	t_vec64f& pwr(const t_vec64f& pwr) { return (m_pwr = pwr); }

private:
	t_vec64f m_pos;
	t_vec64f m_pwr;
};

struct t_camera {
public:
	const t_vec64f&  pos() { return m_pos;  }
	const t_vec64f& zdir() { return m_zdir; }
	const t_vec64f& xdir() { return m_xdir; }
	const t_vec64f& ydir() { return m_ydir; }

	t_vec64f&  pos(const t_vec64f&  pos) { return (m_pos  =  pos); }
	t_vec64f& zdir(const t_vec64f& zdir) { return (m_zdir = zdir); }
	t_vec64f& xdir(const t_vec64f& xdir) { return (m_xdir = xdir); }
	t_vec64f& ydir(const t_vec64f& ydir) { return (m_ydir = ydir); }

private:
	t_vec64f m_pos;
	t_vec64f m_zdir;
	t_vec64f m_xdir;
	t_vec64f m_ydir;
};

struct t_scene {
public:
	t_scene(int argc, char** argv) {
		m_num_emission_batches = (argc >= 2)? std::max( 1, atoi(argv[1])):     1;
		m_num_emission_photons = (argc >= 3)? std::max(10, atoi(argv[2])): 10000;
		m_num_radiance_photons = (argc >= 4)? std::max( 1, atoi(argv[3])):    50;

		m_image_size_x = (argc >= 5)? std::max(1, atoi(argv[4])): IMAGE_SIZE_X;
		m_image_size_y = (argc >= 6)? std::max(1, atoi(argv[5])): IMAGE_SIZE_Y;

		m_max_raytrace_depth = (argc >= 7)? std::max(1, atoi(argv[6])): MAX_RAYTRACE_DEPTH;
		m_num_render_threads = (argc >= 8)? std::max(1, atoi(argv[7])): boost::thread::hardware_concurrency();

		m_photons.reserve(m_num_emission_batches * m_num_emission_photons);
		m_objects.reserve(8);
		m_lights.resize(m_num_emission_batches, t_light());
		m_image.resize(m_image_size_x * m_image_size_y, t_vec64f(-1.0, -1.0, -1.0));

		m_photon_maps.resize(m_num_render_threads);
		m_photon_ngbs.resize(m_num_render_threads);
		m_photon_dsts.resize(m_num_render_threads);

		for (uint64_t n = 0; n < m_num_render_threads; n++) {
			m_photon_maps[n].reserve((m_num_emission_batches * m_num_emission_photons) / m_num_render_threads);
			m_photon_ngbs[n].resize(m_num_radiance_photons, NULL);
			m_photon_dsts[n].resize(m_num_radiance_photons, MAX_COOR_VAL);
		}

		for (uint64_t n = 0; n < m_num_emission_batches; n++) {
			m_lights[n].pos(t_vec64f(50.0 * n - 25.0, 60.0, 85.0));
			m_lights[n].pwr(t_vec64f(M_PI * 25000.0, M_PI * 25000.0, M_PI * 25000.0));
		}

		m_camera.pos(t_vec64f(0.0, 45.0, 300.0));
		m_camera.zdir(t_vec64f(0.0, -0.04, -1.0).norm());
		m_camera.xdir(t_vec64f(m_image_size_x * 0.5135 / m_image_size_y));
		m_camera.ydir((m_camera.xdir() % m_camera.zdir()).norm() * 0.5135);

		// "walls"
		add_object(t_sphere(t_vec64f(-1e5 - 50.0, 50.0,                80.0), t_vec64f(0.25, 0.25, 0.75), t_material(MAT_TYPE_SPEC), 1e5)); // left
		add_object(t_sphere(t_vec64f( 1e5 + 50.0, 50.0,                80.0), t_vec64f(0.75, 0.25, 0.25), t_material(MAT_TYPE_DIFF), 1e5)); // right
		add_object(t_sphere(t_vec64f(        0.0, 50.0,         1e5        ), t_vec64f(0.25, 0.75, 0.25), t_material(MAT_TYPE_DIFF), 1e5)); // back
		add_object(t_sphere(t_vec64f(        0.0, 50.0,        -1e5 + 200.0), t_vec64f(0.00, 0.00, 0.00), t_material(MAT_TYPE_DIFF), 1e5)); // front
		add_object(t_sphere(t_vec64f(        0.0,  1e5,                80.0), t_vec64f(0.75, 0.75, 0.75), t_material(MAT_TYPE_DIFF), 1e5)); // ceil
		add_object(t_sphere(t_vec64f(        0.0, -1e5 + 80.0,         80.0), t_vec64f(0.75, 0.75, 0.75), t_material(MAT_TYPE_DIFF), 1e5)); // floor

		// balls
		add_object(t_sphere(t_vec64f(-25.0, 16.5, 47.0), t_vec64f(1.00, 1.00, 1.00) * 0.999, t_material(MAT_TYPE_SPEC), 16.5)); // mirror
		add_object(t_sphere(t_vec64f(  0.0, 16.5, 88.0), t_vec64f(1.00, 1.00, 1.00) * 0.999, t_material(MAT_TYPE_REFR), 16.5)); // glass
		add_object(t_sphere(t_vec64f( 25.0,  8.5, 60.0), t_vec64f(1.00, 1.00, 1.00) * 0.999, t_material(MAT_TYPE_DIFF),  8.5)); // brick
	}

	void render_image() {
		// primary pass: emit photons into scene from each light-source
		// no point creating an image if no photons impacted any surface
		if (!spawn_photon_emitter_threads(t_vec64u(m_num_render_threads, 1)))
			return;

		build_photon_tree();

		// secondary pass: gather per-pixel radiance
		switch (m_num_render_threads) {
			case 1: {
				// 1x1 block
				spawn_radiance_gatherer_threads(t_vec64u(1, 0));
			} break;

			case 2: {
				// 2x1 blocks
				spawn_radiance_gatherer_threads(t_vec64u(1, 2));
			} break;

			case 4: {
				// 2x2 blocks
				spawn_radiance_gatherer_threads(t_vec64u(2, 2));
			} break;

			case 8: {
				// 2x4 blocks
				spawn_radiance_gatherer_threads(t_vec64u(2, 4));
			} break;

			case 16: {
				// 4x4 blocks
				spawn_radiance_gatherer_threads(t_vec64u(4, 4));
			} break;

			case 32: {
				// 8x4 blocks
				spawn_radiance_gatherer_threads(t_vec64u(4, 8));
			} break;

			case 64: {
				// 8x8 blocks
				spawn_radiance_gatherer_threads(t_vec64u(8, 8));
			} break;

			case 128: {
				// 8x16 blocks
				spawn_radiance_gatherer_threads(t_vec64u(8, 16));
			} break;

			case 256: {
				// 16x16 blocks
				spawn_radiance_gatherer_threads(t_vec64u(16, 16));
			} break;

			default: {
				assert(false);
			} break;
		}
	}

	bool write_image(const char* fname) const {
		FILE* f = fopen(fname, "w");

		if (f == NULL)
			return false;

		fprintf(f, "P3\n%lu %lu\n%d\n", m_image_size_x, m_image_size_y, 255);

		for (uint64_t i = 0; i < m_image.size(); ++i) {
			fprintf(f, "%d %d %d ", tone_map(m_image[i].x()), tone_map(m_image[i].y()), tone_map(m_image[i].z()));
		}

		fclose(f);
		return true;
	}

	void add_object(const t_sphere& s) { m_objects.emplace_back(s); }
	void add_photon(const t_photon& p) { m_photons.emplace_back(p); }

	const std::vector<t_sphere>& get_objects() const { return m_objects; }
	const std::vector<t_photon>& get_photons() const { return m_photons; }

private:
	void build_photon_tree() {
		assert(!m_photons.empty());
		m_kd_tree.build(m_photons);
	}

	bool spawn_photon_emitter_threads(const t_vec64u num_threads) {
		if (m_num_render_threads > 1) {
			std::vector<boost::thread*> threads(m_num_render_threads, NULL);

			// require perfect division of labor
			assert((num_threads.x() * num_threads.y()) == m_num_render_threads);
			assert((m_num_emission_photons % (m_num_emission_batches * m_num_render_threads)) == 0);

			// photons emitted per thread per light
			const uint64_t k = m_num_emission_photons / (m_num_emission_batches * m_num_render_threads);

			for (uint64_t n = 0; n < m_num_render_threads; n++) {
				threads[n] = new boost::thread(boost::bind(&t_scene::emit_photons, this, n, k));
			}

			for (uint64_t n = 0; n < threads.size(); n++) {
				threads[n]->join();
				delete threads[n];
			}
		} else {
			emit_photons(0, m_num_emission_batches * m_num_emission_photons);
		}

		// merge the per-thread maps
		for (uint64_t n = 0; n < m_num_render_threads; n++) {
			for (uint64_t k = 0; k < m_photon_maps[n].size(); k++) {
				m_photons.emplace_back(m_photon_maps[n][k]);
			}
		}

		return (!m_photons.empty());
	}

	void emit_photons(const uint64_t thread_id, const uint64_t num_photons) {
		t_ray ray;
		t_vec64f pwr;

		for (uint64_t k = 0; k < m_num_emission_batches; k++) {
			for (uint64_t n = 0; n < num_photons; ++n) {
				// each batch corresponds to a different light
				spawn_photon(&ray, &pwr, thread_id * num_photons + n, k);
				trace_photon(ray, pwr, thread_id, thread_id * num_photons + n, 0, true);
			}
		}
	}

	void spawn_photon(t_ray* ray, t_vec64f* pwr, const uint64_t photon_id, const uint64_t light_id) const {
		const real64_t p = halton_seq(0, photon_id) * 2.0 * M_PI;
		const real64_t t = std::acos(std::sqrt(1.0 - halton_seq(1, photon_id))) * 2.0;
		const real64_t st = std::sin(t);

		ray->dir(t_vec64f(std::cos(p) * st, std::cos(t), std::sin(p) * st));
		ray->pos(m_lights[light_id].pos());

		*pwr = m_lights[light_id].pwr();
	}

	t_vec64f trace_photon(
		const t_ray ray,
		const t_vec64f pwr,
		const uint64_t thread_id,
		const uint64_t photon_id,
		const uint64_t cur_depth,
		bool emission_pass
	) {
		t_ray_intersection ray_int;

		if ((cur_depth >= m_max_raytrace_depth) || !intersect_objects(ray, ray_int))
			return (t_vec64f());

		// photons should always carry energy
		assert(!emission_pass || pwr.x() > 0.0 || pwr.y() > 0.0 || pwr.z() > 0.0);

		const t_vec64f int_pos = ray_int.pos();
		const t_vec64f int_nrm = ray_int.nrm();

		const t_material& obj_mat = m_objects[ray_int.id()].mat();
		const t_vec64f& obj_clr = m_objects[ray_int.id()].clr();

		switch (obj_mat.type()) {
			case MAT_TYPE_DIFF: {
				if (!emission_pass) {
					// only photons that hit diffuse surfaces are stored in the tree!
					return (calc_radiance_estimate(thread_id, int_pos, obj_clr));
				}

				// remember where each emitted photon ends up
				m_photon_maps[thread_id].emplace_back(t_photon(int_pos, pwr));

				if (halton_seq((cur_depth + 1) * 3 + 1, photon_id) < obj_clr.max()) {
					// since we can not possibly trace all diffuse reflections, sample
					// them and weigh each sample by the reciprocal of its probability
					// (fully-black objects do not reflect any photons)
					const t_vec64f dif_dir = obj_mat.sample_diffuse_reflection_dir(int_nrm, (cur_depth + 1) * 3, photon_id);
					const t_vec64f ref_pwr = obj_clr.mul(pwr) * (1.0 / obj_clr.max());

					// diffuse reflection, ray outside
					return (trace_photon(t_ray(int_pos, dif_dir), ref_pwr, thread_id, photon_id, cur_depth + 1, emission_pass));
				}
			} break;

			case MAT_TYPE_SPEC: {
				// specular reflection, ray outside
				const t_vec64f ref_dir = ray.dir() - int_nrm * 2.0 * int_nrm.inner(ray.dir());
				const t_vec64f ref_pwr = trace_photon(t_ray(int_pos, ref_dir), obj_clr.mul(pwr), thread_id, photon_id, cur_depth + 1, emission_pass);

				return (ref_pwr.mul(obj_clr));
			} break;

			case MAT_TYPE_REFR: {
				// mixed (specular) reflection and refraction
				const t_ray_interaction& ri = obj_mat.calc_fresnel_interaction_dirs(ray.dir(), int_pos, int_nrm, 1.0, 1.5);

				if (ri.internal_reflection())
					return (trace_photon(ri.reflection_ray(), pwr, thread_id, photon_id, cur_depth + 1, emission_pass));

				if (!emission_pass) {
					// when gathering, trace both reflection plus refraction
					// the contribution from each depends on the coefficient
					// of reflection
					const t_vec64f& refl_pwr = trace_photon(ri.reflection_ray(), pwr, thread_id, photon_id, cur_depth + 1, emission_pass);
					const t_vec64f& refr_pwr = trace_photon(ri.refraction_ray(), pwr, thread_id, photon_id, cur_depth + 1, emission_pass);
					return (lerp<t_vec64f>(refl_pwr, refr_pwr, ri.reflection_coeff()).mul(obj_clr));
				}

				// reflection-only
				if (halton_seq((cur_depth + 1) * 3 - 1, photon_id) < ri.reflection_coeff())
					return (trace_photon(ri.reflection_ray(), obj_clr.mul(pwr), thread_id, photon_id, cur_depth + 1, emission_pass));

				// refraction-only
				return (trace_photon(ri.refraction_ray(), obj_clr.mul(pwr), thread_id, photon_id, cur_depth + 1, emission_pass));
			} break;
		}

		return (t_vec64f());
	}



	bool intersect_objects(const t_ray& r, t_ray_intersection& i) const {
		// test each object for ray-intersection, save closest
		real64_t cur_dist = MAX_COOR_VAL;
		real64_t min_dist = MAX_COOR_VAL;

		for (uint64_t n = 0; n < m_objects.size(); ++n) {
			if ((cur_dist = m_objects[n].intersect(r)) >= min_dist)
				continue;

			min_dist = cur_dist;

			i.id(n);
			i.pos(r.pos() + r.dir() * min_dist);
			i.nrm((i.pos() - m_objects[i.id()].pos()).norm());
		}

		return (min_dist < MAX_COOR_VAL);
	}



	void spawn_radiance_gatherer_threads(const t_vec64u num_threads) {
		std::vector<boost::thread*> threads(num_threads.x() * num_threads.y(), NULL);

		if (num_threads.y() == 0) {
			gather_radiance(0, t_vec64u(0, 0), t_vec64u(m_image_size_x, m_image_size_y));
			return;
		}

		assert((m_image_size_x % num_threads.x()) == 0);
		assert((m_image_size_y % num_threads.y()) == 0);

		for (uint64_t y = 0; y < num_threads.y(); y++) {
			for (uint64_t x = 0; x < num_threads.x(); x++) {
				const t_vec64u mins((x + 0) * (m_image_size_x / num_threads.x()), (y + 0) * (m_image_size_y / num_threads.y()));
				const t_vec64u maxs((x + 1) * (m_image_size_x / num_threads.x()), (y + 1) * (m_image_size_y / num_threads.y()));

				assert(threads[y * num_threads.x() + x] == NULL);
				threads[y * num_threads.x() + x] = new boost::thread(boost::bind(&t_scene::gather_radiance, this, y * num_threads.x() + x, mins, maxs));
			}
		}

		for (uint64_t n = 0; n < threads.size(); n++) {
			threads[n]->join();
			delete threads[n];
		}
	}

	t_vec64f calc_radiance_estimate(const uint64_t thread_id, const t_vec64f pos, const t_vec64f clr) {
		t_vec64f est_flux;
		t_vec64f est_radi;

		// reset the thread caches
		for (uint64_t n = 0; n < m_num_radiance_photons; n++) {
			m_photon_ngbs[thread_id][n] = NULL;
			m_photon_dsts[thread_id][n] = MAX_COOR_VAL;
		}

		if (!m_kd_tree.get_nearest_ngbs(m_photon_ngbs[thread_id], m_photon_dsts[thread_id], pos))
			return est_radi;

		uint64_t num_ngb_photons = 0;

		// sum up the flux, normalize it (by the distance of the
		// furthest-away neighbor) to get the estimated radiance
		for (uint64_t j = 0; j < m_num_radiance_photons; num_ngb_photons = j++) {
			const t_photon* p = m_photon_ngbs[thread_id][j];

			// no more neighbors in our buffer
			if (p == NULL)
				break;

			est_flux += (p->pwr() / M_PI);
		}

		est_radi = est_flux.mul(clr);
		est_radi = est_radi * (1.0 / (M_PI * m_photon_dsts[thread_id][num_ngb_photons]));
		// est_radi = est_radi * (1.0 / (dsts[num_ngb_photons] * dsts[num_ngb_photons]));
		return est_radi;
	}

	void gather_radiance(const uint64_t thread_id, const t_vec64u mins, const t_vec64u maxs) {
		// inverse of the total number of primary photon rays (inc. AA)
		const real64_t scale = 1.0 / (m_num_emission_batches * m_num_emission_photons * NUM_AA_RAYS_X * NUM_AA_RAYS_Y);
		const real64_t aa_rx = 1.0 / NUM_AA_RAYS_X;
		const real64_t aa_ry = 1.0 / NUM_AA_RAYS_Y;

		#if (USE_OMP == 1)
		#pragma omp parallel for schedule(dynamic, 1)
		#endif
		for (uint64_t y = mins.y(); y < maxs.y(); ++y) {
			for (uint64_t x = mins.x(); x < maxs.x(); ++x) {
				t_vec64f pxl;
				t_vec64f pwr; // dummy

				// anti-aliasing rays
				for (uint64_t v = 0; v < NUM_AA_RAYS_Y; ++v) {
					for (uint64_t u = 0; u < NUM_AA_RAYS_X; ++u) {
						const t_vec64f cam_dx  = m_camera.xdir() * ( ((x + u * aa_rx + (aa_rx * 0.5)) / m_image_size_x) - 0.5);
						const t_vec64f cam_dy  = m_camera.ydir() * (-((y + v * aa_ry + (aa_ry * 0.5)) / m_image_size_y) + 0.5);
						const t_vec64f pxl_dir = cam_dx + cam_dy + m_camera.zdir();
						const t_ray    pxl_ray = t_ray(m_camera.pos() + pxl_dir * 150.0, pxl_dir.norm());

						pxl += trace_photon(pxl_ray, pwr, thread_id, y * IMAGE_SIZE_X + x, 0, false);
					}
				}

				// final pixel values
				assert(m_image[y * m_image_size_x + x].x() == -1.0);
				m_image[y * m_image_size_x + x] = pxl * scale;
			}
		}
	}

private:
	std::vector<t_sphere> m_objects;
	std::vector<t_photon> m_photons;
	std::vector<t_light> m_lights;
	std::vector<t_vec64f> m_image;

	// per-thread caches
	std::vector< std::vector<t_photon > > m_photon_maps;
	std::vector< std::vector<t_photon*> > m_photon_ngbs;
	std::vector< std::vector< real64_t> > m_photon_dsts;

	t_camera m_camera;
	t_kd_tree m_kd_tree;

	uint64_t m_num_emission_batches;
	uint64_t m_num_emission_photons;
	uint64_t m_num_radiance_photons;

	uint64_t m_image_size_x;
	uint64_t m_image_size_y;

	uint64_t m_max_raytrace_depth;
	uint64_t m_num_render_threads;
};



int main(int argc, char** argv) {
	t_scene scene(argc, argv);

	scene.render_image();
	scene.write_image("image.ppm");
	return 0;
}

