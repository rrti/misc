#include <algorithm>
#include <fstream>
#include <limits>

#include <list>
#include <set>
#include <stack>
#include <vector>

#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <ctime>

template<typename type> struct t_point {
public:
	t_point(type x = 0, type y = 0): m_x(x), m_y(y) {}
	t_point(const t_point<type>& p): m_x(p.x()), m_y(p.y()) {}

	bool operator < (const t_point& p) const {
		if (m_x < p.x())
			return true;
		if (m_x == p.x())
			return (m_y < p.y());
		return false;
	}
	bool operator == (const t_point& p) const { return (m_x == p.x() && m_y == p.y()); }
	bool operator != (const t_point& p) const { return (!((*this) == p)); }

	t_point operator - (const t_point& p) const {
		return (t_point(m_x - p.x(), m_y - p.y()));
	}
	t_point operator / (const type s) const {
		return (t_point(m_x / s, m_y / s));
	}

	t_point cross() const {
		return (t_point(m_y, -m_x));
	}

	type dot(const t_point& p) const {
		return (m_x * p.x() + m_y * p.y());
	}
	float radius() const {
		return (std::sqrt(dot(*this)));
	}

	type  x() const { return m_x; }
	type  y() const { return m_y; }
	type& x()       { return m_x; }
	type& y()       { return m_y; }

private:
	type m_x;
	type m_y;
};

typedef t_point<  int> t_point2i;
typedef t_point<float> t_point2f;




struct t_point_bin {
public:
	void add_point(const t_point2i& p) { m_points.push_back(p); }
	const t_point2i& get_point(unsigned int idx) const { return m_points[idx]; }

	unsigned int get_size() const { return m_points.size(); }

private:
	std::vector<t_point2i> m_points;
};


struct t_point_bin_grid {
public:
	t_point_bin_grid(const t_point2i& num_bins, const t_point2i& bin_dims) {
		m_num_bins = num_bins;
		m_bin_dims = bin_dims;

		m_bins.resize(num_bins.x() * num_bins.y());
	}

	t_point2i calc_bin_coors(const t_point2i& p) const {
		t_point2i coors;
		coors.x() = std::max(0, std::min(m_num_bins.x() - 1, int(p.x() / m_bin_dims.x())));
		coors.y() = std::max(0, std::min(m_num_bins.y() - 1, int(p.y() / m_bin_dims.y())));
		return coors;
	}

	const t_point_bin& get_bin(unsigned int idx) const { assert(idx < m_bins.size()); return m_bins[idx]; }
	      t_point_bin& get_bin(unsigned int idx)       { assert(idx < m_bins.size()); return m_bins[idx]; }

	const t_point_bin& get_bin(const t_point2i& c) const { assert((c.x() * c.y()) < m_bins.size()); return m_bins[c.y() * m_num_bins.x() + c.x()]; }
	      t_point_bin& get_bin(const t_point2i& c)       { assert((c.x() * c.y()) < m_bins.size()); return m_bins[c.y() * m_num_bins.x() + c.x()]; }

	std::set<unsigned int> get_edge_bin_indices() const {
		std::set<unsigned int> indices;

		// for each row: work L2R from col 0 and R2L from col N-1
		for (int row_idx = 0; row_idx < m_num_bins.y(); row_idx++) {
			int l_col_idx = 0;
			int r_col_idx = m_num_bins.x() - 1;

			while (m_bins[row_idx * m_num_bins.x() + l_col_idx].get_size() == 0 && l_col_idx < r_col_idx) {
				l_col_idx++;
			}
			while (m_bins[row_idx * m_num_bins.x() + r_col_idx].get_size() == 0 && r_col_idx > l_col_idx) {
				r_col_idx--;
			}

			if (m_bins[row_idx * m_num_bins.x() + l_col_idx].get_size() != 0)
				indices.insert(row_idx * m_num_bins.x() + l_col_idx);
			if (m_bins[row_idx * m_num_bins.x() + r_col_idx].get_size() != 0)
				indices.insert(row_idx * m_num_bins.x() + r_col_idx);
		}

		// for each col: work T2B from row 0 and B2T from row N-1
		for (int col_idx = 0; col_idx < m_num_bins.x(); col_idx++) {
			int t_row_idx = 0;
			int b_row_idx = m_num_bins.y() - 1;

			while (m_bins[t_row_idx * m_num_bins.x() + col_idx].get_size() == 0 && t_row_idx < b_row_idx) {
				t_row_idx++;
			}
			while (m_bins[b_row_idx * m_num_bins.x() + col_idx].get_size() == 0 && b_row_idx > t_row_idx) {
				b_row_idx--;
			}

			if (m_bins[t_row_idx * m_num_bins.x() + col_idx].get_size() != 0)
				indices.insert(t_row_idx * m_num_bins.x() + col_idx);
			if (m_bins[b_row_idx * m_num_bins.x() + col_idx].get_size() != 0)
				indices.insert(b_row_idx * m_num_bins.x() + col_idx);
		}

		return indices;
	}

private:
	std::vector<t_point_bin> m_bins;

	t_point2i m_num_bins;
	t_point2i m_bin_dims;
};



static const unsigned int NUM_DATA_POINTS = 16;
static const unsigned int POINT_COOR_RANGE_X = 100000u;
static const unsigned int POINT_COOR_RANGE_Y = 100000u;
static const unsigned int NUM_BINS_X = std::max(1.0, std::sqrt(NUM_DATA_POINTS) / 5);
static const unsigned int NUM_BINS_Y = std::max(1.0, std::sqrt(NUM_DATA_POINTS) / 5);

static t_point_bin_grid POINT_BIN_GRID(t_point2i(NUM_BINS_X, NUM_BINS_Y), t_point2i(POINT_COOR_RANGE_X / NUM_BINS_X, POINT_COOR_RANGE_Y / NUM_BINS_Y));

static FILE* DATA_POINTS_FILE = NULL;
static FILE* HULL_POINTS_FILE = NULL;

static unsigned int get_seed_point_index(const std::vector<t_point2i>& data_points) {
	unsigned int seed_point_idx = 0;

	for (unsigned int n = 1; n < data_points.size(); n++) {
		if (data_points[n].y() > data_points[seed_point_idx].y()) {
			continue;
		} else {
			// if the lowest y-coordinate is shared by more than one point
			// in the set, prefer the point with the lowest x-coordinate
			if (data_points[n].y() == data_points[seed_point_idx].y() && data_points[n].x() >= data_points[seed_point_idx].x()) {
				continue;
			}

			seed_point_idx = n;
		}
	}

	return seed_point_idx;
}

static void randomize_data_points(std::vector<t_point2i>& data_points) {
	for (unsigned int n = 0; n < data_points.size(); n++) {
		// positive coordinates only so we know if the seed has been initialized
		data_points[n].x() = std::max(1u, static_cast<unsigned int>(random()) % POINT_COOR_RANGE_X);
		data_points[n].y() = std::max(1u, static_cast<unsigned int>(random()) % POINT_COOR_RANGE_Y);
	}
}

static void add_data_points_to_bins(const std::vector<t_point2i>& data_points, t_point_bin_grid& bins) {
	for (unsigned int n = 0; n < data_points.size(); n++) {
		const t_point2i& coors = bins.calc_bin_coors(data_points[n]);
		      t_point_bin& bin = bins.get_bin(coors);

		bin.add_point(data_points[n]);
	}
}
 




struct t_convex_hull_builder {
public:
	virtual ~t_convex_hull_builder() {}
	virtual void setup(std::vector<t_point2i>& data_points, std::vector<t_point2i>& hull_points) = 0;
	virtual void build(std::vector<t_point2i>& data_points, std::vector<t_point2i>& hull_points) = 0;
};

struct t_graham_scan_convex_hull_builder: public t_convex_hull_builder  {
public:
	static t_point2i& seed_point(const t_point2i& p = t_point2i(0, 0)) {
		static t_point2i seed_pnt = t_point2i(0, 0);

		if ((seed_pnt.x() == 0 && seed_pnt.y() == 0) || (p.x() > 0 && p.y() > 0)) {
			seed_pnt = p;
		}

		return seed_pnt;
	}

	static float point_angle(const t_point2i& p) {
		assert((seed_point()).x() != 0);
		assert((seed_point()).y() != 0);

		const t_point2i s = seed_point();
		const t_point2f v = t_point2f(p.x() - s.x(), p.y() - s.y());

		if (p == s)
			return 0.0f;

		const float r = v.radius();

		assert(r != 0.0f);

		const t_point2f n = t_point2f(v.x() / r, v.y() / r);

		const float rca = n.dot(t_point2f(1.0f, 0.0f));
		const float cca = std::max(-1.0f, std::min(1.0f, rca));

		// return angle in radians
		return (std::acos(cca));
	}

	static bool point_sort_func(const t_point2i& p1, const t_point2i& p2) {
		// NOTE: dot(v,w) = ||v|| * ||w|| * cos(a) but the v's are not unit-length here
		const float v1_angle = point_angle(p1);
		const float v2_angle = point_angle(p2);

		// compare squared magnitudes if angles are equal
		if (v1_angle == v2_angle)
			return (p1.radius() < p2.radius());

		return (v1_angle < v2_angle);
	}


	// compute z-coordinate of cross-product of vectors v1 and v2 where
	//   v1 = p2 - p1 = (x_1, y_1, 0) to (x_2, y_2, 0)
	//   v2 = p3 - p1 = (x_1, y_1, 0) to (x_3, y_3, 0)  ??
	//
	// if == 0, points (p1,p2,p3) are colinear
	// if  > 0, points (p1,p2,p3) form  "left turn" (CCW rotation)
	// if  < 0, points (p1,p2,p3) form "right turn" ( CW rotation)
	static int ccw(const t_point2i& p1, const t_point2i& p2, const t_point2i& p3) {
	    return ((p2.x() - p1.x()) * (p3.y() - p1.y())  -  (p2.y() - p1.y()) * (p3.x() - p1.x()));
	}

	static t_point2i get_prev_point(std::stack<t_point2i>& s) {
		assert(s.size() >= 2);

		const t_point2i top = s.top(); s.pop();
		const t_point2i pen = s.top();

		s.push(top);
		return pen;
	}


	void setup(std::vector<t_point2i>& data_points, std::vector<t_point2i>& hull_points) {
		hull_points.reserve(data_points.size());

		randomize_data_points(data_points);
		add_data_points_to_bins(data_points, POINT_BIN_GRID);

		// find algorithm "seed" (point with lowest y-coordinate)
		// put it in front of all other points s.t it is the only
		// point not touched by sorting
		const unsigned int seed_point_idx = get_seed_point_index(data_points);

		// initialize the seed, idx is always 0 if using raw points
		seed_point(data_points[seed_point_idx]);

		std::swap(data_points[0], data_points[seed_point_idx]);
		std::sort(++(data_points.begin()), data_points.end(), point_sort_func);
	}


	void build(std::vector<t_point2i>& data_points, std::vector<t_point2i>& hull_points) {
		assert(data_points.size() >= 3);
		assert(hull_points.size() == 0);
		// first three points should not be colinear
		assert(ccw(data_points[0], data_points[1], data_points[2]) != 0);

		std::stack<t_point2i> hull_stack;

		// print out the sorted-by-angle points
		for (unsigned int i = 0; i < data_points.size(); i++) {
			fprintf(DATA_POINTS_FILE, "%d\t%d\n", data_points[i].x(), data_points[i].y());
		}

		for (unsigned int i = 0; i < 3; i++) {
			hull_stack.push(data_points[i]);
		}

		for (unsigned int i = 3; i < data_points.size(); i++) {
			while (!hull_stack.empty()) {
				const t_point2i prev_point = get_prev_point(hull_stack);
				const t_point2i curr_point = hull_stack.top();
				const t_point2i next_point = data_points[i];

				// stop popping when we find a left-hand turn
				if (ccw(prev_point, curr_point, next_point) > 0) {
					break;
				} else {
					hull_stack.pop();
				}
			}

			hull_stack.push(data_points[i]);
		}

		hull_stack.push(data_points[0]);

		while (!hull_stack.empty()) {
			fprintf(HULL_POINTS_FILE, "%d\t%d\n", (hull_stack.top()).x(), (hull_stack.top()).y());
			hull_points.push_back(hull_stack.top());
			hull_stack.pop();
		}

		// TODO: turn hull-polygon into triangle fan
		assert(hull_points.size() >= 3);
	}
};




/*
simple convex hull algorithm
	find point P0 with minimum y-coordinate
	let current point P = P0, current axis A = (1, 0)
	repeat
		let C be the point != P whose CCW angle with A is minimal
		let current axis A = (C - P) / (||C - P||)
		let current point P = C
	until P == P0

	this has quadratic time complexity, so we restrict the minimum-angle
	point search (via spatial binning) to a course bounding edge of the
	data-set
*/
struct t_line_wrap_convex_hull_builder: public t_convex_hull_builder {
public:
	void setup(std::vector<t_point2i>& data_points, std::vector<t_point2i>& hull_points) {
		hull_points.reserve(data_points.size());

		randomize_data_points(data_points);
		add_data_points_to_bins(data_points, POINT_BIN_GRID);
	}

	void build(std::vector<t_point2i>& data_points, std::vector<t_point2i>& hull_points) {
		for (unsigned int i = 0; i < data_points.size(); i++) {
			fprintf(DATA_POINTS_FILE, "%d\t%d\n", data_points[i].x(), data_points[i].y());
		}

		// with 10 bins along both x- and y-axis and dense set
		// of points, this will contain 36 bins (10+10+10+10-4
		// where only the corners are duplicates)
		//
		iterate_hull_points(data_points, hull_points, POINT_BIN_GRID.get_edge_bin_indices());
	}

	void iterate_hull_points(
		const std::vector<t_point2i>& data_points,
		      std::vector<t_point2i>& hull_points,
		const std::set<unsigned int>& edge_bin_indices
	) {
		t_point2i seed_point = data_points[get_seed_point_index(data_points)];
		t_point2i curr_point = seed_point;
		t_point2i next_point;
		t_point2f curr_axis = t_point2f(1.0f, 0.0f);

		hull_points.push_back(seed_point);
		fprintf(HULL_POINTS_FILE, "%d\t%d\n", seed_point.x(), seed_point.y());

		do {
			float min_point_angle = M_PI * 2.0f;

			// find next point on hull
			#if 0
			{
				unsigned int min_point_index = 0;

				for (unsigned int n = 0; n < data_points.size(); n++) {
					const t_point2i& p = data_points[n];

					if (p == curr_point)
						continue;

					const t_point2f  v = t_point2f(p.x() - curr_point.x(), p.y() - curr_point.y());
					const t_point2f  w = v / v.radius();

					const float angle = std::acos(std::max(-1.0f, std::min(1.0f, w.dot(curr_axis))));

					if (angle < min_point_angle) {
						min_point_angle = angle;
						min_point_index = n;
					}
				}

				curr_axis = t_point2f(data_points[min_point_index].x() - curr_point.x(), data_points[min_point_index].y() - curr_point.y());
				curr_axis = curr_axis / curr_axis.radius();
				curr_point = data_points[min_point_index];
			}
			#else
			{
				for (auto it = edge_bin_indices.begin(); it != edge_bin_indices.end(); ++it) {
					const t_point_bin& bin = POINT_BIN_GRID.get_bin(*it);

					for (unsigned int n = 0; n < bin.get_size(); n++) {
						const t_point2i& p = bin.get_point(n);

						if (p == curr_point)
							continue;

						const t_point2f  v = t_point2f(p.x() - curr_point.x(), p.y() - curr_point.y());
						const t_point2f  w = v / v.radius();

						const float angle = std::acos(std::max(-1.0f, std::min(1.0f, w.dot(curr_axis))));

						if (angle < min_point_angle) {
							min_point_angle = angle;
							// TODO: bins should store pair<point, index> instances
							// min_point_index = n;

							next_point = p;
						}
					}
				}

				curr_axis = t_point2f(next_point.x() - curr_point.x(), next_point.y() - curr_point.y());
				curr_axis = curr_axis / curr_axis.radius();
				curr_point = next_point;
			}
			#endif

			hull_points.push_back(curr_point);
			fprintf(HULL_POINTS_FILE, "%d\t%d\n", curr_point.x(), curr_point.y());
		} while (curr_point != seed_point);
	}
};




int main(int argc, char** argv) {
	if (argc > 1) {
		srand(atoi(argv[1]));
	} else {
		srand(time(NULL));
	}

	// gnuplot: plot 'data_points.dat', 'hull_points.dat' with lines
	DATA_POINTS_FILE = fopen("data_points.dat", "w");
	HULL_POINTS_FILE = fopen("hull_points.dat", "w");

	assert(DATA_POINTS_FILE != NULL);
	assert(HULL_POINTS_FILE != NULL);

	std::vector<t_point2i> data_points(NUM_DATA_POINTS);
	std::vector<t_point2i> hull_points;
	std::vector<t_convex_hull_builder*> builders = {
		new t_graham_scan_convex_hull_builder(),
		new t_line_wrap_convex_hull_builder(),
	};

	builders[1]->setup(data_points, hull_points);
	builders[1]->build(data_points, hull_points);

	for (unsigned int n = 0; n < builders.size(); n++) {
		delete builders[n];
	}

	fclose(DATA_POINTS_FILE);
	fclose(HULL_POINTS_FILE);
	return 0;
}

