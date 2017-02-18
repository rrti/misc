#include <cassert>
#include <cmath>
#include <cstdio>
#include <cstddef>
#include <cstdlib>
#include <ctime>
#include <limits>

#include <list>
#include <vector>

// quadtree
#define NODE_CHILD_COUNT 4
// octree
// #define NODE_CHILD_COUNT 8
#define MAX_TREE_DEPTH 4

template<typename type> type unit_random() {
	return (random() / static_cast<float>(RAND_MAX));
}


template<typename t> struct t_tuple3 {
public:
	t_tuple3<t>(t _x = 0, t _y = 0, t _z = 0) {
		x() = _x; y() = _y; z() = _z;
	}

	t_tuple3<t>& operator = (const t_tuple3<t>& tuple) {
		x() = tuple.x();
		y() = tuple.y();
		z() = tuple.z();
		return *this;
	}

	t_tuple3<t> operator + (const t_tuple3<t>& tuple) const { return (t_tuple3<t>(x() + tuple.x(), y() + tuple.y(), z() + tuple.z())); }
	t_tuple3<t> operator - (const t_tuple3<t>& tuple) const { return (t_tuple3<t>(x() - tuple.x(), y() - tuple.y(), z() - tuple.z())); }

	t_tuple3<t>& operator += (const t_tuple3<t>& tuple) { x() += tuple.x(); y() += tuple.y(); z() += tuple.z(); return *this; }
	t_tuple3<t>& operator -= (const t_tuple3<t>& tuple) { x() -= tuple.x(); y() -= tuple.y(); z() -= tuple.z(); return *this; }

	t_tuple3<t> operator * (const t scalar) const {
		return (t_tuple3<t>(x() * scalar, y() * scalar, z() * scalar));
	}
	t_tuple3<t>& operator /= (const t scalar) {
		x() /= scalar;
		y() /= scalar;
		z() /= scalar;
		return *this;
	}


	t_tuple3<t>& normalize_ref() { return ((*this) = normalize()); }
	t_tuple3<t>& randomize_ref() { return ((*this) = randomize()); }

	t_tuple3<t> normalize() const {
		t_tuple3<t> r = *this;

		if (r.sq_magnitude() != 0) {
			r /= std::sqrt(r.sq_magnitude());
		}

		return r;
	}
	t_tuple3<t> randomize() const {
		return (t_tuple3<t>((unit_random<t>() * 2) - 1, (unit_random<t>() * 2) - 1, (unit_random<t>() * 2) - 1));
	}


	t_tuple3<t> xz() const { return (t_tuple3<t>(x(), 0, z())); }
	t_tuple3<t> yz() const { return (t_tuple3<t>(0, y(), z())); }
	t_tuple3<t> xy() const { return (t_tuple3<t>(x(), y(), 0)); }

	t inner_product(const t_tuple3<t>& tuple) const { return ((x() * tuple.x()) + (y() * tuple.y()) + (z() * tuple.z())); }
	t sq_magnitude() const { return (inner_product(*this)); }
	t    magnitude() const { return (std::sqrt(sq_magnitude())); }

	t  x() const { return m_values[0]; }
	t  y() const { return m_values[1]; }
	t  z() const { return m_values[2]; }
	t& x()       { return m_values[0]; }
	t& y()       { return m_values[1]; }
	t& z()       { return m_values[2]; }

private:
	t m_values[3];
};

typedef t_tuple3<       float> t_tuple3f;
typedef t_tuple3<unsigned int> t_tuple3ui;




struct t_ray {
	t_ray(const t_tuple3f& pos, const t_tuple3f& dir) {
		m_pos = pos;
		m_dir = dir;
	}

	const t_tuple3f& get_pos() const { return m_pos; }
	const t_tuple3f& get_dir() const { return m_dir; }

private:
	t_tuple3f m_pos;
	t_tuple3f m_dir;
};




struct t_tree_node {
public:
	t_tree_node(unsigned int idx, const t_tuple3f& mins, const t_tuple3f& maxs) {
		m_idx  = idx;
		m_mins = mins;
		m_maxs = maxs;
	}

	// these assume a 0-based indexing scheme, thus
	// <n> must be in [0, ..., 3] for get_child_idx
	unsigned int get_idx() const { return m_idx; }
	unsigned int get_parent_idx() const { return ((m_idx - 1) / NODE_CHILD_COUNT); }
	unsigned int get_child_idx(unsigned int n) const { return ((m_idx * NODE_CHILD_COUNT) + (n + 1)); }

	const t_tree_node* get_child(unsigned int idx) const { return m_children[idx]; }
	      t_tree_node* get_child(unsigned int idx)       { return m_children[idx]; }

	const t_tuple3f& get_mins() const { return m_mins; }
	const t_tuple3f& get_maxs() const { return m_maxs; }

	bool is_leaf() const { return (m_children.empty()); }
	bool contains_point(const t_tuple3f& point) const {
		if (point.x() < m_mins.x() || point.x() > m_maxs.x()) return false;
		if (point.y() < m_mins.y() || point.y() > m_maxs.y()) return false;
		if (point.z() < m_mins.z() || point.z() > m_maxs.z()) return false;
		return true;
	}

	const t_tree_node* find_node(const t_tuple3f& point) const {
		const t_tree_node* node = this;

		// if (!node->contains_point(point))
		//     return NULL;

		if (!node->is_leaf()) {
			// if not a leaf, there will be one and
			// only one child which contains <point>
			//
			for (unsigned int n = 0; n < NODE_CHILD_COUNT; n++) {
				if (m_children[n]->contains_point(point)) {
					node = m_children[n]; break;
				}
			}

			assert(node != this);
			node = node->find_node(point);
		}

		return node;
	}

	void split(std::vector<t_tree_node*>& nodes) {
		assert(is_leaf());
		m_children.resize(NODE_CHILD_COUNT, NULL);

		for (unsigned int n = 0; n < NODE_CHILD_COUNT; n++) {
			assert(get_child_idx(n) < nodes.size());
			assert(nodes[get_child_idx(n)] == NULL);

			t_tuple3f child_mins;
			t_tuple3f child_maxs;

			// n={0,1,2,3} --> lower-layer children ({quad,oc}tree), TL/TR/BL/BR
			// n={4,5,6,7} --> upper-layer children ({     oc}tree), TL/TR/BL/BR
			//
			//   a + (b-a)*0.5  ==  a + b*0.5 - a*0.5  ==  a*0.5 + b*0.5 ==  (a+b)*0.5
			//
			child_mins.x() = ((n & 1) == 0)? m_mins.x(): (m_mins.x() + m_maxs.x()) * 0.5f;
			child_maxs.x() = ((n & 1) == 0)? (m_mins.x() + m_maxs.x()) * 0.5f: m_maxs.x();
			child_mins.z() = ((n & 3)  < 2)? m_mins.z(): (m_mins.z() + m_maxs.z()) * 0.5f;
			child_maxs.z() = ((n & 3)  < 2)? (m_mins.z() + m_maxs.z()) * 0.5f: m_maxs.z();

			// [m_mins.y(), mid.y()][mid.y(), m_maxs.y()]
			// we only need to care about y's for octrees
			switch (NODE_CHILD_COUNT) {
				case 8: {
					child_mins.y() = (m_mins.y() + ((m_maxs.y() - m_mins.y()) * 0.5f * (1 - (n < 4))));
					child_maxs.y() = (m_maxs.y() - ((m_maxs.y() - m_mins.y()) * 0.5f * (0 + (n < 4))));
				} break;
				case 4: {
					child_mins.y() = m_mins.y();
					child_maxs.y() = m_maxs.y();
				} break;
			}

			assert(child_mins.x() >= m_mins.x() && child_maxs.x() <= m_maxs.x());
			assert(child_mins.y() >= m_mins.y() && child_maxs.y() <= m_maxs.y());
			assert(child_mins.z() >= m_mins.z() && child_maxs.z() <= m_maxs.z());

			nodes[get_child_idx(n)] = new t_tree_node(get_child_idx(n), child_mins, child_maxs);
			m_children[n] = nodes[get_child_idx(n)];
		}
	}

	float traverse(const t_ray& ray) const {
		const t_tuple3f& pos = ray.get_pos();
		const t_tuple3f& dir = ray.get_dir();

		assert(contains_point(pos));

		t_tuple3f boundary_dists;

		if (std::fabs(dir.x()) > 0.001f) {
			if (dir.x() >= 0.0f) {
				boundary_dists.x() = (m_maxs.x() - pos.x()) /  dir.x();
			} else {
				boundary_dists.x() = (pos.x() - m_mins.x()) / -dir.x();
			}
		} else {
			boundary_dists.x() = std::numeric_limits<float>::max();
		}

		if (std::fabs(dir.y()) > 0.001f) {
			if (dir.y() >= 0.0f) {
				boundary_dists.y() = (m_maxs.y() - pos.y()) /  dir.y();
			} else {
				boundary_dists.y() = (pos.y() - m_mins.y()) / -dir.y();
			}
		} else {
			boundary_dists.y() = std::numeric_limits<float>::max();
		}

		if (std::fabs(dir.z()) > 0.001f) {
			if (dir.z() >= 0.0f) {
				boundary_dists.z() = (m_maxs.z() - pos.z()) /  dir.z();
			} else {
				boundary_dists.z() = (pos.z() - m_mins.z()) / -dir.z();
			}
		} else {
			boundary_dists.z() = std::numeric_limits<float>::max();
		}

		return (std::min(boundary_dists.x(), std::min(boundary_dists.y(), boundary_dists.z())));
	}

	void print(unsigned int depth) const {
		if (depth == 0)
			printf("[%s]\n", __FUNCTION__);

		for (unsigned int n = 0; n < depth + 1; n++)
			printf("\t");

		// printf("idx=%u leaf=%d\n", m_idx, is_leaf());
		printf("idx=%u mins=<%.1f,%.1f,%.1f> maxs=<%.1f,%.1f,%.1f>\n", m_idx, m_mins.x(), m_mins.y(), m_mins.z(), m_maxs.x(), m_maxs.y(), m_maxs.z());

		if (is_leaf())
			return;

		for (unsigned int n = 0; n < NODE_CHILD_COUNT; n++) {
			assert(m_children[n] != NULL);
			m_children[n]->print(depth + 1);
		}
	}

private:
	std::vector<t_tree_node*> m_children;

	// position in t_tree::m_nodes
	unsigned int m_idx;

	t_tuple3f m_mins;
	t_tuple3f m_maxs;
};


struct t_tree {
public:
	t_tree(const t_tuple3f& root_mins, const t_tuple3f& root_maxs) {
		m_nodes.resize(1024, NULL);
		m_nodes[0] = new t_tree_node(0, root_mins, root_maxs);
	}

	~t_tree() {
		// we don't care about the tree structure here
		for (size_t n = 0; n < m_nodes.size(); n++) {
			delete m_nodes[n]; m_nodes[n] = NULL;
		}

		m_nodes.clear();
	}

	void traverse(t_ray ray, std::list<const t_tree_node*>& nodes) const {
		t_tuple3f pos = ray.get_pos();
		t_tuple3f dir = ray.get_dir();

		const t_tree_node* prev_node = NULL;

		while ((get_root())->contains_point(pos)) {
			// TODO: start search from neighbor of prev_node
			const t_tree_node* curr_node = find_node(pos);

			assert(curr_node != prev_node);
			// assert(curr_node->is_neighbor(prev_node));

			prev_node = curr_node;

			if (curr_node != NULL) {
				// push ray across node boundary
				pos += (dir * (curr_node->traverse(ray) + 0.01f));
				ray = t_ray(pos, dir);

				nodes.push_back(curr_node);
			}
		}
	}

	void print() const { (get_root())->print(0); }
	bool is_empty() const { return ((get_root())->is_leaf()); }

	const t_tree_node* get_root() const { return m_nodes[0]; }
	      t_tree_node* get_root()       { return m_nodes[0]; }

	const t_tree_node* find_node(const t_tuple3f& point) const {
		// note: this might *not* be true if point
		// lies outside volume spanned by the root
		if (!(get_root())->contains_point(point))
			return NULL;

		return (get_root())->find_node(point);
	}

	const std::vector<t_tree_node*>& get_nodes() const { return m_nodes; }
	      std::vector<t_tree_node*>& get_nodes()       { return m_nodes; }

	static void kill(t_tree* tree) { delete tree; }
	static t_tree* create_random_tree(const t_tuple3f& root_mins, const t_tuple3f& root_maxs) {
		t_tree* tree = new t_tree(root_mins, root_maxs);
		t_tree_node* node = tree->get_root();

		for (unsigned int n = 0; n < MAX_TREE_DEPTH; n++) {
			node->split(tree->get_nodes());
			node = node->get_child(random() % NODE_CHILD_COUNT);
		}

		return tree;
	}

private:
	// example for quadtrees
	//   a) 0-based scheme: child = (parent  )*4 + {1,2,3,4}
	//   b) 1-based scheme: child = (parent-1)*4 + {2,3,4,5}
	//
	//   a) produces 0,  1,2,3,4,   5,6,7,8 |  9,10,11,12 | 13,14,15,16 ...
	//   b) produces 1,  2,3,4,5,   6,7,8,9 | 10,11,12,13 | 14,15,16,17 ...
	//
	std::vector<t_tree_node*> m_nodes;
};



void gather_print_nodes(const t_ray& ray, t_tree* tree) {
	printf("[%s] pos=<%f,%f,%f> dir=<%f,%f,%f>\n", __FUNCTION__,
		(ray.get_pos()).x(), (ray.get_pos()).y(), (ray.get_pos()).z(),
		(ray.get_dir()).x(), (ray.get_dir()).y(), (ray.get_dir()).z());

	// nodes visited by ray before exiting volume
	std::list<const t_tree_node*> nodes;

	tree->print();
	tree->traverse(ray, nodes);

	{
		printf("[%s] #nodes=%lu\n", __FUNCTION__, nodes.size());

		for (auto it = nodes.begin(); it != nodes.end(); ++it) {
			printf("\tnode=%u\n", (*it)->get_idx());
		}
	}

	t_tree::kill(tree);
}

int main() {
	srandom(time(NULL));

	// start ray in middle of root's xyz-volume
	const t_tuple3f pos(512.0f, 512.0f, 512.0f);
	const t_tuple3f vec(  0.0f,   0.0f,   0.0f);
	const t_tuple3f dir((vec.randomize()).normalize());

	gather_print_nodes(t_ray(pos, dir), t_tree::create_random_tree(t_tuple3f(0.0f, 0.0f, 0.0f), t_tuple3f(1024.0f, 1024.0f, 1024.0f)));
	return 0;
}

