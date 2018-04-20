#ifndef MEMORY_POOL_TYPES_HDR
#define MEMORY_POOL_TYPES_HDR

#include <cassert>
#include <cstdint>
#include <cstring> // memset

#include <algorithm>
#include <memory>

#include <array>
#include <deque>
#include <vector>

#define ENABLE_MEMORY_POOL

namespace util {
	template<class t_type> inline void safe_destruct(t_type*& p) {
		assert(p != nullptr);
		t_type* tmp = p; p = nullptr; tmp->~t_type();
	}

	template<class t_type> inline void safe_delete(t_type*& p) {
		t_type* tmp = p; p = nullptr; delete tmp;
	}

	template<typename t_type> inline t_type vector_back_pop(std::vector<t_type>& v) {
		const t_type e = v.back(); v.pop_back(); return e;
	}
};




template<size_t t_size> struct t_deque_mem_pool {
private:
	// page-header size; page-indices are stored in the first <h_size> bytes of a page
	// we could also use an intrusive mapping in which each object stores its own page
	// index (less generic) or an explicit <void*, size_t> table (less efficient)
	static constexpr size_t h_size = sizeof(size_t);
	static constexpr size_t p_size = h_size + t_size;

public:
	t_deque_mem_pool() { clear(); }

	template<typename t_type, typename... t_args> t_type* alloc(t_args&&... args) {
		#ifndef ENABLE_MEMORY_POOL
			return (new t_type(std::forward<t_args>(args)...));
		#else
			static_assert(sizeof(t_type) <= t_size, "");

			t_type* p = nullptr;
			uint8_t* m = nullptr;

			m_ctor_call_depth += 1;

			if (m_indcs.empty()) {
				m_pages.emplace_back();

				m = mem(m_pages.size() - 1);
				p = new (m) t_type(std::forward<t_args>(args)...);
			} else {
				// pop before ctor runs; handles recursive construction
				m = mem(util::vector_back_pop(m_indcs));
				p = new (m) t_type(std::forward<t_args>(args)...);
			}

			m_ctor_call_depth -= 1;
			return p;
		#endif
	}

	template<typename t_type> void free(t_type*& p) {
		#ifndef ENABLE_MEMORY_POOL
			util::safe_delete(p);
		#else
			const size_t i = idx(p);

			m_dtor_call_depth += 1;

			assert(mapped(p));
			util::safe_destruct(p);
			std::memset(page(i), 0, p_size);

			// push after dtor runs; it might invoke another ctor
			// which would otherwise claim the page and overwrite
			// *p
			m_indcs.push_back(i);

			m_dtor_call_depth -= 1;
		#endif
	}


	bool in_ctor_call() const { return (m_ctor_call_depth > 0); }
	bool in_dtor_call() const { return (m_dtor_call_depth > 0); }
	bool in_xtor_call() const { return (in_ctor_call() || in_dtor_call()); }

	void reserve(size_t n) { m_indcs.reserve(n); }
	void clear() {
		m_pages.clear();
		m_indcs.clear();

		m_ctor_call_depth = 0;
		m_dtor_call_depth = 0;
	}
	void reset() {
		// destroy all pages directly, without invoking destructors
		for (size_t n = 0; n < m_pages.size(); n++) {
			std::memset(page(n), 0, p_size);
		}

		clear();
	}

	template<typename t_type> bool mapped(const t_type* p) const {
		return ((idx(p) < m_pages.size()) && (page(idx(p)) == hdr(p)));
	}

private:
	// page-header address given a mapped pointer
	template<typename t_type> const uint8_t* hdr(const t_type* p) const { return (reinterpret_cast<const uint8_t*>(p) - h_size); }
	template<typename t_type>       uint8_t* hdr(const t_type* p)       { return (reinterpret_cast<      uint8_t*>(p) - h_size); }

	// page-index corresponding to a mapped pointer (FIXME: alignment?)
	template<typename t_type> size_t idx(const t_type* p) const {
		const uint8_t* h = hdr(p);
		const size_t* i = reinterpret_cast<const size_t*>(h);
		return (*i);
	}

	// pointer to start of page-header given an index
	const uint8_t* page(size_t n) const { return m_pages[n].data(); }
	      uint8_t* page(size_t n)       { return m_pages[n].data(); }

	// pointer to start of allocated region within a page
	uint8_t* mem(size_t n) {
		uint8_t* m = page(n);
		uint8_t* h = reinterpret_cast<uint8_t*>(&n);

		// write the header (i.e. page-index) bytes
		std::memcpy(m, h, h_size);

		return (m + h_size);
	}

private:
	std::deque< std::array<uint8_t, p_size> > m_pages;
	std::vector<size_t> m_indcs;

	size_t m_ctor_call_depth = 0;
	size_t m_dtor_call_depth = 0;
};




// each chunk holds <num_pages> pages, each of size <page_size>
template<size_t max_chunks, size_t page_size, size_t num_pages> struct t_chunked_mem_pool {
public:
	template<typename t_type, typename... t_args> t_type* alloc(t_args&&... a) {
		#ifndef ENABLE_MEMORY_POOL
			return (new t_type(std::forward<t_args>(a)...));
		#else
			static_assert(sizeof(t_type) <= page_size, "");
			return (new (alloc_raw(sizeof(t_type))) t_type(std::forward<t_args>(a)...));
		#endif
	}

	void* alloc_raw(size_t size) {
		uint8_t* ptr = nullptr;

		if (m_indcs.empty()) {
			// pool is full
			if (m_num_chunks == max_chunks)
				return ptr;

			assert(m_chunks[m_num_chunks] == nullptr);
			m_chunks[m_num_chunks].reset(new t_chunk_mem());

			// reserve new indices; in reverse order since each will be popped from the back
			for (size_t j = 0; j < num_pages; j++) {
				m_indcs.push_back((m_num_chunks + 1) * num_pages - j - 1); // i * K + j
			}

			m_num_chunks += 1;
		}

		const size_t idx = util::vector_back_pop(m_indcs);

		assert(size <= page_size);
		memcpy(ptr = page_mem(m_page_index = idx), &idx, sizeof(size_t));
		return (ptr + sizeof(size_t));
	}


	void free_raw(void* ptr) {
		const size_t idx = page_idx(ptr);

		// zero-fill page
		assert(idx < (max_chunks * num_pages));
		memset(page_mem(idx), 0, sizeof(size_t) + page_size);

		m_indcs.push_back(idx);
	}

	template<typename t_type> void free(t_type*& ptr) {
		#ifndef ENABLE_MEMORY_POOL
			util::safe_delete(ptr);
		#else
			static_assert(sizeof(t_type) <= page_size, "");

			t_type* tmp = ptr;

			util::safe_destruct(ptr);
			free_raw(tmp);
		#endif
	}


	void reserve(size_t n) { m_indcs.reserve(std::max(n, num_pages)); }
	void clear() {
		m_indcs.clear();

		// for every allocated chunk, add back all indices
		// (objects are assumed to have already been freed)
		for (size_t i = 0; i < m_num_chunks; i++) {
			for (size_t j = 0; j < num_pages; j++) {
				m_indcs.push_back((i + 1) * num_pages - j - 1);
			}
		}

		m_page_index = 0;
	}


	const uint8_t* page_mem(size_t idx, size_t ofs = 0) const {
		const t_chunk_ptr& chunk_ptr = m_chunks[idx / num_pages];
		const t_chunk_mem& chunk_mem = *chunk_ptr;
		return (&chunk_mem[idx % num_pages][0] + ofs);
	}
	uint8_t* page_mem(size_t idx, size_t ofs = 0) {
		t_chunk_ptr& chunk_ptr = m_chunks[idx / num_pages];
		t_chunk_mem& chunk_mem = *chunk_ptr;
		return (&chunk_mem[idx % num_pages][0] + ofs);
	}

	size_t page_idx(void* ptr) const {
		const uint8_t* raw_ptr = reinterpret_cast<const uint8_t*>(ptr);
		const uint8_t* idx_ptr = raw_ptr - sizeof(size_t);

		return (*reinterpret_cast<const size_t*>(idx_ptr));
	}

	size_t alloc_size() const { return (m_num_chunks * num_pages * page_size); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (m_indcs.size() * page_size); } // size of number of pages that were freed and are awaiting reuse

	bool mapped(void* ptr) const { return ((page_idx(ptr) < (m_num_chunks * num_pages)) && (page_mem(page_idx(ptr), sizeof(size_t)) == ptr)); }
	bool alloced(void* ptr) const { return ((m_page_index < (m_num_chunks * num_pages)) && (page_mem(m_page_index, sizeof(size_t)) == ptr)); }

private:
	// first size_t bytes are reserved for index
	typedef std::array<uint8_t[sizeof(size_t) + page_size], num_pages> t_chunk_mem;
	typedef std::unique_ptr<t_chunk_mem> t_chunk_ptr;

	// could also be a vector to make capacity unbounded
	std::array<t_chunk_ptr, max_chunks> m_chunks;
	std::vector<size_t> m_indcs;

	size_t m_num_chunks = 0;
	size_t m_page_index = 0;
};




template<size_t num_pages, size_t page_size> struct t_array_mem_pool {
public:
	t_array_mem_pool() { clear(); }

	template<typename t_type, typename... t_args> t_type* alloc(t_args&&... args) {
		#ifndef ENABLE_MEMORY_POOL
			return (new t_type(std::forward<t_args>(args)...));
		#else
			static_assert(num_pages != 0, "");
			static_assert(page_size != 0, "");
			static_assert(sizeof(t_type) <= page_size, "");

			t_type* p = nullptr;
			uint8_t* m = nullptr;

			size_t i = 0;

			// allow recursion
			// assert(!in_ctor_call());
			assert(!all_pages_used());

			m_ctor_call_depth += 1;

			if (m_free_page_count == 0) {
				i = m_used_page_count++;
				m = m_pages[m_curr_page_index = i].data();
				p = new (m) t_type(std::forward<t_args>(args)...);
			} else {
				i = m_indcs[--m_free_page_count];
				m = m_pages[m_curr_page_index = i].data();
				p = new (m) t_type(std::forward<t_args>(args)...);
			}

			m_ctor_call_depth -= 1;
			return p;
		#endif
	}

	template<typename t_type> void free(t_type*& p) {
		#ifndef ENABLE_MEMORY_POOL
			util::safe_delete(p);
		#else
			uint8_t* m = reinterpret_cast<uint8_t*>(p);

			// allow recursion
			// assert(!in_dtor_call());
			assert(!all_pages_freed());
			assert(mapped(p));

			m_dtor_call_depth += 1;

			util::safe_destruct(p);
			std::memset(m, 0, page_size);

			// mark page as free
			m_indcs[m_free_page_count++] = base_offset(m) / page_size;

			m_dtor_call_depth -= 1;
		#endif
	}


	size_t alloc_size() const { return (m_used_page_count * page_size); } // size of total number of pages added over the pool's lifetime
	size_t freed_size() const { return (m_free_page_count * page_size); } // size of number of pages that were freed and are awaiting reuse
	size_t total_size() const { return (num_pages * page_size); }
	size_t base_offset(const void* p) const { return (reinterpret_cast<const uint8_t*>(p) - reinterpret_cast<const uint8_t*>(m_pages[0].data())); }

	bool mapped(const void* p) const { return (((base_offset(p) / page_size) < total_size()) && ((base_offset(p) % page_size) == 0)); }
	bool alloced(const void* p) const { return (m_pages[m_curr_page_index].data() == p); }

	bool all_pages_used() const { return (m_used_page_count >= num_pages && m_free_page_count == 0); }
	bool all_pages_freed() const { return (m_free_page_count >= num_pages); }

	bool in_ctor_call() const { return (m_ctor_call_depth > 0); }
	bool in_dtor_call() const { return (m_dtor_call_depth > 0); }
	bool in_xtor_call() const { return (in_ctor_call() || in_dtor_call()); }

	void reserve(size_t) {} // no-op
	void clear() {
		std::memset(m_pages.data(), 0, total_size());
		std::memset(m_indcs.data(), 0, num_pages);

		m_used_page_count = 0;
		m_free_page_count = 0;
		m_curr_page_index = 0;

		m_ctor_call_depth = 0;
		m_dtor_call_depth = 0;
	}

private:
	std::array<std::array<uint8_t, page_size>, num_pages> m_pages;
	std::array<size_t, num_pages> m_indcs;

	size_t m_used_page_count = 0;
	size_t m_free_page_count = 0;
	size_t m_curr_page_index = 0;

	size_t m_ctor_call_depth = 0;
	size_t m_dtor_call_depth = 0;
};




template<typename t_object> class t_vector_mem_pool {
public:
	t_vector_mem_pool(size_t size) { resize(size); }
	~t_vector_mem_pool() { clear(); }

	t_vector_mem_pool(const t_vector_mem_pool&  mp) = delete;
	t_vector_mem_pool(      t_vector_mem_pool&& mp) { *this = std::move(mp); }

	t_vector_mem_pool& operator = (const t_vector_mem_pool&  mp) = delete;
	t_vector_mem_pool& operator = (      t_vector_mem_pool&& mp) {
		m_pages = std::move(mp.m_pages);
		m_indcs = std::move(mp.m_indcs);
		return *this;
	}

public:
	size_t cur_size() const { return (m_pages.size() - m_indcs.size()); }
	size_t max_size() const { return (m_pages.size()); }

	bool empty() const { return (cur_size() == 0); }
	bool full() const { return (cur_size() == max_size()); }

	// for cache-locality, sort m_indcs in *decreasing* order since alloc() will pop from the back
	void sort() { std::sort(m_indcs.begin(), m_indcs.end(), [](size_t a, size_t b) { return (a > b); }); }
	void resize(size_t new_size) {
		const size_t old_size = max_size();

		assert(new_size > 0);

		m_pages.resize(new_size); // requires copy-contructable T's
		m_indcs.reserve(new_size);

		if (new_size >= old_size) {
			// add new batch of free indices
			for (size_t n = old_size; n < new_size; n++) {
				m_indcs.push_back(n);
			}
		} else {
			// throw away leftover free indices that are now out of bounds
			for (size_t n = 0; n < m_indcs.size(); ) {
				if (m_indcs[n] >= new_size) {
					m_indcs[n] = util::vector_back_pop(m_indcs);
					continue;
				}

				n++;
			}
		}

		sort();
	}

	void clear() {
		m_pages.clear();
		m_indcs.clear();

		assert(empty());
		assert(full());
	}

public:
	// index-based API, allows resizing the pool without causing invalidation
	const t_object* get_ptr(size_t idx) const { assert(idx < max_size()); return &m_pages[idx]; }
	      t_object* get_ptr(size_t idx)       { assert(idx < max_size()); return &m_pages[idx]; }

	template<typename... A> size_t alloc(A&&... args) {
		if (full())
			return (max_size());

		const size_t idx = util::vector_back_pop(m_indcs);

		new (get_ptr(idx)) t_object(std::forward<A>(args)...);
		return idx;
	}

	bool free(size_t idx) {
		if (empty())
			return false;
		if (idx >= max_size())
			return false;

		t_object* p = get_ptr(idx);
		p->~t_object();
		memset(p, 0, sizeof(t_object));

		m_indcs.push_back(idx);
		return true;
	}

private:
	std::vector<t_object> m_pages;
	std::vector<size_t> m_indcs;
};

#endif

