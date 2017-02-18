#include <map>
#include <vector>
#include <queue>

#include <string>
#include <cstdio>
 
struct t_node {
public:
	// construct a new leaf node for character c
	t_node(unsigned char c = '\0', unsigned int w = 0) {
		weight = w;
		value  = c;

		child0 = NULL;
		child1 = NULL;
	}

	// construct a new internal node with children
	t_node(const t_node* c0, const t_node* c1) {
		weight = c0->weight + c1->weight;
		value  = '\0';

		child0 = c0;
		child1 = c1;
	}

	t_node(const t_node& n) {
		weight = n.weight;
		value  = n.value;

		child0 = n.child0;
		child1 = n.child1;
	}

	~t_node() {
		child0 = NULL;
		child1 = NULL;
	}

	bool is_leaf() const { return (child0 == NULL || child1 == NULL); }

	bool operator < (const t_node& n) const { return (weight < n.weight); }
	bool operator > (const t_node& n) const { return (weight > n.weight); }

	void free() const {
		if (is_leaf())
			return;

		child0->free();
		child1->free();

		delete child0;
		delete child1;
	}

	void traverse(const std::string& code = "") const {
		if (code.empty())
			printf("[Symbol | Count | Code]\n");

		if (!is_leaf()) {
		    child0->traverse(code + '0');
		    child1->traverse(code + '1');
		} else {
			printf("    %c     %3u     %s\n", value, weight, code.c_str());
		}
	}

private:
	// number of characters represented by this node
	unsigned int weight;

	// for leafs, which character this node encodes
	unsigned char value;

	const t_node* child0;
	const t_node* child1;
};
 


bool build_symbol_table(const std::string& input_string, std::map<unsigned char, unsigned int>& symbol_table) {
	if (input_string.empty())
		return false;

	for (size_t i = 0; i < input_string.size(); i++) {
		if (symbol_table.find(input_string[i]) == symbol_table.end()) {
			symbol_table[input_string[i]] = 1;
		} else {
			symbol_table[input_string[i]] += 1;
		}
	}

	return true;
}

int main(int argc, char** argv) {
	std::map<unsigned char, unsigned int> symbol_table;
	std::priority_queue<t_node, std::vector<t_node>, std::greater<t_node> > queue;
	std::string input_string;

	if (argc > 1) {
		input_string = argv[1];
	} else {
		input_string += "AAAAAAAAAAAAAA" "BBB" "C";
		input_string += "DD" "E" "F" "GGG" "HHHH";
	}

	if (!build_symbol_table(input_string, symbol_table))
		return 0;

	// first push all leaf nodes into the queue
	for (std::map<unsigned char, unsigned int>::const_iterator it = symbol_table.begin(); it != symbol_table.end(); ++it) {
		queue.push(t_node(it->first, it->second));
	}

	// build a Huffman lossless encoding tree (using a
	// std::pqueue which by itself essentially already
	// is the frequency-sorted binary tree we want)
	//
	//   1) remove two smallest-by-weight nodes from queue
	//   2) make a new parent node for these two min-nodes
	//   3) insert new node back into queue (2-for-1)
	//   4) goto 1 until queue contains only one node
	//
	// note that if the weights of the top two nodes are
	// equal the symbol code will depend on the pop-order
	// (but this does not affect the amount of bits used)
	while (queue.size() > 1) {
		const t_node* child0 = new t_node(queue.top()); queue.pop();
		const t_node* child1 = new t_node(queue.top()); queue.pop();

		queue.push(t_node(child0, child1));
	}

	const t_node& root = queue.top();

	root.traverse();
	root.free();
	queue.pop();

	return 0;
}

