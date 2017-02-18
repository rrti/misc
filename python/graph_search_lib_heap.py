class HeapNode:
	def __init__(self, key, val):
		self.key = key
		self.val = val

	def getKey(self): return self.key
	def getVal(self): return self.val

class KeyComparator:
	## compares HeapNode objects by integer key
	def compare(self, node1, node2):
		i1 = node1.key
		i2 = node2.key

		if (i1 < i2): return -1
		elif (i1 == i2): return 0
		else: return 1


class Heap:
	## simple heap data-structure implementation
	## using a vector to store the nodes, which
	## forms the basis of our priority queue for
	## Dijkstra's algorithm
	def __init__(self, keyComp):
		self.nodes = [None]      ## stores HeapNode objects (dummy 1st element)
		self.lastNodeIdx = 0     ## index of last HeapNode
		self.keyComp = keyComp   ## compares the node keys

	def getRoot(self):
		return (self.nodes[1])

	def deleteRoot(self):
		node = None

		if (self.lastNodeIdx == 1):
			node = self.getRoot()
			self.lastNodeIdx = 0
			self.nodes.pop(1)

		elif (self.lastNodeIdx > 1):
			node = self.getRoot()

			## swap root and last node, then remove last
			self.swap(1, self.lastNodeIdx);
			self.nodes.pop(self.lastNodeIdx);
			self.lastNodeIdx -= 1
			self.downHeap(1);

		return node


	def insertNode(self, key, val):
		node = HeapNode(key, val)

		## add new last element to heap
		self.nodes.append(node)
		self.lastNodeIdx += 1

		self.upHeap(self.lastNodeIdx);


	## perform up-bubbling of node
	## at child-index <idxC>
	def upHeap(self, idxC):
		while (idxC > 1):
			idxP = (idxC / 2)

			if (self.keyComp.compare(self.getNode(idxP), self.getNode(idxC)) <= 0):
				break

			self.swap(idxP, idxC)
			idxC = idxP

	## perform down-bubbling of node
	## at parent-index <idxP>
	def downHeap(self, idxP):
		while (self.hasLeftChild(idxP) or self.hasRightChild(idxP)):
			if (not self.hasRightChild(idxP)):
				idxC = (idxP * 2)
			elif (self.keyComp.compare(self.getNode(idxP * 2), self.getNode(idxP * 2 + 1)) <= 0):
				idxC = (idxP * 2)
			else:
				idxC = (idxP * 2) + 1

			if (self.keyComp.compare(self.getNode(idxC), self.getNode(idxP)) < 0):
				self.swap(idxP, idxC)
				idxP = idxC
			else:
				break


	## swaps values of nodes (used in bubbling
	## operations) at indices <idx1> and <idx2>
	def swap(self, idx1, idx2):
		n1 = self.nodes[idx1]
		n2 = self.nodes[idx2]

		self.nodes[idx1] = n2
		self.nodes[idx2] = n1

	## returns HeapNode object at index idx
	def getNode(self, idx):
		return self.nodes[idx]

	## returns index of HeapNode with key <key>
	## or -1 if no such HeapNode object exists
	def findNode(self, key):
		return self.findNode(self, key, 1)
	def findNode(self, key, idx):
		if (self.nodes[idx].key < key):
			if (self.hasLeftChild(idx)):
				return self.findNode(self, key, (idx * 2))
			else:
				return -1
		elif (self.nodes[idx].key == key):
			return idx
		else:
			if (self.hasRightChild(idx)):
				return self.findNode(self, key, (idx * 2) + 1)
			else:
				return -1

	## returns if node at index <idx> has a left child
	def hasLeftChild(self, idx):
		return ((idx * 2) <= (self.lastNodeIdx))

	## returns if node at index <idx> has a right child
	def hasRightChild(self, idx):
		return (((idx * 2) + 1) <= (self.lastNodeIdx))


	def isEmpty(self):
		return (self.lastNodeIdx == 0)
	def getSize(self):
		return (self.lastNodeIdx)

