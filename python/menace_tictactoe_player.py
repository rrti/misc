## implements MENACE (Matchbox-Educable Noughts And Crosses Engine)
##
## this algorithm is an ingenious (and deceptively simple) early
## example of what would later be known as reinforcement learning

import sys
import time
import random

class Grid:
	def __init__(self):
		self.size = 3
		self.numMoves = 0
		self.cells = [' '] * 9

	def Reset(self):
		self.numMoves = 0
		for i in range(self.size * self.size):
			self.cells[i] = ' '

	def FromHash(self, h):
		self.numMoves = int(h[0])
		for i in range(self.size * self.size):
			self.cells[i] = h[i + 1]

	def Clone(self):
		g = Grid()
		g.numMoves = self.numMoves

		for idx in range(self.size * self.size):
			g.cells[idx] = self.cells[idx]

		return g

	## pretty-print the grid
	def __repr__(self):
		s = "\n"
		r = "---+---+---"

		for row in range(self.size):
			if (row > 0):
				s += ('\n' + r + '\n')

			for col in range(self.size):
				i = (row * self.size) + col
				s += (' '+ self.cells[i] + ' ')

				if (col < self.size - 1):
					s += '|'

		s += '\n'
		return s

	## note: rotation-independent hash here?
	def __hash__(self):
		s = "";
		for i in range(self.size * self.size):
			s += self.cells[i]

		return "%s%s" % (self.numMoves, s)

	## check two grids for equality
	## (does not compare numMoves)
	def __eq__(self, other):
		for i in range(self.size * self.size):
			if (self.cells[i] != other.cells[i]):
				return False

		return True


	def IsGameOver(self):
		if (self.numMoves < 5):
			## no winner yet
			return False
		else:
			## rows
			b1 = (self.cells[0] != ' ' and self.cells[0] == self.cells[1] and self.cells[1] == self.cells[2])
			b2 = (self.cells[3] != ' ' and self.cells[3] == self.cells[4] and self.cells[4] == self.cells[5])
			b3 = (self.cells[6] != ' ' and self.cells[6] == self.cells[7] and self.cells[7] == self.cells[8])
			## cols
			b4 = (self.cells[0] != ' ' and self.cells[0] == self.cells[3] and self.cells[3] == self.cells[6])
			b5 = (self.cells[1] != ' ' and self.cells[1] == self.cells[4] and self.cells[4] == self.cells[7])
			b6 = (self.cells[2] != ' ' and self.cells[2] == self.cells[5] and self.cells[5] == self.cells[8])
			## diags
			b7 = (self.cells[0] != ' ' and self.cells[0] == self.cells[4] and self.cells[4] == self.cells[8])
			b8 = (self.cells[6] != ' ' and self.cells[6] == self.cells[4] and self.cells[4] == self.cells[2])

			return (b1 or b2 or b3 or b4 or b5 or b6 or b7 or b8)

	def IsDraw(self):
		return (self.numMoves == 9 and (not self.IsGameOver()))

	def GetPlayer(self):
		if (self.numMoves & 1):
			## odd move (O moves second)
			return 'O'
		else:
			## even move (X moves first)
			return 'X'

	def GetLegalMoves(self):
		L = []
		for i in range(self.size * self.size):
			if (self.cells[i] == ' '): L.append(i)
		return L

	def IsLegalMove(self, idx):
		return (self.cells[idx] == ' ')

	## <player> is 'X' or 'O', <cell> is
	## a one-dimensional grid index (in
	## row-major order), move assumed to
	## be legal
	def DoMove(self, player, idx):
		self.cells[idx] = player
		self.numMoves += 1

	def DelMove(self, idx):
		self.cells[idx] = ' '
		self.numMoves -= 1

	## swap the contents of two cells
	def SwapCells(self, idx0, idx1):
		cell = self.cells[idx0]
		self.cells[idx0] = self.cells[idx1]
		self.cells[idx1] = cell


	## helper funcs to transform an individual move
	def RotateMoveR090(self, idx): L = [2, 5, 8,   1, 4, 7,   0, 3, 6]; r = L[idx]; del L; return r
	def RotateMoveR180(self, idx): return self.RotateMoveR090(self.RotateMoveR090(idx))
	def RotateMoveR270(self, idx): return self.RotateMoveR090(self.RotateMoveR090(self.RotateMoveR090(idx)))
	def RotateMoveL090(self, idx): L = [6, 3, 0,   7, 4, 1,   8, 5, 2]; r = L[idx]; del L; return r
	def RotateMoveL180(self, idx): return self.RotateMoveL090(self.RotateMoveL090(idx))
	def RotateMoveL270(self, idx): return self.RotateMoveL090(self.RotateMoveL090(self.RotateMoveL090(idx)))

	def FlipMoveH(self, idx):   L = [6, 7, 8,   3, 4, 5,   0, 1, 2]; r = L[idx]; del L; return r
	def FlipMoveV(self, idx):   L = [2, 1, 0,   5, 4, 3,   8, 7, 6]; r = L[idx]; del L; return r
	def FlipMoveDL(self, idx):  L = [8, 5, 2,   7, 4, 1,   6, 3, 0]; r = L[idx]; del L; return r
	def FlipMoveDR(self, idx):  L = [0, 3, 6,   1, 4, 7,   2, 5, 8]; r = L[idx]; del L; return r


	## rotate grid 90 degrees (left)
	def RotateGridL090(self):
		rows =  [self.cells[2], self.cells[5], self.cells[8]]
		rows += [self.cells[1], self.cells[4], self.cells[7]]
		rows += [self.cells[0], self.cells[3], self.cells[6]]

		for i in range(self.size * self.size):
			self.cells[i] = rows[i]

		del rows

	def RotateGridL180(self):
		self.RotateGridL090()
		self.RotateGridL090()
	def RotateGridL270(self):
		self.RotateGridL090()
		self.RotateGridL090()
		self.RotateGridL090()

	## rotate grid 90 degrees (right)
	def RotateGridR090(self):
		rows =  [self.cells[6], self.cells[3], self.cells[0]]
		rows += [self.cells[7], self.cells[4], self.cells[1]]
		rows += [self.cells[8], self.cells[5], self.cells[2]]

		for i in range(self.size * self.size):
			self.cells[i] = rows[i]

		del rows

	def RotateGridR180(self):
		self.RotateGridR090()
		self.RotateGridR090()
	def RotateGridR270(self):
		self.RotateGridR090()
		self.RotateGridR090()
		self.RotateGridR090()


	## flip grid horizontally (over middle ROW)
	def FlipGridH(self):
		self.SwapCells(0, 6)
		self.SwapCells(1, 7)
		self.SwapCells(2, 8)
	## flip grid vertically (over middle COL)
	def FlipGridV(self):
		self.SwapCells(0, 2)
		self.SwapCells(3, 5)
		self.SwapCells(6, 8)

	## flip grid diagonally (over BL to TR diagonal)
	def FlipGridDL(self):
		self.SwapCells(0, 8)
		self.SwapCells(1, 5)
		self.SwapCells(3, 7)
	## flip grid diagonally (over TL to BR diagonal)
	def FlipGridDR(self):
		self.SwapCells(2, 6)
		self.SwapCells(1, 3)
		self.SwapCells(5, 7)


	def GetEqualGrids(self):
		if (self.numMoves == 0):
			return []

		L = []
		h = self.__hash__()

		g0 = self.Clone(); g0.FlipGridH()
		g1 = self.Clone(); g1.FlipGridV()
		g2 = self.Clone(); g2.FlipGridDL()
		g3 = self.Clone(); g3.FlipGridDR()

		g4 = self.Clone(); g4.RotateGridR090()
		g5 = self.Clone(); g5.RotateGridR180()
		g6 = self.Clone(); g6.RotateGridR270()

		## eliminate identity transformations (but
		## do not check if g0, g1, etc produce the
		## same transformed grid)
		## note: store a pointer to the inverse MOVE
		## transformation funcion to make life easier
		if (g0.__hash__() != h): L.append((g0, Grid.FlipMoveH))
		if (g1.__hash__() != h): L.append((g1, Grid.FlipMoveV))
		if (g2.__hash__() != h): L.append((g2, Grid.FlipMoveDL))
		if (g3.__hash__() != h): L.append((g3, Grid.FlipMoveDR))
		if (g4.__hash__() != h): L.append((g4, Grid.RotateMoveL090))
		if (g5.__hash__() != h): L.append((g5, Grid.RotateMoveL180))
		if (g6.__hash__() != h): L.append((g6, Grid.RotateMoveL270))

		return L

	def GetEqualGridHashes(self):
		D = {}
		egs = self.GetEqualGrids()

		## save only the unique transforms
		for (eg, invMoveTransFunc) in egs:
			D[eg.__hash__()] = (self.__hash__(), invMoveTransFunc)

		return D






## generates the following number of grids:
## 1. 986409 (9 + 9 * 8 + 9 * 8 * 7 + ... + 9!) naively
## 2.   6045 if duplicates from naive way are filtered
##
## 3. 294777 grids if terminal (game-over) branches are not entered
## 4.   4535 if duplicates from non-terminal branches are filtered
##
## "duplicate" in the sense that multiple branches can
## lead to the same grid, not in the symmetrical sense
##
def GenAllGrids(g, lgrids, dgrids):
	moves = g.GetLegalMoves()

	for m in moves:
		g.DoMove(g.GetPlayer(), m)

		b1 = g.IsGameOver()
		b2 = dgrids.has_key(g.__hash__())

		if (not b1 and not b2):
			## note: redundant to store g
			## in both lgrids and dgrids
			dgrids[g.__hash__()] = g.Clone()
			lgrids.append(g.Clone())

			GenAllGrids(g, lgrids, dgrids)

		g.DelMove(m)

## for each now-unique grid at each depth, get rid of all
## its symmetrical equivalents if they aren't removed yet
## (this leaves a total of 629 "canonical" grids excluding
## the empty grid)
def RemSymmetricGrids(lgrids, dgrids):
	gridCounts = [0] * 10
	gridCCounts = [0] * 10
	runningCount = 0

	for g in lgrids:
		gh = g.__hash__()
		eghs = g.GetEqualGridHashes()

		if (dgrids.has_key(gh)):
			gridCounts[g.numMoves] += 1

			for egh in eghs:
				if (dgrids.has_key(egh)):
					del dgrids[egh]

	for i in range(10):
		runningCount += gridCounts[i]
		gridCCounts[i] += runningCount

	xCount = (gridCounts[0] + gridCounts[2] + gridCounts[4] + gridCounts[6] + gridCounts[8])
	oCount = (gridCounts[1] + gridCounts[3] + gridCounts[5] + gridCounts[7] + gridCounts[9])

	print("[RemSymmetricGrids]")
	print("\tgrid counts per stage: %s" % gridCounts)
	print("\tcumulative grid counts: %s" % gridCCounts)
	print("\tcount of grids player X can encounter: %d" % xCount)
	print("\tcount of grids player O can encounter: %d" % oCount)
	print("")

def PrintEqualGrids(dgrids, numMoves):
	for k in dgrids:
		if (dgrids[k].numMoves == numMoves):
			print(dgrids[k])



def InitBeadBoxes(dgrids, bboxes):
	## we simulate one MENACE machine which plays 'X'
	## and starts first (so it has the first, second,
	## ..., fifth move [0, 2, 4, 6, 8], ie. the even
	## ones), we therefore need 5 weight multipliers
	## [5, 4, 3, 2, 1]
	## actually we need only 4 since machine cannot
	## make itself lose by placing the final X
	##
	## note that we store ALL legal moves, not just
	## the ones that are equivalent due to symmetry
	## (since doing so would require rounding up all
	## the identity transformations T(g) = g for all
	## grids and applying them to every legal move to
	## find the equal A's, B's, C's, etc
	weights = [4, 3, 2, 1, 1]

	for k in dgrids:
		v = dgrids[k]
		bboxes[k] = {}

		if (v.numMoves & 1 == 0 or v.numMoves == 9):
			## simpler representation, but makes it a bit
			## more difficult to update the move weights
			## (while choosing a move is trivial)
			## bboxes[k] = (v.GetLegalMoves()) * weights[v.numMoves >> 1]

			lm = v.GetLegalMoves()
			for m in lm:
				bboxes[k][m] = weights[v.numMoves >> 1]
			del lm

	del weights

def UpdateBeadBoxWeights(hist, winner):
	for (box, move) in hist:
		if (winner == 'X'):
			box[move] += 1
			## box.append(move)
		else:
			if (box[move] > 1):
				box[move] -= 1

			## if (box.count(move) > 1):
			##	box.remove(move)



## grab a bead by roulette-selection
def SelectMove(box):
	moves = box.keys()
	weights = box.values()

	wsum = float(sum(weights))
	move = -1
	rand = random.random()
	prob = 0.0

	for m in moves:
		prob += (box[m] / wsum)

		if (prob >= rand):
			move = m
			break

	del moves
	del weights
	return move
	## return random.choice(box)

def Play(g, dgrids, bboxes, numGames):
	winsX = 0; winsO = 0
	winsT = 0; draws = 0
	game = 0
	hist = []
	tick = time.time()

	while (game < numGames):
		if (g.IsDraw()):
			g.Reset()

			del hist; hist = []
			game += 1; draws += 1
		else:
			if (g.IsGameOver()):
				if (g.numMoves & 1):
					winner = 'X'; winsX += 1
				else:
					winner = 'O'; winsO += 1

				## apply the reinforcements (kill
				## this line to disable learning)
				UpdateBeadBoxWeights(hist, winner)
				g.Reset()

				del hist; hist = []
				game += 1; winsT += 1
			else:
				player = g.GetPlayer()

				if (player == 'X'):
					## we are the machine and have to make a move
					gh = g.__hash__()

					## add the hash for this grid since it might
					## be the canonical one itself (in such cases
					## there is no defined inverse transformation
					## function)
					eghs = g.GetEqualGridHashes()
					eghs[gh] = (gh, None)

					## invMTF stays None if <g> is the canonical grid
					invMTF = None
					box = None
					moves = None
					move = -1

					## look up the bead-box for this grid by getting
					## the hashes of all of its transformations and
					## checking each one if it happens to be the key
					## of a box
					for egh in eghs:
						if (bboxes.has_key(egh)):
							invMTF = eghs[egh][1]
							box = bboxes[egh]
							move = SelectMove(box)
							break

					## save the opened bead-box and move taken from it
					hist.append((box, move))

					if (invMTF != None):
						move = invMTF(g, move)

				else:
					## we are the "human" (or second MENACE machine)
					## playing 'O' and make only stupid random moves
					moves = g.GetLegalMoves()
					move = random.choice(moves)

				g.DoMove(player, move)
				del moves

	print("[Play]")
	print("\twins for player X: %d of %d (%.2f%%)" % (winsX, numGames, (float(winsX) / numGames) * 100))
	print("\twins for player O: %d of %d (%.2f%%)" % (winsO, numGames, (float(winsO) / numGames) * 100))
	print("\tnumber of draws:   %d of %d (%.2f%%)" % (draws, numGames, (float(draws) / numGames) * 100))
	print("\telapsed game-time: %.2fs" % (time.time() - tick))



def main(argc, argv):
	g = Grid()

	lgrids = []
	dgrids = {g.__hash__(): g}
	bboxes = {}

	GenAllGrids(g, lgrids, dgrids)

	print("[main] len(lgrids) before removing symmetric grids: %d" % len(lgrids))
	print("[main] len(dgrids) before removing symmetric grids: %d" % len(dgrids))

	RemSymmetricGrids(lgrids, dgrids)
	InitBeadBoxes(dgrids, bboxes)

	print("[main] len(lgrids) after removing symmetric grids: %d" % len(lgrids))
	print("[main] len(dgrids) after removing symmetric grids: %d" % len(dgrids))

	Play(g, dgrids, bboxes, 10000)

	del g
	del lgrids
	del dgrids
	del bboxes
	return 0

sys.exit(main(len(sys.argv), sys.argv))

