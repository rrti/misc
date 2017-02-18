import random

class Node:
	def __init__(self, num):
		self.num = num
		self.ngbs = []

	def addNgb(self, n): self.ngbs.append(n)
	def getNgbs(self): return self.ngbs
	def getNum(self): return self.num

	## set/get the cost of shortest weighted path
	## from source to this vertex (where source is
	## variable with every call to Dijkstra())
	"""
	def setCost(self, c): self.cost = c
	def getCost(self): return self.cost
	"""
	## set/get the parent of this vertex in the
	## shortest weighted path from source to self
	## found so far (where source is variable with
	## every call to Dijkstra())
	"""
	def setParent(self, p): self.p = p
	def getParent(self): return self.p

	def reset(self):
		self.c = infCost
		self.p = None
	"""

class Edge:
	def __init__(self, n1, n2, w, directed):
		self.n1 = n1
		self.n2 = n2
		self.w = w
		## if directed is true, then edge only
		## runs from n1 to n2 (not vice versa),
		## otherwise node order is unimportant
		## note: currently this is always false
		self.directed = directed

	def getNodes(self): return (self.n1, self.n2)
	def getWeight(self): return self.w

class Graph:
	def __init__(self, numNodes, directed, complete):
		self.numNodes = numNodes
		self.infCost = 99999999
		self.nodes = [Node(i) for i in xrange(numNodes)]
		self.edges = []

		## fill the edge- and pathcost-matrix with
		## dummy values so we are guaranteed that
		## m[i][j] and m[j][i] both exist at the
		## same time (important when we want to
		## create undirected graphs)
		self.edgeMatrix = [[None for i in xrange(numNodes)] for j in xrange(numNodes)]
		self.pathCosts = [[self.infCost for i in xrange(numNodes)] for j in xrange(numNodes)]

		self.addEdges(directed, complete)

	## prints the weights of each direct
	## connection (edge) between i and j
	def __repr__(self):
		s = "matrix of weights of direct connections (edges) between i and j:\n\t"

		for j in xrange(self.numNodes):
			s += (str("j=%d" % j) + "\t\t")

		s += '\n'

		for i in xrange(self.numNodes):
			s += (str("i=%d" % i) + '\t')

			for j in xrange(self.numNodes):
				if (not self.edgeExists(i, j)):
					## no direct connection between i and j
					## (means weight of edge is infinite)
					s += ("INF" + "\t\t")
				else:
					s += (str(self.getEdgeWeight(i, j)) + "\t\t")

			s += '\n'

		return s

	## prints the values of each shortest path
	## from i to j (INF if none found); call this
	## after running Floyd-Warshall or Dijkstra
	def showPathCosts(self):
		s = "matrix of costs of paths from i to j:\n\t"

		for j in xrange(self.numNodes):
			s += (str("j=%d" % j) + "\t\t")

		s += '\n'

		for i in xrange(self.numNodes):
			s += (str("i=%d" % i) + '\t')

			for j in xrange(self.numNodes):
				if ((self.pathCosts[i][j]) == self.infCost):
					## no path from i to j was found, infinite cost
					s += ("INF" + "\t\t")
				else:
					s += (str(self.pathCosts[i][j]) + "\t\t")

			s += '\n'

		return s


	## initialize the edges and their weights (randomly)
	## note: fills the edge matrix and edge list with
	## references to the same heap-allocated objects
	def addEdges(self, directed, complete):
		for i in xrange(self.numNodes):
			for j in xrange(self.numNodes):
				if (i == j):
					## always an edge from i to i with weight 0
					## note: i not added to its own neighbors-list
					e = Edge(self.nodes[i], self.nodes[i], 0, False)

					self.edgeMatrix[i][i] = e
					self.edges.append(e)
				else:
					if (complete):
						if (self.edgeExists(i, j)):
							continue

						## graph contains all possible (bi-directional)
						## edges (i, j) == (j, i); in directed graphs
						## i and j would have to be doubly-connected
						## (but not necessarily with equal weights)
						w = int(random.random() * 100) + 1
						e = Edge(self.nodes[i], self.nodes[j], w, False)

						self.edgeMatrix[i][j] = e
						self.edgeMatrix[j][i] = e
						self.nodes[i].addNgb(self.nodes[j])
						self.nodes[j].addNgb(self.nodes[i])
						self.edges.append(e)
					else:
						## graph contains randomly chosen subset of all
						## possible (bi-directional) edges
						if (random.random() > 0.5):
							w = int(random.random() * 100) + 1
							e = Edge(self.nodes[i], self.nodes[j], w, False)

							self.edgeMatrix[i][j] = e
							self.nodes[i].addNgb(self.nodes[j])
							self.edges.append(e)


	## initialize cost of each shortest path
	## from i to j to that of edgeCost(i, j)
	## or to infinity, call this before running
	## Floyd-Warshall or Dijkstra
	def initPathCosts(self, dijkstra):
		for i in xrange(self.numNodes):
			for j in xrange(self.numNodes):
				if (not self.edgeExists(i, j) or dijkstra):
					self.pathCosts[i][j] = self.infCost
				else:
					self.pathCosts[i][j] = self.getEdgeWeight(i, j)


	def getNumNodes(self): return self.numNodes
	def getNodes(self): return self.nodes

	## get and set the cost of the path from i to j
	def getPathCost(self, i, j): return self.pathCosts[i][j]
	def setPathCost(self, i, j, c): self.pathCosts[i][j] = c
	## get the weight of edge (i, j)
	def getEdgeWeight(self, i, j): return (self.edgeMatrix[i][j].getWeight())
	def edgeExists(self, i, j): return (self.edgeMatrix[i][j] != None)



"""
def initGraph(graph, numNodes):
	## graph is a boolean adjacency matrix
	## edge/path costs are integer matrices
	edgeCosts = []
	pathCosts = []

	## initialize the edges (randomly)
	for i in xrange(0, numNodes):
		graph.append([])

		for j in xrange(0, numNodes):
			if (i == j):
				## always an edge from i to i
				graph[i].append(True)
			else:
				## direct connection from i to j
				graph[i].append(random.random() > 0.5)

	## initialize the edge costs (randomly)
	for i in xrange(0, numNodes):
		edgeCosts.append([])
		pathCosts.append([])

		for j in xrange(0, numNodes):
			if (i == j):
				## edgeCost(i, i) = 0
				edgeCosts[i].append(0)
			else:
				if (graph[i][j]):
					## edgeCost(i, j) = random value in xrange [1, 100]
					c = int(random.random() * 100) + 1
					edgeCosts[i].append(c)
				else:
					## no direct connection between i and j, edgeCost(i, j) = INF
					edgeCosts[i].append(99999999)

			## initialize cost of each shortest path
			## from i to j to that of edgeCost(i, j)
			pathCosts[i].append(edgeCosts[i][j])
"""

