#include <algorithm>
#include <functional>

#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>

#include <limits>
#include <vector>

static inline size_t log2i(size_t N) {
	size_t n = 0;

	while (N > 1) {
		N >>= 1;
		n  += 1;
	}

	return n;
}



template<typename T> struct cmp_lt_ftor {
public:
	bool operator()(const T a, const T b) const { return (a < b); }
public:
	static constexpr T lim() { return (std::numeric_limits<T>::max()); }
};

template<typename T> struct cmp_gt_ftor {
public:
	bool operator()(const T a, const T b) const { return (a > b); }
public:
	static constexpr T lim() { return (std::numeric_limits<T>::min()); }
};



template<typename T, typename F> void insert_sort(std::vector<T>& elems, T elem, F comp) {
	const size_t K = elems.size();

	size_t i =     0;
	size_t j = K - 2;

	// find insertion position for <elem> (could also use
	// binary search, but not really worth it for small K)
	while (i < K) {
		if (comp(elem, elems[i]))
			break;

		i += 1;
	}

	// bail if no position was found at all
	if (i == K)
		return;

	// special case
	if (K == 1) {
		elems[0] = elem;
		return;
	}

	// move elements over from right to left; overwrite the last
	// no real need for this, could just swap <elem> into place
	while (j >= i) {
		elems[j + 1] = elems[j];

		// avoid underflow
		if (j == 0)
			break;

		j -= 1;
	}

	elems[i] = elem;
}



//
// find the <K> best (smallest or largest) elements in sorted order
// same as std::partial_sort, but not the fastest possible approach
// (that would involve using a heap and run in O(N * log2(K) or even
// a specialized quicksort running in O(N + K * log2(K))
//
template<typename T, typename F, size_t C = 1> void find_k_best_elems(
	std::vector<T>& data_elems,
	std::vector<T>& best_elems,
	const F& comp_ftor
) {
	const size_t num_data_elems = data_elems.size();
	const size_t num_best_elems = std::min(num_data_elems, best_elems.size());

	assert(num_data_elems >= 1);
	assert(num_best_elems >= 1);

	if (num_best_elems <= (C * log2i(num_data_elems))) {
		// O(N * K); strictly faster if K <= (C * log2(N))
		for (size_t n = 0; n < num_data_elems; n++) {
			insert_sort(best_elems, data_elems[n], comp_ftor);
		}
	} else {
		// O(N * log2(N))
		std::sort(data_elems.begin(), data_elems.end(), comp_ftor);

		for (size_t k = 0; k < num_best_elems; k++) {
			best_elems[k] = data_elems[k];
		}
	}
}

int main() {
	srandom(time(nullptr));

	typedef cmp_lt_ftor<int> comp_type;

	const size_t N = 16384;
	const size_t K =    10;

	std::vector<int> data_elems(N,              0  );
	std::vector<int> best_elems(K, comp_type::lim());

	for (size_t n = 0; n < N; n++) {
		data_elems[n] = random() % data_elems.size();
	}

	find_k_best_elems(data_elems, best_elems, comp_type());

	printf("[%s]\n", __FUNCTION__);

	for (size_t n = 0; n < best_elems.size(); n++) {
		printf("\t%luth-best=%d\n", n, best_elems[n]);
	}

	return 0;
}

