#ifndef SIMPLE_SPATIAL_HASH_GRID_HDR
#define SIMPLE_SPATIAL_HASH_GRID_HDR

#include <cassert>
#include <cstddef>
#include <vector>


#if 1
template<typename type, size_t size> struct t_vec {
	t_vec(type _x = type(0), type _y = type(0), type _z = type(0)) {
		x = _x;
		y = _y;
		z = _z;
	}

	t_vec<type, size> operator - (const t_vec& v) const { return (t_vec<type, size>(x - v.x, y - v.y, z - v.z)); }
	t_vec<type, size> operator * (const type s) const { return (t_vec<type, size>(x * s, y * s, z * s)); }

	type operator [] (size_t i) const { assert(i < size); return *(&x + i); }

	type sql() const { return (x*x + y*y + z*z); }

	type x;
	type y;
	type z;
};

typedef t_vec<size_t, 2> t_vec2i;
typedef t_vec<size_t, 3> t_vec3i;
typedef t_vec< float, 3> t_vec3f;
#endif


template<typename t_point, typename t_query> class t_spatial_hash_grid {
public:
	void reserve(size_t num_cells) {
		m_buckets.clear();
		m_buckets.resize(num_cells, 0);
	}

	void insert(const std::vector<t_point>& points, float radius) {
		set_cell_radius(radius);
		compute_aa_bbox(points);
		compute_indices(points);
	}

	bool gather(const std::vector<t_point>& points, t_query& query) {
		const t_vec3f query_pos = query.get_pos();

		const t_vec3f min_dist = query_pos - m_bbox_mins;
		const t_vec3f max_dist = m_bbox_maxs - query_pos;

		// ignore queries outside our 3D bounding box of points
		for (unsigned int i = 0; i < 3; i++) {
			if (min_dist[i] < 0.0f) return false;
			if (max_dist[i] < 0.0f) return false;
		}

		// get the number of cells between bbox_mins and query_pos
		// round this to an integer to obtain the 3D spatial index
		// of the cell containing query_pos
		const t_vec3f coor3f = min_dist * m_cell_size_inv;
		const t_vec3f coor3i = t_vec3f(size_t(coor3f.x), size_t(coor3f.y), size_t(coor3f.z));
		const t_vec3f frac3f = coor3f - coor3i;

		const size_t px = coor3i.x;
		const size_t py = coor3i.y;
		const size_t pz = coor3i.z;

		// also look in the 2^3 adjacent spatial cells whose member
		// points can (potentially) overlap us; cell-size is double
		// the point radius
		// NOTE: is the 0 < coor3f < 0.5 case handled correctly here?
		const size_t pxo = px + ((frac3f.x >= 0.5f) * 2 - 1);
		const size_t pyo = py + ((frac3f.y >= 0.5f) * 2 - 1);
		const size_t pzo = pz + ((frac3f.z >= 0.5f) * 2 - 1);

		const t_vec3i cells[] = {
			{px , py , pz }, // (x    , y    , z    )
			{px , py , pzo}, // (x    , y    , z+/-1)
			{px , pyo, pz }, // (x    , y+/-1, z    )
			{px , pyo, pzo}, // (x    , y+/-1, z+/-1)
			{pxo, py , pz }, // (x+/-1, y    , z    )
			{pxo, py , pzo}, // (x+/-1, y    , z+/-1)
			{pxo, pyo, pz }, // (x+/-1, y+/-1, z    )
			{pxo, pyo, pzo}, // (x+/-1, y+/-1, z+/-1)
		};

		for (size_t cell_idx = 0; cell_idx < (sizeof(cells) / sizeof(cells[0])); cell_idx++) {
			const t_vec2i& range = get_index_range(get_bucket_index(cells[cell_idx]));

			for (size_t range_idx = range.x; range_idx < range.y; range_idx++) {
				const size_t point_idx = m_indices[range_idx];

				const t_point& point = points[point_idx];
				const t_vec3f& point_pos = point.get_pos();

				if ((query_pos - point_pos).sql() > m_radius_sq)
					continue;

				query.add(point);
			}
		}

		return true;
	}

private:
	void compute_aa_bbox(const std::vector<t_point>& points) {
		m_bbox_mins = t_vec3f( 1e9f);
		m_bbox_maxs = t_vec3f(-1e9f);

		for (size_t i = 0; i < points.size(); i++) {
			const t_vec3f& pos = points[i].get_pos();

			for (unsigned int j = 0; j < 3; j++) {
				m_bbox_maxs[j] = std::max(m_bbox_maxs[j], pos[j]);
				m_bbox_mins[j] = std::min(m_bbox_mins[j], pos[j]);
			}
		}
	}

	void compute_indices(const std::vector<t_point>& points) {
		assert(!points.empty());
		assert(!m_buckets.empty());

		std::fill(m_buckets.begin(), m_buckets.end(), 0);

		m_indices.clear();
		m_indices.resize(points.size(), 0);

		// for each bucket, count the number of points mapping into it
		for (size_t i = 0; i < points.size(); i++) {
			const t_vec3f& p_pos = points[i].get_pos();
			const size_t b_idx = get_bucket_index(p_pos);

			m_buckets[b_idx]++;
		}

		// run exclusive prefix-sum, s.t. for each bucket i the index-range
		// of all points within it is given by (buckets[i + 1] - buckets[i])
		// buckets[i] holds the start of this range, buckets[i + 1] the end
		size_t cell_sum = 0;

		for (size_t i = 0; i < m_buckets.size(); i++) {
			const size_t num_points = m_buckets[i];
			m_buckets[i] = cell_sum;
			cell_sum += num_points;
		}

		// finally, for each point j look up its bucket again and use the
		// prefix-sum value stored there as an index which maps back to j
		//
		// after this, R = (buckets[i] - buckets[i - 1]) is the index-range
		// of points in bucket i and indices[R.x: R.y - 1] holds the actual
		// point-indices j for bucket i (avoids the requirement for buckets
		// to explicitly store points, but assumes they are static)
		for (size_t j = 0; j < points.size(); j++) {
			const t_vec3f& p_pos = points[j].get_pos();

			const size_t b_idx = get_bucket_index(p_pos);
			const size_t p_idx = m_buckets[b_idx]++;

			m_indices[p_idx] = j;
		}
	}


	void set_cell_radius(float radius) {
		m_radius     = radius;
		m_radius_sq  = radius * radius;

		m_cell_size     = m_radius * 2.0f;
		m_cell_size_inv = 1.0f / m_cell_size;
	}


	t_vec2i get_index_range(size_t bucket_idx) const {
		if (bucket_idx == 0)
			return t_vec2i(0, m_buckets[0]);

		return (t_vec2i(m_buckets[bucket_idx - 1], m_buckets[bucket_idx]));
	}

	size_t get_bucket_index(const t_vec3i& coord) const {
		const size_t x = coord.x * 73856093;
		const size_t y = coord.y * 19349663;
		const size_t z = coord.z * 83492791;

		// "hash" the spatial coordinates to form an index into m_buckets
		// any number of buckets can represent the volume of spatial cells
		// inside the bounding-box
		return ((x ^ y ^ z) % m_buckets.size());
    }

	size_t get_bucket_index(const t_vec3f& point) const {
		const t_vec3f min_dist = point - m_bbox_mins;

		// first calculate which virtual spatial cell the 3D spatial
		// coordinate <point> is in based on bounding box dimensions
		// (found during grid construction), then map it to a linear
		// bucket index
		// const t_vec3i coor3i = {std::floor(m_cell_size_inv * min_dist.x), std::floor(m_cell_size_inv * min_dist.y), std::floor(m_cell_size_inv * min_dist.z)}
		const t_vec3f coor3f = min_dist * m_cell_size_inv;
		const t_vec3i coor3i = {size_t(coor3f.x), size_t(coor3f.y), size_t(coor3f.z)};

		return (get_bucket_index(coor3i));
	}

private:
	t_vec3f m_bbox_mins;
	t_vec3f m_bbox_maxs;

	std::vector<size_t> m_indices;
	std::vector<size_t> m_buckets;

	float m_radius;
	float m_radius_sq;
	float m_cell_size;
	float m_cell_size_inv;
};

#endif

