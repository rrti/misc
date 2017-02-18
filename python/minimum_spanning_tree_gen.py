import sys
import random

class graph_node:
	def __init__(self, index = 0):
		self.node_index = index
		self.is_visited = False

	def get_index(self): return self.node_index
	def get_visited_state(self): return self.is_visited
	def set_visited_state(self, state): self.is_visited = state

class graph_edge:
	def __init__(self, weight = 0.0):
		self.indices = [-1, -1]
		self.weight = weight

	def get_weight(self): return self.weight
	def set_weight(self, weight): self.weight = weight
	def get_node_index(self, node_num): return self.indices[node_num]
	def set_node_index(self, node_num, index): self.indices[node_num] = index
	def get_node_ngbr_index(self, node, trigger_assert):
		if (node.get_index() == self.indices[0]): return self.indices[1]
		if (node.get_index() == self.indices[1]): return self.indices[0]
		if (trigger_assert): assert(False)
		return -1

class simple_graph:
	def __init__(self):
		self.nodes = []
		self.edges = []

	def is_empty(self): return (len(self.nodes) == 0)
	def assert_is_sane(self):
		for i in xrange(len(self.nodes)):
			assert(i == self.nodes[i].get_index())

	def set_node(self, node): self.nodes[node.get_index()] = node
	def add_node(self, node): self.nodes += [node]
	def add_edge(self, edge): self.edges += [edge]

	def set_nodes(self, nodes): self.nodes = nodes
	def set_edges(self, edges): self.edges = edges
	def get_nodes(self): return self.nodes
	def get_edges(self): return self.edges

	def get_random_node(self):
		assert(not(self.is_empty()))
		nodes = self.get_nodes()
		index = int(random.random() * len(nodes))
		assert(index < len(nodes))
		return nodes[index]

	def get_node_neighbors(self, node):
		edges = self.get_edges()
		nodes = self.get_nodes()
		ngbrs = []

		for edge in edges:
			## edges are bi-directional, so either end could match <node>
			## also store the edge in case of match since we need it later
			ngbr_idx = edge.get_node_ngbr_index(node, False)

			if (ngbr_idx == -1):
				continue

			ngbr = nodes[ngbr_idx]
			ngbrs += [(ngbr, edge)]

		return ngbrs

	def print_graph(self, label):
		nodes = self.get_nodes()
		edges = self.get_edges()

		print("[print_graph][%s] #nodes=%d #edges=%d" % (label, len(nodes), len(edges)))

		for edge in edges:
			assert(edge.get_node_index(0) == nodes[edge.get_node_index(0)].get_index())
			assert(edge.get_node_index(1) == nodes[edge.get_node_index(1)].get_index())
			print("\t[%d]--<%.2f>--[%d]" % (edge.get_node_index(0), edge.get_weight(), edge.get_node_index(1)))




def get_minimum_weight_unvisited_neighbor_edge(node_ngbrs, tree_edges):
	## graphs may not contain orphan nodes
	assert(len(node_ngbrs) != 0)

	min_ngbr_edge = None

	for ngbr_tuple in node_ngbrs:
		ngbr_node = ngbr_tuple[0]
		ngbr_edge = ngbr_tuple[1]

		if (ngbr_node.get_visited_state()):
			continue

		## note: this only works if edges from the
		## original graph are inserted into the MST
		if (tree_edges != None):
			assert(not(ngbr_edge in tree_edges))

		if (min_ngbr_edge == None or ngbr_edge.get_weight() < min_ngbr_edge.get_weight()):
			min_ngbr_edge = ngbr_edge

	return min_ngbr_edge



def build_regular_spanning_tree(graph):
	span_tree = simple_graph()
	node_list = [graph.get_random_node()]

	span_tree.set_nodes([None] * len(graph.get_nodes()))

	node_list[0].set_visited_state(True)
	span_tree.set_node(graph_node(node_list[0].get_index()))

	def node_expand_func_bfs(nodes, node): return nodes + [ngbr_node]
	def node_expand_func_dfs(nodes, node): return [ngbr_node] + nodes

	while (len(node_list) != 0):
		curr_node = node_list[0]
		node_list = node_list[1: ]
		ngbr_nodes = graph.get_node_neighbors(curr_node)

		for ngbr_node_tuple in ngbr_nodes:
			ngbr_node = ngbr_node_tuple[0]

			if (ngbr_node.get_visited_state()):
				continue

			assert(ngbr_node != curr_node)

			node_list = node_expand_func_bfs(node_list, ngbr_node)

			## fill spanning-tree with fresh unvisited nodes
			tree_node = graph_node(ngbr_node.get_index())
			tree_edge = graph_edge()

			tree_edge.set_node_index(0, curr_node.get_index())
			tree_edge.set_node_index(1, ngbr_node.get_index())

			span_tree.set_node(tree_node)
			span_tree.add_edge(tree_edge)

			ngbr_node.set_visited_state(True)

	return span_tree


##
## Prim:
##   1: Initialize a tree with a single vertex, chosen arbitrarily from the graph.
##   2: Grow the tree by one edge:
##      of the edges that connect the tree to vertices not yet in the
##      tree, find the minimum-weight edge, and transfer it to the tree.
##   3: Repeat step 2 (until all vertices are in the tree).
##
## Kruskal:
##   1: create a forest F (a set of trees), where each vertex in the graph is a separate tree
##   2: create a set S containing all the edges in the graph
##   3: while S is nonempty and F is not yet spanning
##      remove an edge with minimum weight from S
##      if that edge connects two different trees, then add it
##      to the forest, combining two trees into a single tree
##
## Prim's algorithm, slightly modified
def build_minimum_spanning_tree(graph):
	graph_nodes = graph.get_nodes()

	span_tree = simple_graph()
	span_tree.set_nodes([None] * len(graph_nodes))

	tree_nodes = span_tree.get_nodes()
	tree_edges = span_tree.get_edges()

	graph_nodes[0].set_visited_state(True)

	for node in graph_nodes:
		span_tree.set_node(graph_node(node.get_index()))
		node_ngbrs = graph.get_node_neighbors(node)

		## get the minimum-weight edge (that does not yet exist in
		## the spanning tree) from <node> to one of its neighbors;
		## can be None
		min_wgt_edge = get_minimum_weight_unvisited_neighbor_edge(node_ngbrs, tree_edges)

		## if no such edge, this node is already connected
		if (min_wgt_edge == None):
			continue

		min_wgt_ngbr = graph_nodes[min_wgt_edge.get_node_ngbr_index(node, True)]

		assert(min_wgt_ngbr != None)
		assert(min_wgt_ngbr != node)

		min_wgt_ngbr.set_visited_state(True)
		span_tree.add_edge(min_wgt_edge)

	## clone the edges after building the MST
	for i in xrange(len(tree_edges)):
		old_edge = tree_edges[i]
		new_edge = graph_edge(old_edge.get_weight())

		new_edge.set_node_index(0, old_edge.get_node_index(0))
		new_edge.set_node_index(1, old_edge.get_node_index(1))
		tree_edges[i] = new_edge

	return span_tree



def build_undirected_graph(graph_type = 0):
	graph = simple_graph()
	nodes = None
	edges = None

	if (graph_type == 0):
		## I-shape
		nodes = [graph_node(i) for i in xrange(2)]
		edges = [graph_edge(1.0 + int(random.random() * 100.0)) for i in xrange(1)]

		edges[0].set_node_index(0, nodes[0].get_index())
		edges[0].set_node_index(1, nodes[1].get_index())

	elif (graph_type == 1):
		## triangle (three nodes, three edges, one redundant)
		nodes = [graph_node(i) for i in xrange(3)]
		edges = [graph_edge(1.0 + int(random.random() * 100.0)) for i in xrange(3)]

		## edge = (n0, n1)
		## edges[0].set_weight(5.0)
		edges[0].set_node_index(0, nodes[0].get_index())
		edges[0].set_node_index(1, nodes[1].get_index())
		## edge = (n1, n2)
		## edges[1].set_weight(10.0)
		edges[1].set_node_index(0, nodes[1].get_index())
		edges[1].set_node_index(1, nodes[2].get_index())
		## edge = (n2, n0)
		## edges[2].set_weight(15.0)
		edges[2].set_node_index(0, nodes[2].get_index())
		edges[2].set_node_index(1, nodes[0].get_index())

	elif (graph_type == 2):
		## Z-shape (no redundant edges)
		nodes = [graph_node(i) for i in xrange(4)]
		edges = [graph_edge(1.0 + int(random.random() * 100.0)) for i in xrange(3)]

		## edge = (n0, n1)
		## edges[0].set_weight(5.0)
		edges[0].set_node_index(0, nodes[0].get_index())
		edges[0].set_node_index(1, nodes[1].get_index())
		## edge = (n1, n3)
		## edges[1].set_weight(15.0)
		edges[1].set_node_index(0, nodes[1].get_index())
		edges[1].set_node_index(1, nodes[3].get_index())
		## edge = (n3, n2)
		## edges[2].set_weight(5.0)
		edges[2].set_node_index(0, nodes[3].get_index())
		edges[2].set_node_index(1, nodes[2].get_index())

	elif (graph_type == 3):
		## quadrilateral (four nodes, five edges, two redundant)
		nodes = [graph_node(i) for i in xrange(4)]
		edges = [graph_edge(1.0 + int(random.random() * 100.0)) for i in xrange(5)]

		## edge = (n0, n1)
		edges[0].set_node_index(0, nodes[0].get_index())
		edges[0].set_node_index(1, nodes[1].get_index())
		## edge = (n2, n3)
		edges[1].set_node_index(0, nodes[2].get_index())
		edges[1].set_node_index(1, nodes[3].get_index())
		## edge = (n0, n2)
		edges[2].set_node_index(0, nodes[0].get_index())
		edges[2].set_node_index(1, nodes[2].get_index())
		## edge = (n1, n3)
		edges[3].set_node_index(0, nodes[1].get_index())
		edges[3].set_node_index(1, nodes[3].get_index())
		## edge = (n1, n2)
		edges[4].set_node_index(0, nodes[1].get_index())
		edges[4].set_node_index(1, nodes[2].get_index())

	for node in nodes: graph.add_node(node)
	for edge in edges: graph.add_edge(edge)

	graph.assert_is_sane()
	return graph

def main(argc, argv):
	gt = int(random.random() * 4)
	g0 = build_undirected_graph(gt)
	g1 = build_undirected_graph(gt)

	rst = build_regular_spanning_tree(g0)
	mst = build_minimum_spanning_tree(g1)

	g1.print_graph("cug(%d)" % gt)
	rst.print_graph("rst(%d)" % gt)
	mst.print_graph("mst(%d)" % gt)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

