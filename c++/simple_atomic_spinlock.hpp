#ifndef SIMPLE_ATOMIC_SPINLOCK_HDR
#define SIMPLE_ATOMIC_SPINLOCK_HDR

#include <atomic>

#define USE_STD_ATOMIC
#ifdef USE_STD_ATOMIC
typedef std::atomic<bool> t_atomic_bool;
#else
typedef volatile t_atomic_bool;
#endif



// only useful in dual-thread scenario where
// one wait()'s for the other to cont()'inue
struct t_binary_spinlock {
public:
	void wait(t_atomic_bool& cond) {
		m_syn_flag = true;

		// wait until other thread calls cont()
		while (!m_ack_flag);

		// notify other thread we have seen the ack
		cond = true;
	}

	void cont(t_atomic_bool& cond) {
		if (!m_syn_flag)
			return;

		m_ack_flag = true;

		// loop until other thread has seen our ack
		while (!cond);
	}

	void reset() { m_syn_flag = false; m_ack_flag = false; }
	bool used() const { return (m_syn_flag && m_ack_flag); }

private:
	t_atomic_bool m_syn_flag;
	t_atomic_bool m_ack_flag;
};



class t_atomic_spinlock {
public:
	t_atomic_spinlock(): m_state(ATOMIC_FLAG_INIT) {}

	void lock() {
		// busy-wait; uses more cycles but has lowest latency
		while (m_state.test_and_set(std::memory_order_acquire));
	}
	void unlock() {
		m_state.clear(std::memory_order_release);
	}

private:
	std::atomic_flag m_state;
};

#endif

