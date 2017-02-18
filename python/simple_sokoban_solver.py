import sys
import time
import re
import random
## import pygame
## import pyglet



## www.sourcecode.se/sokoban/levels (level data)
## www.sourcecode.se/sokoban/download/LevelFormat.htm (level format)
##
## format is xml, each .slc can contain one or more levels
EXAMPLE_LEVEL_STRING =                              \
	"<Level Id=\"103\" Width=\"6\" Height=\"9\">\n" \
	"	<L>####</L>\n"                              \
	"	<L>#.@#</L>\n"                              \
	"	<L>#.$#</L>\n"                              \
	"	<L>#$ #</L>\n"                              \
	"	<L>#  ##</L>\n"                             \
	"	<L>#   #</L>\n"                             \
	"	<L># # ##</L>\n"                            \
	"	<L># $ .#</L>\n"                            \
	"	<L>######</L>\n"                            \
	"</Level>\n"

CELL_BIT_WALL = (1 << 1)
CELL_BIT_PLLT = (1 << 2) ## pallet
CELL_BIT_GOAL = (1 << 3)
CELL_BIT_PLYR = (1 << 4) ## player
CELL_BIT_VOID = (1 << 5)

VALID_CELL_CHARS = {
	' ': 0,
	'#': CELL_BIT_WALL,
	'$': CELL_BIT_PLLT,
	'.': CELL_BIT_GOAL,
	'@': CELL_BIT_PLYR,
	'+': CELL_BIT_PLYR | CELL_BIT_GOAL,
	'*': CELL_BIT_PLLT | CELL_BIT_GOAL,
}

MOVE_TYPE_PLYR = 0
MOVE_TYPE_PLLT = 1

## grid has four-connected topology
LEGAL_MOVE_DIRS = [(-1, 0), (1, 0), (0, -1), (0, 1)]

## "<Level Id="Example 1" Width="15" Height="8">"
## "<L> #$$#........#$$#</L>"
LEVEL_DATA_REGEX = re.compile(r'\s*<(L|l)evel\s+(I|i)d.*>')
LEVEL_LINE_REGEX = re.compile(r'\s*<L>.*</L>')




def unit_test_regexes():
	level_data_str = "<Level Id=\"1\" Width=\"18\" Height=\"14\">"
	level_line_str = "<L> #$$##############</L>"

	assert(re.match(LEVEL_DATA_REGEX, level_data_str))
	assert(re.match(LEVEL_LINE_REGEX, level_line_str))



class Level:
	def __init__(self, ncols, nrows, cells = None, preqs = None):
		self.ncols = ncols
		self.nrows = nrows

		if (cells == None):
			self.cells = [CELL_BIT_VOID] * (ncols * nrows)
		else:
			self.cells = cells

		if (preqs == None):
			self.preqs = [-1] * (ncols * nrows)
		else:
			self.preqs = preqs

		self.plyr = [-1 * ncols, -1 * nrows]

		## determined during loading
		self.num_pllts = 0
		self.num_goals = 0

	def __repr__(self):
		div = '\n\t' + '+' + ("-----+" * self.ncols) + '\n'
		pad = '|' + ("     |" * self.ncols)
		rep = div[:]

		for row_idx in xrange(self.nrows):
			rep += ('\t' + pad + '\n\t')
			rep += "|"

			for col_idx in xrange(self.ncols):
				rep += self.get_cell_str(col_idx, row_idx)

			rep += ('\n\t' + pad)
			rep += div

		return rep

	def clone(self):
		lvl = Level(self.ncols, self.nrows, self.cells[:])

		lvl.plyr = self.plyr[:]
		lvl.num_pllts = self.num_pllts
		lvl.num_goals = self.num_goals
		return lvl

	def set_prev_state(self, s): self.prev_state = s
	def get_prev_state(self): return self.prev_state

	def get_state_str(self):
		state = [self.plyr[:]] + ([None] * self.num_pllts)
		count = 1

		## we only need the positions of the pallets
		## (and player) to uniquely identify a state
		for idx in xrange(self.ncols * self.nrows):
			if (self.has_pllt(idx % self.ncols, idx / self.ncols)):
				state[count] = (idx % self.ncols, idx / self.ncols)
				count += 1

		return ("%s" % state)


	def get_num_cols(self): return self.ncols
	def get_num_rows(self): return self.nrows

	def is_valid_col(self, col_idx): return (col_idx >= 0 and col_idx < self.ncols)
	def is_valid_row(self, row_idx): return (row_idx >= 0 and row_idx < self.nrows)

	def is_free_cell(self, col_idx, row_idx):
		bits = self.get_cell(col_idx, row_idx)
		if ((bits & CELL_BIT_WALL) != 0): return False
		if ((bits & CELL_BIT_PLLT) != 0): return False
		return True

	def get_cell_idx(self, col_idx, row_idx):
		assert(self.is_valid_col(col_idx))
		assert(self.is_valid_row(row_idx))
		return (row_idx * self.ncols + col_idx)

	def get_cell(self, col_idx, row_idx): return self.cells[self.get_cell_idx(col_idx, row_idx)]
	def set_cell(self, col_idx, row_idx, val): self.cells[self.get_cell_idx(col_idx, row_idx)] = val
	def get_cell_str(self, col_idx, row_idx):
		bits = self.get_cell(col_idx, row_idx)

		if ((bits & CELL_BIT_PLYR) != 0):
			return "  P  |"
		if ((bits & CELL_BIT_WALL) != 0):
			return "  W  |"
		if ((bits & CELL_BIT_PLLT) != 0):
			return "  B  |"
		if ((bits & CELL_BIT_GOAL) != 0):
			return "  .  |"
		if ((bits & CELL_BIT_VOID) != 0):
			return "XXXXX|"

		## free cell
		return "     |"

	def has_bits(self, col_idx, row_idx, bits):
		if (not self.is_valid_col(col_idx)): return False
		if (not self.is_valid_row(row_idx)): return False
		return ((self.get_cell(col_idx, row_idx) & bits) != 0)
	def has_wall(self, col_idx, row_idx): return (self.has_bits(col_idx, row_idx, CELL_BIT_WALL))
	def has_pllt(self, col_idx, row_idx): return (self.has_bits(col_idx, row_idx, CELL_BIT_PLLT))
	def has_goal(self, col_idx, row_idx): return (self.has_bits(col_idx, row_idx, CELL_BIT_GOAL))
	def has_plyr(self, col_idx, row_idx): return (self.has_bits(col_idx, row_idx, CELL_BIT_PLYR))
	def has_void(self, col_idx, row_idx): return (self.has_bits(col_idx, row_idx, CELL_BIT_VOID))

	def finalize(self): return (self.num_pllts == self.num_goals)

	def add_pllt(self, pllt): self.num_pllts += 1
	def add_goal(self, goal): self.num_goals += 1


	"""
	## NOTE: this way we need to update both boxes and goals in move_*
	def finalize(self):
		pllts = []
		goals = []

		for idx in xrange(self.ncols * self.nrows):
			col = idx % self.ncols
			row = idx / self.ncols
			bits = self.get_cell(col, row)

			if ((bits & (CELL_BIT_PLLT | CELL_BIT_GOAL)) != 0):
				pllts += [(col, row)]
				goals += [(col, row)]
				continue

			if ((bits & CELL_BIT_PLLT) != 0): pllts += [(col, row)]
			if ((bits & CELL_BIT_GOAL) != 0): goals += [(col, row)]

		assert(len(pllts) == len(goals))

	def add_pllt(self, pllt): self.pllts += [pllt]
	def add_goal(self, goal): self.goals += [goal]
	def is_solved(self):
		ret = True

		## check if all pallets are on goal-cells
		for pllt in self.pllts:
			bits = self.get_cell(pllt[0], pllt[1])
			ret &= ((bits & CELL_BIT_GOAL) != 0)

		return ret
	"""


	def is_solved(self):
		num_solved_pllts = 0

		for idx in xrange(self.ncols * self.nrows):
			col = idx % self.ncols
			row = idx / self.ncols
			bits = self.get_cell(col, row)

			num_solved_pllts += ((bits & CELL_BIT_PLLT) != 0 and ((bits & CELL_BIT_GOAL) != 0))

		return (num_solved_pllts == self.num_pllts)


	def is_wall_blocked_h(self, col, row, off):
		assert(self.is_valid_row(row + off))

		b0 = False
		b1 = False

		for i in xrange(1, self.ncols):
			if ((col - i) < 0):
				break

			## check for a hole in the wall
			## check for a goal along the wall
			## check for a corner along the wall
			if (not self.has_wall(col - i, row + off)): b0 = False; break
			if (    self.has_goal(col - i, row      )): b0 = False; break
			if (    self.has_wall(col - i, row      )): b0 =  True; break
		for i in xrange(1, self.ncols):
			if ((col + i) >= self.ncols):
				break

			if (not self.has_wall(col + i, row + off)): b1 = False; break
			if (    self.has_goal(col + i, row      )): b1 = False; break
			if (    self.has_wall(col + i, row      )): b1 =  True; break

		return (b0 and b1)

	def is_wall_blocked_v(self, col, row, off):
		assert(self.is_valid_col(col + off))

		b0 = False
		b1 = False

		for i in xrange(1, self.nrows):
			if ((row - i) < 0):
				break

			if (not self.has_wall(col + off, row - i)): b0 = False; break
			if (    self.has_goal(col      , row - i)): b0 = False; break
			if (    self.has_wall(col      , row - i)): b0 =  True; break
		for i in xrange(1, self.nrows):
			if ((row + i) >= self.nrows):
				break

			if (not self.has_wall(col + off, row + i)): b1 = False; break
			if (    self.has_goal(col      , row + i)): b1 = False; break
			if (    self.has_wall(col      , row + i)): b1 =  True; break

		return (b0 and b1)


	def is_blocked(self):
		for idx in xrange(self.ncols * self.nrows):
			col = idx % self.ncols
			row = idx / self.ncols

			bits = self.get_cell(col, row)
			pllt = ((bits & CELL_BIT_PLLT) != 0)
			goal = ((bits & CELL_BIT_GOAL) != 0)

			## only consider non-goaled pallets
			if (pllt == 0): continue
			if (goal == 1): continue

			wall_l = self.has_wall(col - 1, row    ); ngb_l = self.has_pllt(col - 1, row    )
			wall_r = self.has_wall(col + 1, row    ); ngb_r = self.has_pllt(col + 1, row    )
			wall_t = self.has_wall(col,     row - 1); ngb_t = self.has_pllt(col,     row - 1)
			wall_b = self.has_wall(col,     row + 1); ngb_b = self.has_pllt(col,     row + 1)

			## detect whether box has been moved into a corner
			## TODO: if pallet is next to wall, scan along entire wall for corners
			if (wall_l and wall_t): return True
			if (wall_l and wall_b): return True
			if (wall_r and wall_t): return True
			if (wall_r and wall_b): return True

			## also look for adjacent pallets along walls
			##   ...[col - 1, row - 1][col, row - 1][col + 1, row - 1]...
			##   ...[col - 1, row    ][col, row,   ][col + 1, row,   ]...
			##   ...[col - 1, row + 1][col, row + 1][col + 1, row + 1]...
			##
			## horizontal cases: pallet to left or right
			if (ngb_l and wall_t and self.has_wall(col - 1, row - 1)): return True
			if (ngb_l and wall_b and self.has_wall(col - 1, row + 1)): return True
			if (ngb_r and wall_t and self.has_wall(col + 1, row - 1)): return True
			if (ngb_r and wall_b and self.has_wall(col + 1, row + 1)): return True
			## vertical cases: pallet to top or bottom
			if (ngb_t and wall_l and self.has_wall(col - 1, row - 1)): return True
			if (ngb_t and wall_r and self.has_wall(col + 1, row - 1)): return True
			if (ngb_b and wall_l and self.has_wall(col - 1, row + 1)): return True
			if (ngb_b and wall_r and self.has_wall(col + 1, row + 1)): return True
			## check if pallet has been moved against wall and cannot be freed
			if (wall_l and self.is_wall_blocked_v(col, row, -1)): return True
			if (wall_r and self.is_wall_blocked_v(col, row, +1)): return True
			if (wall_t and self.is_wall_blocked_h(col, row, -1)): return True
			if (wall_t and self.is_wall_blocked_h(col, row, +1)): return True

		return False


	def solve(self):
		initial_time = time.time()

		active_states = [self]
		closed_states = {}

		## initial state has no predecessor
		self.set_prev_state(None)

		def expand_dfs(L, n): return ([n] + L)
		def expand_bfs(L, n): return (L + [n])

		while (len(active_states) > 0):
			state = active_states[0]
			active_states = active_states[1: ]

			if (state.is_solved()):
				return (state, len(closed_states), time.time() - initial_time)
			if (closed_states.has_key(state.get_state_str())):
				continue

			closed_states[state.get_state_str()] = True

			## do not bother generating successors if we are blocked
			if (state.is_blocked()):
				continue

			## for each possible move, clone level and push onto stack
			for move_dir in LEGAL_MOVE_DIRS:
				next_state = state.clone()

				if (next_state.move_plyr(state.get_plyr_pos(), move_dir)):
					active_states = expand_bfs(active_states, next_state)
					next_state.set_prev_state(state)

		return (None, len(closed_states), time.time() - initial_time)


	def get_plyr_pos(self): return self.plyr
	def set_plyr_pos(self, col_idx, row_idx):
		assert(self.is_free_cell(col_idx, row_idx))

		self.plyr[0] = col_idx
		self.plyr[1] = row_idx


	def is_valid_base_move(self, move_dir):
		## if (abs(move_dir[0]) > 1): return False
		## if (abs(move_dir[1]) > 1): return False
		##
		## do not allow diagonal moves
		## return (abs(sum(move_dir[0] + move_dir[1])) == 1)
		return (move_dir in LEGAL_MOVE_DIRS)

	def is_valid_move_dir(self, base_pos, move_dir, move_type):
		if (not self.is_valid_base_move(move_dir)):
			return False

		dst_col = base_pos[0] + move_dir[0]
		dst_row = base_pos[1] + move_dir[1]

		if (not self.is_valid_col(dst_col)): return False
		if (not self.is_valid_row(dst_row)): return False

		dst_bits = self.get_cell(dst_col, dst_row)

		## nothing can move into walls
		if ((dst_bits & CELL_BIT_WALL) != 0):
			return False
		## if there is a crate on the destination cell,
		## check if it can be pushed out of the way by
		## player
		if ((dst_bits & CELL_BIT_PLLT) != 0):
			if (move_type == MOVE_TYPE_PLLT):
				return False
			return (self.is_valid_move_dir((dst_col, dst_row), move_dir, MOVE_TYPE_PLLT))

		return True


	## player may not move into walls or cause crates to
	## move into walls or other crates, but can push them
	## otherwise
	##
	## if movable crate exists:
	##   pre: free at x, crate at x+1, player at x+2
	##   post: crate at x, player at x+1, free at x+2
	def move_plyr(self, plyr_pos, move_dir):
		if (not self.is_valid_move_dir(plyr_pos, move_dir, MOVE_TYPE_PLYR)):
			return False

		src_col = plyr_pos[0]
		src_row = plyr_pos[1]
		dst_col = plyr_pos[0] + move_dir[0]
		dst_row = plyr_pos[1] + move_dir[1]
		src_bits = self.get_cell(src_col, src_row)
		dst_bits = self.get_cell(dst_col, dst_row)

		assert((src_bits & CELL_BIT_PLYR) != 0)
		assert((dst_bits & CELL_BIT_PLYR) == 0)

		if ((dst_bits & CELL_BIT_PLLT) != 0):
			if (not self.move_pllt((dst_col, dst_row), move_dir)):
				return False

		src_bits &= (~CELL_BIT_PLYR)
		dst_bits &= (~CELL_BIT_PLLT)
		dst_bits |= ( CELL_BIT_PLYR)

		self.set_cell(src_col, src_row, src_bits)
		self.set_cell(dst_col, dst_row, dst_bits)
		self.set_plyr_pos(dst_col, dst_row)
		return True

	## crates may not be moved into walls or other crates
	def move_pllt(self, pllt_pos, move_dir):
		if (not self.is_valid_move_dir(pllt_pos, move_dir, MOVE_TYPE_PLLT)):
			return False

		src_col = pllt_pos[0]
		src_row = pllt_pos[1]
		dst_col = pllt_pos[0] + move_dir[0]
		dst_row = pllt_pos[1] + move_dir[1]
		src_bits = self.get_cell(src_col, src_row)
		dst_bits = self.get_cell(dst_col, dst_row)

		assert((src_bits & CELL_BIT_PLLT) != 0)
		assert((dst_bits & CELL_BIT_PLLT) == 0)

		src_bits &= (~CELL_BIT_PLLT)
		dst_bits &= (~CELL_BIT_PLYR)
		dst_bits |= ( CELL_BIT_PLLT)

		self.set_cell(src_col, src_row, src_bits)
		self.set_cell(dst_col, dst_row, dst_bits)

		assert((self.get_cell(src_col, src_row) & CELL_BIT_PLLT) == 0)
		assert((self.get_cell(dst_col, dst_row) & CELL_BIT_PLLT) != 0)
		return True


	def trace_path(self, node):
		path = []

		while (node[2] != None):
			path += [(node[0], node[1])]
			node = node[2]

		path.reverse()
		return path

	def get_path(self, src_node, dst_node, path_time):
		src_node_col = src_node[0]
		src_node_row = src_node[1]
		dst_node_col = dst_node[0]
		dst_node_row = dst_node[1]

		node_queue = [(src_node_col, src_node_row, None)]
		path_nodes = []

		if (not self.is_valid_col(src_node_col)): return path_nodes
		if (not self.is_valid_row(src_node_row)): return path_nodes
		if (not self.is_valid_col(dst_node_col)): return path_nodes
		if (not self.is_valid_row(dst_node_row)): return path_nodes
		if (not self.is_free_cell(src_node_col, src_node_row)): return path_nodes
		if (not self.is_free_cell(dst_node_col, dst_node_row)): return path_nodes

		## mark source-node as visited
		self.preqs[self.get_cell_idx(src_node_col, src_node_row)] = path_time

		while (len(node_queue) > 0):
			curr_node = node_queue[0]
			node_queue = node_queue[1: ]

			if ((curr_node[0] == dst_node[0]) and (curr_node[1] == dst_node[1])):
				path_nodes = [src_node] + self.trace_path(curr_node)
				break

			## BFS-expand neighbors
			for ngb_offset in LEGAL_MOVE_DIRS:
				ngb_col = curr_node[0] + ngb_offset[0]
				ngb_row = curr_node[1] + ngb_offset[1]

				if (not self.is_valid_col(ngb_col)): continue
				if (not self.is_valid_row(ngb_row)): continue

				ngb_idx = self.get_cell_idx(ngb_col, ngb_row)
				ngb_bits = self.get_cell(ngb_col, ngb_row)

				## check if node was already visited during this request
				## note: when executing depth- or best-first search here,
				## continuing would not ensure finding the SHORTEST path
				if (self.preqs[ngb_idx] >= path_time): continue

				if ((ngb_bits & CELL_BIT_WALL) != 0): continue
				if ((ngb_bits & CELL_BIT_PLLT) != 0): continue

				## mark neighbor as visited by this request
				self.preqs[ngb_idx] = path_time

				## achtung: this is a recursive structure
				node_ngb = (ngb_col, ngb_row, curr_node)
				node_queue = node_queue + [node_ngb]

		return path_nodes




def read_level_line(row_str, row_num, level_object):
	row_start_idx = row_str.find("<L>") + 3
	num_wall_chrs = 0

	for row_idx in xrange(row_start_idx, len(row_str)):
		cell_char = row_str[row_idx]

		## check if we are between the <L> ... </L> markers
		if (not (cell_char in VALID_CELL_CHARS)):
			continue

		cell_mask = VALID_CELL_CHARS[cell_char]
		num_wall_chrs += ((cell_mask & CELL_BIT_WALL) != 0)

		## if no wall characters have been seen yet, we are to
		## the left of the level proper but within its bounds
		if (num_wall_chrs == 0):
			continue

		if ((cell_mask & CELL_BIT_PLYR) != 0):
			level_object.set_plyr_pos(row_idx - row_start_idx, row_num)
		if ((cell_mask & CELL_BIT_PLLT) != 0):
			level_object.add_pllt((row_idx - row_start_idx, row_num))
		if ((cell_mask & CELL_BIT_GOAL) != 0):
			level_object.add_goal((row_idx - row_start_idx, row_num))

		level_object.set_cell(row_idx - row_start_idx, row_num, cell_mask)

	return 0

def read_level(file_lines, curr_line_index, level_objects):
	level_ncols_prefix_str = "h=\"" ## 3
	level_nrows_prefix_str = "t=\"" ## 3

	level_data_str = file_lines[curr_line_index]
	level_rows_cnt = 0
	level_cols_cnt = 0

	level_data_str_ncols_idx = level_data_str.find(level_ncols_prefix_str)
	level_data_str_nrows_idx = level_data_str.find(level_nrows_prefix_str, level_data_str_ncols_idx + 3)

	assert(level_data_str_ncols_idx >= 0)
	assert(level_data_str_nrows_idx >= 0)

	level_data_str_ncols_end = level_data_str.find("\"", level_data_str_ncols_idx + 3)
	level_data_str_nrows_end = level_data_str.find("\"", level_data_str_nrows_idx + 3)

	assert(level_data_str_ncols_end >= 0)
	assert(level_data_str_nrows_end >= 0)

	level_data_ncols = int(level_data_str[level_data_str_ncols_idx + 3: level_data_str_ncols_end])
	level_data_nrows = int(level_data_str[level_data_str_nrows_idx + 3: level_data_str_nrows_end])

	assert(level_data_str.find("\"%d\"" % level_data_ncols) >= 0)
	assert(level_data_str.find("\"%d\"" % level_data_nrows) >= 0)

	level_object = Level(level_data_ncols, level_data_nrows)
	level_objects.append(level_object)

	## assume contiguous data per level
	for level_row_index in xrange(curr_line_index + 1, len(file_lines)):
		if (re.match(LEVEL_LINE_REGEX, file_lines[level_row_index])):
			level_cols_cnt = read_level_line(file_lines[level_row_index], level_rows_cnt, level_object)
			level_rows_cnt += 1

			assert(level_cols_cnt <= level_data_ncols)
		else:
			break

	## assert(level_cols_cnt <= level_data_ncols)
	assert(level_rows_cnt == level_data_nrows)
	assert(level_object.finalize())

	return level_rows_cnt

def read_levels(file_lines):
	level_objects = []
	curr_line_idx = 0

	while (curr_line_idx < len(file_lines)):
		if (re.match(LEVEL_DATA_REGEX, file_lines[curr_line_idx])):
			curr_line_idx += read_level(file_lines, curr_line_idx, level_objects)
		else:
			curr_line_idx += 1

	return level_objects

def read_slc_file(slc_filename):
	slc_file_str = EXAMPLE_LEVEL_STRING

	if (len(slc_filename) != 0):
		slc_file_hnd = open(slc_filename, 'r')
		slc_file_str = slc_file_hnd.read()
		slc_file_hnd.close()

	slc_lvl_objs = read_levels(slc_file_str.split('\n'))

	assert(len(slc_lvl_objs) != 0)
	return slc_lvl_objs


def play_level(lvl):
	print("[play][(w,h)=(%d,%d)][solved=%d][blocked=%d]" % (lvl.get_num_cols(), lvl.get_num_rows(), lvl.is_solved(), lvl.is_blocked()))
	print("%s" % lvl)

	if (False):
		for i in xrange(10):
			src = (random.randint(0, lvl.ncols - 1), random.randint(0, lvl.nrows - 1))
			dst = (random.randint(0, lvl.ncols - 1), random.randint(0, lvl.nrows - 1))

			print("\tpath(%s-->%s)=%s" % (src, dst, lvl.get_path(src, dst, i)))

	solver_stats = lvl.solve()
	solved_state = solver_stats[0]

	if (solved_state == None):
		print("\tno solution (%d states explored)..." % solver_stats[1])
		return False

	print("\tsolution (%d states explored)..." % solver_stats[1])

	## dump the solution in reverse order
	while (solved_state != None):
		print("%s" % solved_state)
		solved_state = solved_state.get_prev_state()

	return True

def main(argc, argv):
	unit_test_regexes()

	if (argc < 3):
		print("[main] usage: %s <level file> <level number>" % (argv[0]))
		return 0

	lvl_set = argv[1]
	lvl_idx = int(argv[2])

	lvl_objs = read_slc_file(lvl_set)
	lvl_obj = lvl_objs[ lvl_idx % len(lvl_objs) ]

	print("[main][set=\"%s\"][levels=%d][index=%d]" % (lvl_set, len(lvl_objs), lvl_idx % len(lvl_objs)))
	play_level(lvl_obj)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

