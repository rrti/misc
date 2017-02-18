import time

from graph_search_lib_heap import *
from graph_search_lib_objs import Node

## note: because graph is complete we can reach any node
## j from any node i directly, so all path costs will be
## equal to that of edge (i, j) (which is randomly chosen)
def FloydWarshallAllPairsShortestPaths(graph):
	graph.initPathCosts(False)

	## don't count initialization time
	t1 = time.time()

	for k in xrange(graph.getNumNodes()):
		## for each pair (i, j) in range [0, n - 1]
		for i in xrange(graph.getNumNodes()):
			for j in xrange(graph.getNumNodes()):
				graph.setPathCost(i, j, min(graph.getPathCost(i, j), graph.getPathCost(i, k) + graph.getPathCost(k, j)))

	return (t1, time.time())



def DijkstraSearch(graph, sourceNode):
	## take the source vertex, set the weight
	## of the shortest path to 0 and push it
	## onto the priority queue
	nodes = graph.getNodes()
	i = sourceNode.getNum()
	graph.setPathCost(i, i, 0)

	priQueue = Heap(KeyComparator())
	priQueue.insertNode(0, i)

	while (not priQueue.isEmpty()):
		hNode = priQueue.deleteRoot()       ## heap-node h
		vNode = nodes[hNode.getVal()]       ## graph-node v
		j = vNode.getNum()
		jCost = graph.getPathCost(i, j)     ## cost of reaching v/j from s/i
		jNgbs = vNode.getNgbs()             ## all edges e = (v, u)

		## for each neighbor k of j
		for kNode in jNgbs:
			k = kNode.getNum()
			kCost = graph.getPathCost(i, k)
			edgeWeight = graph.getEdgeWeight(j, k)

			if ((edgeWeight + jCost) < kCost):
				graph.setPathCost(i, k, edgeWeight + jCost)

				## kNode.setCost(edgeWeight + jCost)
				## kNode.setParent(vNode)
				## priQueue.insertNode(kNode.getCost(), k)

				## note: we should not insert k if it is
				## already present in heap (ie. if there
				## exists a <cost, num> pair such that
				## num == k), since it means extra work
				priQueue.insertNode(graph.getPathCost(i, k), k)

def DijkstraAllPairsShortestPaths(graph):
	## compute all shortest paths in graph
	## from n to every other node, for all
	## source-nodes n
	graph.initPathCosts(True)

	## don't count initialization time
	t1 = time.time()

	for n in graph.getNodes():
		DijkstraSearch(graph, n)

	return (t1, time.time())

