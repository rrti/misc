#ifndef SIMPLE_FLAT_MAP_HDR
#define SIMPLE_FLAT_MAP_HDR

#include <cassert>
#include <cstddef>

#include <algorithm>
#include <functional>
#include <vector>

#define USE_CUSTOM_ITERATOR

namespace util {
	template<typename t_iter, typename t_pred, typename t_type>
	static t_iter bfind(t_iter l, t_iter r, const t_pred& p, const t_type& e, bool b = true) {
		const t_iter i = std::lower_bound(l, r, e, p);

		// if e < *i, then i is the would-be position
		if ((i == r) || (b && p(e, *i)))
			return r;

		return i;
	}
};

// maintains a sorted std::vector of elements; best used
// when inserts are rare but search and erase are common
// similar to (though much simpler than) boost::flat_map,
// but our erase always runs in O(log(N)) time by merely
// tombstoning elements
template<typename t_key_type, typename t_val_type> struct t_vector_map {
public:
	typedef  std::pair<t_key_type, t_val_type>  t_elem_pair;
	typedef  std::pair<t_key_type,       bool>  t_flag_pair;

	typedef  std::function<bool(const t_elem_pair& a, const t_elem_pair& b)>  t_sort_pred;
	typedef  std::function< int(const t_elem_pair& a, const t_elem_pair& b)>  t_find_pred;

	typedef  typename std::vector<t_elem_pair>::iterator  t_iter_type_i;
	typedef  typename std::vector<t_elem_pair>::const_iterator  t_citer_type_i;


	#ifdef USE_CUSTOM_ITERATOR
	template<typename t_elem, typename t_flag, bool fwd = true> struct t_iter {
	public:
		// satisfy iterator_traits
		typedef std::random_access_iterator_tag iterator_category;
		typedef ptrdiff_t difference_type;

		typedef t_elem value_type;
		typedef t_elem* pointer;
		typedef t_elem& reference;

	public:
		explicit t_iter() {
			m_elem_ptr = nullptr;
			m_sent_ptr = nullptr;
			m_flag_ptr = nullptr;
		}
		explicit t_iter(t_elem* elem_ptr, t_elem* end_ptr, t_flag* flag_ptr) {
			m_elem_ptr = elem_ptr;
			m_sent_ptr = end_ptr;
			m_flag_ptr = flag_ptr;

			// move to the first non-erased element
			advance();
		}
		t_iter& operator = (const t_iter& i) {
			m_elem_ptr = i.m_elem_ptr;
			m_sent_ptr = i.m_sent_ptr;
			m_flag_ptr = i.m_flag_ptr;
			return *this;
		}

		// prefix
		t_iter operator ++ () {
			if (m_elem_ptr == m_sent_ptr)
				return *this;

			// advance at least once, skip erased elements
			step();
			advance();
			return *this;
		}
		// postfix
		t_iter operator ++ (int) {
			t_iter i = *this;
			*this = ++(*this);
			return i;
		}

		t_iter& operator += (size_t n) { m_elem_ptr += n; m_flag_ptr += n; return *this; }
		t_iter& operator -= (size_t n) { m_elem_ptr -= n; m_flag_ptr -= n; return *this; }

		t_iter operator + (size_t n) const { return (t_iter{m_elem_ptr + n, m_sent_ptr, m_flag_ptr + n}); }
		t_iter operator - (size_t n) const { return (t_iter{m_elem_ptr - n, m_sent_ptr, m_flag_ptr - n}); }

		ptrdiff_t operator - (const t_iter& i) const { return (m_elem_ptr - i.m_elem_ptr); }

		t_elem* operator -> () const { return ( m_elem_ptr); }
		t_elem* operator -> ()       { return ( m_elem_ptr); }
		t_elem& operator  * () const { return (*m_elem_ptr); }
		t_elem& operator  * ()       { return (*m_elem_ptr); }

		bool operator == (const t_iter& i) const { return (m_elem_ptr == i.m_elem_ptr); }
		bool operator != (const t_iter& i) const { return (m_elem_ptr != i.m_elem_ptr); }

		bool operator < (const t_iter& i) const { return (m_elem_ptr < i.m_elem_ptr); }
		bool operator > (const t_iter& i) const { return (m_elem_ptr > i.m_elem_ptr); }

	private:
		void step() {
			m_elem_ptr += (fwd * 2 - 1);
			m_flag_ptr += (fwd * 2 - 1);
		}
		void advance() {
			while (m_elem_ptr != m_sent_ptr && m_flag_ptr->second) {
				step();
			}
		}

	private:
		t_elem* m_elem_ptr;
		t_elem* m_sent_ptr; // sentinel
		t_flag* m_flag_ptr;
	};

	// advance() would break bfind, so use internal and external types
	typedef  t_iter<      t_elem_pair,       t_flag_pair, true>  t_iter_type;
	typedef  t_iter<const t_elem_pair, const t_flag_pair, true>  t_citer_type;
	#else
	// internal and external types are the same
	typedef  t_iter_type_i  t_iter_type;
	typedef  t_citer_type_i  t_citer_type;
	#endif


public:
	t_vector_map() { clear(); }
	t_vector_map(size_t n) {
		clear();
		reserve(n);
	}

	t_vector_map(const t_vector_map& map) { *this = map; }
	t_vector_map(t_vector_map&& map) { *this = std::move(map); }

	t_vector_map& operator = (const t_vector_map& map) {
		m_elems = map.m_elems;
		m_flags = map.m_flags;

		m_sort_pred = map.m_sort_pred;
		m_find_pred = map.m_find_pred;

		m_num_sorted_elems = map.m_num_sorted_elems;
		m_num_erased_elems = map.m_num_erased_elems;
		return *this;
	}

	t_vector_map& operator = (t_vector_map&& map) {
		m_elems = std::move(map.m_elems);
		m_flags = std::move(map.m_flags);

		m_sort_pred = std::move(map.m_sort_pred);
		m_find_pred = std::move(map.m_find_pred);

		m_num_sorted_elems = map.m_num_sorted_elems;
		m_num_erased_elems = map.m_num_erased_elems;
		return *this;
	}

	// number of non-erased sorted elements
	size_t size() const { return m_num_sorted_elems; }
	size_t capacity() const { return (m_elems.capacity()); }

	bool empty() const { return (size() == 0); }
	bool sorted() const { return (map_size() == vec_size()); }
	bool erased(size_t n) const { return (get_flag(n)); }

	bool valid() const {
		#ifdef DEBUG
		// count the number of *in-order* elements, erased or not
		// this should always equal the size of the sorted region
		size_t num_sorted_elems = 1;
		size_t num_erased_elems = 0;
		for (size_t n = 0; n < (map_size() - 1); n++) { num_sorted_elems += m_sort_pred(m_elems[n], m_elems[n + 1]); }
		for (size_t n = 0; n < (map_size()    ); n++) { num_erased_elems += erased(n); }
		return ((num_sorted_elems == map_size()) && (num_erased_elems == m_num_erased_elems));
		#else
		return true;
		#endif
	}


	#ifndef USE_CUSTOM_ITERATOR
	t_citer_type cbegin() const { return (cbegin_i()); }
	t_citer_type   cend() const { return (  cend_i()); }
	t_citer_type  citer(const t_citer_type_i& i) const { return i; }

	t_iter_type begin() { return (begin_i()); }
	t_iter_type   end() { return (  end_i()); }
	t_iter_type  iter(const t_iter_type_i& i) { return i; }
	#else
	t_citer_type cbegin() const { return (t_citer_type{m_elems.data()             , m_elems.data() + map_size(), m_flags.data()             }); }
	t_citer_type   cend() const { return (t_citer_type{m_elems.data() + map_size(), m_elems.data() + map_size(), m_flags.data() + map_size()}); }
	t_citer_type  citer(const t_citer_type_i& i) const { return (t_citer_type{m_elems.data() + (i - m_elems.cbegin()), m_elems.data() + map_size(), m_flags.data() + (i - m_elems.cbegin())}); }

	t_iter_type begin() { return (t_iter_type{m_elems.data()             , m_elems.data() + map_size(), m_flags.data()             }); }
	t_iter_type   end() { return (t_iter_type{m_elems.data() + map_size(), m_elems.data() + map_size(), m_flags.data() + map_size()}); }
	t_iter_type  iter(const t_iter_type_i& i) { return (t_iter_type{m_elems.data() + (i - m_elems.begin()), m_elems.data() + map_size(), m_flags.data() + (i - m_elems.begin())}); }
	#endif



	t_citer_type cfind(const t_key_type& k) const { return (cfind({k, t_val_type()})); }
	t_citer_type cfind(const t_elem_pair& e) const {
		const t_citer_type_i i = cfind_i(e);
		if (i == cend_i())
			return (cend());
		if (get_flag(i - cbegin_i()))
			return (cend());
		return (citer(i));
	}

	t_iter_type find(const t_key_type& k) { return (find({k, t_val_type()})); }
	t_iter_type find(const t_elem_pair& e) {
		const t_iter_type_i i = find_i(e);
		if (i == end_i())
			return (end());
		if (get_flag(i - begin_i()))
			return (end());
		return (iter(i));
	}


	// inserts while maintaining sorted order; O(log(N)) if a slot can be recycled
	std::pair<t_iter_type, bool> emplace(t_elem_pair&& e) { return (insert(e)); }
	std::pair<t_iter_type, bool> insert(const t_elem_pair& e) {
		assert(valid());

		const std::pair<t_iter_type_i, bool>& i = insert_i(e);

		assert(i.first != end_i());
		// would be nicer to do this here, but conflicts internally
		// set_ctrs(1, -1 * get_flag(iter - begin_i()));
		set_flag(i.first - begin_i(), false);
		assert(valid());

		return (std::make_pair<>(iter(i.first), i.second));
	}

	void insert(t_iter_type first, t_iter_type last) {
		// assume caller has already reserve'd
		for (; first != last; ++first) {
			push_back(*first);
		}

		sort();
	}

	bool erase(const t_key_type& k) { return (erase({k, t_val_type()})); }
	bool erase(const t_citer_type_i& i) {
		assert((i - cbegin_i()) >= 0);
		assert(valid());

		// already erased, do nothing
		if (get_flag(i - cbegin_i()))
			return false;

		// mark as erased, update counters
		assert(valid());
		set_flag(i - cbegin_i(), true);
		set_ctrs(-1, 1);

		// destruct only the value; key is needed for future inserts
		// (i->first ).~t_key_type();
		(i->second).~t_val_type();
		return true;
	}

	bool erase(const t_elem_pair& e) {
		const t_citer_type_i& i = cfind_i(e);

		if (i == cend_i())
			return false;

		assert(e.first == i->first);
		return (erase(i));
	}


	void sort() {
		if (sorted())
			return;

		// note: can not sort pairs with const keys, write-protect iterators
		std::sort(m_elems.begin(), m_elems.begin() + vec_size(), m_sort_pred);
		std::sort(m_flags.begin(), m_flags.begin() + vec_size(), m_sort_pred);

		t_iter_type_i i = begin_i();

		// filter duplicates; without this we would need linear search fallback
		// if push_back added any duplicates, adjust the num_erased count here
		for (size_t n = 1; n < vec_size(); n++) {
			if (m_elems[n].first != i->first) {
				i += 1;
				*i = m_elems[n];
			} else {
				set_ctrs(0, -1 * get_flag(i - begin_i()));
				set_flag(i - begin_i(), false);
			}
		}

		// remove excess space left by duplicates; would confuse ::sorted
		while (vec_size() > ((i - begin_i()) + 1)) {
			pop_back();
		}

		m_num_sorted_elems = vec_size() - m_num_erased_elems;

		assert(sorted());
		assert(valid());
	}


	void push_back(const t_elem_pair& e) {
		m_flags.push_back({e.first, false});
		m_elems.push_back(e);
	}
	void emplace_back(t_elem_pair&& e) {
		m_flags.push_back({e.first, false});
		m_elems.emplace_back(e.first, e.second);
	}
	void pop_back() {
		set_ctrs((-1 * sorted() * (1 - get_flag(vec_size() - 1))), (-1 * sorted() * get_flag(vec_size() - 1)));

		m_elems.pop_back();
		m_flags.pop_back();
	}

	void clear() {
		m_elems.clear();
		m_flags.clear();

		// always sort according to key_type::operator<
		m_sort_pred = [](const t_elem_pair& a, const t_elem_pair& b) { return (a.first < b.first); };
		m_find_pred = [](const t_elem_pair& a, const t_elem_pair& b) { return ((a.first < b.first)? -1: (a.first > b.first)? +1: 0); };

		m_num_sorted_elems = 0;
		m_num_erased_elems = 0;
	}
	void reserve(size_t n) {
		m_elems.reserve(n);
		m_flags.reserve(n);
	}

private:
	size_t map_size() const { return (m_num_sorted_elems + m_num_erased_elems); }
	size_t vec_size() const { return (m_elems.size()); }

	bool get_flag(size_t n) const { return m_flags[n].second; }
	void set_flag(size_t n, bool b) { m_flags[n].second = b; }

	void set_ctrs(int ns, int ne) {
		assert(ns >= 0 || m_num_sorted_elems > 0);
		assert(ne >= 0 || m_num_erased_elems > 0);
		m_num_sorted_elems += ns;
		m_num_erased_elems += ne;
	}

	void push_front(const t_elem_pair& e) {
		push_back(e);

		// move all elements one slot to the right
		for (size_t n = vec_size() - 1; n != 0; n -= 1) {
			std::swap(m_elems[n - 1], m_elems[n]);
			std::swap(m_flags[n - 1], m_flags[n]);
		}

		m_elems[0] = e;
		m_flags[0] = {e.first, false};

		assert(valid());
	}


	std::pair<t_iter_type_i, bool> insert_i(const t_elem_pair& e) {
		assert(map_size() != 0);
		assert(valid());

		if (m_elems.empty()) {
			push_back(e);
			set_flag(0, false);

			std::swap(m_elems.back(), m_elems[0]);
			std::swap(m_flags.back(), m_flags[0]);

			assert((*begin_i()) == e);
			set_ctrs(1, 0);
			assert(valid());
			return (std::make_pair(begin_i(), true));
		}

		// insert at front; worst-case
		if (m_sort_pred(e, m_elems[0])) {
			push_front(e);
			assert((*begin_i()) == e);
			set_ctrs(1, 0);
			assert(valid());
			return (std::make_pair(begin_i(), true));
		}

		// insert at end (of map-segment); best-case
		if (m_sort_pred(m_elems[map_size() - 1], e)) {
			push_back(e);
			set_flag(map_size(), false);

			// re-purpose the first non-map element
			std::swap(m_elems.back(), m_elems[map_size()]);
			std::swap(m_flags.back(), m_flags[map_size()]);

			assert((*end_i()) == e);
			set_ctrs(1, 0);
			assert(valid());
			// map-region has been extended by 1; always true
			assert((end_i() - 1) != m_elems.end());
			return (std::make_pair(end_i() - 1, true));
		}


		// pass false so bfind returns the "would be" position for missing but in-range elements
		const t_iter_type_i i = util::bfind(begin_i(), begin_i() + map_size(), m_sort_pred, e, false);

		// can't use std::distance with custom iterators
		assert((i - begin_i()) >=          0);
		assert((i - begin_i()) <  map_size());
		assert(valid());

		// check if we have a previously erased slot; caller resets flag
		if (get_flag(i - begin_i())) {
			*i = e;
			set_ctrs(1, -1);
			return (std::make_pair(i, true));
		}

		// new element, prevent duplicates
		if (e.first == i->first)
			return (std::make_pair(i, false));

		return (std::make_pair(insert_mid(e), true));
	}

	t_iter_type_i insert_mid(const t_elem_pair& e) {
		size_t n = 0;

		push_back(e);
		assert(m_elems.back() == e);
		assert(valid());

		// swap the new element into position; same as moving existing elements over
		for (n = vec_size() - 1; (n != 0 && m_sort_pred(m_elems[n], m_elems[n - 1])); n -= 1) {
			std::swap(m_elems[n - 1], m_elems[n]);
			std::swap(m_flags[n - 1], m_flags[n]);
		}

		// vector has grown by 1 element; LEQ comparison is valid
		assert(n <= map_size());
		set_ctrs(1, 0);
		assert(m_elems[n] == e);
		assert(valid());

		return (begin_i() + n);
	}


	// note: only the sorted region of elements is searched
	t_citer_type_i cfind_i(const t_key_type& k) const { return (cfind_i({k, t_val_type()})); }
	t_citer_type_i cfind_i(const t_elem_pair& e) const { return (util::bfind(cbegin_i(), cend_i(), m_sort_pred, e)); }

	t_iter_type_i find_i(const t_key_type& k) { return (find_i({k, t_val_type()})); }
	t_iter_type_i find_i(const t_elem_pair& e) { return (begin_i() + (cfind_i(e) - cbegin_i())); }


	t_citer_type_i cbegin_i() const { return (m_elems.cbegin()             ); }
	t_citer_type_i   cend_i() const { return (m_elems.cbegin() + map_size()); }

	t_iter_type_i begin_i() { return (m_elems.begin()             ); }
	t_iter_type_i   end_i() { return (m_elems.begin() + map_size()); }

private:
	std::vector<t_elem_pair> m_elems;
	std::vector<t_flag_pair> m_flags;

	t_sort_pred m_sort_pred;
	t_find_pred m_find_pred;

	// number of non-erased sorted elements
	size_t m_num_sorted_elems;
	// number of external erase() calls made
	size_t m_num_erased_elems;
};

#endif

