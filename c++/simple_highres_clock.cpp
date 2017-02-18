// whether to use timers from {boost,std}::chrono on windows
#define FORCE_CHRONO_TIMERS
// whether to use timers from boost::chrono or std::chrono
#define FORCE_BOOST_CHRONO

// needed for QPC which wants qword-aligned LARGE_INTEGER's
#define __FORCE_ALIGN_STACK__ __attribute__ ((force_align_arg_pointer))
#define USE_NATIVE_WINDOWS_CLOCK (defined(WIN32) && !defined(FORCE_CHRONO_TIMERS))

#if USE_NATIVE_WINDOWS_CLOCK
#include <windows.h>
#endif

#include <boost/cstdint.hpp>
#include <cassert>
#include <cstdio>


// only works if __cplusplus is defined properly by compiler
// #if ((__cplusplus > 199711L) && !defined(FORCE_BOOST_CHRONO))
#if (!defined(FORCE_BOOST_CHRONO))
	#define TIME_USING_STDCHRONO
	#undef gt

	#include <chrono>
	namespace chrono { using namespace std::chrono; };
#else
	#define TIME_USING_LIBCHRONO
	#undef gt

	#include <boost/chrono/include.hpp>
	namespace chrono { using namespace boost::chrono; };
#endif


// [win32] determines whether to use TGT or QPC
// if USE_NATIVE_WINDOWS_CLOCK has been defined
static bool HIGHRES_MODE =  true;
static bool CLOCK_INITED = false;


struct t_hres_clock {
public:
	// NOTE:
	//   1e-x are double-precision literals but T can be float
	//   floats only provide ~6 decimal digits of precision so
	//   to_ssecs is inaccurate in that case
	template<typename T>  static  T to_ssecs(const boost::int64_t ns) { return (ns * 1e-9); }
	template<typename T>  static  T to_msecs(const boost::int64_t ns) { return (ns * 1e-6); }
	template<typename T>  static  T to_usecs(const boost::int64_t ns) { return (ns * 1e-3); }
	template<typename T>  static  T to_nsecs(const boost::int64_t ns) { return (ns       ); }

	// specializations
	// template<>  static  boost::int64_t to_ssecs<boost::int64_t>(const boost::int64_t ns) { return (ns / boost::int64_t(1e9)); }
	// ...
	static boost::int64_t to_ssecs(const boost::int64_t ns) { return (ns / boost::int64_t(1e9)); }
	static boost::int64_t to_msecs(const boost::int64_t ns) { return (ns / boost::int64_t(1e6)); }
	static boost::int64_t to_usecs(const boost::int64_t ns) { return (ns / boost::int64_t(1e3)); }

	// these convert inputs to nanoseconds
	template<typename T>  static  boost::int64_t nsecs_from_ssecs(const T  s) { return ( s * boost::int64_t(1e9)); }
	template<typename T>  static  boost::int64_t nsecs_from_msecs(const T ms) { return (ms * boost::int64_t(1e6)); }
	template<typename T>  static  boost::int64_t nsecs_from_usecs(const T us) { return (us * boost::int64_t(1e3)); }
	template<typename T>  static  boost::int64_t nsecs_from_nsecs(const T ns) { return (ns                      ); }


	static void push_tick_rate(bool hres = true) {
		assert(!CLOCK_INITED);

		HIGHRES_MODE = hres;
		CLOCK_INITED = true;

		#if USE_NATIVE_WINDOWS_CLOCK
		// set the number of milliseconds between interrupts
		// NOTE: this is a *GLOBAL* setting, not per-process
		if (!HIGHRES_MODE) {
			timeBeginPeriod(1);
		}
		#endif
	}
	static void pop_tick_rate() {
		assert(CLOCK_INITED);

		#if USE_NATIVE_WINDOWS_CLOCK
		if (!HIGHRES_MODE) {
			timeEndPeriod(1);
		}
		#endif
	}



	#if USE_NATIVE_WINDOWS_CLOCK
	__FORCE_ALIGN_STACK__
	static boost::int64_t get_ticks_windows() {
		assert(CLOCK_INITED);

		if (HIGHRES_MODE) {
			// NOTE:
			//   SDL 1.2 by default does not use QueryPerformanceCounter
			//   SDL 2.0 does, but code does not seem aware of its issues
			//
			//   QPC is an interrupt-independent (unlike timeGetTime & co)
			//   *virtual* timer that runs at a "fixed" frequency which is
			//   derived from hardware, but can be *severely* affected by
			//   thermal drift (heavy CPU load will change the precision)
			//
			//   more accurately QPC is an *interface* to either the TSC
			//   or the HPET or the ACPI timer, MS claims "it should not
			//   matter which processor is called" and setting the thread
			//   affinity is only necessary in case QPC picks TSC (which
			//   can happen if ACPI BIOS code is broken)
			//
			//      const DWORD_PTR oldMask = SetThreadAffinityMask(::GetCurrentThread(), 0);
			//      QueryPerformanceCounter(...);
			//      SetThreadAffinityMask(::GetCurrentThread(), oldMask);
			//
			//   TSC is not invariant and completely unreliable on multi-core
			//   systems, but there exists an enhanced TSC on modern hardware
			//   which IS invariant (check CPUID 80000007H:EDX[8]) --> useful
			//   because reading TSC is much faster than an API call like QPC
			//
			//   the range of possible frequencies is extreme (KHz - GHz) and
			//   the hardware counter might only have a 32-bit register while
			//   QuadPart is a 64-bit integer --> no monotonicity guarantees
			//   (especially in combination with TSC if thread switches cores)
			LARGE_INTEGER tickFreq;
			LARGE_INTEGER currTick;

			if (!QueryPerformanceFrequency(&tickFreq))
				return (nsecs_from_msecs<boost::int64_t>(0));

			QueryPerformanceCounter(&currTick);

			// we want the raw tick (uncorrected for frequency)
			//
			// if clock ticks <freq> times per second, then the
			// total number of {milli,micro,nano}seconds elapsed
			// for any given tick is <tick> / <freq / resolution>
			// eg. if freq = 15000Hz and tick = 5000, then
			//
			//        secs = 5000 / (15000 / 1e0) =                    0.3333333
			//   millisecs = 5000 / (15000 / 1e3) = 5000 / 15.000000 =       333
			//   microsecs = 5000 / (15000 / 1e6) = 5000 /  0.015000 =    333333
			//    nanosecs = 5000 / (15000 / 1e9) = 5000 /  0.000015 = 333333333
			//
			if (tickFreq.QuadPart >= boost::int64_t(1e9)) return (from_nsecs<boost::uint64_t>(std::max(0.0, currTick.QuadPart / (tickFreq.QuadPart * 1e-9))));
			if (tickFreq.QuadPart >= boost::int64_t(1e6)) return (from_usecs<boost::uint64_t>(std::max(0.0, currTick.QuadPart / (tickFreq.QuadPart * 1e-6))));
			if (tickFreq.QuadPart >= boost::int64_t(1e3)) return (from_msecs<boost::uint64_t>(std::max(0.0, currTick.QuadPart / (tickFreq.QuadPart * 1e-3))));

			return (from_ssecs<boost::int64_t>(std::max(0LL, currTick.QuadPart)));
		}

		// timeGetTime is affected by time{Begin,End}Period whereas
		// GetTickCount is not ---> resolution of the former can be
		// configured but not for a specific process (they both read
		// from a shared counter that is updated by the system timer
		// interrupt)
		// it returns "the time elapsed since Windows was started"
		// (usually not a very large value so there is little risk
		// of overflowing)
		//
		// note: there is a GetTickCount64 but no timeGetTime64
		return (nsecs_from_msecs<boost::uint32_t>(timeGetTime()));
	}
	#endif

	static boost::int64_t get_ticks() {
		assert(CLOCK_INITED);

		#if USE_NATIVE_WINDOWS_CLOCK
		return (get_ticks_windows());
		#else
		// high_res_clock may just be an alias of this, no use to branch on HIGHRES_MODE
		// const chrono::system_clock::time_point cur_time = chrono::system_clock::now();
		const chrono::high_resolution_clock::time_point cur_time = chrono::high_resolution_clock::now();
		const chrono::nanoseconds run_time = chrono::duration_cast<chrono::nanoseconds>(cur_time.time_since_epoch());

		// number of ticks since chrono's epoch (note that there exist
		// very strong differences in behavior between the boost:: and
		// std:: versions with gcc 4.6.1)
		return (run_time.count());
		#endif
	}

	static const char* get_name() {
		assert(CLOCK_INITED);

		#if USE_NATIVE_WINDOWS_CLOCK
			if (HIGHRES_MODE) {
				return "win32::QueryPerformanceCounter";
			} else {
				return "win32::TimeGetTime";
			}
		#else
			#ifdef TIME_USING_LIBCHRONO
			return "boost::chrono::high_resolution_clock";
			#endif
			#ifdef TIME_USING_STDCHRONO
			return "std::chrono::high_resolution_clock";
			#endif
		#endif
	}
};



struct t_clock_tick {
public:
	t_clock_tick(): m_tick(0) {}

	// common-case constructor
	template<typename T> explicit t_clock_tick(const T msecs) {
		set_tick(t_hres_clock::nsecs_from_msecs(msecs));
	}

	void set_tick(boost::int64_t t) { m_tick = t; }
	boost::int64_t get_tick() const { return m_tick; }

	t_clock_tick& operator += (const t_clock_tick ct)       { m_tick += ct.get_tick(); return *this; }
	t_clock_tick& operator -= (const t_clock_tick ct)       { m_tick -= ct.get_tick(); return *this; }
	t_clock_tick& operator %= (const t_clock_tick ct)       { m_tick %= ct.get_tick(); return *this;    }
	t_clock_tick  operator -  (const t_clock_tick ct) const { return (get_clock_tick(m_tick - ct.get_tick())); }
	t_clock_tick  operator +  (const t_clock_tick ct) const { return (get_clock_tick(m_tick + ct.get_tick())); }
	t_clock_tick  operator %  (const t_clock_tick ct) const { return (get_clock_tick(m_tick % ct.get_tick())); }

	bool operator <  (const t_clock_tick ct) const { return (m_tick <  ct.get_tick()); }
	bool operator >  (const t_clock_tick ct) const { return (m_tick >  ct.get_tick()); }
	bool operator <= (const t_clock_tick ct) const { return (m_tick <= ct.get_tick()); }
	bool operator >= (const t_clock_tick ct) const { return (m_tick >= ct.get_tick()); }


	// short-hands for to_*secs_t
	boost::int64_t to_ssecs_i() const { return (to_ssecs_t<boost::int64_t>()); }
	boost::int64_t to_msecs_i() const { return (to_msecs_t<boost::int64_t>()); }
	boost::int64_t to_usecs_i() const { return (to_usecs_t<boost::int64_t>()); }
	boost::int64_t to_nsecs_i() const { return (to_nsecs_t<boost::int64_t>()); }

	float to_ssecs_f() const { return (to_ssecs_t<float>()); }
	float to_msecs_f() const { return (to_msecs_t<float>()); }
	float to_usecs_f() const { return (to_usecs_t<float>()); }
	float to_nsecs_f() const { return (to_nsecs_t<float>()); }

	template<typename T> T to_ssecs_t() const { return (t_hres_clock::to_ssecs<T>(m_tick)); }
	template<typename T> T to_msecs_t() const { return (t_hres_clock::to_msecs<T>(m_tick)); }
	template<typename T> T to_usecs_t() const { return (t_hres_clock::to_usecs<T>(m_tick)); }
	template<typename T> T to_nsecs_t() const { return (t_hres_clock::to_nsecs<T>(m_tick)); }


	static t_clock_tick get_curr_time(bool init_call = false) {
		assert(get_epoch_time() != 0 || init_call);
		return (get_clock_tick(t_hres_clock::get_ticks()));
	}
	static t_clock_tick get_init_time() {
		assert(get_epoch_time() != 0);
		return (get_clock_tick(get_epoch_time()));
	}
	static t_clock_tick get_diff_time() {
		return (get_curr_time() - get_init_time());
	}

	static void set_epoch_time(const t_clock_tick ct) {
		assert(get_epoch_time() == 0);
		get_epoch_time() = ct.get_tick();
		assert(get_epoch_time() != 0);
	}

	// initial time (arbitrary epoch, e.g. program start)
	// all other time-points will be larger than this if
	// the clock is monotonically increasing
	static boost::int64_t& get_epoch_time() {
		static boost::int64_t epoch_time = 0;
		return epoch_time;
	}

	static t_clock_tick time_from_nsecs(const boost::int64_t ns) { return (get_clock_tick(t_hres_clock::nsecs_from_nsecs(ns))); }
	static t_clock_tick time_from_usecs(const boost::int64_t us) { return (get_clock_tick(t_hres_clock::nsecs_from_usecs(us))); }
	static t_clock_tick time_from_msecs(const boost::int64_t ms) { return (get_clock_tick(t_hres_clock::nsecs_from_msecs(ms))); }
	static t_clock_tick time_from_ssecs(const boost::int64_t  s) { return (get_clock_tick(t_hres_clock::nsecs_from_ssecs( s))); }

private:
	// convert integer to t_clock_tick object (n is interpreted as number of nanoseconds)
	static t_clock_tick get_clock_tick(const boost::int64_t n) { t_clock_tick ct; ct.set_tick(n); return ct; }

private:
	boost::int64_t m_tick;
};



int main() {
	t_hres_clock::push_tick_rate();
	t_clock_tick::set_epoch_time(t_clock_tick::get_curr_time(true));

	{
		printf("[%s] clock=%s epoch_time=%ld\n", __FUNCTION__, t_hres_clock::get_name(), t_clock_tick::get_epoch_time());

		for (unsigned int n = 0; n < 64; n++) {
			printf("\tn=%u diff_time=%ld\n", n, (t_clock_tick::get_diff_time()).get_tick());
		}
	}

	t_hres_clock::pop_tick_rate();
	return 0;
}

