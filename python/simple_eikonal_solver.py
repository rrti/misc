import sys

GRID_SIZE_X = 9
GRID_SIZE_Y = 9


class cell:
	def __init__(self, xidx, yidx,  speed = 1.0, time = -1.0):
		self.set_grid_coordinates(xidx, yidx)
		self.set_solve_call_counter(-1)
		self.set_wavefront_speed(speed)
		self.set_wavefront_time(time)

	def set_grid_coordinates(self, xidx, yidx): self.coors = (xidx, yidx)
	def get_grid_coordinates(self): return self.coors

	def set_solve_call_counter(self, cnt): self.solve_call_counter = cnt
	def get_solve_call_counter(self): return self.solve_call_counter

	## note: a speed of 0.0 indicates this cell is blocked
	def set_wavefront_speed(self, speed): self.speed = max(speed, 0.0)
	def get_wavefront_speed(self): return self.speed

	def set_wavefront_time(self, time): self.time = time
	def get_wavefront_time(self): return self.time


class grid:
	def __init__(self, xsize, ysize):
		self.xsize = xsize
		self.ysize = ysize
		self.cells = [cell(i % xsize, i / xsize) for i in xrange(xsize * ysize)]

	def __repr__(self):
		s = "\n"
		for yidx in xrange(self.ysize):
			for xidx in xrange(self.xsize):
				c = self.get_cell(xidx, yidx)
				s += (" %.2f " % c.get_wavefront_time())
			s += "\n"
		return s

	def get_cell(self, xidx, yidx):
		if (xidx < 0 or xidx >= self.xsize): return None
		if (yidx < 0 or yidx >= self.ysize): return None
		assert((self.cells[yidx * self.xsize + xidx].get_grid_coordinates())[0] == xidx)
		assert((self.cells[yidx * self.xsize + xidx].get_grid_coordinates())[1] == yidx)
		return (self.cells[yidx * self.xsize + xidx])

	def get_ngb_cells(self, cell_coors):
		xidx = cell_coors[0]
		yidx = cell_coors[1]

		ngbs = []

		if (xidx >              0): ngbs.append(self.get_cell(xidx - 1, yidx    ))
		if (xidx < self.xsize - 1): ngbs.append(self.get_cell(xidx + 1, yidx    ))
		if (yidx >              0): ngbs.append(self.get_cell(xidx    , yidx - 1))
		if (yidx < self.ysize - 1): ngbs.append(self.get_cell(xidx    , yidx + 1))

		return ngbs



	def find_quadratic_solution_case(self, p, q = None, r = None, s = None):
		t = 0.0

		if (q == None and r == None and s == None):
			t = p.get_wavefront_time() + (1.0 / p.get_wavefront_speed())
		elif (q != None and r == None and s == None):
			a = (p.get_wavefront_time() + (1.0 / (p.get_wavefront_speed())))
			b = (q.get_wavefront_time() + (1.0 / (q.get_wavefront_speed())))
			t = (a + b) * 0.5
			t = min(a, b)
		elif (q != None and r != None and s == None):
			a = (p.get_wavefront_time() + (1.0 / (p.get_wavefront_speed())))
			b = (q.get_wavefront_time() + (1.0 / (q.get_wavefront_speed())))
			c = (r.get_wavefront_time() + (1.0 / (r.get_wavefront_speed())))
			t = (a + b + c) * 0.333
			t = min(a, b, c)
		else:
			## only reachable for source-nodes
			a = (p.get_wavefront_time() + (1.0 / (p.get_wavefront_speed())))
			b = (q.get_wavefront_time() + (1.0 / (q.get_wavefront_speed())))
			c = (r.get_wavefront_time() + (1.0 / (r.get_wavefront_speed())))
			d = (s.get_wavefront_time() + (1.0 / (s.get_wavefront_speed())))
			t = (a + b + c + d) * 0.25
			t = min(a, b, c, d)

		return t

	##
	## solve the quadratic (T(x,y) - a)**2 + (T(x,y) - b)**2 = (1/F(x,y))**2 for T(x,y)
	## where a = min(T(x + 1, y), T(x - 1, y)) and b = min(T(x, y + 1), T(x, y - 1))
	##
	def find_quadratic_solution(self, cell_coors, solve_call_counter):
		xidx = cell_coors[0]
		yidx = cell_coors[1]

		cell_c = (self.get_cell(xidx,     yidx))
		cell_r = (self.get_cell(xidx + 1, yidx))
		cell_l = (self.get_cell(xidx - 1, yidx))
		cell_u = (self.get_cell(xidx, yidx - 1))
		cell_d = (self.get_cell(xidx, yidx + 1))

		## only consider T-values for *visited* nodes
		## (e.g. walls are never visited by the solver)
		## this means we may need to drop a dimension
		exclude_r = (cell_r == None or cell_r.get_solve_call_counter() < solve_call_counter)
		exclude_l = (cell_l == None or cell_l.get_solve_call_counter() < solve_call_counter)
		exclude_u = (cell_u == None or cell_u.get_solve_call_counter() < solve_call_counter)
		exclude_d = (cell_d == None or cell_d.get_solve_call_counter() < solve_call_counter)
		exclude_s = exclude_r + exclude_l + exclude_u + exclude_d

		assert(exclude_s != 4)

		if (exclude_s == 0):
			a = min(r.get_wavefront_time(), l.get_wavefront_time())
			b = min(d.get_wavefront_time(), u.get_wavefront_time())
			c = c.get_wavefront_speed()
			t = -1.0 * c*c * (a*c - 2.0*a*b*c*c + b*b*c*c - 2.0)

			t_pos =  (t ** 0.5) + a*c*c + b*c*c
			t_neg = -(t ** 0.5) + a*c*c + b*c*c
			t_pos /= (2.0 * c*c)
			t_neg /= (2.0 * c*c)

			## return (min(t_pos, t_neg))
			return (self.find_quadratic_solution_case(cell_r, cell_l, cell_u, cell_d))

		if (exclude_s == 1):
			if (exclude_r): return (self.find_quadratic_solution_case(cell_l, cell_u, cell_d))
			if (exclude_l): return (self.find_quadratic_solution_case(cell_r, cell_u, cell_d))
			if (exclude_u): return (self.find_quadratic_solution_case(cell_r, cell_l, cell_d))
			if (exclude_d): return (self.find_quadratic_solution_case(cell_r, cell_l, cell_u))

		if (exclude_s == 2):
			if ((not exclude_r) and (not exclude_u)): return (self.find_quadratic_solution_case(cell_r, cell_u))
			if ((not exclude_r) and (not exclude_d)): return (self.find_quadratic_solution_case(cell_r, cell_d))
			if ((not exclude_l) and (not exclude_u)): return (self.find_quadratic_solution_case(cell_l, cell_u))
			if ((not exclude_l) and (not exclude_d)): return (self.find_quadratic_solution_case(cell_l, cell_d))
			if ((not exclude_r) and (not exclude_l)): return (self.find_quadratic_solution_case(cell_r, cell_l))
			if ((not exclude_u) and (not exclude_d)): return (self.find_quadratic_solution_case(cell_u, cell_d))

		if (exclude_s == 3):
			if (not exclude_r): return (self.find_quadratic_solution_case(cell_r))
			if (not exclude_l): return (self.find_quadratic_solution_case(cell_l))
			if (not exclude_u): return (self.find_quadratic_solution_case(cell_u))
			if (not exclude_d): return (self.find_quadratic_solution_case(cell_d))

		assert(False)


	def get_min_cell(self, wavefront_cells):
		min_cell = None
		min_cell_wavefront_time = 1e30

		for cell in wavefront_cells:
			if (min_cell == None or cell.get_wavefront_time() < min_cell_wavefront_time):
				min_cell = cell
				min_cell_wavefront_time = cell.get_wavefront_time()

		assert(min_cell != None)
		assert(min_cell.get_wavefront_time() >= 0.0)
		return min_cell



	## simple fast-marching method; essentially identical to
	## best-first expansion (i.e. Dijkstra with 0-heuristic)
	def solve(self, source_cells, solve_call_counter = 0):
		wavefront_cells = []

		for source_cell in source_cells:
			source_cell.set_wavefront_time(0.0)
			source_cell.set_solve_call_counter(solve_call_counter)
			wavefront_cells.append(source_cell)

		while (len(wavefront_cells) > 0):
			cell = self.get_min_cell(wavefront_cells)
			wavefront_cells.remove(cell)

			cell_ngbs = self.get_ngb_cells(cell.get_grid_coordinates())

			if (False):
				## this pattern will cause wavefront to contain cells
				## whose T-values have not been calculated yet, which
				## means the min-cell is always one with T=-1.0 (will
				## break propagation if more than one source exists)
				if (cell.get_solve_call_counter() < solve_call_counter and cell.get_wavefront_speed() > 0.0):
					cell.set_wavefront_time(self.find_quadratic_solution(cell.get_grid_coordinates(), solve_call_counter))
					cell.set_solve_call_counter(solve_call_counter)

				for cell_ngb in cell_ngbs:
					if (cell_ngb.get_solve_call_counter() == solve_call_counter):
						continue
					wavefront_cells.append(cell_ngb)
			else:
				for cell_ngb in cell_ngbs:
					if (cell_ngb.get_solve_call_counter() >= solve_call_counter):
						continue
					if (cell_ngb.get_wavefront_speed() <= 0.0):
						continue

					cell_ngb.set_wavefront_time(self.find_quadratic_solution(cell_ngb.get_grid_coordinates(), solve_call_counter))
					cell_ngb.set_solve_call_counter(solve_call_counter)

					wavefront_cells.append(cell_ngb)



def main(argc, argv):
	g = grid(GRID_SIZE_X, GRID_SIZE_Y)
	s = [g.get_cell(1, 1), g.get_cell(GRID_SIZE_X - 1, 1)]

	for y in xrange(1, GRID_SIZE_Y):
		(g.get_cell(GRID_SIZE_X >> 1, y)).set_wavefront_speed(0.0)

	## TODO:
	##   we need to *define* the initial wavefront and the
	##   velocity (vector) each portion of it is traveling
	##   outward at
	##   currently a wave starts at each source-cell and
	##   moves outward along all grid axes at equal speed
	g.solve(s)

	print("[main][grid]\n%s" % g)
	return 0

sys.exit(main(len(sys.argv), sys.argv))

