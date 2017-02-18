#include <cassert>
#include <cmath>
#include <new>
#include <queue>
#include <vector>
#include <boost/thread/mutex.hpp>

#define USE_THREADSAFE_POOL 1

namespace memory_pool {
	enum t_growth_policy {
		POLICY_ADD = 0,
		POLICY_LOG = 1,
		POLICY_MUL = 2,
	};



	template<typename T> class t_object_pool;

	template<typename T>
	class t_pool_object {
	typedef t_pool_object<T> t_object_type;
	typedef t_object_pool<t_object_type> t_pool_type;
	public:
		virtual ~t_pool_object() {}

		// re-route all (de)allocations to the pool
		static void* operator new(size_t) {
			return ((t_pool_type::get_instance())->acquire());
		}
		static void operator delete(void* ptr) {
			(t_pool_type::get_instance())->yield(static_cast<t_object_type*>(ptr));
		}
	};



	template<class T>
	class t_object_pool {
		// only these objects may call acquire and yield
		friend T;

	public:
		virtual ~t_object_pool<T>() {
			free(m_data); m_data = nullptr;
		}


		static t_object_pool<T>* get_instance() {
			static t_object_pool<T>* instance = nullptr;

			if (instance == nullptr)
				instance = new t_object_pool<T>();

			return instance;
		}

		static void free_instance(t_object_pool<T>* instance) {
			delete instance;
		}


		void reserve(size_t num_objects) {
			#if (USE_THREADSAFE_POOL == 1)
			boost::mutex::scoped_lock lock(m_mutex);
			#endif

			set_capacity(num_objects);
		}


		t_growth_policy get_growth_policy() const { return m_growth_policy; }
		void set_growth_policy(t_growth_policy growth_policy) { m_growth_policy = growth_policy; }

		size_t get_size() {
			#if (USE_THREADSAFE_POOL == 1)
			boost::mutex::scoped_lock lock(m_mutex);
			#endif

			return m_object_count;
		}

	private:
		t_object_pool<T>(t_growth_policy p = POLICY_ADD): m_data(nullptr) {
			m_object_count = 0;
			m_growth_policy = p;
		}


		T* acquire() {
			#if (USE_THREADSAFE_POOL == 1)
			boost::mutex::scoped_lock lock(m_mutex);
			#endif

			// grab more memory if needed
			if (m_offset_queue.empty())
				expand();

			const size_t offset = m_offset_queue.front();

			m_offset_queue.pop();
			return (offset_to_ptr(offset));
		}

		void yield(T* ptr) {
			#if (USE_THREADSAFE_POOL == 1)
			boost::mutex::scoped_lock lock(m_mutex);
			#endif

			m_offset_queue.push(ptr_to_offset(ptr));
		}

		void expand() {
			size_t new_object_count = 0;

			switch (m_growth_policy) {
				case POLICY_ADD: {
					new_object_count = m_object_count + 1;
				} break;
				case POLICY_LOG: {
					new_object_count = m_object_count + std::log(m_object_count);
				} break;
				case POLICY_MUL: {
					new_object_count = m_object_count * 2;
				} break;
			}

			// always expand by at least one object
			if (new_object_count == m_object_count) {
				new_object_count = m_object_count + 1;
			}

			set_capacity(new_object_count);
			assert(!m_offset_queue.empty());
		}

		void set_capacity(size_t num_objects) {
			if (num_objects < m_object_count)
				return;

			T* data = static_cast<T*>(std::realloc(m_data, num_objects * sizeof(T)));

			if (data == nullptr)
				throw (std::bad_alloc());

			m_data = data;

			// add the new T-sized blocks
			for (size_t offset = m_object_count; offset < num_objects; ++offset) {
				m_offset_queue.push(offset);
			}

			m_object_count = num_objects;
		}

	private:
		// converts an offset to an address
		T* offset_to_ptr(size_t offset) const { return (m_data + offset); }
		// converts an address to an offset to be used in the queue
		size_t ptr_to_offset(const T* ptr) const { return (ptr - m_data); }

	private:
		// NOTE:
		//   all acquire()'d pointers are offsets relative to this address
		//   (meaning they can be invalidated when the pool is expand()'ed)
		T* m_data;

		size_t m_object_count;
		t_growth_policy m_growth_policy;

		std::queue<size_t> m_offset_queue;
		boost::mutex m_mutex;
	};
}



int main() {
	typedef memory_pool::t_pool_object<int> t_int_obj;
	typedef memory_pool::t_object_pool<t_int_obj> t_pool_obj;

	t_pool_obj* pool = t_pool_obj::get_instance();
	pool->reserve(32);

	std::vector<t_int_obj*> objects;
	objects.reserve(pool->get_size());

	for (size_t n = 0; n < pool->get_size(); n++) {
		objects.push_back(new t_int_obj());
	}
	for (size_t n = 0; n < pool->get_size(); n++) {
		delete objects[n];
	}

	t_pool_obj::free_instance(pool);
	return 0;
}

