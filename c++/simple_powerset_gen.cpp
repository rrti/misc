#include <cstdio>
#include <vector>
#include <set>

typedef std::set<int> set_type;
typedef std::vector< set_type > set_array_type;
typedef std::vector< set_array_type > power_set_type;



template<typename T> void gen_powerset_aux(
	const typename std::set<T>& set,
	const typename std::set<T>::const_iterator& set_it,
	typename std::set<T>& sub_set,
	typename std::vector< std::vector< std::set<T> > >& pwr_set,
	const size_t cur_depth,
	const size_t min_depth,
	const size_t max_depth
) {
	if (cur_depth > max_depth)
		return;

	if (cur_depth >= min_depth) {
		// make a copy of the subset at this depth
		pwr_set[cur_depth].push_back(sub_set);
	}

	for (typename std::set<T>::iterator it = set_it; it != set.end(); ++it) {
		typename std::set<T>::iterator nit = it;

		sub_set.insert(*it);
		gen_powerset_aux(set, ++nit, sub_set, pwr_set, cur_depth + 1, min_depth, max_depth);
		sub_set.erase(*it);
	}
}

template<typename T> void gen_powerset(
	const typename std::set<T>& set,
	      typename std::vector< std::vector< std::set<T> > >& pwr_set,
	const size_t min_depth,
	const size_t max_depth
) {
	if (set.empty())
		return;

	std::set<T> sub_set;

	// the power-set of S contains all subsets of size 1, 2, ..., |S|
	// there are (|S| over K) subsets of size K, collect all of them
	pwr_set.resize(set.size() + 1);

	gen_powerset_aux(set, set.begin(), sub_set, pwr_set, 0, min_depth, max_depth);
}



void enumerate_elements(const power_set_type& pwr_set) {
	// enumerate all <|S| over N=n+1> subsets of size <N>
	for (size_t n = 0; n < pwr_set.size(); n++) {
		const set_array_type& sub_set_array = pwr_set[n];

		printf("[n=%lu]\n", n);

		// enumerate all elements of the <k>-th subset of size <n+1>
		for (size_t k = 0; k < sub_set_array.size(); k++) {
			const std::set<int>& sub_set = sub_set_array[k];

			printf("\t[k=%lu] = {", k + 1);

			for (auto it = sub_set.begin(); it != sub_set.end(); ++it) {
				auto iit = it;

				printf("%d", *it);

				if (++iit != sub_set.end()) {
					printf(", ");
				}
			}

			printf("}\n");
		}
	}
}

int main() {
	set_type set = {1, 2, 3, 4, 5, 6};
	power_set_type pwr_set;

	gen_powerset(set, pwr_set, 0, set.size());
	enumerate_elements(pwr_set);

	return 0;
}

