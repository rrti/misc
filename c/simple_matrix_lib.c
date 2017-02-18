#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>

static inline void free_(void** p) { free(*p); *p = NULL; }

#ifndef safe_free
#define safe_free(p) free_((void**) &p)
#endif

static inline float fblend_(float a, float b, float w) { return (a * (1.0f - w) + b * w); }
static inline float fmin_(float a, float b) { return (fblend_(a, b, a > b)); }
static inline float fmax_(float a, float b) { return (fblend_(a, b, a < b)); }
static inline float fclamp_(float v, float a, float b) { return (fmax_(a, fmin_(b, v))); }



static void matrix_print(const float* matrix, const unsigned int num_cols, const unsigned int num_rows, const char* header) {
	printf("[%s][%s]\n", __func__, header);

	for (unsigned int row_idx = 0; row_idx < num_rows; row_idx++) {
		printf("\t");

		for (unsigned int col_idx = 0; col_idx < num_cols; col_idx++) {
			printf(" %+6.4f", matrix[row_idx * num_cols + col_idx]);
		}

		printf("\n");
	}
}

float* matrix_transpose(const float* matrix, const unsigned int num_cols, const unsigned int num_rows) {
	float* tr_matrix = (float*) malloc(sizeof(float) * num_cols * num_rows);

	// row-major to column-major (if the input-matrix is RM) or vv.
	for (unsigned int row_idx = 0; row_idx < num_rows; row_idx++) {
		for (unsigned int col_idx = 0; col_idx < num_cols; col_idx++) {
			tr_matrix[col_idx * num_rows + row_idx] = matrix[row_idx * num_cols + col_idx];
		}
	}

	return tr_matrix;
}

static void matrix_swap_rows(
	float* matrix,
	const unsigned int num_cols,
	const unsigned int num_rows,
	const unsigned int src_row_idx,
	const unsigned int dst_row_idx
) {
	(void) num_rows;

	if (src_row_idx == dst_row_idx)
		return;

	for (unsigned int col_idx = 0; col_idx < num_cols; col_idx++) {
		const float src_val = matrix[src_row_idx * num_cols + col_idx];
		const float dst_val = matrix[dst_row_idx * num_cols + col_idx];

		matrix[src_row_idx * num_cols + col_idx] = dst_val;
		matrix[dst_row_idx * num_cols + col_idx] = src_val;
	}
}

static float* matrix_extract_rref(
	const float* const matrix_lhs,
	      float* const matrix_rhs,
	const unsigned int size_lhs,
	const unsigned int size_rhs,
	const float eps,
	unsigned int* rank
) {
	float* rref_matrix = (float*) malloc(sizeof(float) * size_lhs * (size_lhs + size_rhs));

	// initialize rref_matrix, augment it if wanted
	for (unsigned int row_idx = 0; row_idx < size_lhs; ++row_idx) {
		for (unsigned int col_idx = 0; col_idx < size_lhs; ++col_idx) {
			rref_matrix[row_idx * size_lhs + col_idx] = matrix_lhs[row_idx * size_lhs + col_idx];
		}
		for (unsigned int col_idx = size_lhs; col_idx < size_lhs + size_rhs; ++col_idx) {
			rref_matrix[row_idx * size_lhs + col_idx] = matrix_rhs[row_idx * size_lhs + (col_idx - size_lhs)];
		}
	}

	bool finished = false;

	// perform forward Gauss-Jordan elimination to get matrix in reduced row-echelon form
	for (unsigned int lhs_row_idx = 0; lhs_row_idx < size_lhs; ++lhs_row_idx) {
		if (fabs(rref_matrix[lhs_row_idx * size_lhs + lhs_row_idx]) < eps) {
			if (lhs_row_idx == (size_lhs - 1))
				return NULL;

			matrix_swap_rows(rref_matrix, size_lhs, size_lhs, lhs_row_idx, lhs_row_idx + 1);
		}

		const float val = rref_matrix[lhs_row_idx * size_lhs + lhs_row_idx];

		if ((finished |= (fabs(val) < eps)))
			break;

		for (unsigned int col_idx = 0; col_idx < size_lhs + size_rhs; ++col_idx) {
			rref_matrix[lhs_row_idx * size_lhs + col_idx] /= val;
		}

		for (unsigned int row_idx = lhs_row_idx + 1; row_idx < size_lhs; ++row_idx) {
			const float mult = rref_matrix[row_idx * size_lhs + lhs_row_idx];

			for (unsigned int col_idx = 0; col_idx < size_lhs + size_rhs; ++col_idx) {
				rref_matrix[row_idx * size_lhs + col_idx] -= (mult * rref_matrix[lhs_row_idx * size_lhs + col_idx]);
			}
		}
	}

	if (!finished) {
		// backward substitution steps
		for (int lhs_row_idx = size_lhs - 1; lhs_row_idx >= 0; --lhs_row_idx) {
			for (int row_idx = lhs_row_idx - 1; row_idx >= 0; --row_idx) {
				const float mult = rref_matrix[row_idx * size_lhs + lhs_row_idx];

				for (unsigned int col_idx = 0; col_idx < size_lhs + size_rhs; ++col_idx) {
					rref_matrix[row_idx * size_lhs + col_idx] -= (mult * rref_matrix[lhs_row_idx * size_lhs + col_idx]);
				}
			}
		}
	}

	for (unsigned int row_idx = 0; row_idx < size_lhs; ++row_idx) {
		for (unsigned int col_idx = 0; col_idx < size_rhs; ++col_idx) {
			matrix_rhs[row_idx * size_lhs + col_idx] = rref_matrix[row_idx * size_lhs + (col_idx + size_lhs)];
		}

		(*rank) += (fabs(rref_matrix[row_idx * size_lhs + row_idx]) > eps);
	}

	return rref_matrix;
}

static unsigned int matrix_rank(
	const float* matrix,
	const unsigned int size,
	float* row_factors,
	float* col_factors,
	float row_factor_wgt,
	float col_factor_wgt
) {
	unsigned int    matrix_rank = 0;
	unsigned int tr_matrix_rank = 0;

	{
		// can only be applied to square matrices so size = num_rows = num_cols
		// after applying GJE, rank = number of rows with >= 1 non-zero element
		float* tr_matrix      = matrix_transpose(matrix, size, size);
		float* tr_matrix_rref = matrix_extract_rref(tr_matrix, NULL, size, 0, 0.0001f, &tr_matrix_rank);
		float*    matrix_rref = matrix_extract_rref(   matrix, NULL, size, 0, 0.0001f, &   matrix_rank);

		float row_factor_sum = 0.0f;
		float col_factor_sum = 0.0f;

		// top rows of RREF(M) and RREF(M^T) now contain M's factors
		for (unsigned int idx = 0; idx < size; idx++) {
			row_factors[idx] =    matrix_rref[idx]; row_factor_sum += fabs(row_factors[idx]);
			col_factors[idx] = tr_matrix_rref[idx]; col_factor_sum += fabs(col_factors[idx]);
		}

		assert(row_factor_sum > 0.0f);
		assert(col_factor_sum > 0.0f);

		// normalize the factors on caller's request
		for (unsigned int idx = 0; idx < size; idx++) {
			row_factors[idx] /= fblend_(1.0f, row_factor_sum, fclamp_(row_factor_wgt, 0.0f, 1.0f));
			col_factors[idx] /= fblend_(1.0f, col_factor_sum, fclamp_(col_factor_wgt, 0.0f, 1.0f));
		}

		// fundamental theorem of LA implies column rank
		// and row rank of matrix should ALWAYS be equal
		assert(matrix_rank == tr_matrix_rank);

		safe_free(tr_matrix     );
		safe_free(   matrix_rref);
		safe_free(tr_matrix_rref);
	}

	return matrix_rank;
}



static bool symmetric_separable_filter(
	const float* raw_filter,
	      float* row_filter,
	      float* col_filter,
	const int filter_width,
	bool* symmetric,
	bool* separable
) {
	// a two-dimensional (but represented as a one-dimensional array of values) filter
	// kernel is separable iff it can be expressed as the outer product of two vectors
	// note: this immediately implies only square filters are (potentially) separable
	//
	// symmetry does NOT imply linear separability, so test if rank == 1 ((v^T)*w == M)
	// the filter's decomposition into vectors v and w is given by RREF(M) and RREF(M^T)
	*symmetric = true;
	*separable = (matrix_rank(raw_filter, filter_width, row_filter, col_filter, 0.0f, 0.0f) == 1);

	for (int j = 0; j < filter_width; j++) {
		for (int i = 0; i < filter_width; i++) {
			const int idx_a = (                     j) * filter_width + (                     i);
			const int idx_b = (                     j) * filter_width + ((filter_width - 1) - i);
			const int idx_c = ((filter_width - 1) - j) * filter_width + (                     i);
			const int idx_d = ((filter_width - 1) - j) * filter_width + ((filter_width - 1) - i);

			assert(idx_a < (filter_width * filter_width));
			assert(idx_b < (filter_width * filter_width));
			assert(idx_c < (filter_width * filter_width));
			assert(idx_d < (filter_width * filter_width));

			(*symmetric) &= (raw_filter[idx_a] == raw_filter[idx_b]);
			(*symmetric) &= (raw_filter[idx_a] == raw_filter[idx_c]);
			(*symmetric) &= (raw_filter[idx_a] == raw_filter[idx_d]);
		}
	}

	return (/* (*symmetric) && */ (*separable));
}



int main() {
	float*    matrix      = (float*) malloc(sizeof(float) * 3 * 3);
	float*    matrix_rref = NULL;
	float* tr_matrix      = NULL;
	float* tr_matrix_rref = NULL;

	float* row_filter = (float*) malloc(sizeof(float) * 3); // v
	float* col_filter = (float*) malloc(sizeof(float) * 3); // w

	unsigned int    matrix_rank = 0;
	unsigned int tr_matrix_rank = 0;

	bool symmetric_filter = false;
	bool separable_filter = false;


	// a two-dimensional filter kernel is separable if it can be expressed
	// as the outer product (convolution) of two (one-dimensional) vectors
	// v and w, for example
	//
	//   -1 0 1
	//   -2 0 2 == [1, 2, 1]^T * [-1, 0, 1]
	//   -1 0 1
	//
	// v and w are the RREF's of the kernel and its transpose respectively
	matrix[0] = -1.0f; matrix[1] = 0.0f; matrix[2] = 1.0f;
	matrix[3] = -2.0f; matrix[4] = 0.0f; matrix[5] = 2.0f;
	matrix[6] = -1.0f; matrix[7] = 0.0f; matrix[8] = 1.0f;

	tr_matrix      = matrix_transpose(matrix, 3, 3);
	tr_matrix_rref = matrix_extract_rref(tr_matrix, NULL, 3, 0, 0.0001f, &tr_matrix_rank);
	   matrix_rref = matrix_extract_rref(   matrix, NULL, 3, 0, 0.0001f, &   matrix_rank);


	assert(symmetric_separable_filter(matrix, row_filter, col_filter, 3, &symmetric_filter, &separable_filter));
	assert(matrix_rank == tr_matrix_rank);

	matrix_print(   matrix,      3, 3, "          M  ");
	matrix_print(tr_matrix,      3, 3, "     TRAN(M) ");
	matrix_print(   matrix_rref, 3, 3, "RREF(     M )");
	matrix_print(tr_matrix_rref, 3, 3, "RREF(TRAN(M))");

	matrix_print(row_filter, 3, 1, "V");
	matrix_print(col_filter, 3, 1, "W");


	safe_free(row_filter);
	safe_free(col_filter);

	safe_free(   matrix     );
	safe_free(   matrix_rref);
	safe_free(tr_matrix     );
	safe_free(tr_matrix_rref);
	return 0;
}

