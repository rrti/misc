import sys
import random

class t_node:
	def __init__(self, index, label):
		self.index = index
		self.label = label
	def __repr__(self):
		return ("<%d::%d>" % (self.index, self.label))

	def get_idx(self): return self.index
	def get_lbl(self): return self.label

class t_graph:
	def __init__(self, nodes, edges):
		self.nodes = nodes
		self.edges = {}

		## set outgoing edges for each node
		for e in edges:
			src_node_idx = e[0]
			tgt_node_idx = e[1]

			assert(nodes[src_node_idx].get_idx() == src_node_idx)
			assert(nodes[tgt_node_idx].get_idx() == tgt_node_idx)

			if (self.edges.has_key(src_node_idx)):
				self.edges[src_node_idx] += [tgt_node_idx]
			else:
				self.edges[src_node_idx] = [tgt_node_idx]


	def has_inc_edges(self, tgt_node_idx):
		for src_node_idx in self.edges:
			if (tgt_node_idx in self.edges[src_node_idx]):
				return True
		return False

	def has_out_edges(self, src_node_idx):
		if (self.edges.has_key(src_node_idx)):
			return (len(self.edges[src_node_idx]) > 0)
		return False


	## retrieve all incoming edges for target-node
	def get_inc_edges(self, tgt_node_idx):
		inc_edges = []

		if (True or self.has_inc_edges(tgt_node_idx)):
			for src_node_idx in self.edges:
				if (tgt_node_idx in self.edges[src_node_idx]):
					inc_edges += [(src_node_idx, tgt_node_idx)]

		return inc_edges

	## retrieve (a copy of) all outgoing edges for source-node
	def get_out_edges(self, src_node_idx):
		if (self.has_out_edges(src_node_idx)):
			return (self.edges[src_node_idx][:])
		return []


	## retrieve all nodes with no incoming edges
	def get_out_nodes(self):
		out_nodes = []

		for node in self.nodes:
			if (not (self.has_inc_edges(node.get_idx()))):
				out_nodes += [node]

		return out_nodes


	def topo_sort(self):
		## initialize sources as the set of all nodes with no incoming edges
		## note: this auto-sorts by increasing index due to use of dictionary
		source_nodes = {out_node.get_idx(): out_node for out_node in self.get_out_nodes()}
		sorted_nodes = []

		## remove first entry from S
		def remove_first_node(S):
			K = S.keys()
			k = K[0]
			n = S[k]
			del S[k]
			return n
		## remove random entry from S
		def remove_random_node(S):
			K = S.keys()
			k = K[random.randint(0, len(K) - 1)]
			n = S[k]
			del S[k]
			return n

		while (len(source_nodes) > 0):
			cur_src_node = remove_random_node(source_nodes)
			sorted_nodes += [cur_src_node]

			## get all the outgoing edges of <cur_src_node>
			node_out_edges = self.get_out_edges(cur_src_node.get_idx())

			for cur_tgt_node_idx in node_out_edges:
				## remove edge (src, tgt) from outgoing list of <src>
				self.edges[cur_src_node.get_idx()].remove(cur_tgt_node_idx)

				## if <src> has no more outgoing edges, delete its edge-list
				if (len(self.edges[cur_src_node.get_idx()]) == 0):
					del self.edges[cur_src_node.get_idx()]
					assert(not self.has_out_edges(cur_src_node.get_idx()))

				## if <tgt> no longer has incoming edges, make it a new source
				if (not self.has_inc_edges(cur_tgt_node_idx)):
					source_nodes[cur_tgt_node_idx] = self.nodes[cur_tgt_node_idx]

		## the graph should no longer contain *any* edges
		## otherwise there must have been a cycle present
		##
		## return the nodes in topologically-sorted order
		## there are many possible orderings based on the
		## scheme with which source-nodes are visited
		assert(len(self.edges) == 0)
		return sorted_nodes

def main(argc, argv):
	labels = [7, 5, 3,  11, 8,  2, 9, 10]
	idents = range(len(labels))

	nodes = [
		t_node(idents[i], labels[i]) for i in xrange(len(idents))
	]
	edges = [
		(idents[0], idents[3]), (idents[0], idents[4]), (idents[1], idents[3]),
		(idents[2], idents[4]), (idents[5], idents[7]),
		(idents[3], idents[5]), (idents[3], idents[6]), (idents[3], idents[7]), (idents[4], idents[6]),
	]

	graph = t_graph(nodes, edges)
	order = graph.topo_sort()

	print("[main]\n\tnodes(g)=%s\n\torder(g)=%s" % (nodes, order))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

