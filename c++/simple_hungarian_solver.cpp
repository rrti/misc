#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <limits>
#include <vector>

#define BASE_INDEX 1



template<typename type>
void solve_ap_rec(
	const std::vector< std::vector<type> >& costs,
	std::vector<size_t>& agnts,
	std::vector<size_t>& tasks,
	std::vector<size_t>& pairs,
	type cur_cost,
	type* min_cost
) {
	bool recursed = false;

	for (size_t a = BASE_INDEX; a < costs.size(); a++) {
		if (agnts[a] != -1lu)
			continue;

		for (size_t t = BASE_INDEX; t < costs[a].size(); t++) {
			if (tasks[t] != -1lu)
				continue;

			agnts[a] = t;
			tasks[t] = a;

			// only recurse if this partial path has not already been ruled out
			if ((cur_cost + costs[a][t]) <= *min_cost)
				solve_ap_rec(costs,  agnts, tasks, pairs,  cur_cost + costs[a][t], min_cost);

			agnts[a] = -1lu;
			tasks[t] = -1lu;

			recursed = true;
		}
	}

	if (recursed)
		return;
	if (cur_cost >= *min_cost)
		return;

	// no more available assignments
	//
	// this means we have a completed one possible
	// assignment-path and are at the bottom of the
	// recursion
	*min_cost = cur_cost;

	// store the assignment
	pairs = agnts;
	// std::copy(agnts.begin(), agnts.end(), pairs.begin());
}

template<typename type>
std::vector<size_t> solve_ap_rec(const std::vector< std::vector<type> >& costs) {
	assert(costs.size() == costs[0].size());

	// minimize over all possible assignment-paths recursively
	// single root-leaf traversal has O(N ^ 3) complexity, but
	// the entire tree has exponentially many leafs
	//
	// A = (a1 a2 a3)
	// T = (t1 t2 t3)
	// C = MIN(
	//   SUM(COST(a1,t1), COST(a2,t2), COST(a3,t3)),
	//   SUM(COST(a1,t1), COST(a2,t3), COST(a3,t2)),
	//   SUM(COST(a1,t2), COST(a2,t1), COST(a3,t3)),
	//   SUM(COST(a1,t2), COST(a2,t3), COST(a3,t1)),
	//   SUM(COST(a1,t3), COST(a2,t2), COST(a3,t1)),
	//   SUM(COST(a1,t3), COST(a2,t1), COST(a3,t2)),
	// )
	//
	type min_cost = std::numeric_limits<type>::max();

	std::vector<size_t> agnts(costs.size(), -1lu);
	std::vector<size_t> tasks(costs.size(), -1lu);
	std::vector<size_t> pairs(costs.size(), -1lu);

	solve_ap_rec(costs,  agnts, tasks, pairs,  0, &min_cost);
	return pairs;
}



// enhanced Hungarian method to solve the Assignment Problem
// originally by Andrei Lopatin, shorter and runs in O(N ^ 3)
// note: requires zero-augmented square matrices as input
//
template<typename type>
std::vector<size_t> solve_ap_ehm(const std::vector< std::vector<type> >& costs) {
	const size_t num_pairs = costs.size();

	std::vector<type> row_costs(num_pairs, 0);
	std::vector<type> col_costs(num_pairs, 0);

	std::vector<size_t> task_agent_tbl(num_pairs, 0);
	std::vector<size_t> agent_task_tbl(num_pairs, 0);
	std::vector<size_t> task_subst_tbl(num_pairs, 0);

	const type MAX_VAL = std::numeric_limits<type>::max();

	for (size_t agt_idx = BASE_INDEX; agt_idx < num_pairs; agt_idx++) {
		// make sure the matrix is square (|T| = |A|)
		assert(costs[agt_idx].size() == costs.size());

		std::vector<type> cur_task_costs(num_pairs, MAX_VAL);
		std::vector<type> ass_task_flags(num_pairs,   false);

		size_t min_tsk = 0;
		size_t opt_tsk = 0;

		task_agent_tbl[0] = agt_idx;

		do {
			// agent that can do <min_tsk> for lowest cost
			const size_t min_agt = task_agent_tbl[min_tsk];

			type cst_dif = MAX_VAL;

			// mark task as assigned
			ass_task_flags[min_tsk] = true;

			// see if an improvement can be made
			for (size_t tsk_idx = BASE_INDEX; tsk_idx < num_pairs; tsk_idx++) {
				if (ass_task_flags[tsk_idx])
					continue;

				assert(min_agt < num_pairs);

				const type tsk_cost = costs[min_agt][tsk_idx];
				const type cur_cost = tsk_cost - row_costs[min_agt] - col_costs[tsk_idx];

				if (cur_cost < cur_task_costs[tsk_idx]) {
					// task <tsk_idx> can be done for cost <cur_cost>
					cur_task_costs[tsk_idx] = cur_cost;
					task_subst_tbl[tsk_idx] = min_tsk;
				}
				if (cur_task_costs[tsk_idx] < cst_dif) {
					// keep track of the new cheapest task
					cst_dif = cur_task_costs[tsk_idx];
					opt_tsk = tsk_idx;
				}
			}

			min_tsk = opt_tsk;
			opt_tsk = 0;

			assert(cst_dif != MAX_VAL);
			assert(min_tsk < num_pairs);

			for (size_t tsk_idx = 0; tsk_idx < num_pairs; tsk_idx++) {
				if (ass_task_flags[tsk_idx]) {
					row_costs[ task_agent_tbl[tsk_idx] ] += cst_dif;
					col_costs[                tsk_idx  ] -= cst_dif;
				} else {
					cur_task_costs[tsk_idx] -= cst_dif;
				}
			}
		} while (task_agent_tbl[min_tsk] != 0);


		do {
			task_agent_tbl[min_tsk] = task_agent_tbl[ task_subst_tbl[min_tsk] ];
			               min_tsk  =                 task_subst_tbl[min_tsk]  ;
		} while (min_tsk != 0);
	}

	// set the final (agent, task) pairings
	for (size_t tsk_idx = 0; tsk_idx < num_pairs; tsk_idx++) {
		agent_task_tbl[ task_agent_tbl[tsk_idx] ] = tsk_idx;
	}

	return agent_task_tbl;
}



template<typename type>
std::vector< std::vector<type> > gen_cost_matrix(size_t size, size_t vmax) {
	std::vector< std::vector<type> > costs(size);

	for (size_t n = 0; n < size; n++) {
		costs[n].resize(size, type(0));

		if (n == 0)
			continue;

		for (size_t k = 1; k < size; k++) {
			costs[n][k] = random() % vmax;
		}
	}

	return costs;
}

template<typename type>
type calc_cost(const std::vector< std::vector<type> >& costs, const std::vector<size_t>& pairs) {
	type cost = 0;

	for (size_t agt_idx = BASE_INDEX; agt_idx < costs.size(); agt_idx++) {
		#if 0
		printf("agent=%lu task=%lu cost=%d\n", agt_idx, pairs[agt_idx], costs[agt_idx][ pairs[agt_idx] ]);
		#endif
		cost += costs[agt_idx][ pairs[agt_idx] ];
	}

	return cost;
}



int main(int argc, char** argv) {
	srandom((argc <= 1)? time(NULL): atoi(argv[1]));

	#if 0
	const std::vector< std::vector<int> > costs = {
		{ 0, 0, 0, 0},
		{ 0, 2, 3, 3},
		{ 0, 3, 2, 3},
		{ 0, 3, 3, 2},
	};
	#else
	// const std::vector< std::vector<int> > costs = gen_cost_matrix<int>(6, 20);
	const std::vector< std::vector<int> > costs = {
		{0,  0,  0,  0,  0,  0},
		{0, 15, 16, 17, 18, 19}, 
		{0,  1,  1,  3,  1, 16}, 
		{0,  2,  8,  8,  6, 17},
		{0,  2,  2,  2,  8, 18},
		{0,  4,  6,  7,  8, 19},
	};
	#endif

	const std::vector<size_t>& pairs_ehm = solve_ap_ehm(costs);
	const std::vector<size_t>& pairs_rec = solve_ap_rec(costs);

	printf("[%s] cost=%d (%d)\n", __FUNCTION__, calc_cost(costs, pairs_ehm), calc_cost(costs, pairs_rec));
	return 0;
}

