#include <algorithm>
#include <limits>
#include <vector>

#include <cassert>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cmath>

template<typename type> type square(type x) { return (x * x); }

// point or vector (single-column matrix)
template<typename type> struct t_tuple {
	t_tuple<type>() {}
	t_tuple<type>(size_t num_rows) {
		m_values.resize(num_rows, type(0));
	}
	t_tuple<type>(const t_tuple& p) {
		m_values.resize(p.get_num_rows());

		for (size_t i = 0; i < get_num_rows(); i++) {
			(*this)[i] = p[i];
		}
	}

	t_tuple<type>& operator = (const t_tuple<type>& p) {
		m_values = p.get_values(); return *this;
	}

	t_tuple<type> operator + (const t_tuple<type>& p) const { assert(get_num_rows() == p.get_num_rows()); t_tuple r(get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { r[i] = (*this)[i] + p[i]; } return r; }
	t_tuple<type> operator - (const t_tuple<type>& p) const { assert(get_num_rows() == p.get_num_rows()); t_tuple r(get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { r[i] = (*this)[i] - p[i]; } return r; }
	t_tuple<type> operator * (const t_tuple<type>& p) const { assert(get_num_rows() == p.get_num_rows()); t_tuple r(get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { r[i] = (*this)[i] * p[i]; } return r; }
	t_tuple<type> operator / (const type           s) const {                                             t_tuple r(get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { r[i] = (*this)[i] /    s; } return r; }

	t_tuple<type>& operator += (const t_tuple<type>& p) { assert(get_num_rows() == p.get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { (*this)[i] += p[i]; } return *this; }
	t_tuple<type>& operator -= (const t_tuple<type>& p) { assert(get_num_rows() == p.get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { (*this)[i] -= p[i]; } return *this; }
	t_tuple<type>& operator *= (const t_tuple<type>& p) { assert(get_num_rows() == p.get_num_rows()); for (size_t i = 0; i < get_num_rows(); i++) { (*this)[i] *= p[i]; } return *this; }
	t_tuple<type>& operator /= (const type           s) {                                             for (size_t i = 0; i < get_num_rows(); i++) { (*this)[i] /=    s; } return *this; }

	type inner_product(const t_tuple<type>& p) const {
		type r = type(0);

		// dimensions must match
		assert(get_num_rows() == p.get_num_rows());

		for (size_t i = 0; i < get_num_rows(); i++)
			r += ((*this)[i] * p[i]);

		return r;
	}

	type squared_length() const { return (inner_product(*this)); }


	// 0=x, 1=y, etc
	type  get_value(size_t i) const { assert(i < get_num_rows()); return m_values[i]; }
	type& get_value(size_t i)       { assert(i < get_num_rows()); return m_values[i]; }

	const std::vector<type>& get_values() const { return m_values; }
	      std::vector<type>& get_values()       { return m_values; }

	const type* get_values_raw() const { assert(!m_values.empty()); return &m_values[0]; }
	      type* get_values_raw()       { assert(!m_values.empty()); return &m_values[0]; }

	#if 1
	type  operator [] (size_t i) const { return (get_value(i)); }
	type& operator [] (size_t i)       { return (get_value(i)); }
	#endif


	size_t get_num_rows() const { return (m_values.size()); }
	size_t get_num_cols() const { return (              1); }

private:
	std::vector<type> m_values;
};



template<typename type> struct t_matrix {
public:
	t_matrix<type>(size_t num_rows, size_t num_cols): m_num_rows(num_rows), m_num_cols(num_cols) {
		// zero-fill
		m_values.resize(num_rows * num_cols, type(0));
	}
	t_matrix<type>(const t_matrix<type>& m) {
		*this = m;
	}
	t_matrix<type>(const std::vector< t_tuple<type> >& cols) {
		assert(!cols.empty());

		m_num_cols = cols.size();
		m_num_rows = cols[0].get_num_rows();

		m_values.resize(m_num_rows * m_num_cols);

		// NB: assumes column-major layout
		for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++) {
			memcpy(&m_values[col_idx * get_num_rows()], cols[col_idx].get_values_raw(), sizeof(type) * get_num_rows());
		}
	}


	t_matrix<type>& operator = (const t_matrix<type>& m) {
		m_num_rows = m.get_num_rows();
		m_num_cols = m.get_num_cols();

		m_values = m.get_values();
		return *this;
	}


	// matrix * matrix
	t_matrix<type> operator * (const t_matrix<type>& m) const {
		t_matrix<type> r(get_num_rows(), m.get_num_cols());

		// inner dimensions must match
		assert(get_num_cols() == m.get_num_rows());

		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++) {
			for (size_t col_idx = 0; col_idx < m.get_num_cols(); col_idx++) {
				const t_tuple<type>& row =   get_row_vec(row_idx);
				const t_tuple<type>& col = m.get_col_vec(col_idx);

				r.set_value(calc_idx(row_idx, col_idx), row.inner_product(col));
			}
		}

		return r;
	}

	// matrix * vector
	t_tuple<type> operator * (const t_tuple<type>& p) const {
		t_tuple<type> r(get_num_rows());

		// inner dimensions must match
		assert(get_num_cols() == p.get_num_rows());

		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++)
			r[row_idx] = p.inner_product(get_row_vec(row_idx));

		return r;
	}


	// matrix * scalar
	t_matrix<type> operator * (const type s) {
		t_matrix<type> ret = *this;

		for (size_t n = 0; n < get_size_sq(); n++)
			ret.get_value(n) *= s;

		return ret;
	}

	t_matrix<type> operator / (const type s) {
		return ((*this) * (type(1) / s));
	}


	t_matrix<type> transpose() const {
		t_matrix<type> r(get_num_cols(), get_num_rows());

		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++) {
			for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++) {
				// move element at (row, col) to (col, row)
				r.set_value(r.calc_idx(col_idx, row_idx), get_value(calc_idx(row_idx, col_idx)));
			}
		}

		return r;
	}


	void set_identity() {
		// only defined for square matrices
		assert(get_num_rows() == get_num_cols());
		assert(get_size_sq() != 0);
		// IEEE754 guarantees a zero-bitpattern maps to 0.0
		memset(&m_values[0], 0, get_size_sq() * sizeof(type));

		// set the identity elements
		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++) {
			m_values[calc_idx(row_idx, row_idx)] = type(1);
		}
	}


	void set_value(size_t i, type v) {
		assert(i < get_size_sq());
		m_values[i] = v;
	}


	#if 1
	// boxed versions
	t_tuple<type> get_row_vec(size_t row_idx) const {
		t_tuple<type> r(get_num_cols());

		for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++)
			r[col_idx] = get_value(row_idx, col_idx);

		return r;
	}
	t_tuple<type> get_col_vec(size_t col_idx) const {
		t_tuple<type> r(get_num_rows());

		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++)
			r[row_idx] = get_value(row_idx, col_idx);

		return r;
	}
	#endif


	// if using row-major layout
	// size_t calc_idx(size_t row_idx, size_t col_idx) const { return (row_idx * get_num_cols() + col_idx); }
	// if using col-major layout
	size_t calc_idx(size_t row_idx, size_t col_idx) const { return (col_idx * get_num_rows() + row_idx); }

	type  get_value(size_t i) const { assert(i < get_size_sq()); return m_values[i]; }
	type& get_value(size_t i)       { assert(i < get_size_sq()); return m_values[i]; }

	type  get_value(size_t row_idx, size_t col_idx) const { return (get_value(calc_idx(row_idx, col_idx))); }
	type& get_value(size_t row_idx, size_t col_idx)       { return (get_value(calc_idx(row_idx, col_idx))); }

	const std::vector<type>& get_values() const { return m_values; }
	      std::vector<type>& get_values()       { return m_values; }

	const type* get_values_raw() const { assert(!m_values.empty()); return &m_values[0]; }
	      type* get_values_raw()       { assert(!m_values.empty()); return &m_values[0]; }

	#if 0
	type  operator [] (size_t i) const { return (get_value(i)); }
	type& operator [] (size_t i)       { return (get_value(i)); }
	#endif


	size_t get_num_rows() const { return m_num_rows; }
	size_t get_num_cols() const { return m_num_cols; }
	size_t get_size_sq() const { return (m_values.size()); }


	type calc_elemwise_sum() const {
		type v = type(0);

		for (size_t i = 0; i < get_size_sq(); i++) {
			v += m_values[i];
		}

		return v;
	}

	type calc_pairwise_squared_sum() const {
		type v = type(0);

		// sum the squares for each Carthesian pair of elements ((xx)^2, (xy)^2, ...)
		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++) {
			for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++) {
				if (row_idx == col_idx)
					continue;

				v += square(get_value(row_idx, col_idx));
			}
		}

		return v;
	}


	void serialize(FILE* f) const {
		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++) {
			for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++) {
				fprintf(f, "%f\t", get_value(row_idx, col_idx));
			}

			fprintf(f, "\n");
		}
	}

	void print(const char* hdr = "") const {
		printf("%s\n", hdr);
		printf("\t");

		for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++)
			printf("[col=%lu]", col_idx);

		printf("\n");

		for (size_t row_idx = 0; row_idx < get_num_rows(); row_idx++) {
			printf("\t");

			for (size_t col_idx = 0; col_idx < get_num_cols(); col_idx++)
				printf(" %.3f ", get_value(row_idx, col_idx));

			printf("\n");
		}
	}

private:
	size_t m_num_rows;
	size_t m_num_cols;

	// column-major ordering; for large (data)
	// matrices this needs to live on the heap
	std::vector<type> m_values;
};


typedef float t_value_type;
typedef t_matrix<t_value_type> t_matrix_type;
typedef t_tuple<t_value_type> t_tuple_type;

typedef std::vector<t_tuple_type> t_tuple_vector;
typedef std::vector<t_value_type> t_value_vector;

typedef std::pair<size_t, t_value_type> t_index_value_pair;

typedef std::pair<t_matrix_type, t_matrix_type> t_matrix_pair;
typedef std::pair<t_tuple_vector, t_value_vector> t_vector_pair;



t_tuple_type calc_data_avg(const t_matrix_type& mat) {
	t_tuple_type avg(mat.get_num_cols());

	assert(mat.get_num_rows() != 0);

	for (size_t n = 0; n < mat.get_num_rows(); n++)
		avg += mat.get_row_vec(n);

	return (avg / mat.get_num_rows());
}

t_tuple_type calc_data_var(const t_matrix_type& mat, const t_tuple_type& avg) {
	t_tuple_type var(avg.get_num_rows());

	assert(mat.get_num_rows() != 0);

	for (size_t n = 0; n < mat.get_num_rows(); n++)
		var += ((mat.get_row_vec(n) - avg) * (mat.get_row_vec(n) - avg));

	return (var / mat.get_num_rows());
}

t_tuple_type calc_data_std(const t_tuple_type var) {
	t_tuple_type r(var.get_num_rows());

	for (size_t n = 0; n < var.get_num_rows(); n++)
		r[n] = std::sqrt(var[n]);

	return r;
}

t_matrix_type calc_data_cov(const t_matrix_type& mat, const t_tuple_type avg) {
	// <mat> is an <NxD> matrix with N data-points of dimensionality D
	t_matrix_type cov(mat.get_num_cols(), mat.get_num_cols());

	assert(mat.get_num_rows() != 0);

	for (size_t n = 0; n < mat.get_num_rows(); n++) {
		const t_tuple_type p = (mat.get_row_vec(n) - avg) * (mat.get_row_vec(n) - avg);

		// sum the covariances for each Carthesian pair of dimensions (xx, xy, xz, ...)
		for (size_t row = 0; row < cov.get_num_rows(); row++) {
			for (size_t col = 0; col < cov.get_num_cols(); col++) {
				// cov[p][q] += (pt[p] * pt[q])
				cov.set_value(cov.calc_idx(row, col), cov.get_value(row, col) + p[row] * p[col]);
			}
		}
	}

	return (cov / mat.get_num_rows());
}



//
// symmetric Schur-decomposition (useful for covariance
// matrices which are always symmetric by definition)
//
t_tuple_type calc_sym_schur_rotation_angles(const t_matrix_type& mat, size_t row, size_t col, float eps = 0.01f) {
	t_tuple_type angle_pair(2);

	assert(row < col);
	assert(row < mat.get_num_rows());
	assert(col < mat.get_num_cols());

	if (std::fabs(mat.get_value(row, col)) > eps) {
		t_value_type r = mat.get_value(col, col) - mat.get_value(row, row);
		t_value_type t = 0.0f;

		if ((r /= (2.0f * mat.get_value(row, col))) >= 0.0f) {
			t =  1.0f / ( r + std::sqrt(1.0f + r*r));
		} else {
			t = -1.0f / (-r + std::sqrt(1.0f + r*r));
		}

		// x=cos(t), y=sin(t)
		angle_pair[0] = 1.0f / std::sqrt(1.0f + t*t);
		angle_pair[1] = t * angle_pair[0];
	} else {
		angle_pair[0] = 1.0f;
		angle_pair[1] = 0.0f;
	}

	return angle_pair;
}

//
// calculates eigen-vectors and eigen-values using the Jacobi decomposition
//
t_matrix_pair calc_eigen_decomposition(const t_matrix_type& mat) {
	t_matrix_type eig_val_mat = mat;
	t_matrix_type eig_vec_mat(mat.get_num_rows(), mat.get_num_cols());
	t_matrix_type jac_rot_mat(mat.get_num_rows(), mat.get_num_cols());

	// set eigen-values to the identity-matrix
	eig_vec_mat.set_identity();

	t_value_type min_norm_sq = std::numeric_limits<t_value_type>::max();
	t_value_type cur_norm_sq = 0.0f;

	for (size_t iter = 0; iter < 10; iter++) {
		size_t max_row = 0;
		size_t max_col = 1;

		// find indices of largest non-diagonal element
		for (size_t cur_row = 0; cur_row < eig_val_mat.get_num_rows(); cur_row++) {
			for (size_t cur_col = 0; cur_col < eig_val_mat.get_num_cols(); cur_col++) {
				if (cur_row == cur_col)
					continue;

				const t_value_type cur_val = std::fabs(eig_val_mat.get_value(cur_row, cur_col));
				const t_value_type max_val = std::fabs(eig_val_mat.get_value(max_row, max_col));

				if (cur_val <= max_val)
					continue;

				max_row = cur_row;
				max_col = cur_col;
			}
		}

		const t_tuple_type rot_angles = calc_sym_schur_rotation_angles(eig_val_mat, max_row, max_col);

		// set the Jacobi rotation matrix
		jac_rot_mat.set_identity();
		jac_rot_mat.set_value(jac_rot_mat.calc_idx(max_row, max_row),  rot_angles[0]);
		jac_rot_mat.set_value(jac_rot_mat.calc_idx(max_row, max_col),  rot_angles[1]);
		jac_rot_mat.set_value(jac_rot_mat.calc_idx(max_col, max_row), -rot_angles[1]);
		jac_rot_mat.set_value(jac_rot_mat.calc_idx(max_col, max_col),  rot_angles[0]);

		// accumulate rotations, bring eig_val_mat closer to diagonal form
		eig_vec_mat = eig_vec_mat * jac_rot_mat;
		eig_val_mat = (jac_rot_mat.transpose() * eig_val_mat) * jac_rot_mat;

		if ((cur_norm_sq = eig_val_mat.calc_pairwise_squared_sum()) >= min_norm_sq)
			break;

		min_norm_sq = cur_norm_sq;
	}

	return (t_matrix_pair(eig_vec_mat, eig_val_mat));
}



int index_value_pair_cmp(const t_index_value_pair& a, const t_index_value_pair& b) {
	if (a.second > b.second) return  1;
	if (a.second < b.second) return -1;
	return 0;
}

t_vector_pair sort_eigen_decomposition(const t_matrix_pair& eigen_mats, size_t num_eigen_vecs) {
	const t_matrix_type& eig_vecs = eigen_mats.first;
	const t_matrix_type& eig_vals = eigen_mats.second;

	// one eigen-vector for each data dimension
	std::vector<t_tuple_type> sorted_vecs(std::min(eig_vecs.get_num_cols(), num_eigen_vecs));
	std::vector<t_value_type> sorted_vals(std::min(eig_vals.get_num_cols(), num_eigen_vecs));

	std::vector<t_index_value_pair> index_value_pairs(eig_vals.get_num_cols());

	// diagonal elements are the eigen-values
	for (size_t n = 0; n < eig_vals.get_num_cols(); n++) {
		index_value_pairs[n].first  = n;
		index_value_pairs[n].second = eig_vals.get_value(n, n);
	}

	// sort (index, value) pairs by value
	std::sort(index_value_pairs.begin(), index_value_pairs.end(), index_value_pair_cmp);

	// put vectors and values in the same order
	for (size_t n = 0; n < num_eigen_vecs; n++) {
		sorted_vecs[n] = eig_vecs.get_col_vec(index_value_pairs[n].first);
		sorted_vals[n] = index_value_pairs[n].second;
	}

	return (t_vector_pair(sorted_vecs, sorted_vals));
}



int main() {
	FILE* RAW_DATA_POINTS_FILE = fopen("raw_data_points.dat", "w");
	FILE* PCA_DATA_POINTS_FILE = fopen("pca_data_points.dat", "w");

	assert(RAW_DATA_POINTS_FILE != NULL);
	assert(PCA_DATA_POINTS_FILE != NULL);

	t_matrix_type raw_data_mat(16, 2);
	t_matrix_type pca_data_mat(16, 1);

	// initialize data
	for (size_t row = 0; row < raw_data_mat.get_num_rows(); row++) {
		for (size_t col = 0; col < raw_data_mat.get_num_cols(); col++) {
			raw_data_mat.set_value(raw_data_mat.calc_idx(row, col), (col == 0)? (row % 4): (row / 4));
		}
	}

	const t_matrix_type data_cov_mat = calc_data_cov(raw_data_mat, calc_data_avg(raw_data_mat));

	const t_matrix_pair& paired_eigen_decomp = calc_eigen_decomposition(data_cov_mat);
	const t_vector_pair& sorted_eigen_decomp = sort_eigen_decomposition(paired_eigen_decomp, pca_data_mat.get_num_cols());

	const t_matrix_type  pca_proj_mat = t_matrix_type(sorted_eigen_decomp.first);
	const t_matrix_type& eig_vecs_mat = paired_eigen_decomp.first;
	const t_matrix_type& eig_vals_mat = paired_eigen_decomp.second;

	// project data onto eigen-vectors; data is NxD and
	// proj is DxK (where K <= D) so result will be NxK
	pca_data_mat = raw_data_mat * pca_proj_mat;

	raw_data_mat.print("[RAW_DATA_MAT]");
	pca_data_mat.print("[PCA_DATA_MAT]");
	data_cov_mat.print("[DATA_COV_MAT]");
	eig_vecs_mat.print("[EIG_VECS_MAT]");
	eig_vals_mat.print("[EIG_VALS_MAT]");

	raw_data_mat.serialize(RAW_DATA_POINTS_FILE);
	pca_data_mat.serialize(PCA_DATA_POINTS_FILE);

	fclose(RAW_DATA_POINTS_FILE);
	fclose(PCA_DATA_POINTS_FILE);
	return 0;
}

