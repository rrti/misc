TRAVERSAL_MODE_DFS = 0
TRAVERSAL_MODE_BFS = 1



class Node:
	def __init__(self, nodeID, parent):
		self.nodeID = nodeID
		self.parent = parent
		self.children = []
	def __repr__(self):
		return ("%s" % str(self.nodeID))

	def IsRoot(self): return (self.parent == None)
	def GetID(self): return self.nodeID
	def AddChild(self, n): self.children += [n]
	def GetChild(self, i): return self.children[i]
	def GetChildCount(self): return len(self.children)
	def Print(self, depth):
		print("%sid=%d" % ('\t' * (depth + 1), self.nodeID))
		for c in self.children:
			c.Print(depth + 1)



def ExpandNodeDFS(nodes, node):
	for i in xrange(node.GetChildCount()):
		childNode = node.GetChild(node.GetChildCount() - i - 1)
		nodes = [childNode] + nodes
	return nodes

def ExpandNodeBFS(nodes, node):
	for i in xrange(node.GetChildCount()):
		childNode = node.GetChild(i)
		nodes = nodes + [childNode]
	return nodes



def GraphTraverse(rootNode, nodeFunc):
	nodes = [rootNode]
	queue = []

	while (len(nodes) > 0):
		node = nodes[0]
		nodes = nodes[1: ]
		queue = queue + [node]

		## need to collect children in reverse order
		## to get the "proper" DFS traversal, but in
		## forward order for BFS
		##
		## DFS --> list is stack
		## BFS --> list is queue
		nodes = nodeFunc(nodes, node)

	return queue

## trees are simple graphs without cycles
root = Node(0, None)
root.AddChild(Node(1, root))
root.AddChild(Node(2, root))
root.GetChild(0).AddChild(Node(3, root.GetChild(0)))
root.GetChild(0).AddChild(Node(4, root.GetChild(0)))
root.GetChild(1).AddChild(Node(5, root.GetChild(1)))
root.GetChild(1).AddChild(Node(6, root.GetChild(1)))
## root.Print(0)

print("")
print("[DFS]: %s" % GraphTraverse(root, ExpandNodeDFS))
print("[BFS]: %s" % GraphTraverse(root, ExpandNodeBFS))

