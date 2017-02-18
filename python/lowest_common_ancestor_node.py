class Node:
	def __init__(self, parent, childA, childB, depth):
		self.parent = parent
		self.childA = childA
		self.childB = childB
		self.depth = depth

	def GetParent(self): return self.parent
	def GetDepth(self): return self.depth

## NOTE:
##   assumes a linked tree-structure in which each node stores
##   a pointer to its parent as well as its depth (distance from
##   the root)
##
##   if we represent our tree in the usual array-form (such that
##   child = parent * 2 + {0, 1}, etc.) then given a difference
##   k in depth between A and B we can calculate the index of the
##   equal-depth ancestor in O(1) time via {A, B}->index / denom,
##   where denom = (2 ^ k) = (1 << k)
##
def LowestCommonAncestor(nodeA, nodeB):
	while (nodeA.GetDepth() > nodeB.GetDepth()):
		nodeA = nodeA.GetParent()
	while (nodeB.GetDepth() > nodeA.GetDepth()):
		nodeB = nodeB.GetParent()

	assert(nodeA != None)
	assert(nodeB != None)

	while (nodeA.GetParent() != nodeB.GetParent()):
		nodeA = nodeA.GetParent()
		nodeB = nodeB.GetParent()

	assert(nodeA == nodeB)
	return nodeA

