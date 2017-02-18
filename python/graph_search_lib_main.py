import sys
import random

from graph_search_lib_objs import Graph
from graph_search_lib_algs import FloydWarshallAllPairsShortestPaths
from graph_search_lib_algs import DijkstraAllPairsShortestPaths

def main(argc, argv):
	numNodes = ((argc == 2) and int(argv[1])) or 50

	## note: technically should use same graph (cloned) for both
	## searches, since the edge-weights are randomly initialized
	random.seed(12345); fwGraph = Graph(numNodes, False, True)
	random.seed(12345); dkGraph = Graph(numNodes, False, True)

	(t1, t2) = FloydWarshallAllPairsShortestPaths(fwGraph)
	(t3, t4) = DijkstraAllPairsShortestPaths(dkGraph)

	print("[main]")
	print("\tFloyd-Warshall APSP execution time, %d nodes: %.2fs" % (numNodes, t2 - t1))
	print("\tDijkstra APSP execution time, %d nodes: %.2fs" % (numNodes, t4 - t3))

sys.exit(main(len(sys.argv), sys.argv))

