#include <cassert>
#include <cstddef>

#include <algorithm>
#include <functional>
#include <iostream>
#include <vector>



enum {
	CMP_LT = -1,
	CMP_EQ =  0,
	CMP_GT =  1,
};

template<typename type> class ext_vector: public std::vector<type> {
public:
	ext_vector<type>(                                  ): std::vector<type>(          ) {}
	ext_vector<type>(size_t size, size_t init = type(0)): std::vector<type>(size, init) {}

	ext_vector<type>(const std::vector<type>& v): std::vector<type>(v) {}

	ext_vector<type> operator + (const ext_vector<type>& v) {
		ext_vector<type> r(this->size() + v.size(), 0);

		for (size_t n = 0; n < this->size(); n++) {
			r[n] = (*this)[n];
		}
		for (size_t n = this->size(); n < (this->size() + v.size()); n++) {
			r[n] = v[n - this->size()];
		}

		return r;
	}
};



template<typename type> void
print_array(const ext_vector<type>& v) {
	std::cout << "[";

	for (size_t i = 0; i < v.size(); i++) {
		std::cout << v[i];
		std::cout << ((i < (v.size() - 1))? ", ": "");
	}

	std::cout << "]";
	std::cout << std::endl;
}

template<typename type> type sum_array(const ext_vector<type>& v, size_t min_idx, size_t max_idx) {
	type sum = type(0);

	for (size_t n = min_idx; n <= max_idx; n++)
		sum += v[n];

	return sum;
}


template<typename type> bool
is_array_sorted(const ext_vector<type>& v, const std::function<int(type, type)>& f) {
	if (v.size() >= 2) {
		const int s = f(v[0], v[1]);

		for (size_t i = 1; i < (v.size() - 1); i++) {
			if (f(v[i], v[i + 1]) == s)
				continue;

			return false;
		}
	}

	return true;
}



// descending-order comparator
template<typename type> int compare_func_dec(type v1, type v2) {
	if (v1 > v2) return CMP_LT;
	if (v1 < v2) return CMP_GT;
	return CMP_EQ;
}

// ascending-order comparator
template<typename type> int compare_func_inc(type v1, type v2) {
	if (v1 > v2) return CMP_GT;
	if (v1 < v2) return CMP_LT;
	return CMP_EQ;
}



namespace lib_msort {
	template<typename type> std::pair< ext_vector<type>, ext_vector<type> >
	split_array(const ext_vector<type>& u) {
		// in case <u> has a non-even number of elements,
		// add the left-over element to the RHS sub-array
		ext_vector<type> v((u.size() >> 1) +                0 );
		ext_vector<type> w((u.size() >> 1) + int(u.size() & 1));

		// play it safe on empty inputs
		if (u.empty())
			return (std::pair< ext_vector<type>, ext_vector<type> >(v, w));
		if (u.size() == 1)
			return (std::pair< ext_vector<type>, ext_vector<type> >(u, v));

		std::copy(u.begin(),                   u.end() - (u.size() >> 1), v.begin());
		std::copy(u.begin() + (u.size() >> 1), u.end(),                   w.begin());

		return (std::pair< ext_vector<type>, ext_vector<type> >(v, w));
	}

	template<typename type> ext_vector<type>
	merge_arrays(const ext_vector<type>& u, const ext_vector<type>& v, const std::function<int(type, type)>& f) {
		assert(is_array_sorted(u, f));
		assert(is_array_sorted(v, f));

		// note: too big if intersect(u,v) != {}, should reserve+push
		ext_vector<type> w(u.size() + v.size(), 0);

		size_t u_idx = 0;
		size_t v_idx = 0;
		size_t w_idx = 0;

		bool loop = true;

		while (loop) {
			if (u_idx >= u.size()) {
				if (!(loop &= (v_idx < v.size())))
					break;

				// <u> exhausted, consume rest of <v>
				w[w_idx++] = v[v_idx++];
				continue;
			}
			if (v_idx >= v.size()) {
				#if 0
				// already checked above
				if (!(loop &= (u_idx < u.size())))
					break;
				#endif

				// <v> exhausted, consume rest of <u>
				w[w_idx++] = u[u_idx++];
				continue;
			}

			switch (f(u[u_idx], v[v_idx])) {
				case CMP_LT: { w[w_idx++] = u[u_idx++]; } break;
			//	case CMP_EQ: { w[w_idx++] = u[u_idx++]; } break;
				case CMP_GT: { w[w_idx++] = v[v_idx++]; } break;
			}
		}

		return w;
	}

	template<typename type> ext_vector<type> sort_array(const ext_vector<type>& v, const std::function<int(type, type)>& f) {
		if (v.size() <= 1)
			return v;

		const std::pair< ext_vector<type>, ext_vector<type> >& p = split_array(v);
		const ext_vector<type>& w = merge_arrays(sort_array(p.first, f), sort_array(p.second, f), f);
		return w;
	}



	// in-place version (TODO)
	template<typename type> void sort_array_ip(ext_vector<type>&, const std::function<int(type, type)>&) {
	}
}


namespace lib_qsort {
	template<typename type> size_t calc_pivot_index(const ext_vector<type>& v, size_t min_idx, size_t max_idx) {
		size_t idx = 0;

		assert(min_idx < max_idx);

		// calculate the average and the element closest in value to this average
		// max_idx is inclusive, so number of elements is one more
		const type sum = sum_array(v, min_idx, max_idx);
		const type avg = sum / ((max_idx + 1) - min_idx);
		      type dif = std::numeric_limits<type>::max();

		for (size_t n = min_idx; n <= max_idx; n++) {
			if (std::max<type>(v[n] - avg, avg - v[n]) < dif) {
				dif = std::max<type>(v[n] - avg, avg - v[n]);
				idx = n;
			}
		}

		return idx;
	}

	template<typename type> void partition_array(ext_vector<type>& v, const std::function<int(type, type)>& f, size_t min_idx, size_t max_idx) {
		if (min_idx >= max_idx)
			return;

		const size_t raw_pivot_idx = calc_pivot_index(v, min_idx, max_idx);
			  size_t new_pivot_idx = 0;

		const type raw_pivot_val = v[raw_pivot_idx];

		size_t i = min_idx;
		size_t j = max_idx;

		{
			// calculate the number of elements less than the pivot
			for (size_t n = min_idx; n <= max_idx; n++) {
				// new_pivot_idx += (v[n] < raw_pivot_val);
				new_pivot_idx += (f(v[n], raw_pivot_val) == -1);
			}

			// put the pivot in its proper place
			std::swap(v[raw_pivot_idx], v[new_pivot_idx += min_idx]);
		}

		// look for pairs of values that are inverted wrt the pivot
		while (i < new_pivot_idx && j > new_pivot_idx) {
			const int a = f(v[i], raw_pivot_val);
			const int b = f(v[j], raw_pivot_val);

			if (a == CMP_GT && b == CMP_LT) {
				std::swap(v[i], v[j]);

				i++;
				j--;
			} else {
				if (a == CMP_LT) { i++; }
				if (b == CMP_GT) { j--; }
			}
		}

		if ((max_idx - min_idx) > 1) {
			partition_array(v, f, min_idx, new_pivot_idx);
			partition_array(v, f, new_pivot_idx + 1, max_idx);
		}
	}

	// in-place recursive version
	template<typename type> void sort_array_ip(ext_vector<type>& v, const std::function<int(type, type)>& f) {
		partition_array(v, f, 0, v.size() - 1);
	}



	template<typename type> ext_vector<type> sort_array(const ext_vector<type>& v, const std::function<int(type, type)>& f) {
		if (v.size() <= 1)
			return v;

		// use the average value as pivot
		const type pivot = sum_array(v, 0, v.size() - 1) / v.size();

		ext_vector<type> vlt; vlt.reserve(v.size());
		ext_vector<type> veq; veq.reserve(v.size());
		ext_vector<type> vgt; vgt.reserve(v.size());

		for (size_t n = 0; n < v.size(); n++) {
			switch (f(v[n], pivot)) {
				case CMP_LT: { vlt.push_back(v[n]); } break;
				case CMP_EQ: { veq.push_back(v[n]); } break;
				case CMP_GT: { vgt.push_back(v[n]); } break;
			}
		}

		return (sort_array(vlt, f) + veq + sort_array(vgt, f));
	}
}





int main() {
	const std::function<int(int, int)> f = &compare_func_inc<int>;

	const ext_vector<int> u = { {325435, 123, 717, 2687, 79374, 56987, 3456, 1123095, 25968} };
	const ext_vector<int> v = lib_msort::sort_array(u, f);
	const ext_vector<int> w = lib_qsort::sort_array(u, f);

	ext_vector<int> a = u;
	ext_vector<int> b = u;

	lib_msort::sort_array_ip(a, f);
	lib_qsort::sort_array_ip(b, f);

	print_array(u);
	print_array(v); assert(is_array_sorted(v, f));
	print_array(w); assert(is_array_sorted(w, f));
	print_array(a);
	print_array(b); assert(is_array_sorted(b, f));
	return 0;
}

