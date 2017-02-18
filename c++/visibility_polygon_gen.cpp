#include <cassert>
#include <cmath>
#include <cstddef> // size_t (outside std::)
#include <cstdint> // uint32_t

#include <algorithm>
#include <limits>
#include <vector>

template<typename type> struct t_tuple2 {
public:
	t_tuple2(type _x = type(0), type _y = type(0)) {
		m_x = _x;
		m_y = _y;
	}

	t_tuple2 operator + (const t_tuple2& t) const { return (t_tuple2(x() + t.x(), y() + t.y())); }
	t_tuple2 operator - (const t_tuple2& t) const { return (t_tuple2(x() - t.x(), y() - t.y())); }
	t_tuple2 operator * (const type s) const { return (t_tuple2(x() * s, y() * s)); }
	t_tuple2 operator - () const { return (t_tuple2(-x(), -y())); }

	t_tuple2 abs() const { return (t_tuple2(std::abs(x()), std::abs(y()))); }
	t_tuple2 normalize() const {
		assert(sqr_len() > 0.0f);
		return ((*this) * (1.0f / len()));
	}

	type dot(const t_tuple2& t) const { return (x() * t.x() + y() * t.y()); }
	type sqr_len() const { return (dot(*this)); }
	type     len() const { return (std::sqrt(sqr_len())); }

	type  operator [] (size_t idx) const { assert(idx < 2); return *(&m_x + idx); }
	type& operator [] (size_t idx)       { assert(idx < 2); return *(&m_x + idx); }

	type  x() const { return m_x; }
	type  y() const { return m_y; }
	type& x()       { return m_x; }
	type& y()       { return m_y; }

	bool equals(const t_tuple2& tup, const t_tuple2& eps = t_tuple2(0.001f, 0.001f)) const {
		return (std::abs(x() - tup.x()) <= eps.x() && std::abs(y() - tup.y()) <= eps.y());
	}

private:
	type m_x;
	type m_y;
};

typedef t_tuple2< float> t_float2;
typedef t_tuple2<size_t> t_uint2;



struct t_ray {
	t_ray() {}
	t_ray(const t_float2& pos, const t_float2& dir) {
		m_pos = pos;
		m_dir = dir;
	}

	const t_float2& pos() const { return m_pos; }
	const t_float2& dir() const { return m_dir; }

	// parametric position
	t_float2 pos(float t) const { return (m_pos + m_dir * t); }

private:
	t_float2 m_pos;
	t_float2 m_dir;
};



struct t_line {
public:
	t_line() {}
	t_line(const t_uint2& i, const t_float2& n) {
		m_indices = i;
		m_normal = n;
	}

	const t_uint2& indices() const { return m_indices; }
	const t_float2& normal() const { return m_normal; }

private:
	// indices of vertices making up this segment
	t_uint2 m_indices;
	// determines which side of this segment is front
	t_float2 m_normal;
};



struct t_vert {
public:
	t_vert() {}
	t_vert(const t_float2& c, const t_uint2& i) {
		m_coords = c;
		m_indices = i;
	}

	const t_float2& coords() const { return m_coords; }
	const t_uint2& indices() const { return m_indices; }

	      t_float2& coords()       { return m_coords; }
	      t_uint2& indices()       { return m_indices; }

private:
	t_float2 m_coords;

	// indices of lines that start or end at this vertex
	// (assume simple scene topology and only allow two)
	t_uint2 m_indices;
};



struct t_poly {
public:
	t_poly() { m_coords.reserve(3); }
	t_poly(size_t n) { m_coords.reserve(n); }

	void add_point(const t_float2& c) {
		m_coords.emplace_back(c);
	}

	void tesselate(const t_float2& pov, std::vector<t_poly>& tris) {
		tris.reserve(m_coords.size());

		for (size_t n = 0; n < m_coords.size(); n++) {
			const t_float2 v = (m_coords[(n + 0)                  ] - pov).normalize();
			const t_float2 w = (m_coords[(n + 1) % m_coords.size()] - pov).normalize();

			// test for colinearity
			if (v.dot(w) > 0.99995f)
				continue;

			t_poly tri(3);
			tri.add_point(pov);
			tri.add_point(m_coords[(n + 0)                  ]);
			tri.add_point(m_coords[(n + 1) % m_coords.size()]);
			tris.emplace_back(tri);
		}
	}

private:
	std::vector<t_float2> m_coords;
};



struct t_scene* g_scene = nullptr;
struct t_scene {
public:
	enum {
		INT_TYPE_NONE = 0,
		INT_TYPE_PASS = 1,
		INT_TYPE_STOP = 2,
	};

	t_scene() {
		assert(g_scene == nullptr);
		g_scene = this;

		m_pov.resize(max_pov_idx() + 1);
	}

	~t_scene() {
		assert(g_scene == this);
		g_scene = nullptr;
	}


	const t_ray& get_pov(size_t idx) const { return m_pov[idx]; }

	void set_pov(size_t idx                  ) { assert(idx < max_pov_idx()); m_pov[max_pov_idx()] = m_pov[idx]; }
	void set_pov(size_t idx, const t_ray& pov) { assert(idx < max_pov_idx()); m_pov[        idx  ] =   pov;      }

	void add_vert(const t_vert& v) { m_verts.emplace_back(v); }
	void add_line(const t_line& l) { m_lines.emplace_back(l); }

	void trace_rays(t_poly& vis_poly) const {
		// sort vertices by increasing angle w.r.t. POV
		// this way the visibility-polygon can be built
		// easily, by "connecting the dots" of each ray
		// intersection
		// operates on a copy because lines store fixed
		// indices into m_verts which must remain valid
		std::vector<t_vert> sorted_verts(m_verts.size());

		std::copy(m_verts.begin(), m_verts.end(), sorted_verts.begin());
		std::sort(sorted_verts.begin(), sorted_verts.end(), compare_vertices);

		// get the active POV
		const t_ray& pov = m_pov[max_pov_idx()];

		for (size_t n = 0; n < sorted_verts.size(); n++) {
			const t_vert& v = sorted_verts[n];
			const t_ray r = t_ray(pov.pos(), (v.coords() - pov.pos()).normalize());

			trace_ray(r, vis_poly);
		}
	}

private:
	void trace_ray(const t_ray& ray, t_poly& poly) const {
		t_float2 sp = ray.pos(std::numeric_limits<float>::max());
		t_float2 pp = ray.pos(std::numeric_limits<float>::max());
		t_float2 ip;

		// test ray against each line-segment (silly)
		// note that a ray can generate multiple (convex) corner
		// intersections before terminating, but only one should
		// be tracked for the final polygon
		for (size_t n = 0; n < m_lines.size(); n++) {
			switch (intersect_line(ray, m_lines[n], &ip)) {
				case INT_TYPE_NONE: {
					continue;
				} break;
				case INT_TYPE_PASS: {
					// convex corner
					if ((ip - ray.pos()).sqr_len() < (pp - ray.pos()).sqr_len()) {
						pp = ip;
					}
				} break;
				case INT_TYPE_STOP: {
					// concave corner or blocking line
					if ((ip - ray.pos()).sqr_len() < (sp - ray.pos()).sqr_len()) {
						sp = ip;
					}
				} break;
			}
		}

		// include a *passing* intersection-point only if it
		// is closer than the nearest blocking intersection
		// (t_poly::tesselate can handle colinearity)
		//
		// FIXME:
		//   needs to be added SECOND if ray passes "to the left" of convex corner
		//   needs to be added FIRST if ray passes "to the right" of convex corner
		//   (because the sweep sorts by increasing angle w.r.t. POV)
		if ((pp - ray.pos()).sqr_len() < (sp - ray.pos()).sqr_len())
			poly.add_point(pp);

		poly.add_point(sp);
	}


	uint8_t intersect_line(const t_ray& ray, const t_line& line, t_float2* ip) const {
		const t_vert& v0 = m_verts[ (line.indices()).x() ];
		const t_vert& v1 = m_verts[ (line.indices()).y() ];

		// note: line direction is *not* normalized, so <tw> values
		// between 0 and 1 actually mean an intersection point lies
		// within the line's length
		const t_float2& wp = v0.coords();
		const t_float2  wd = v1.coords() - wp;

		const t_float2& rp = ray.pos();
		const t_float2& rd = ray.dir();

		if (rd.dot(line.normal()) >= 0.0f)
			return INT_TYPE_NONE;
		if (rd.equals(wd) || rd.equals(-wd))
			return INT_TYPE_NONE;

		const float tw = (rd.x() * (wp.y() - rp.y()) + rd.y() * (rp.x() - wp.x())) / (wd.x() * rd.y() - wd.y() * rd.x());
		const float tr = (wp.x() + wd.x() * tw - rp.x()) / rd.x();

		if ((tr >= 0.0f) && (tw >= 0.0f && tw <= 1.0f)) {
			assert(ip != nullptr);

			// test if ray glances either corner of this line
			if (tw <= 0.005f && convex_corner(v0, ray)) {
				*ip = v0.coords();
				return INT_TYPE_PASS;
			}
			if (tw >= 0.995f && convex_corner(v1, ray)) {
				*ip = v1.coords();
				return INT_TYPE_PASS;
			}

			*ip = ray.pos(tr);
			return INT_TYPE_STOP;
		}

		return INT_TYPE_NONE;
	}

	// test if the lines at this vertex form a convex or
	// a concave corner with respect to the incoming ray
	// (whether it can pass or is blocked respectively)
	bool convex_corner(const t_vert& v, const t_ray& r) const {
		const t_line& w0 = m_lines[ (v.indices()).x() ];
		const t_line& w1 = m_lines[ (v.indices()).y() ];

		const t_float2& dir = r.dir();

		const float dp0 = dir.dot(w0.normal());
		const float dp1 = dir.dot(w1.normal());
		return (dp0 >= 0.0f || dp1 >= 0.0f);
	}


	static float pseudo_angle(const t_float2& v) {
		// sort vectors without calculating actual angles
		const float angle = v.x() / (std::abs(v.x()) + std::abs(v.y()));
		const float alpha = (v.y() < 0.0f);

		// return (sign(v.x()) * (angle - 1.0f));
		// return (sign(v.y()) * (1.0f - angle));
		//
		return ((angle - 1.0f) * (0.0f + alpha)  +  (1.0f - angle) * (1.0f - alpha));
	}

	static bool compare_vertices(const t_vert& p, const t_vert& q) {
		const t_ray& pov = g_scene->get_pov(t_scene::max_pov_idx());

		const t_float2 dp = p.coords() - pov.pos();
		const t_float2 dq = q.coords() - pov.pos();

		return (pseudo_angle(dp) < pseudo_angle(dq));
	}

	static size_t max_pov_idx() { return 4; }

private:
	std::vector<t_vert> m_verts;
	std::vector<t_line> m_lines;
	std::vector<t_ray> m_pov;
};



static const t_float2 SCENE_POVS[] = {
	t_float2( 0.0f, -1.0f),
	t_float2( 0.0f,  1.0f),
	t_float2( 1.0f,  0.0f),
	t_float2(-1.0f,  0.0f),
};

static const t_float2 VERT_COORDS[] = {
	t_float2(-10.0f, -10.0f), // v0 (tl)
	t_float2( 10.0f, -10.0f), // v1 (tr)
	t_float2( 10.0f,  10.0f), // v2 (br)
	t_float2(-10.0f,  10.0f), // v3 (bl)

	t_float2( 7.0f, -8.0f), // v4
	t_float2( 9.0f, -8.0f), // v5
	t_float2( 9.0f, -6.0f), // v6
	t_float2( 7.0f, -6.0f), // v7
};

static const t_float2 LINE_NORMALS[] = {
	t_float2( 0.0f,  1.0f), // l0 (tl-tr)
	t_float2(-1.0f,  0.0f), // l1 (tr-br)
	t_float2( 0.0f, -1.0f), // l2 (br-bl)
	t_float2( 1.0f,  0.0f), // l3 (bl-tl)

	t_float2( 0.0f, -1.0f), // l4
	t_float2( 1.0f,  0.0f), // l5
	t_float2( 0.0f,  1.0f), // l6
	t_float2(-1.0f,  0.0f), // l7
};

// which vertices make up which line
static const t_uint2 LINE_VERT_IDS[] = {
	t_uint2(0, 1), // l0
	t_uint2(1, 2), // l1
	t_uint2(2, 3), // l2
	t_uint2(3, 0), // l3

	t_uint2(4, 5), // l4
	t_uint2(5, 6), // l5
	t_uint2(6, 7), // l6
	t_uint2(7, 4), // l7
};



int main() {
	t_scene scene;
	t_poly poly;

	std::vector<t_vert> verts;
	std::vector<t_line> lines;
	std::vector<t_poly> tris;

	for (size_t n = 0; n < (sizeof(VERT_COORDS) / sizeof(VERT_COORDS[0])); n++) {
		verts.emplace_back(t_vert(VERT_COORDS[n], t_uint2(-1u, -1u)));
	}
	for (size_t n = 0; n < (sizeof(LINE_VERT_IDS) / sizeof(LINE_VERT_IDS[0])); n++) {
		lines.emplace_back(t_line(LINE_VERT_IDS[n], LINE_NORMALS[n]));

		// line <n> starts at vertex v0 and ends at vertex v1
		t_line& l = lines.back();
		t_vert& v0 = verts[ (l.indices()).x() ];
		t_vert& v1 = verts[ (l.indices()).y() ];

		assert((v0.indices()).x() == -1u); (v0.indices()).x() = n;
		assert((v1.indices()).y() == -1u); (v1.indices()).y() = n;
	}

	for (size_t n = 0; n < verts.size(); n++) { scene.add_vert(verts[n]); }
	for (size_t n = 0; n < lines.size(); n++) { scene.add_line(lines[n]); }

	for (size_t n = 0; n < (sizeof(SCENE_POVS) / sizeof(SCENE_POVS[0])); n++) {
		scene.set_pov(n, t_ray(t_float2(), SCENE_POVS[n]));
	}

	// set active POV and sweep out the visibility-polygon
	scene.set_pov(0);
	scene.trace_rays(poly);

	// split it into triangles for rendering
	poly.tesselate((scene.get_pov(0)).pos(), tris);
	return 0;
}

