#ifndef LUA_MEM_POOL_HDR
#define LUA_MEM_POOL_HDR

#include <cstddef>
#include <vector>
#include <unordered_map>

#define LMP_TRACK_ALLOCS 1

struct t_lua_ctx_data;
struct t_lua_mem_pool {
public:
	t_lua_mem_pool(size_t pool_index): m_global_index(pool_index) { reserve(16384); }
	~t_lua_mem_pool() { clear(); }

	t_lua_mem_pool(const t_lua_mem_pool& p) = delete;
	t_lua_mem_pool(t_lua_mem_pool&& p) = delete;

	t_lua_mem_pool& operator = (const t_lua_mem_pool& p) = delete;
	t_lua_mem_pool& operator = (t_lua_mem_pool&& p) = delete;

public:
	enum {
		STAT_NIA = 0, // number of internal allocs
		STAT_NEA = 1, // number of external allocs
		STAT_NRA = 2, // number of recycled allocs
		STAT_NCB = 3, // number of chunk bytes currently in use
		STAT_NBB = 4, // number of block bytes alloced in total
	};

public:
	static t_lua_mem_pool* acquire_shared_ptr();
	static t_lua_mem_pool* acquire_ptr(const t_lua_ctx_data* ctx);
	static bool release_ptr(const t_lua_ctx_data* ctx);

	static void free_shared();
	static void init_static();
	static void kill_static();

	// pass as first argument to lua_newstate(lua_Alloc af, void* ud)
	static void* vm_alloc_func(void* ud, void* ptr, size_t osize, size_t nsize);

public:
	void clear() {
		delete_blocks();
		clear_stats(true);
		clear_tables();
	}

	void reserve(size_t size) {
		m_free_chunks_table.reserve(size);
		m_chunk_count_table.reserve(size);

		#if (LMP_TRACK_ALLOCS == 1)
		m_alloc_blocks.reserve(size / 16);
		#endif
	}

	void delete_blocks();
	void* alloc(size_t size);
	void* realloc(void* ptr, size_t nsize, size_t osize);
	void free(void* ptr, size_t size);

	void log_stats(const char* header = "") const;
	void clear_stats(bool b) {
		m_alloc_stats[STAT_NIA] *= (1 - b);
		m_alloc_stats[STAT_NEA] *= (1 - b);
		m_alloc_stats[STAT_NRA] *= (1 - b);
		m_alloc_stats[STAT_NCB] *= (1 - b);
		m_alloc_stats[STAT_NBB] *= (1 - b);
	}
	void clear_tables() {
		m_free_chunks_table.clear();
		m_chunk_count_table.clear();
	}

	size_t  get_alloc_stat(size_t i          ) const { return (m_alloc_stats[i]    ); }
	size_t& set_alloc_stat(size_t i, size_t s)       { return (m_alloc_stats[i] = s); }

	size_t get_global_index() const { return m_global_index; }
	size_t get_shared_count() const { return m_shared_count; }
	size_t inc_shared_count(size_t i) { return (m_shared_count += i); }

public:
	static constexpr size_t MIN_ALLOC_SIZE = sizeof(void*);
	static constexpr size_t MAX_ALLOC_SIZE = (1024 * 1024) - 1;

	static bool is_enabled() { return true; }

private:
	std::unordered_map<size_t, void*> m_free_chunks_table;
	std::unordered_map<size_t, size_t> m_chunk_count_table;

	#if (LMP_TRACK_ALLOCS == 1)
	std::vector<void*> m_alloc_blocks;
	#endif

	size_t m_alloc_stats[5] = {0, 0, 0, 0, 0};
	size_t m_global_index = 0;
	size_t m_shared_count = 0;
};


// use context to wrap pool
struct lua_State;
struct t_lua_ctx_data {
	t_lua_mem_pool* lmp = nullptr;
	lua_State* lvm = nullptr;

	// acquire-flags
	bool share_pool = false;
	bool clear_pool = false;
};

#endif

