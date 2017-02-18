import sys

class Edge:
	def __init__(self, src_node, dst_node, capacity):
		self.src_node = src_node
		self.dst_node = dst_node
		self.capacity = capacity

	def __repr__(self):
		return ("[%c]--<%d>--[%c]" % (self.src_node, self.capacity, self.dst_node))

	def set_rev_edge(self, edge): self.redge = edge
	def get_rev_edge(self): return self.redge

class FlowNetwork:
	def __init__(self, nodes, edges):
		self.node_edges = {}
		self.edge_flows = {}

		for n in nodes: self.add_node(n)
		for e in edges: self.add_edge(e[0], e[1], e[2])

	def add_node(self, node):
		self.node_edges[node] = []

	def add_edge(self, u, v, w = 0):
		assert(u != v)

		edge_uv = Edge(u, v, w)
		edge_vu = Edge(v, u, 0)

		edge_uv.set_rev_edge(edge_vu)
		edge_vu.set_rev_edge(edge_uv)

		self.node_edges[u].append(edge_uv)
		self.node_edges[v].append(edge_vu)

		self.edge_flows[edge_uv] = 0
		self.edge_flows[edge_vu] = 0

	def get_outgoing_edges(self, v):
		return self.node_edges[v]


	def print_flow(self):
		print("[print_flow]")

		for edge in self.edge_flows:
			print("\tedge = %s :: flow = %d" % (edge, self.edge_flows[edge]))


	## find the shortest augmenting path with unit-flow
	def find_path(self, src_node, dst_node, path):
		if (src_node == dst_node):
			return path

		ret_path = None

		for edge in self.get_outgoing_edges(src_node):
			edge_res_capacity = edge.capacity - self.edge_flows[edge]
			path_edge_augment = (edge, edge_res_capacity)

			if (edge_res_capacity <= 0):
				continue
			if (path_edge_augment in path):
				continue

			## recursive BFS expansion
			ret_path = self.find_path(edge.dst_node, dst_node, path + [path_edge_augment])
			## recursive DFS expansion (does not guarantee shortest path)
			## ret_path = self.find_path(edge.dst_node, dst_node, [path_edge_augment] + path)

			if (ret_path != None):
				break

		return ret_path

	## compute max-flow using the Ford-Fulkerson method
	## TODO: compare and contrast with push-relabeling
	##
	## as long as there is a path from the source (start node)
	## to the sink (end node), with available capacity on all
	## edges in the path, send flow along one of these paths
	##
	def max_flow_ffk(self, source, sink):
		flow_path = self.find_path(source, sink, [])
		num_iters = 0

		while (flow_path != None):
			## find the edge along path with minimum residual capacity
			min_res_flow = min(edge_res_cap for edge, edge_res_cap in flow_path)

			## add and remove this residue to the flow
			for edge, edge_res_cap in flow_path:
				self.edge_flows[edge               ] += min_res_flow
				self.edge_flows[edge.get_rev_edge()] -= min_res_flow

			flow_path = self.find_path(source, sink, [])
			num_iters += 1

		return (sum(self.edge_flows[edge] for edge in self.get_outgoing_edges(source)))

def build_flow_network():
	nodes = ['s', 'o', 'p', 'q', 'r', 't']
	edges = [
		(nodes[0], nodes[1], 3),  (nodes[0], nodes[2], 3),
		(nodes[1], nodes[2], 2),  (nodes[1], nodes[3], 3),
		(nodes[2], nodes[4], 2),  (nodes[4], nodes[5], 3),
		(nodes[3], nodes[4], 4),  (nodes[3], nodes[5], 2),
	]

	return (FlowNetwork(nodes, edges))

def main(argc, argv):
	fn = build_flow_network()
	mf = fn.max_flow_ffk('s', 't')

	fn.print_flow()

	print("[%s] max_flow(s, t)=%d" % (__name__, mf))
	return 0

sys.exit(main(len(sys.argv), sys.argv))

