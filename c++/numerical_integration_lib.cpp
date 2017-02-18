#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstdlib>

#if 0
#include <fstream>
#include <iomanip>
#endif

#include <functional>
#include <utility>
#include <vector>

#define USE_EXPLICIT_EULER 0
#define USE_STORMER_VERLET 1


// time-varying, position-independent
// static float acc(float t) { return (5.0f * std::sin(t) * t * t);
static float acc(float t) { return (5.0f * std::cos(t)); }

template<typename type> static type mix(const type& v, const type& w, float a) { return (v * (1.0f - a) + w * (0.0f + a)); }


struct t_vec3f {
public:
	t_vec3f(float _x = 0.0f, float _y = 0.0f, float _z = 0.0f): m_x(_x), m_y(_y), m_z(_z) {}

	t_vec3f& operator += (const t_vec3f& v) {
		x() += v.x();
		y() += v.y();
		z() += v.z();
		return *this;
	}

	t_vec3f operator + (const t_vec3f& v) const { return (t_vec3f(x() + v.x(), y() + v.y(), z() + v.z())); }
	t_vec3f operator - (const t_vec3f& v) const { return (t_vec3f(x() - v.x(), y() - v.y(), z() - v.z())); }
	t_vec3f operator * (float s) const { return (t_vec3f(x() * s, y() * s, z() * s)); }
	t_vec3f operator / (float s) const { return (t_vec3f(x() / s, y() / s, z() / s)); }

	float& x()       { return m_x; }
	float& y()       { return m_y; }
	float& z()       { return m_z; }
	float  x() const { return m_x; }
	float  y() const { return m_y; }
	float  z() const { return m_z; }

private:
	float m_x;
	float m_y;
	float m_z;
};




struct t_deriv_state {
public:
	t_deriv_state operator + (const t_deriv_state& ds) const { return {v + ds.v, a + ds.a}; }

	t_deriv_state operator * (float s) const { return {v * s, a * s}; }
	t_deriv_state operator / (float s) const { return {v / s, a / s}; }

public:
	t_vec3f v;
	t_vec3f a;
};


struct t_physx_state {
public:
	t_physx_state operator + (const t_deriv_state& ds) const {
		return {x0 * 0.0f,  x1 + ds.v, v1 + ds.a};
	}

	t_deriv_state eval_deriv(const t_deriv_state& ds, float ct, float dt) const {
		// Euler-step state from s(t) to s(t) + ds*dt; also sample acceleration at ct+dt
		// u = s + ds*dt = (s.x, s.v) + (ds.v, ds.a)*dt = (s.x + ds.v*dt, s.v + ds.a*dt)
		const t_physx_state psu = (*this) + ds * dt;
		const t_deriv_state dsu = {psu.v1, sample_acc(ct + dt)};
		return dsu;
	}

	t_vec3f sample_acc(float ct) const {
		return {acc(ct), 0.0f, 0.0f};
	}

	// velocity is explicit for Euler and Runge-Kutta, but
	// implicit ((x2 - x0) / (dt * 2.0) or (x2 - x1) / dt)
	// for Verlet
	const t_vec3f& pos() const { return x1; }
	const t_vec3f& vel() const { return v1; }

public:
	t_vec3f x0; // pos(t - 1); Verlet only
	t_vec3f x1; // pos(t); all integrators
	t_vec3f v1; // vel(t); Euler and Runge-Kutta
};

typedef std::pair<t_physx_state, float> t_data_pair;




class t_base_integrator {
public:
	// <ct> is current time (corresponding to state <ps>), <dt> is step-size
	virtual void init (t_physx_state& ps, float ct, float dt) const { (void) ps; (void) ct; (void) dt; }
	virtual void stepi(t_physx_state& ps, float ct, float dt) const { ps = step(ps, ct, dt); }

	virtual t_physx_state step(const t_physx_state& ps, float ct, float dt) const = 0;
};




class t_euler_integrator: public t_base_integrator {
public:
	t_physx_state step(const t_physx_state& ps, float ct, float dt) const override {
		t_physx_state ss;

		#if (USE_EXPLICIT_EULER == 1)
		// explicit forward Euler (pos first, vel second)
		ss.x1 = ps.x1 + (ps.v1                         * dt);
		ss.v1 = ps.v1 + (ps.sample_acc(ct + dt * 0.0f) * dt);
		#else
		// [semi-]implicit forward Euler (vel first, pos second)
		// dt*0.5 is stable, dt*0 drifts right, dt*1 drifts left
		ss.v1 = (ps.v1 + ps.sample_acc(ct + dt * 0.0f) * dt);
		ss.x1 = (ps.x1 + ss.v1                         * dt);
		#endif

		return ss;
	}
};


class t_verlet_integrator: public t_base_integrator {
public:
	t_physx_state step(const t_physx_state& ps, float ct, float dt) const override {
		t_physx_state ss;

		const t_vec3f a1 = ps.sample_acc(ct + dt * 0.0f);
		// const t_vec3f a2 = ps.sample_acc(ct + dt * 1.0f);

		const t_vec3f x1 = ps.x0 + (ps.v1 * dt) + (a1 * dt * dt * 0.5f);
		const t_vec3f x2 = ps.x1 * 2.0f - ps.x0 + (a1 * dt * dt       );

		#if (USE_STORMER_VERLET == 1)
		// Stormer-Verlet; derive velocity from positions x2 and x1
		// executes the first Verlet-integration step when ct = 0.0
		ss.x0 = mix(ps.x0, ps.x1, ct != 0.0f);
		ss.x1 = mix(   x1,    x2, ct != 0.0f);
		ss.v1 = (ss.x1 - ps.x0) / (dt * 2.0f);
		#else
		// Velocity-Verlet; requires a2 = acc(ct + dt*1.0)
		ss.x1 = ps.x1 + ps.v1 * dt + (a1     ) * dt * dt * 0.5f;
		ss.v1 =         ps.v1      + (a1 + a2) * dt *      0.5f;
		#endif

		return ss;
	}
};


class t_rkutta_integrator: public t_base_integrator {
public:
	t_deriv_state sample_state_deriv(const t_physx_state& ps, const t_deriv_state& ds,  float ct, float dt) const {
		return (ps.eval_deriv(ds, ct, dt));
	}

	t_deriv_state blend_derivs(const t_deriv_state& a, const t_deriv_state& b, const t_deriv_state& c, const t_deriv_state& d) const {
		constexpr float w0 = 2.00000000f;
		constexpr float w1 = 0.16666667f;
		const t_deriv_state mid = (b       + c) * w0;
		const t_deriv_state sum = (a + mid + d) * w1;
		return sum;
	}

	t_physx_state step_euler(const t_physx_state& ps, float ct, float dt) const {
		const t_deriv_state ds0;
		const t_deriv_state ds1 = sample_state_deriv(ps, ds0, ct, dt * 0.0f);
		return (ps + ds1 * dt);
	}

	t_physx_state step(const t_physx_state& ps, float ct, float dt) const override {
		// return (step_euler(ps, ct, dt));

		const t_deriv_state  ds0;
		const t_deriv_state& ds1 = sample_state_deriv(ps, ds0, ct, dt * 0.0f);
		const t_deriv_state& ds2 = sample_state_deriv(ps, ds1, ct, dt * 0.5f);
		const t_deriv_state& ds3 = sample_state_deriv(ps, ds2, ct, dt * 0.5f);
		const t_deriv_state& ds4 = sample_state_deriv(ps, ds3, ct, dt * 1.0f);

		return (ps + blend_derivs(ds1, ds2, ds3, ds4) * dt);
	}
};




void integrate(t_base_integrator& bi, t_physx_state& ps, std::vector<t_data_pair>& v, float st, float dt) {
	bi.init(ps, st, dt);

	// initial state, pre-integration
	v[0].first  = ps;
	v[0].second = st;

	for (size_t j = 1; j < v.size(); j++) {
		// keep a copy of the state after each integration step
		v[j].second = st + (j - 1) * dt;
		v[j].first  = (ps = bi.step(ps, v[j].second, dt));
	}
}

void integrate_euler(std::vector<t_data_pair>& v, float st, float dt) {
	t_physx_state ps;
	t_euler_integrator ei;

	integrate(ei, ps, v, st, dt);
}

void integrate_verlet(std::vector<t_data_pair>& v, float st, float dt) {
	t_physx_state ps;
	t_verlet_integrator vi;

	integrate(vi, ps, v, st, dt);
}

void integrate_rkutta(std::vector<t_data_pair>& v, float st, float dt) {
	t_physx_state ps;
	t_rkutta_integrator ri;

	integrate(ri, ps, v, st, dt);
}


void run(size_t num_steps, float start_time, float delta_time) {
	const std::function<void(std::vector<t_data_pair>&, float, float)> funcs[3] = {integrate_euler, integrate_verlet, integrate_rkutta};
	const std::string names[3 * 2] = {"euler_p.dat", "euler_v.dat", "verlet_p.dat", "verlet_v.dat", "rkutta_p.dat", "rkutta_v.dat"};

	std::vector<t_data_pair> sdata[3];
	#if 0
	std::ofstream files[3 * 2];
	#endif

	for (size_t n = 0; n < 3; n++) {
		sdata[n].resize(1 + num_steps);
		funcs[n](sdata[n], start_time, delta_time);
	}

	for (size_t n = 0; n < 3; n++) {
		#if 0
		files[n * 2 + 0].open(names[n * 2 + 0], std::ios::out);
		files[n * 2 + 0] << std::setprecision(10);
		#else
		FILE* pos_file = fopen(names[n * 2 + 0].c_str(), "w");
		FILE* vel_file = fopen(names[n * 2 + 1].c_str(), "w");
		#endif

		for (size_t k = 0; k < sdata[n].size(); k++) {
			const t_data_pair& dp = sdata[n][k];
			const t_physx_state& ps = dp.first;

			// put position on x-axis, time on y-axis
			#if 0
			files[n * 2 + 0] << (ps.x1.x()) << "\t";
			files[n * 2 + 0] << (dp.second) << "\n";
			#else
			fprintf(pos_file, "%f\t%f\n", ps.x1.x(), dp.second);
			fprintf(vel_file, "%f\t%f\n", ps.v1.x(), dp.second);
			#endif
		}

		#if 0
		files[n].flush();
		files[n].close();
		#else
		fclose(pos_file);
		fclose(vel_file);
		#endif
	}
}


int main(int argc, char** argv) {
	const size_t iter_count = (argc > 1)? std::atoi(argv[1]): 1000;
	const  float start_time = (argc > 2)? std::atof(argv[2]): 0.0f;
	const  float delta_time = (argc > 3)? std::atof(argv[3]): 0.1f;

	run(iter_count, start_time, delta_time);
	return 0;
}

