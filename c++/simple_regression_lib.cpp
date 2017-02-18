#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cmath>
#include <ctime>

// typedef unsigned int uint_type;
typedef size_t uint_type;

static const uint_type DATA_POINT_COOR_RANGE_X = 100;
static const uint_type DATA_POINT_COOR_RANGE_Y = 100;

// delta is the ratio between x- and y-variances
// of the assumed!-to-be-independent measurement
// ERRORS (not the measurements themselves)
//
//   y_i = y*_i + e_i
//   x_i = x*_i + n_i
//   delta = variance(e) / variance(n)
//
// if data-point measurements used the same real
// method/sensors/whatever along both axes, this
// should be equal to 1 so we get pure orthogonal
// regression
static const float DEMING_REGRESSION_DELTA = 1.0f;

template<typename t> t square(t x) { return (x * x); }

template<typename t> struct t_point {
	t_point(t x = t(0), t y = t(0), t z = t(0)) { m_x = x; m_y = y; m_z = z; }
	t_point operator + (const t_point& p) const { return t_point(m_x + p.x(), m_y + p.y(), m_z + p.z()); }
	t_point operator - (const t_point& p) const { return t_point(m_x - p.x(), m_y - p.y(), m_z - p.z()); }
	t_point operator * (const t s) { return t_point(m_x * s, m_y * s, m_z * s); }
	t_point operator / (const t s) { return t_point(m_x / s, m_y / s, m_z / s); }
	t_point& operator += (const t_point& p) { m_x += p.x(); m_y += p.y(); m_z += p.z(); return *this; }
	t_point& operator -= (const t_point& p) { m_x -= p.x(); m_y -= p.y(); m_z -= p.z(); return *this; }
	t_point& operator /= (const t s) { m_x /= s; m_y /= s; m_z /= s; return *this; }

	t squared_length() const { return ((m_x * m_x) + (m_y * m_y) + (m_z * m_z)); }

	t  x() const { return m_x; }
	t  y() const { return m_y; }
	t  z() const { return m_z; }
	t& x()       { return m_x; }
	t& y()       { return m_y; }
	t& z()       { return m_z; }

private:
	t m_x;
	t m_y;
	t m_z;
};

typedef t_point<float> t_point2f;
typedef t_point<  int> t_point2i;



template<typename point_type>
void simple_line_regression(
	const std::vector<point_type>& data_points,
	      std::vector<point_type>& data_params
) {
	assert(data_points.size() >= 2);

	point_type means;
	point_type sigmas;
	point_type coeffs;

	// calculate means (x,y)
	for (uint_type n = 0; n < data_points.size(); n++) {
		means += data_points[n];
	}

	means /= data_points.size();

	// calculate variances (x,y) and (x,y)-covariance
	for (uint_type n = 0; n < data_points.size(); n++) {
		const point_type dev = data_points[n] - means;

		sigmas.x() += (dev.x() * dev.x());
		sigmas.y() += (dev.y() * dev.y());
		sigmas.z() += (dev.x() * dev.y());
	}

	// normalize variances
	sigmas /= (data_points.size() - 0);

	// coeffs.x() := a = beta, coeffs.y() := b := alpha
	coeffs.x() = sigmas.z() / sigmas.x();
	coeffs.y() = means.y() - coeffs.x() * means.x();

	#if 0
	// generate points on the regression line (for gnuplot)
	for (uint_type n = 0; n < data_points.size(); n++) {
		const float x = ((n * 1.0f) / data_points.size()) * DATA_POINT_COOR_RANGE_X;
		const float y = coeffs.x() * x + coeffs.y();

		data_params.push_back(point_type(x, y));
	}
	#endif

	data_params.push_back(coeffs);
}



template<typename point_type>
void deming_line_regression(
	const std::vector<point_type>& data_points,
	      std::vector<point_type>& data_params
) {
	assert(data_points.size() >= 2);
	data_params.reserve(data_points.size() + 1);

	point_type means;
	point_type sigmas;
	point_type coeffs;

	// calculate means (x,y)
	for (uint_type n = 0; n < data_points.size(); n++) {
		means += data_points[n];
	}

	means /= data_points.size();

	// calculate variances (x,y) and (x,y)-covariance
	for (uint_type n = 0; n < data_points.size(); n++) {
		const point_type dev = data_points[n] - means;

		sigmas.x() += (dev.x() * dev.x());
		sigmas.y() += (dev.y() * dev.y());
		sigmas.z() += (dev.x() * dev.y());
	}

	// normalize variances (NOTE: wiki says N-1 here?)
	sigmas /= (data_points.size() - 0);

	// coeffs.x() := a = b1, coeffs.y() := b := b0
	coeffs.z() += square(sigmas.y() - DEMING_REGRESSION_DELTA * sigmas.x());
	coeffs.z() += (4.0f * DEMING_REGRESSION_DELTA * square(sigmas.z()));

	coeffs.z() = std::sqrt(coeffs.z());
	coeffs.x() = ((sigmas.y() - DEMING_REGRESSION_DELTA * sigmas.x()) + coeffs.z()) / (2.0f * sigmas.z());
	coeffs.y() = (means.y() - coeffs.x() * means.x());

	#if 0
	// generate points on the regression line (for gnuplot)
	for (uint_type n = 0; n < data_points.size(); n++) {
		const float x = ((n * 1.0f) / data_points.size()) * DATA_POINT_COOR_RANGE_X;
		const float y = coeffs.x() * x + coeffs.y();

		data_params.push_back(point_type(x, y));
	}
	#else
	for (uint_type n = 0; n < data_points.size(); n++) {
		const float y_model = coeffs.x() * data_points[n].x() + coeffs.y();
		const float y_error = data_points[n].y() - y_model;

		const float x_error = y_error * (coeffs.y() / (coeffs.y() * coeffs.y() + DEMING_REGRESSION_DELTA));
		const float x_model = data_points[n].x() + x_error;

		data_params.push_back(point_type(x_model, y_model));
	}
	#endif

	data_params.push_back(coeffs);
}



template<typename point_type>
static void randomize_data_points(std::vector<point_type>& data_points) {
	for (uint_type n = 0; n < data_points.size(); n++) {
		data_points[n].x() = std::max(uint_type(1), static_cast<uint_type>(random()) % DATA_POINT_COOR_RANGE_X);
		data_points[n].y() = std::max(uint_type(1), static_cast<uint_type>(random()) % DATA_POINT_COOR_RANGE_Y);
	}
}



int main() {
	srandom(time(NULL));

	FILE* DATA_POINTS_FILE = fopen("data_points.dat", "w");
	FILE* DATA_PARAMS_FILE = fopen("data_params.dat", "w");

	assert(DATA_POINTS_FILE != NULL);
	assert(DATA_PARAMS_FILE != NULL);

	std::vector<t_point2f> data_points(10);
	std::vector<t_point2f> data_params;

	randomize_data_points(data_points);
	simple_line_regression(data_points, data_params);
	// deming_line_regression(data_points, data_params);

	assert(data_params.size() == (data_points.size() + 1));

	for (uint_type n = 0; n < data_points.size(); n++) {
		fprintf(DATA_POINTS_FILE, "%f\t%f\n", data_points[n].x(), data_points[n].y());
		fprintf(DATA_PARAMS_FILE, "%f\t%f\n", data_params[n].x(), data_params[n].y());
	}

	fclose(DATA_POINTS_FILE);
	fclose(DATA_PARAMS_FILE);
	return 0;
}

