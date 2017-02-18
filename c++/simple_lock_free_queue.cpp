#include <atomic>
#include <cstdint>
#include <exception>

#include <boost/chrono.hpp>
#include <boost/thread.hpp>



template<typename t_counter> struct t_atomic_counter {
public:
	t_atomic_counter(t_counter val = 0): m_val(val) {
	}

	// note: *prefix* increment, so add&fetch
	t_counter operator ++ () {
		#ifdef _MSC_VER
		return InterlockedIncrement64(&m_val);
		#elif defined(__APPLE__)
		return OSAtomicIncrement64(&m_val);
		#else // assume GCC
		return __sync_add_and_fetch(&m_val, t_counter(1));
		#endif
	}

	t_counter operator += (t_counter x) {
		#ifdef _MSC_VER
		return InterlockedExchangeAdd64(&m_val, t_counter(x));
		#elif defined(__APPLE__)
		return OSAtomicAdd64(t_counter(x), &m_val);
		#else
		return __sync_fetch_and_add(&m_val, t_counter(x));
		#endif
	}

	t_counter operator -= (t_counter x) {
		#ifdef _MSC_VER
		return InterlockedExchangeAdd64(&m_val, t_counter(-x));
		#elif defined(__APPLE__)
		return OSAtomicAdd64(t_counter(-x), &m_val);
		#else
		return __sync_fetch_and_add(&m_val, t_counter(-x));
		#endif
	}

	operator t_counter () const { return m_val; }

private:
	#ifdef _MSC_VER
	__declspec(align(8)) t_counter m_val;
	#else
	__attribute__ ((aligned (8))) t_counter m_val;
	#endif
};



struct t_thread_backoff {
public:
	t_thread_backoff(size_t min_delay = 1, size_t max_delay = 5) {
		m_min_delay = min_delay;
		m_max_delay = max_delay;
		m_max_limit = min_delay;
	}

	void sleep() {
		const boost::posix_time::millisec sleep_time_ms((random() / RAND_MAX) * m_max_limit);

		// update the upper delay bound
		m_max_limit = std::min(m_max_delay, m_max_limit * 2);

		// put the calling thread to sleep for a short random period
		try {
			boost::this_thread::sleep(sleep_time_ms);
		} catch (const std::exception& e) {
		}
	}

private:
	size_t m_min_delay;
	size_t m_max_delay;
	size_t m_max_limit;
};



//
// Lamport queue (lock-free, but it assumes a
// *single* producer and a *single* consumer)
//
template<typename t_elem> class t_wait_free_queue {
public:
	t_wait_free_queue(size_t capacity, const t_thread_backoff& enq_backoff, const t_thread_backoff& deq_backoff) {
		m_data.resize(capacity);

		m_enq_backoff = enq_backoff;
		m_deq_backoff = deq_backoff;

		m_head_idx = 0;
		m_tail_idx = 0;
	}


	bool enqueue(t_elem e) {
		// called on the producer thread
		if (is_full()) {
			++m_blocked_enqueues;
			m_enq_backoff.sleep();
			return false;
		}

		m_data[(m_tail_idx++) % m_data.size()] = e;
		return true;
    }

	t_elem dequeue(bool* ret = nullptr) {
		// called on the consumer thread
		if (is_empty()) {
			++m_blocked_dequeues;
			m_deq_backoff.sleep();

			if (ret != nullptr)
				*ret = false;

			return (t_elem());
		}

		if (ret != nullptr)
			*ret = true;

		return m_data[(m_head_idx++) % m_data.size()];
	}

	bool is_empty() const { return ((m_tail_idx - m_head_idx) == 0); }
	bool is_full() const { return ((m_tail_idx - m_head_idx) == m_data.size()); }

private:
	std::vector<t_elem> m_data;

	t_thread_backoff m_enq_backoff;
	t_thread_backoff m_deq_backoff;

	// statistics trackers
	t_atomic_counter<boost::int64_t> m_blocked_enqueues;
	t_atomic_counter<boost::int64_t> m_blocked_dequeues;

	volatile std::atomic<size_t> m_head_idx;
	volatile std::atomic<size_t> m_tail_idx;
};

