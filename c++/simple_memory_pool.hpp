#ifndef SIMPLE_MEMORY_POOL_HDR
#define SIMPLE_MEMORY_POOL_HDR

#include <cassert>
#include <cstdint>
#include <cstring> // memset

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


template<size_t t_size> struct t_mem_pool {
private:
	// page-header size; page-indices are stored in the first <h_size> bytes of a page
	// we could also use an intrusive mapping in which each object stores its own page
	// index (less generic) or an explicit <void*, size_t> table (less efficient)
	static constexpr size_t h_size = sizeof(size_t);
	static constexpr size_t p_size = h_size + t_size;

public:
	template<typename t_type, typename... t_args> t_type* alloc(t_args&&... args) {
		#ifndef ENABLE_MEMORY_POOL
			return (new t_type(std::forward<t_args>(args)...));
		#else
			static_assert(sizeof(t_type) <= t_size, "");

			t_type* p = nullptr;
			uint8_t* m = nullptr;

			if (m_indcs.empty()) {
				m_pages.emplace_back();

				m = mem(m_pages.size() - 1);
				p = new (m) t_type(std::forward<t_args>(args)...);
			} else {
				// pop before ctor runs; handles recursive construction
				m = mem(util::vector_back_pop(m_indcs));
				p = new (m) t_type(std::forward<t_args>(args)...);
			}

			return p;
		#endif
	}

	template<typename t_type> void free(t_type*& p) {
		#ifndef ENABLE_MEMORY_POOL
			util::safe_delete(p);
		#else
			const size_t i = idx(p);

			assert(mapped(p));
			util::safe_destruct(p);
			std::memset(page(i), 0, p_size);

			// push after dtor runs; it might invoke another ctor
			// which would otherwise claim the page and overwrite
			// *p
			m_indcs.push_back(i);
		#endif
	}


	void reserve(size_t n) { m_indcs.reserve(n); }
	void clear() {
		m_pages.clear();
		m_indcs.clear();
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
	const uint8_t* page(size_t n) const { return &m_pages[n][0]; }
	      uint8_t* page(size_t n)       { return &m_pages[n][0]; }

	// pointer to start of allocated region within a page
	uint8_t* mem(size_t n) {
		uint8_t* m = page(n);
		uint8_t* h = reinterpret_cast<uint8_t*>(&n);

		// write the header (i.e. page-index) bytes
		std::memcpy(m, h, h_size);

		return (m + h_size);
	}

private:
	std::deque< uint8_t[p_size] > m_pages;
	std::vector<size_t> m_indcs;
};

#endif

