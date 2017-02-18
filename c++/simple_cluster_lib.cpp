#include <limits>
#include <list>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstdio>
#include <ctime>

typedef unsigned int uint_type;
// typedef size_t uint_type;

// NOTE:
//   do not impose a limit on the number of clusters, as that
//   will leave points orphaned if using the "simple" cluster
//   algorithm
static const     float  MAX_POINT_CLUSTER_DIST = 10.0f;
static const uint_type  MAX_NUMBER_DATA_POINTS = 10000;
static const uint_type DATA_POINT_COOR_RANGE_X =   100;
static const uint_type DATA_POINT_COOR_RANGE_Y =   100;
static const uint_type  MAX_POINTS_PER_CLUSTER =    50;

template<typename t> struct t_point {
	t_point(t x = t(0), t y = t(0)) { m_x = x; m_y = y; }
	t_point operator + (const t_point& p) const { return t_point(m_x + p.x(), m_y + p.y()); }
	t_point operator - (const t_point& p) const { return t_point(m_x - p.x(), m_y - p.y()); }
	t_point& operator += (const t_point& p) { m_x += p.x(); m_y += p.y(); return *this; }
	t_point& operator -= (const t_point& p) { m_x -= p.x(); m_y -= p.y(); return *this; }
	t_point& operator /= (const t s) { m_x /= s; m_y /= s; return *this; }

	t squared_length() const { return ((m_x * m_x) + (m_y * m_y)); }

	t  x() const { return m_x; }
	t  y() const { return m_y; }
	t& x()       { return m_x; }
	t& y()       { return m_y; }

private:
	t m_x;
	t m_y;
};

typedef t_point<float> t_point2f;
typedef t_point<  int> t_point2i;



struct t_base_cluster {
public:
	uint_type get_size() const { return m_points.size(); }

	virtual bool point_in_cluster(const t_point2f& point) const = 0;
	virtual bool add_point(const t_point2f& point) = 0;

protected:
	std::list<t_point2f> m_points;
};

//
// note: only really suitable for uniformly distributed random data!
//
struct t_spherical_cluster: public t_base_cluster {
public:
	t_spherical_cluster(const t_point2f& point) {
		// pass an extreme radius because m_center is not initialized yet
		add_point(point, std::numeric_limits<float>::max());
	}

	const t_point2f& get_center() const { return m_center; }
	void set_center(const t_point2f& p) { m_center = p; }

	bool point_in_cluster(const t_point2f& point) const { return (point_in_cluster(point, MAX_POINT_CLUSTER_DIST)); }
	bool point_in_cluster(const t_point2f& point, float radius) const { return ((point - m_center).squared_length() <= (radius * radius)); }

	bool add_point(const t_point2f& point) { return (add_point(point, MAX_POINT_CLUSTER_DIST)); }
	bool add_point(const t_point2f& point, float radius) {
		t_point2f new_center = point;

		if (get_size() >= MAX_POINTS_PER_CLUSTER)
			return false;
		if (!point_in_cluster(point, radius))
			return false;

		// calculate where new center will be if we add <point>
		for (auto it = m_points.begin(); it != m_points.end(); ++it) {
			new_center += (*it);
		}

		new_center /= (m_points.size() + 1);

		// need to check if adding new point will not move center
		// in such a way as to increase distance from any existing
		// point to it beyond <radius>
		for (auto it = m_points.begin(); it != m_points.end(); ++it) {
			if (((*it) - new_center).squared_length() > (radius * radius)) {
				return false;
			}
		}

		set_center(new_center);
		m_points.push_back(point);
		return true;
	}

private:
	t_point2f m_center;
};



template<typename point_type>
static void randomize_data_points(std::vector<point_type>& data_points) {
	for (uint_type n = 0; n < data_points.size(); n++) {
		data_points[n].x() = std::max(uint_type(1), static_cast<uint_type>(random()) % DATA_POINT_COOR_RANGE_X);
		data_points[n].y() = std::max(uint_type(1), static_cast<uint_type>(random()) % DATA_POINT_COOR_RANGE_Y);
	}
}



template<typename cluster_type, typename point_type>
void add_point_to_cluster(std::list<cluster_type>& clusters, const point_type& point) {
	if (clusters.empty()) {
		clusters.push_back(cluster_type(point));
		return;
	}

	bool added = false;

	for (auto it = clusters.begin(); it != clusters.end(); ++it) {
		cluster_type& cluster = *it;

		if (added |= cluster.add_point(point)) {
			break;
		}
	}

	if (!added) {
		clusters.push_back(cluster_type(point));
	}
}

template<typename cluster_type, typename point_type>
void generate_simple_clusters(const std::vector<point_type>& points, std::list<cluster_type>& clusters) {
	for (uint_type n = 0; n < points.size(); n++) {
		add_point_to_cluster(clusters, points[n]);
	}
}



template<typename point_type>
void kmeans_assign_points(
	const std::vector<point_type>& points,
	const std::vector<point_type>& cluster_centers,
	      std::vector<uint_type>& cluster_points
) {
	for (uint_type n = 0; n < points.size(); n++) {
		uint_type min_cluster_idx = 0;

		for (uint_type k = min_cluster_idx + 1; k < cluster_centers.size(); k++) {
			if ((cluster_centers[k] - points[n]).squared_length() < (cluster_centers[min_cluster_idx] - points[n]).squared_length()) {
				min_cluster_idx = k;
			}
		}

		// map point <n> to cluster <min_cluster_idx>
		cluster_points[n] = min_cluster_idx;
	}
}

template<typename point_type>
uint_type kmeans_update_centers(
	const std::vector<point_type>& points,
	const std::vector<uint_type>& cluster_points,
	      std::vector<uint_type>& cluster_sizes,
	      std::vector<point_type>& cluster_centers,
	float epsilon = 0.001f
) {
	uint_type num_converged_clusters = 0;

	for (uint_type k = 0; k < cluster_centers.size(); k++) {
		const point_type cluster_center = cluster_centers[k];

		// reset the count of points belonging to cluster k
		cluster_sizes[k] = 0;

		// for each cluster, calculate its new center
		for (uint_type n = 0; n < points.size(); n++) {
			if (cluster_points[n] == k) {
				cluster_sizes[k] += 1;
				cluster_centers[k] += points[n];
			}
		}

		// prevent possible div0's
		cluster_centers[k] /= std::max(uint_type(1), cluster_sizes[k]);
		num_converged_clusters += ((cluster_center - cluster_centers[k]).squared_length() < epsilon);
	}

	return num_converged_clusters;
}

template<typename cluster_type, typename point_type>
void generate_kmeans_clusters(
	uint_type num_clusters,
	uint_type num_iterations,
	const std::vector<point_type>& points,
	      std::list<cluster_type>& clusters
) {
	std::vector<point_type> cluster_centers(num_clusters);
	std::vector<uint_type> cluster_points(points.size(), 0);
	std::vector<uint_type> cluster_sizes(num_clusters, 0);

	assert(num_clusters <= points.size());

	// step 0: initialize centers (randomly, Forgy method)
	for (uint_type k = 0; k < cluster_centers.size(); k++) {
		cluster_centers[k] = points[random() % points.size()];
	}

	for (uint_type i = 0; i < num_iterations; i++) {
		// step 1: assign points
		kmeans_assign_points(points, cluster_centers, cluster_points);

		// step 2: update centers
		const uint_type num_converged_clusters = kmeans_update_centers(points, cluster_points, cluster_sizes, cluster_centers);

		// bail if no cluster has moved more than <epsilon> since last update
		if (num_converged_clusters == num_clusters) {
			break;
		}
	}

	for (uint_type k = 0; k < cluster_centers.size(); k++) {
		// note: could also add the actual points here, but unnecessary
		clusters.push_back(cluster_type(cluster_centers[k]));
	}
}



int main() {
	srandom(time(NULL));

	FILE* DATA_POINTS_FILE = fopen("data_points.dat", "w");
	FILE* DATA_CLSTRS_FILE = fopen("data_clstrs.dat", "w");

	assert(DATA_POINTS_FILE != NULL);
	assert(DATA_CLSTRS_FILE != NULL);

	std::vector<t_point2f> data_points(MAX_NUMBER_DATA_POINTS);
	std::list<t_spherical_cluster> data_clusters;

	randomize_data_points(data_points);
	// generate_simple_clusters(data_points, data_clusters);
	generate_kmeans_clusters(5, 1000, data_points, data_clusters);

	for (uint_type n = 0; n < data_points.size(); n++) {
		fprintf(DATA_POINTS_FILE, "%f\t%f\n", data_points[n].x(), data_points[n].y());
	}

	for (auto it = data_clusters.begin(); it != data_clusters.end(); ++it) {
		assert((*it).get_size() != 0);
		fprintf(DATA_CLSTRS_FILE, "%f\t%f\n", ((*it).get_center()).x(), ((*it).get_center()).y());
	}

	fclose(DATA_POINTS_FILE);
	fclose(DATA_CLSTRS_FILE);
	return 0;
}

