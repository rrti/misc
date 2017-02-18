#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static int is_power_of_2(size_t i) {
	return ((i & (i - 1)) == 0);
}



#define DECLARE_COMMON_FUNCS(type)                                                           \
	static void print_ ## type ## _array(const type* array, const char* fmt, size_t size) {  \
		for (size_t n = 0; n < size; n++) {                                                  \
			printf(fmt, array[n]);                                                           \
		}                                                                                    \
                                                                                             \
		printf("\n");                                                                        \
	}                                                                                        \

#define DECLARE_MERGESORT_FUNCS(type)                                                     \
	static void merge_ ## type ## _array_chunks(                                          \
		const type* lhs,                                                                  \
		const type* rhs,                                                                  \
		type* dst,                                                                        \
		size_t lhs_size,                                                                  \
		size_t rhs_size                                                                   \
	) {                                                                                   \
		size_t i = 0;                                                                     \
		size_t j = 0;                                                                     \
		size_t k = 0;                                                                     \
                                                                                          \
		while (i < lhs_size && j < rhs_size) {                                            \
			if (lhs[i] < rhs[j]) {                                                        \
				dst[k++] = lhs[i++];                                                      \
			} else {                                                                      \
				dst[k++] = rhs[j++];                                                      \
			}                                                                             \
		}                                                                                 \
                                                                                          \
		while (i < lhs_size) { dst[k++] = lhs[i++]; }                                     \
		while (j < rhs_size) { dst[k++] = rhs[j++]; }                                     \
	}                                                                                     \
                                                                                          \
	static void msort_ ## type ## _array_ip(type* array, size_t size) {                   \
		type* w = NULL;                                                                   \
                                                                                          \
		if (!is_power_of_2(size))                                                         \
			return;                                                                       \
                                                                                          \
		if ((w = calloc(size, sizeof(int))) == NULL)                                      \
			return;                                                                       \
                                                                                          \
		/*                                                                                \
		 * (k,j) = {                                                                      \
		 *   (1,0), (1,2), (1,4), (1, 6), (1,8), (1,10), (1,12), (1,14)                   \
		 *   (2,0), (2,4), (2,8), (2,12),                                                 \
		 *   (4,0), (4,8),                                                                \
		 *   (8,0),                                                                       \
		 * }                                                                              \
		 * chunks = {                                                                     \
		 *   (0,1), (2, 3), (4, 5), ( 6, 7), (8,9), (10,11), (12,13), (14,15)             \
		 *   (0,2), (4, 6), (8,10), (12,14),                                              \
		 *   (0,4), (8,12),                                                               \
		 *   (0,8),                                                                       \
		 * }                                                                              \
		 */                                                                               \
		for (size_t k = 1; k < size; k *= 2) {                                            \
			for (size_t j = 0; j < (size - k); j += (2 * k)) {                            \
				merge_ ## type ## _array_chunks(array + j, array + j + k, w + j,  k, k);  \
			}                                                                             \
                                                                                          \
			/* after each round, copy back the semi-sorted results */                     \
			for (size_t j = 0; j < size; j++) {                                           \
				array[j] = w[j];                                                          \
			}                                                                             \
		}                                                                                 \
                                                                                          \
		free(w);                                                                          \
	}

#define DECLARE_QUICKSORT_FUNCS(type)                                                                      \
	static void swap_ ## type(type* a, type* b) {                                                          \
		const type t = *a;                                                                                 \
                                                                                                           \
		*a = *b;                                                                                           \
		*b = t;                                                                                            \
	}                                                                                                      \
                                                                                                           \
	static size_t partition_ ## type ## _array(                                                            \
		type* array,                                                                                       \
		const size_t min_idx,                                                                              \
		const size_t max_idx,                                                                              \
		const size_t cur_pivot_idx                                                                         \
	) {                                                                                                    \
		const type pivot_val = array[cur_pivot_idx];                                                       \
		size_t nxt_pivot_idx = min_idx;                                                                    \
                                                                                                           \
		/* temporarily move pivot value to end of subarray */                                              \
		swap_ ## type(&array[cur_pivot_idx], &array[max_idx]);                                             \
                                                                                                           \
		for (size_t i = min_idx; i < max_idx; i++) {                                                       \
			if (array[i] <= pivot_val) {                                                                   \
				assert(nxt_pivot_idx <= max_idx);                                                          \
				swap_ ## type(&array[nxt_pivot_idx++], &array[i]);                                         \
			}                                                                                              \
		}                                                                                                  \
                                                                                                           \
		/* move pivot value back to its final position */                                                  \
		assert(nxt_pivot_idx <= max_idx);                                                                  \
		swap_ ## type(&array[max_idx], &array[nxt_pivot_idx]);                                             \
		return nxt_pivot_idx;                                                                              \
	}                                                                                                      \
                                                                                                           \
	static void qsort_ ## type ## _array_ip(type* array, const size_t min_idx, const size_t max_idx) {     \
		if (min_idx >= max_idx)                                                                            \
			return;                                                                                        \
                                                                                                           \
		/* NOTE: this is the lazy (bad) way of picking a pivot */                                          \
		const size_t cur_pivot_idx = (min_idx + max_idx) >> 1;                                             \
		const size_t nxt_pivot_idx = partition_ ## type ## _array(array, min_idx, max_idx, cur_pivot_idx); \
                                                                                                           \
		qsort_ ## type ## _array_ip(array, min_idx, nxt_pivot_idx);                                        \
		qsort_ ## type ## _array_ip(array, nxt_pivot_idx + 1, max_idx);                                    \
	}



DECLARE_COMMON_FUNCS(int)
DECLARE_MERGESORT_FUNCS(int)
DECLARE_QUICKSORT_FUNCS(int)
// DECLARE_COMMON_FUNCS(float)
// DECLARE_MERGESORT_FUNCS(float)
// DECLARE_QUICKSORT_FUNCS(float)



int main() {
	int array_ms[] = {
		     50,    3, 32432,   567,     9,     34,      23,    112,
		5946840, 4763,  9325, 65236, 76547, 835743, 4657025, 124569,
	};
	int array_qs[] = {
		     50,    3, 32432,   567,     9,     34,      23,    112,
		5946840, 4763,  9325, 65236, 76547, 835743, 4657025, 124569,
	};

	print_int_array   (array_qs, "%d ",  sizeof(array_qs) / sizeof(array_qs[0])     );
	msort_int_array_ip(array_ms,         sizeof(array_ms) / sizeof(array_ms[0])     );
	qsort_int_array_ip(array_qs,     0, (sizeof(array_qs) / sizeof(array_qs[0])) - 1);
	print_int_array   (array_ms, "%d ",  sizeof(array_ms) / sizeof(array_ms[0])     );
	print_int_array   (array_qs, "%d ",  sizeof(array_qs) / sizeof(array_qs[0])     );

	return 0;
}

