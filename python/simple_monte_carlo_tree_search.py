import random
import sys

GRID_SIZE    = 3
GRID_SIZE_SQ = GRID_SIZE * GRID_SIZE

PLAYER_BLANK = -1
PLAYER_CROSS =  0
PLAYER_ROUND =  1

class game_move:
	def __init__(self, params):
		self.params = params
		self.execed = False

	def get_params(self):
		return self.params

	def execute(self, state):
		assert(not self.is_executed())
		self.execed = True
		return (state.do_copy_move(self.params[1], self.params[0]))
	def is_executed(self):
		return self.execed

class game_state:
	def __init__(self, gcells = [PLAYER_BLANK] * GRID_SIZE_SQ, player = PLAYER_CROSS, nmoves = 0):
		assert(gcells != None and len(gcells) == GRID_SIZE_SQ)
		assert(player == PLAYER_CROSS or player == PLAYER_ROUND)

		self.gcells = gcells
		self.player = player
		self.nmoves = nmoves

	def __repr__(self, tabs = ""):
		hdr = ("PLAYER TO MOVE: %c" % self.get_player_symbol(self.player))
		div = (tabs + "+---" * GRID_SIZE) + "+\n" + tabs
		## rep = hdr + ("\n" + div)
		rep = ("\n" + div)

		for row in xrange(GRID_SIZE):
			rep += "|"

			for col in xrange(GRID_SIZE):
				c = self.gcells[self.coors_to_index(col, row)]

				if (c == PLAYER_BLANK): rep += "   |"
				if (c == PLAYER_CROSS): rep += " X |"
				if (c == PLAYER_ROUND): rep += " O |"

			rep += ("\n" + div)

		return (rep + "\n")

	def coors_to_index(self, col, row):
		return (row * GRID_SIZE + col)
	def index_to_coors(self, index):
		return (index % GRID_SIZE, index / GRID_SIZE) ## (x=col, y=row)

	def gen_legal_moves(self, player):
		moves = []

		if (self.is_gameover()[0]):
			return moves

		for index in xrange(len(self.gcells)):
			if (self.gcells[index] == PLAYER_BLANK):
				moves.append(game_move((index, player, self.index_to_coors(index))))

		return moves


	def get_heuristic_score(self):
		## TODO: count the number and length of partial sequences for player
		game_over = self.is_gameover()

		if (game_over[0]):
			if (game_over[1] == PLAYER_CROSS): return  10.0
			if (game_over[1] == PLAYER_ROUND): return -10.0

		return 0.0



	def check_gameover(self):
		return (self.nmoves >= (GRID_SIZE * 2 - 1))
	def is_gameover(self):
		ret = False
		plr = PLAYER_BLANK

		if (not self.check_gameover()):
			return (ret, plr)

		## check all cells in row <row>
		def is_gameover_row(cells, row):
			init = cells[self.coors_to_index(0, row)]
			over = True

			for col in xrange(1, GRID_SIZE):
				curr = cells[self.coors_to_index(col, row)]
				over &= (curr == init and curr != PLAYER_BLANK)
				## if (not over): break

			return (over, init)

		## check all cells in column <col>
		def is_gameover_col(cells, col):
			init = cells[self.coors_to_index(col, 0)]
			over = True

			for row in xrange(1, GRID_SIZE):
				curr  = cells[self.coors_to_index(col, row)]
				over &= (curr == init and curr != PLAYER_BLANK)
				## if (not over): break

			return (over, init)

		## check diagonals
		def is_gameover_diag_tlbr(cells):
			init = cells[self.coors_to_index(0, 0)]
			over = True

			for index in xrange(1, GRID_SIZE):
				curr  = cells[self.coors_to_index(index, index)]
				over &= (curr == init and curr != PLAYER_BLANK)
				## if (not over): break

			return (over, init)

		def is_gameover_diag_bltr(cells):
			init = cells[self.coors_to_index(0, GRID_SIZE - 1)]
			over = True

			for index in xrange(1, GRID_SIZE):
				curr  = cells[self.coors_to_index(index, GRID_SIZE - 1 - index)]
				over &= (curr == init and curr != PLAYER_BLANK)
				## if (not over): break

			return (over, init)


		if (not ret):
			## TL-BR diagonal
			tup  = is_gameover_diag_tlbr(self.gcells)
			ret |= tup[0]

			if (ret):
				plr = tup[1]

		if (not ret):
			## BL-TR diagonal
			tup  = is_gameover_diag_bltr(self.gcells)
			ret |= tup[0]

			if (ret):
				plr = tup[1]

		if (not ret):
			## check rows
			for row in xrange(GRID_SIZE):
				tup  = is_gameover_row(self.gcells, row)
				ret |= tup[0]

				if (ret):
					plr = tup[1]
					break

		if (not ret):
			## check columns
			for col in xrange(GRID_SIZE):
				tup  = is_gameover_col(self.gcells, col)
				ret |= tup[0]

				if (ret):
					plr = tup[1]
					break

		return (ret, plr)


	def get_num_moves(self): return self.nmoves
	def get_curr_player(self): return self.player
	def get_next_player(self, player): return ((player + 1) % 2)
	def get_player_symbol(self, player):
		if (player == PLAYER_CROSS): return 'X'
		if (player == PLAYER_ROUND): return 'O'
		return ''

	def do_self_move(self, player, index):
		assert(player == PLAYER_CROSS or player == PLAYER_ROUND)
		assert(index < len(self.gcells))
		assert(self.gcells[index] == PLAYER_BLANK)

		self.gcells[index] = player
		self.player = self.get_next_player(player)
		self.nmoves = self.get_num_moves() + 1
		return self

	def do_copy_move(self, player, index):
		assert(player == PLAYER_CROSS or player == PLAYER_ROUND)
		assert(index < len(self.gcells))
		assert(self.gcells[index] == PLAYER_BLANK)

		grid = self.gcells[:]
		grid[index] = player
		return (game_state(grid, self.get_next_player(player), self.get_num_moves() + 1))




class tree_node:
	def __init__(self, state, depth):
		self.state = state
		## child nodes, added on-the-fly by executing moves (remains empty if none)
		self.nodes = []
		## legal moves available in this game-state (empty for draws and gameovers)
		self.moves = state.gen_legal_moves(state.get_curr_player())

		self.depth = depth
		self.count = 0
		self.value = 0.0

	def __repr__(self, cur_depth = 0, min_depth = 0):
		rep = ""

		if (cur_depth >= min_depth):
			rep += ("%s<COUNT=%d VALUE=%f>\n" % ('\t' * cur_depth, self.count, self.value))
			rep += self.state.__repr__('\t' * cur_depth)

		for node in self.nodes:
			rep += node.__repr__(cur_depth + 1, min_depth)

		return rep

	def add_child(self, c):
		assert(c != None)
		self.nodes += [c]

	def get_child(self, index):
		assert(not self.is_leaf())
		return (self.nodes[index])

	def get_random_child(self):
		assert(not self.is_leaf())
		return (random.choice(self.nodes))

	def get_max_value_child(self, eps = 0.05):
		## decide whether to explore or exploit
		if (random.random() < eps):
			return (self.get_random_child())

		n = self.nodes[0]

		for i in xrange(1, len(self.nodes)):
			c = self.nodes[i]

			if (c.get_value() > n.get_value()):
				n = c

		return n


	def do_new_random_move(self):
		assert(not self.is_leaf())
		assert(not self.is_fully_expanded())

		move = random.choice(self.moves)

		## we want a previously *unexecuted* move
		while (move.is_executed()):
			move = random.choice(self.moves)

		## move.execute creates a clone of <self.state>
		## this is passed into a new tree-node instance
		## which becomes a child of <self>
		return (tree_node(move.execute(self.state), self.depth + 1))


	def add_value(self, v): self.value += v
	def get_value(self): return self.value
	def add_count(self, c): self.count += c
	def get_count(self): return self.count

	def eval_state(self):
		## no moves available means draw *OR* game-over
		assert(self.is_leaf())
		return (self.state.get_heuristic_score())


	def is_fully_expanded(self):
		assert(not self.is_leaf())

		moves_executed = 0

		for move in self.moves:
			moves_executed += move.is_executed()

		## true if all moves for this state have been exhausted
		## (at that point we have as many children as there were
		## moves)
		return (moves_executed == len(self.moves))

	## if no legal moves are available, this node
	## has no (and can not have) children either
	def is_leaf(self): return (len(self.moves) == 0)
	## inner-node is any node that has exhausted
	## its (non-empty) set of possible moves, i.e.
	## has been fully explored
	def is_inner(self): return ((not self.is_leaf()) and self.is_fully_expanded())



def mcts_eval_tree(node, depth):
	value = 0.0

	def mcts_rand_walk(node, cur_depth, max_depth):
		if ((not node.is_leaf()) and (cur_depth < max_depth)):
			node.add_child(node.do_new_random_move())
			return (mcts_rand_walk(node.get_child(-1), cur_depth + 1, max_depth))

		return (node.eval_state())

	if (node.is_inner()):
		## node was fully explored previously and now has children
		## randomly descend further down the tree until we hit the
		## fringe, using eg. epsilon-greedy selection
		##
		## value = mcts_eval_tree(node.get_random_child(), depth + 1)
		value = mcts_eval_tree(node.get_max_value_child(), depth + 1)
	else:
		## node is still not fully explored, choose a random move
		## and add it as a new (i.e. not previously visited) child
		## then take a random path down to a leaf-node and evaluate
		## it
		## NOTE: the length of the walk is relative to current depth
		value = mcts_rand_walk(node, depth, depth + 10)

	node.add_value(value)
	node.add_count(1)
	return value



def play_game(game_inst):
	while (True):
		moves = game_inst.gen_legal_moves(game_inst.get_curr_player())

		## draw or gameover
		if (len(moves) == 0):
			break

		move_object = random.choice(moves)
		move_params = move_object.get_params()

		game_inst.do_self_move(game_inst.get_curr_player(), move_params[0])

	is_gameover = game_inst.is_gameover()
	game_winner = (is_gameover[0] and ("%c" % game_inst.get_player_symbol(is_gameover[1]))) or "<none>"

	print("[play_game] winner: %s, state: %s" % (game_winner, game_inst.__repr__("\t")))

def eval_game(game_tree, num_evals):
	tree_vals = [0.0] * num_evals

	## perform some number of evaluations
	for i in xrange(num_evals):
		tree_vals[i] = mcts_eval_tree(game_tree, 0)

	print("[eval_game] tree: %s" % game_tree.__repr__(1))
	return tree_vals

def main(argc, argv):
	rand_seed = ((argc > 1) and int(argv[1])) or 1
	num_evals = ((argc > 2) and int(argv[2])) or 1

	if (rand_seed != -1):
		random.seed(rand_seed)

	if (False):
		play_game(game_state())
	else:
		eval_game(tree_node(game_state(), 0), num_evals)

	return 0

sys.exit(main(len(sys.argv), sys.argv))

