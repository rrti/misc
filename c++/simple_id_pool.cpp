#include <cassert>
#include <cstddef>
#include <cstdio>
#include <ctime>

#include <algorithm>
#include <vector>

#define POOL_SIZE 8



template<typename id_type> struct t_id_pool {
public:
	t_id_pool(size_t num_ids = POOL_SIZE) {
		assert(num_ids > 0);

		m_idx_uid_tbl.reserve(num_ids);
		m_uid_idx_tbl.reserve(num_ids);

		for (size_t n = 0; n < num_ids; n++) {
			m_idx_uid_tbl.push_back(n);
			m_uid_idx_tbl.push_back(n);
		}

		std::random_shuffle(m_idx_uid_tbl.begin(), m_idx_uid_tbl.end());

		// setup the bidirectional mapping
		for (size_t n = 0; n < num_ids; n++) {
			m_uid_idx_tbl[ m_idx_uid_tbl[n] ] = n;
		}
	}

	// number of remaining free ID's; total number of ID's
	size_t get_size() const { return (m_idx_uid_tbl.size()); }
	size_t get_capa() const { return (m_uid_idx_tbl.size()); }

	// always at least doubles the pool capacity; O(n)
	void expand(size_t num_new_ids = POOL_SIZE) {
		const size_t cur_size = get_size();
		const size_t cur_capa = get_capa();
		const size_t ext_capa = std::max(num_new_ids, cur_capa);

		for (size_t n = 0; n < ext_capa; n++) {
			m_idx_uid_tbl.push_back(cur_capa + n    );
			m_uid_idx_tbl.push_back(cur_size + n - 1);
		}

		// shuffle the newcomers
		std::random_shuffle(m_idx_uid_tbl.begin() + cur_size, m_idx_uid_tbl.end());

		// setup the bidirectional mapping (newcomers only)
		for (size_t n = cur_size; n < cur_size + ext_capa; n++) {
			assert(                                  n   < get_size());
			assert(static_cast<size_t>(m_idx_uid_tbl[n]) < get_capa());

			m_uid_idx_tbl[ m_idx_uid_tbl[n] ] = n;
		}
	}

	void print_elems(const char* s) {
		#ifndef NDEBUG
		printf("\n[%s][%s][size=%lu]\n", __func__, s, get_size());

		for (size_t n = 0; n < get_capa(); n++) {
			// add separator to mark start of used capacity (garbage)
			if (n == get_size())
				printf("\t----------\n");

			printf("\t[idx=%2lu -> uid=%2d ][ uid=%2lu -> idx=%+2d]\n", n, m_idx_uid_tbl[n], n, m_uid_idx_tbl[n]);
		}
		#endif
	}

	// extracts a random ID; O(1)
	id_type extract_id() {
		assert(get_size() > 0);

		const id_type nid = m_idx_uid_tbl.back();
		const id_type idx = m_uid_idx_tbl[nid];

		assert(static_cast<size_t>(nid) < get_capa());

		m_idx_uid_tbl.pop_back();
		m_uid_idx_tbl[nid] = -idx - 1;

		return nid;
	}

	// puts an ID back in the pool; O(1)
	void recycle_id(id_type id) {
		assert(m_uid_idx_tbl[id] < 0);

		if (get_size() == 0) {
			m_idx_uid_tbl.push_back(id);
			m_uid_idx_tbl[id] = get_size() - 1;
			return;
		}

		// 1) pick a random still-free ID <rid>
		// 2) insert <id> at the index of <rid>
		// 3) insert <rid> at the end of the pool
		//
		// this ensures a subsequent extract_id() call will not
		// return an ID that was just recycled, which preserves
		// randomness
		const id_type idx = random() % get_size();
		const id_type rid = m_idx_uid_tbl[idx];

		assert(m_uid_idx_tbl[rid] >= 0);

		m_idx_uid_tbl[idx] = id;
		m_uid_idx_tbl[id] = idx;

		m_idx_uid_tbl.push_back(rid);
		m_uid_idx_tbl[rid] = get_size() - 1;
	}

	// extracts a specified ID; O(1)
	bool reserve_id(id_type id) {
		if (static_cast<size_t>(id) >= get_capa())
			return false;
		if (m_uid_idx_tbl[id] < 0)
			return false;

		const id_type idx = m_uid_idx_tbl[id];
		const id_type tid = m_idx_uid_tbl.back();

		assert(static_cast<size_t>(idx) < get_size());
		assert(static_cast<size_t>(tid) < get_capa());

		m_idx_uid_tbl[idx] = tid;
		m_uid_idx_tbl[tid] = idx;
		m_idx_uid_tbl.pop_back();

		// mark index of the ID as used, as in extract_id()
		// mapping an index to -index - 1 is invertible and
		// works for 0 which would otherwise require special
		// treatment
		m_uid_idx_tbl[id] = -m_uid_idx_tbl[id] - 1;
		return true;
	}

private:
	std::vector< id_type > m_idx_uid_tbl;
	std::vector< id_type > m_uid_idx_tbl;
};



int main(int argc, char** argv) {
	const unsigned int seed = (argc > 1)? std::atoi(argv[1]): time(nullptr);

	printf("[%s] seed=%u\n", __func__, seed);
	srandom(seed);

	t_id_pool<int32_t> pool;

	pool.reserve_id(5);
	pool.reserve_id(3);
	pool.reserve_id(0);
	pool.reserve_id(7);
	pool.extract_id();
	pool.expand();
	assert(!pool.reserve_id(5));
	assert(!pool.reserve_id(3));
	assert(!pool.reserve_id(0));
	assert(!pool.reserve_id(7));
	pool.extract_id();
	pool.recycle_id(7);
	pool.recycle_id(0);
	pool.recycle_id(3);
	pool.recycle_id(5);

	while (pool.get_size() > 0) {
		printf("[%s] id=%d\n", __func__, pool.extract_id());
	}

	return 0;
}

