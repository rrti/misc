#include <cstdio>
#include <functional>
#include <vector>


template<typename val_type>
struct binop_add: std::binary_function<val_type, val_type, val_type> {
public:
	val_type operator ()(val_type a, val_type b) const { return (a + b); }
public:
	static constexpr val_type base_val() { return (val_type(0)); }
};

template<typename val_type>
struct binop_mul: std::binary_function<val_type, val_type, val_type> {
public:
	val_type operator ()(val_type a, val_type b) const { return (a * b); }
public:
	static constexpr val_type base_val() { return (val_type(1)); }
};


template<typename val_type, typename fun_type = std::binary_function<val_type, val_type, val_type> >
class t_fenwick_tree {
public:
	t_fenwick_tree(const std::vector<val_type>& data_values) {
		insert_values(data_values);
	}
	t_fenwick_tree(size_t num_values) {
		m_node_values.resize(1 + num_values, fun_type::base_val());
	}
	t_fenwick_tree(const t_fenwick_tree& tree) {
		m_node_values.assign((tree.get_node_values()).begin(), (tree.get_node_values()).end());
	}
	t_fenwick_tree(t_fenwick_tree&& tree) {
		m_node_values.swap(tree.get_node_values());
	}

private:
	// AND a given index with its own two's-complement
	// (clears all bits except for the rightmost 1-bit)
	//
	//   idx=4 --> dif=4&(~4+1)=4 --> prv=0,nxt=8
	//   idx=5 --> dif=5&(~5+1)=1 --> prv=4,nxt=6
	//   idx=6 --> dif=6&(~6+1)=2 --> prv=4,nxt=8
	//
	size_t get_dif_node_idx(size_t idx) const { return (idx & (~idx + 1)); }
	size_t get_nxt_node_idx(size_t idx) const { return (idx + get_dif_node_idx(idx)); }
	size_t get_prv_node_idx(size_t idx) const { return (idx - get_dif_node_idx(idx)); }

public:
	const std::vector<val_type>& get_node_values() const { return m_node_values; }
	      std::vector<val_type>& get_node_values()       { return m_node_values; }

	bool insert_value(size_t idx, val_type val) {
		if ((idx += 1) >= m_node_values.size())
			return false;

		while (idx < m_node_values.size()) {
			val_type& ref = m_node_values[idx];
			ref = fun_type()(val, ref);
			idx = get_nxt_node_idx(idx);
		}

		return true;
	}

	// calculate the prefix-value up to index <idx>
	// (tree can store prefixes for any binop-type)
	val_type get_prefix(size_t idx) const {
		val_type ret = fun_type::base_val();

		if ((idx += 1) >= m_node_values.size())
			return ret;

		while (idx > 0) {
			ret = fun_type()(ret, m_node_values[idx]);
			idx = get_prv_node_idx(idx);
		}

		return ret;
	}

private:
	// construct the tree, one element at a time
	// note: the root node ([0]) is never touched
	void insert_values(const std::vector<val_type>& data_values) {
		m_node_values.resize(1 + data_values.size(), fun_type::base_val());

		for (size_t n = 0; n < data_values.size(); n++) {
			insert_value(n, data_values[n]);
		}
	}

private:
	std::vector<val_type> m_node_values;
};



int main() {
	const std::vector<int> sum_data({3, 2, -1, 6, 5, 4, -3, 3, 7,  2,  3});
	const std::vector<int> mul_data({1, 2,  3, 4, 5, 6,  7, 8, 9, 10, 11});

	const t_fenwick_tree<int, binop_add<int> > sum_tree(sum_data); // additive prefixes
	const t_fenwick_tree<int, binop_mul<int> > mul_tree(mul_data); // multiplicative prefixes

	printf("[%s][1]\n", __func__);

	for (size_t i = 0; i < sum_data.size(); i++) {
		printf("\tpresum(%lu)={%d,%d}\n", i, sum_tree.get_prefix(i), mul_tree.get_prefix(i));
	}

	return 0;
}

