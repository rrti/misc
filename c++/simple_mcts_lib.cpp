#include <cassert>
#include <cstddef>
#include <vector>

#include <chrono>
#include <random>

#define USE_CPP11_RNG 1

namespace mcts_lib {
	#if (USE_CPP11_RNG == 1)
	template<typename t_uint = uint64_t, typename t_real = double> struct t_uniform_rng {
	public:
		t_uniform_rng(uint64_t seed = 0) {
			if (seed == 0) {
				const std::chrono::system_clock::time_point cur_time = std::chrono::system_clock::now();
				const std::chrono::nanoseconds run_time = std::chrono::duration_cast<std::chrono::nanoseconds>(cur_time.time_since_epoch());

				m_sampler.seed(run_time.count());
			} else {
				m_sampler.seed(seed);
			}
		}

		t_uint next_uint() { return (m_int_distrib(m_sampler)); }
		t_real next_real() { return (m_real_distrib(m_sampler)); }

	private:
		std::mt19937 m_sampler;

		std::uniform_int_distribution<t_uint> m_int_distrib;
		std::uniform_real_distribution<t_real> m_real_distrib;
	};

	#else

	// this should never be used (statistically weak, no thread-safety, etc)
	template<typename t_uint = uint64_t, typename t_real = double> struct t_uniform_rng {
	public:
		t_uniform_rng(uint64_t seed = 0) {
			if (seed == 0) {
				srandom(time(nullptr));
			} else {
				srandom(seed);
			}
		}

		t_uint next_uint() { return (random()); }
		t_real next_real() { return ((next_uint() * t_real(1)) / std::numeric_limits<t_uint>::max()); }
	};
	#endif

	typedef float t_real_type;
	typedef t_uniform_rng<uint32_t, t_real_type> t_rng_type;

	struct t_abstract_game_move;
	struct t_abstract_game_state {
	public:
		virtual ~t_abstract_game_state() {}

		// heuristically evaluate <this> state
		virtual t_real_type get_heuristic_score() const = 0;

		// generate all legal moves available in <this> state
		virtual bool gen_legal_moves(std::vector<t_abstract_game_move*>& moves) const = 0;

		// returns a copy of <this> with <move> applied
		virtual t_abstract_game_state* do_copy_move(t_abstract_game_move* move) const = 0;
	};


	struct t_abstract_game_move {
	public:
		t_abstract_game_move(): m_executed(false) { }
		virtual ~t_abstract_game_move() {}

		// apply <this> to <state>
		t_abstract_game_state* execute(t_abstract_game_state* state) {
			assert(!is_executed());
			set_executed();
			return (state->do_copy_move(this));
		}

		void set_executed() { m_executed = true; }
		bool is_executed() const { return m_executed; }

	protected:
		bool m_executed;
	};



	struct t_tree_node {
	public:
		t_tree_node(const t_tree_node* parent, t_abstract_game_state* state) {
			m_state = state;

			m_depth = (parent != nullptr)? (parent->get_depth() + 1): 0;
			m_count = 0;
			m_value = t_real_type(0);

			if (state->gen_legal_moves(m_moves)) {
				m_nodes.reserve(m_moves.size());
			}
		}

		~t_tree_node() {
			delete m_state; m_state = nullptr;

			for (size_t n = 0; n < get_num_nodes(); n++) {
				delete m_nodes[n]; m_nodes[n] = nullptr;
			}

			for (size_t n = 0; n < get_num_moves(); n++) {
				delete m_moves[n]; m_moves[n] = nullptr;
			}

			m_nodes.clear();
			m_moves.clear();
		}


		t_tree_node* do_new_random_move(t_rng_type* rng) {
			assert(!is_leaf());
			assert(!is_fully_expanded());

			t_abstract_game_move* move = m_moves[rng->next_uint() % get_num_moves()];

			// we want a previously *unexecuted* move
			while (move->is_executed())
				move = m_moves[rng->next_uint() % get_num_moves()];

			// move.execute creates a clone of <self.state>
			// this is passed into a new tree-node instance
			// which becomes a child of <this>
			return (new t_tree_node(this, move->execute(m_state)));
		}


		const t_tree_node* get_child(size_t idx) const { assert(idx < get_num_nodes()); return m_nodes[idx]; }
		      t_tree_node* get_child(size_t idx)       { assert(idx < get_num_nodes()); return m_nodes[idx]; }


		t_tree_node* get_random_child(t_rng_type* rng) {
			assert(!m_nodes.empty());
			return m_nodes[rng->next_uint() % get_num_nodes()];
		}

		t_tree_node* get_max_value_child(t_rng_type* rng, t_real_type eps = t_real_type(0.05)) {
			assert(!m_nodes.empty());

			// decide whether to explore or exploit
			if (rng->next_real() < eps)
				return (get_random_child(rng));

			t_tree_node* n = m_nodes[0];

			for (size_t i = 1; i < get_num_nodes(); i++) {
				t_tree_node* c = m_nodes[i];

				if (c->get_value() > n->get_value()) {
					n = c;
				}
			}

			return n;
		}

		t_tree_node* add_child(t_tree_node* node) {
			m_nodes.push_back(node);
			return node;
		}


		t_real_type add_value(t_real_type value) { return (m_value += value); }
		size_t add_count(size_t count) { return (m_count += count); }

		t_real_type get_value() const { return m_value; }
		t_real_type eval_state() const { return (m_state->get_heuristic_score()); }

		size_t get_num_nodes() const { return (m_nodes.size()); }
		size_t get_num_moves() const { return (m_moves.size()); }
		size_t get_depth() const { return m_depth; }
		size_t get_count() const { return m_count; }


		bool is_fully_expanded() const {
			assert(!is_leaf());

			size_t moves_executed = 0;

			for (size_t n = 0; n < get_num_moves(); n++)
				moves_executed += m_moves[n]->is_executed();

			// true if all moves for this state have been exhausted
			// (at that point we have as many children as there were
			// moves)
			// TODO: can be cached
			return (moves_executed == get_num_moves());
		}

		// if no legal moves are available, this node
		// has no (and can not have) children either
		bool is_leaf() const { return (m_moves.empty()); }

		// inner-node is any node that has exhausted
		// its (non-empty) set of possible moves, i.e.
		// has been fully explored
		bool is_inner() const { return ((!is_leaf()) && is_fully_expanded()); }

	private:
		t_abstract_game_state* m_state;

		// child nodes, added on-the-fly by executing moves (remains empty if none)
		std::vector<t_tree_node*> m_nodes;

		// legal moves available in this game-state (empty for draws and gameovers)
		std::vector<t_abstract_game_move*> m_moves;

		// depth in the tree at which this node lives
		size_t m_depth;
		// number of times this node has been visited
		size_t m_count;

		// accumulated heuristic value
		t_real_type m_value;
	};



	struct t_tree_search {
	public:
		t_tree_search(size_t num_evals, size_t max_depth) {
			m_num_evals = num_evals;
			m_max_depth = max_depth;
		}

		void execute(t_tree_node* node) {
			if (m_vals.size() != m_num_evals) {
				m_vals.clear();
				m_vals.resize(m_num_evals, t_real_type(0));
			}

			// store the value of each search-execution
			for (size_t n = 0; n < m_num_evals; n++) {
				m_vals[n] = eval_node(node, 0, m_max_depth);
			}
		}

		const std::vector<t_real_type>& get_values() const { return m_vals; }
		      std::vector<t_real_type>& get_values()       { return m_vals; }

	private:
		t_real_type eval_node(t_tree_node* node, size_t cur_depth, size_t max_depth) {
			t_real_type value = t_real_type(0);

			if (node->is_inner()) {
				// node was fully explored previously and now has children;
				// descend further down the tree until we hit a fringe node
				// (using eg. epsilon-greedy selection)
				//
				value = eval_node(node->get_max_value_child(&m_uniform_rng), cur_depth + 1, max_depth);
			} else {
				// node is still not fully explored, choose a random move
				// and add it as a new (i.e. not previously visited) child
				// then take a random walk down to a leaf-node and evaluate
				// it; the length of the walk is relative to current depth
				//
				value = rand_path(node, cur_depth, cur_depth + max_depth);
			}

			node->add_value(value);
			node->add_count(1);
			return value;
		}

		t_real_type rand_path(t_tree_node* node, size_t cur_depth, size_t max_depth) {
			if ((!node->is_leaf()) && (cur_depth < max_depth))
				return (rand_path(node->add_child(node->do_new_random_move(&m_uniform_rng)), cur_depth + 1, max_depth));

			return (node->eval_state());
		}

	private:
		size_t m_num_evals;
		size_t m_max_depth;

		t_rng_type m_uniform_rng;

		std::vector<t_real_type> m_vals;
	};
};



int main(int argc, char** argv) {
	const size_t num_evals = (argc > 1)? std::atoi(argv[1]): 100;
	const size_t max_depth = (argc > 2)? std::atoi(argv[2]):  10;

	mcts_lib::t_tree_node tree(nullptr, nullptr);
	mcts_lib::t_tree_search search(num_evals, max_depth);

	search.execute(&tree);
	return 0;
}

