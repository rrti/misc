#include <cassert>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <vector>

// #define BINHEAP_DEBUG_LOGIC
// #define BINHEAP_DEBUG_PRINT

#ifdef BINHEAP_DEBUG_PRINT
#include <string>
#endif

#define NUM_HEAP_NODES  25000u
#define MAX_HEAP_VALUE 100000u



#if 0
#define NODE_CMP_EQ(a, b) (a->operator==(b)) // equivalent to ((*a) == (*b))
#define NODE_CMP_LT(a, b) (a->operator< (b)) // equivalent to ((*a) <  (*b))
#define NODE_CMP_LE(a, b) (a->operator<=(b)) // equivalent to ((*a) <= (*b))
#define NODE_CMP_GT(a, b) (a->operator> (b)) // equivalent to ((*a) >  (*b))
#define NODE_CMP_GE(a, b) (a->operator>=(b)) // equivalent to ((*a) >= (*b))
#endif

#define NODE_CMP_LT(a, b) (a->get_heap_priority() <  b->get_heap_priority())
#define NODE_CMP_LE(a, b) (a->get_heap_priority() <= b->get_heap_priority())
#define NODE_CMP_GT(a, b) (a->get_heap_priority() >  b->get_heap_priority())



struct t_node {
public:
	t_node(size_t id): m_id(id) {}
	virtual ~t_node() {}

	virtual size_t get_id() const { return m_id; }

	virtual void set_heap_position(size_t p) = 0;
	virtual void set_heap_priority( float p) = 0;

	virtual size_t get_heap_position() const = 0;
	virtual  float get_heap_priority() const = 0;

	#if 0
	virtual bool operator == (const t_node&) const = 0;
	virtual bool operator <  (const t_node&) const = 0;
	virtual bool operator <= (const t_node&) const = 0;
	virtual bool operator >  (const t_node&) const = 0;
	virtual bool operator >= (const t_node&) const = 0;
	#endif

protected:
	size_t m_id;
};

struct t_ext_node: public t_node {
public:
	t_ext_node(size_t id): t_node(id) {
		m_heap_position = -1u;
		m_heap_priority = -1.0f;
	}

	void set_heap_position(size_t p) { m_heap_position = p; }
	void set_heap_priority( float p) { m_heap_priority = p; }

	size_t get_heap_position() const { return m_heap_position; }
	 float get_heap_priority() const { return m_heap_priority; }

private:
	size_t m_heap_position;
	 float m_heap_priority;
};



template<class node_type> class t_binary_heap {
public:
	t_binary_heap(        ) {   clear( ); }
	t_binary_heap(size_t n) { reserve(n); }
	~t_binary_heap(       ) {   clear( ); }

	// interface functions
	void push(node_type n) {
		#ifdef BINHEAP_DEBUG_LOGIC
		check_heap_property(0);
		#endif

		assert(m_cur_idx < m_nodes.size());

		// park new node at first free spot
		m_nodes[m_cur_idx] = n;
		m_nodes[m_cur_idx]->set_heap_position(m_cur_idx);

		if (m_cur_idx == m_max_idx)
			m_nodes.resize(m_nodes.size() * 2);

		m_cur_idx += 1;
		m_max_idx = m_nodes.size() - 1;

		// move new node up if necessary
		if (size() > 1)
			inc_heap(m_cur_idx - 1);

		#ifdef BINHEAP_DEBUG_LOGIC
		check_heap_property(0);
		#endif
	}

	void pop() {
		#ifdef BINHEAP_DEBUG_LOGIC
		check_heap_property(0);
		#endif

		assert(m_cur_idx <= m_max_idx);

		// exchange root and last node
		if (size() > 1)
			swap_nodes(0, m_cur_idx - 1);

		// former position of last node is now free
		m_cur_idx -= 1;
		assert(m_cur_idx <= m_max_idx);

		// kill old root
		m_nodes[m_cur_idx]->set_heap_position(-1u);
		m_nodes[m_cur_idx] = NULL;

		// move new root (former last node) down if necessary
		if (size() > 1)
			dec_heap(0);

		#ifdef BINHEAP_DEBUG_LOGIC
		check_heap_property(0);
		#endif
	}

	node_type top() {
		assert(!empty());
		return m_nodes[0];
	}


	// utility functions
	bool empty() const { return (size() == 0); }
	bool full() const { return (size() >= capacity()); }

	size_t size() const { return (m_cur_idx); }
	size_t capacity() const { return (m_max_idx + 1); }

	void clear() {
		m_nodes.clear();

		m_cur_idx =  0;
		m_max_idx = -1u;
	}

	void reserve(size_t size) {
		m_nodes.clear();
		m_nodes.resize(size, NULL);

		m_cur_idx =        0;
		m_max_idx = size - 1;
	}

	// acts like reserve(), but without re-allocating
	void reset() {
		assert(!m_nodes.empty());

		m_cur_idx =                  0;
		m_max_idx = m_nodes.size() - 1;
	}

	// call this if outside code has modified a node's priority
	void resort(const node_type n) {
		assert(n != NULL);
		assert(valid_idx(n->get_heap_position()));

		const size_t n_idx = n->get_heap_position();
		const size_t n_rel = is_sorted(n_idx);

		switch (n_rel) {
			case 0: {
				// bail if <n> does not actually break the heap-property
				return;
			} break;
			case 1: {
				// parent of <n> is larger, move <n> further up
				inc_heap(n_idx);
			} break;
			case 2: {
				// parent of <n> is smaller, move <n> further down
				dec_heap(n_idx);
			} break;
		}

		#ifdef BINHEAP_DEBUG_LOGIC
		check_heap_property(0);
		#endif
	}


	void check_heap_property(size_t idx) const {
		#ifdef BINHEAP_DEBUG_LOGIC
		if (valid_idx(idx)) {
			assert(is_sorted(idx) == 0);

			check_heap_property(l_child_idx(idx));
			check_heap_property(r_child_idx(idx));
		}
		#endif
	}

	#ifdef BINHEAP_DEBUG_PRINT
	void debug_print(size_t idx, size_t calls, const std::string& tabs) const {
		if (calls == 0)
			return;

		if (valid_idx(idx)) {
			debug_print(r_child_idx(idx), calls - 1, tabs + "\t");
			printf("%shi=%u :: hp=%.2f\n", tabs.c_str(), m_nodes[idx]->get_heap_position(), m_nodes[idx]->get_heap_priority());
			debug_print(l_child_idx(idx), calls - 1, tabs + "\t");
		}
	}
	#endif

private:
	size_t  parent_idx(size_t n_idx) const { return ((n_idx  - 1) >> 1); }
	size_t l_child_idx(size_t n_idx) const { return ((n_idx << 1)  + 1); }
	size_t r_child_idx(size_t n_idx) const { return ((n_idx << 1)  + 2); }

	// tests if <idx> belongs to the tree built over [0, m_cur_idx)
	bool valid_idx(size_t idx) const { return (idx < m_cur_idx); }

	// test whether the node at <nidx> is in the right place
	// parent must have smaller / equal value, children must
	// have larger / equal value
	//
	size_t is_sorted(size_t n_idx) const {
		const size_t   p_idx =  parent_idx(n_idx);
		const size_t l_c_idx = l_child_idx(n_idx);
		const size_t r_c_idx = r_child_idx(n_idx);

		if (n_idx != 0) {
			// check if parent > node
			assert(valid_idx(p_idx));
			assert(m_nodes[p_idx] != NULL);
			assert(m_nodes[n_idx] != NULL);

			if (NODE_CMP_GT(m_nodes[p_idx], m_nodes[n_idx])) {
				return 1;
			}
		}

		if (valid_idx(l_c_idx)) {
			// check if l_child < node
			assert(m_nodes[l_c_idx] != NULL);
			assert(m_nodes[  n_idx] != NULL);

			if (NODE_CMP_LT(m_nodes[l_c_idx], m_nodes[n_idx])) {
				return 2;
			}
		}
		if (valid_idx(r_c_idx)) {
			// check if r_child < node
			assert(m_nodes[r_c_idx] != NULL);
			assert(m_nodes[  n_idx] != NULL);

			if (NODE_CMP_LT(m_nodes[r_c_idx], m_nodes[n_idx])) {
				return 2;
			}
		}

		return 0;
	}


	void inc_heap(size_t c_idx) {
		size_t p_idx = 0;

		while (c_idx >= 1) {
			p_idx = parent_idx(c_idx);

			assert(valid_idx(p_idx));
			assert(valid_idx(c_idx));

			if (NODE_CMP_LE(m_nodes[p_idx], m_nodes[c_idx]))
				break;

			swap_nodes(p_idx, c_idx);

			// move up one level
			c_idx = p_idx;
		}
	}

	void dec_heap(size_t p_idx) {
		size_t   c_idx = -1u;
		size_t l_c_idx = l_child_idx(p_idx);
		size_t r_c_idx = r_child_idx(p_idx);

		while (valid_idx(l_c_idx) || valid_idx(r_c_idx)) {
			if (!valid_idx(r_c_idx)) {
				// node only has a left child
				c_idx = l_c_idx;
			} else {
				if (NODE_CMP_LE(m_nodes[l_c_idx], m_nodes[r_c_idx])) {
					// left child is smaller or equal to right child
					// according to the node total ordering, pick it
					c_idx = l_c_idx;
				} else {
					// pick the right child
					c_idx = r_c_idx;
				}
			}

			// parent is smaller (according to the
			// node total ordering) than the child
			if (NODE_CMP_LE(m_nodes[p_idx], m_nodes[c_idx]))
				break;

			swap_nodes(p_idx, c_idx);

			  p_idx = c_idx;
			l_c_idx = l_child_idx(p_idx);
			r_c_idx = r_child_idx(p_idx);
		}
	}


	void swap_nodes(size_t idx1, size_t idx2) {
		assert(idx1 != idx2);
		assert(idx1 < m_cur_idx);
		assert(idx2 < m_cur_idx);

		node_type n1 = m_nodes[idx1];
		node_type n2 = m_nodes[idx2];

		assert(n1 != n2);
		assert(n1->get_heap_position() == idx1);
		assert(n2->get_heap_position() == idx2);

		m_nodes[idx1] = n2;
		m_nodes[idx2] = n1;

		n2->set_heap_position(idx1);
		n1->set_heap_position(idx2);
	}

private:
	size_t m_cur_idx; // index of first free (unused) slot
	size_t m_max_idx; // index of last free (unused) slot

	std::vector<node_type> m_nodes;
};



int main() {
	srandom(time(NULL));

	t_binary_heap<t_node*>* heap = new t_binary_heap<t_node*>(NUM_HEAP_NODES);
	t_node* node = NULL;

	for (unsigned int n = 0; n < NUM_HEAP_NODES; n++) {
		node = new t_ext_node(n);
		node->set_heap_priority(random() % MAX_HEAP_VALUE);
		heap->push(node);
	}

	while (!heap->empty()) {
		node = heap->top();
		heap->pop();
		printf("[%s] node=%p (identity=%lu priority=%f)\n", __FUNCTION__, node, node->get_id(), node->get_heap_priority());
		delete node;
	}

	delete heap;
	return 0;
}
